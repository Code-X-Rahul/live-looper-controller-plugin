# Pitfalls Research

**Domain:** DAW plugin (AU/VST3) — control surface for live looping
**Researched:** 2026-04-15
**Confidence:** HIGH (JUCE docs verified, Ableton Live docs verified) / MEDIUM (cross-track architecture — community knowledge)

## Critical Pitfalls

### Pitfall 1: A Single Plugin Instance Cannot See Other Tracks' Devices

**What goes wrong:**
The project requires "seeing the state of all looper devices across multiple tracks in one panel." However, an AU/VST3 plugin instance lives on **one track** within the DAW. The VST3 and AU specifications provide **no API** for a plugin to discover, enumerate, or control devices on other tracks. A plugin loaded on Track 3 cannot query Track 5's Looper device parameters. The plugin is sandboxed to its own instance. If you build this assuming cross-track device discovery is possible via the plugin API, the entire architecture falls apart.

**Why it happens:**
DAW plugin formats (VST3, AU) were designed for audio processing, not for control surface duties. The host (Ableton Live) manages tracks and routing — plugins receive audio/MIDI from the host and output audio/MIDI back. The plugin has no concept of "tracks" or "other devices" in the DAW. Developers experienced with Max for Live or Ableton's Python Remote Scripts may assume similar APIs exist for plugins — they don't.

**How to avoid:**
- **Design from day one** with an inter-plugin communication layer. Each track gets a plugin instance (a "satellite"), and instances communicate via IPC (inter-process communication) — shared memory, UDP sockets, or named pipes — to aggregate state into a single "controller" view.
- Alternatively, design a **standalone controller app** that communicates with lightweight satellite plugins on each track via a local network or IPC bridge. The standalone app provides the unified control surface; each satellite plugin exposes its track's looper state.
- A third option: leverage **MIDI routing**. The plugin outputs MIDI messages that the user routes in Ableton to each looper's parameters via MIDI mapping. This sacrifices auto-discovery but works within existing DAW mechanisms.
- Whatever path you choose, the architecture document must explicitly define **how cross-track state flows** — do not treat this as a later optimization.

**Warning signs:**
- You find yourself writing `getAllTracks()` or `getDeviceOnTrack()` methods that have no backing API
- Plugin code assumes it can enumerate other plugins/devices
- The architecture treats the plugin as a singleton that "knows about" the whole session
- Unit tests that mock a "track list" without acknowledging it must come from outside the plugin API

**Phase to address:** Phase 1 (Architecture) — this is the make-or-break architectural decision

---

### Pitfall 2: Blocking the Audio Thread in processBlock()

**What goes wrong:**
The `processBlock()` callback runs on the real-time audio thread. Any blocking operation — memory allocation, mutex locks, file I/O, network calls, `std::cout`, string formatting — will cause audio dropouts, glitches, or crashes. This is the #1 cause of DAW plugin crashes reported in production. What makes it insidious: the crash rarely happens at the line that blocked — it manifests as a DAW freeze, a macOS watch dog kill, or a Windows audio stack timeout, making diagnosis very difficult.

**Why it happens:**
Developers are used to "normal" programming where blocking is acceptable. The audio thread operates under hard real-time constraints (typically needing to complete within 1-3ms at 44.1kHz with 256 samples per block). Even `std::vector::push_back()` can trigger a heap allocation and block. JUCE's documentation explicitly warns: `parameterValueChanged()` callbacks run synchronously and must be "VERY fast, and avoid blocking." Developers also sometimes use `juce::CriticalSection` or `std::mutex` across the audio-thread/message-thread boundary, causing priority inversion.

**How to avoid:**
- **Never allocate** on the audio thread. Pre-allocate all buffers in `prepareToPlay()`.
- **Never lock** a non-real-time-safe mutex on the audio thread. Use `juce::SpinLock` (short critical sections only) or, better, lock-free communication via `juce::AbstractFifo`, `std::atomic`, or `juce::AsyncUpdater`.
- **Never call** any system API, file I/O, or network function from `processBlock()` or any callback that may be called from the audio thread.
- Use `juce::AsyncUpdater` or `juce::ChangeBroadcaster` to pass state from the audio thread to the message thread for UI updates.
- Use `juce::ScopedNoDenormals` at the top of `processBlock()` to avoid denormal-related CPU spikes.
- In this project specifically, any IPC/networking for cross-track communication MUST happen on the message thread, never on the audio thread.

**Warning signs:**
- `new`, `malloc`, or `std::vector::push_back` calls in `processBlock()` or `parameterValueChanged()`
- `CriticalSection::enter()` or `std::mutex::lock()` in audio-thread code paths
- `juce::File`, `juce::URL`, or `std::cout` calls from the audio callback
- Audio dropouts that only appear under load or with many tracks
- DAW freezing or "plugin not responding" messages

**Phase to address:** Phase 1 (Core architecture) — thread model must be defined before writing any process code

---

### Pitfall 3: Parameter ID Mismatch Across VST3/AU Formats

**What goes wrong:**
JUCE generates different parameter IDs for VST3 and AU formats. In VST3, parameter IDs are hashed from the string you provide; in AU, they're generated differently. When a user saves an Ableton Live set with a VST3 version of your plugin and then opens it with the AU version (or vice versa on macOS, where Ableton can load either), all parameter associations break. Automation curves become detached, state recall fails, and the plugin loads with default values — a show-stopper for live performance where recall reliability is critical.

**Why it happens:**
Before JUCE 7, parameter IDs were auto-generated from parameter name strings with different hashing for each format. JUCE 7+ introduced `juce::ParameterID` with an explicit version hint, but many tutorials and examples still use the old string-only API. Ableton Live on macOS supports both VST3 and AU, and may load whichever format it finds first. If the user has both installed, Ableton could load a different format than expected.

**How to avoid:**
- **Always use `juce::ParameterID` with a version hint** when creating parameters:
  ```cpp
  juce::AudioParameterFloat(
      juce::ParameterID("looper_state", 1),  // 1 = version, increment when changing
      "Looper State",
      ...);
  ```
- **Never rely on parameter name strings as IDs** — use `ParameterID` explicitly.
- **Pick one format as canonical for v1** (VST3 is recommended — works on both macOS and Windows) and document that users should use VST3.
- **Test state save/load across formats** early. If you ship both VST3 and AU, test Ableton loading a `.als` set with both formats.
- **Consider not shipping AU at all for v1** if Windows isn't a priority — reduces format-related bugs by 50%.

**Warning signs:**
- Parameters using old-style string IDs without `ParameterID` version hints
- No cross-format state recall testing
- Users reporting that saved projects don't recall parameter values after switching plugin format
- Automation envelopes that target wrong parameters after format change

**Phase to address:** Phase 1 (when defining parameter architecture)

---

### Pitfall 4: Ableton Live's "Configure Mode" Limits Parameter Visibility

**What goes wrong:**
Ableton Live does not automatically expose all VST3/AU plugin parameters in its device panel. Plugins with more than 64 parameters open with an **empty panel** in Live, and users must manually enter "Configure Mode" and click/tweak each parameter they want exposed. Even for plugins with ≤64 parameters, some parameters may not be "published" to the DAW. For a controller plugin that needs looper parameters (Record, Overdub, Play, Stop, State, etc.) to be visible and mappable, this is a critical constraint. If your plugin exports too many parameters or doesn't mark them correctly, Live users won't be able to map them.

**Why it happens:**
Ableton Live's design philosophy treats plugin parameters as something users opt into, not something automatically fully exposed. The VST3 and AU specs allow plugins to declare parameters with different categories and flags. Ableton uses these flags to decide what appears in its panel. Ableton also requires that you **actually change a parameter's value** in some cases for it to appear in Configure Mode — just declaring it isn't always enough.

**How to avoid:**
- Keep your plugin's exposed parameter count **well under 64** (per instance). A single looper controller needs maybe 5-10 parameters (state display, record, overdub, play, stop). Don't export internal/debug parameters via `addParameter()`.
- Use `juce::AudioParameterFloat`, `juce::AudioParameterChoice`, and `juce::AudioParameterBool` (which are properly exposed to the host). Avoid adding parameters programmatically in loops that exceed 64.
- Mark parameters as automated-friendly by providing proper `getText()` / `getValueForText()` implementations so they display meaningful state in Ableton's panel.
- **Test early and often in Ableton Live** — open your plugin, check that all intended parameters appear in the device panel, and that they can be MIDI-mapped.
- For a controller plugin, consider whether your parameters should be **automatable** (workflow: automation lanes) or **not automatable** (live control only). Use `AudioProcessorParameter::genericParameter` category vs. more specific categories.

**Warning signs:**
- Plugin loads in Ableton but its parameter panel is empty or incomplete
- Parameters visible in the plugin editor don't appear in Ableton's device panel
- MIDI mapping in Ableton can't find your parameters
- More than 64 parameters declared in `createParameterLayout()`

**Phase to address:** Phase 2 (first Ableton integration testing)

---

### Pitfall 5: Plugin Editor Lifecycle — Destroyed But Processor Lives On

**What goes wrong:**
The `AudioProcessorEditor` (your plugin's GUI) can be created and destroyed multiple times during the lifetime of the `AudioProcessor`. In Ableton Live, closing the plugin window destroys the editor; reopening it creates a new one. The processor always outlives the editor. If you store UI state (e.g., which loopers are expanded, scroll position, animation timers) in the editor rather than the processor, that state is lost every time the window closes. For a live performance tool, this means your carefully arranged view resets when you close the plugin window — devastating on stage.

**Why it happens:**
Many plugin tutorials store state in the editor for simplicity. Ableton aggressively manages plugin windows to conserve screen real estate and CPU. It may also destroy editors when switching device views or minimizing tracks. The JUCE documentation states that the editor should be a "view" of the processor's state, but this is easy to forget when prototyping.

**How to avoid:**
- **All persistent state must live in the AudioProcessor** (or its AudioProcessorValueTreeState), never in the editor.
- The editor should only contain: UI layout code, attachment objects (`SliderAttachment`, `ButtonAttachment`), and transient visual state (animation frames, highlight states).
- The processor must be able to function correctly **without** the editor existing.
- Use `AudioProcessorValueTreeState::Parameter` attachments as the bridge between processor state and editor UI — they automatically handle editor creation/destruction.
- Test by programmatically closing and reopening the editor (Ableton: close plugin window, reopen). All UI state should restore correctly.
- For a controller plugin with a visual grid of looper states, store which loopers are visible, sort order, and view preferences in the processor's state tree.

**Warning signs:**
- Editor class has member variables that hold view state (scroll position, selected looper index, filter state)
- Closing and reopening the plugin window loses user configuration
- Timer callbacks in the editor that should be running even when the window is hidden
- Controller logic (MIDI message sending, state polling) triggered from editor methods

**Phase to address:** Phase 2 (UI implementation)

---

### Pitfall 6: JUCE 8 AGPLv3 Licensing for Commercial Distribution

**What goes wrong:**
JUCE 8 is dual-licensed: AGPLv3 or the JUCE commercial license. AGPLv3 requires that you make your entire source code available under AGPLv3 terms if you distribute the software — including network use. For a commercial plugin sold through a marketplace, this is usually unacceptable. If you start developing under AGPLv3 thinking you'll switch later, you may have already incorporated JUCE code patterns that are hard to disentangle, or you may have already distributed builds that trigger the AGPLv3 source-disclosure requirement.

**Why it happens:**
JUCE 7 and earlier had more permissive licensing (some modules were ISC). JUCE 8 moved all modules to AGPLv3/commercial. Developers familiar with the old licensing may not realize the change. The personal use exception has a revenue threshold (~$50K USD), which seems generous but can be hit by a successful plugin product.

**How to avoid:**
- Decide early: open-source (AGPLv3) or commercial license? This affects the entire project.
- If commercial: budget for the JUCE commercial license from the start. It's per-developer, annual.
- If keeping it free/open-source: ensure AGPLv3 compatibility with all other dependencies.
- Don't distribute AGPLv3-licensed builds to testers/customers without understanding the source disclosure requirement.
- The JUCE personal tier allows closed-source if revenue is under the threshold — but verify the current threshold on juce.com.

**Warning signs:**
- No license decision documented in the project
- Distributing plugin binaries without a JUCE commercial license while using JUCE 8
- Including JUCE code without proper attribution or license compliance

**Phase to address:** Phase 0 (pre-development setup) — before writing any code

---

### Pitfall 7: Ableton Looper's State Machine Doesn't Match Simple On/Off Toggles

**What goes wrong:**
The Ableton Looper device has a complex state machine (empty → recording → playing → overdubbing → stopped), and its multi-purpose transport button has context-dependent behavior (single click = play/overdub toggle, double click = stop, long press = undo/clear). If your controller plugin maps "Record," "Overdub," "Play," and "Stop" as simple on/off toggle buttons, they won't map correctly to the Looper's actual parameter behavior. The Looper's "State" parameter is typically an integer/discrete value, not separate boolean toggles. Sending "Record = On" via MIDI CC to a looper that's currently in "Play" state may trigger an unexpected transition or be ignored entirely.

**Why it happens:**
Designers think in terms of simple button actions (tap to record, tap to play). The Looper's transport behavior is designed for footswitch operation: during playback, pressing the button switches to overdub; during overdub, pressing switches to play. This doesn't map to separate "Play" and "Overdub" buttons in the obvious way. The parameter layout of the Looper device exposes parameters like "Transport" (a discrete choice), "Recording State," etc., which don't align with naive button-per-action mapping.

**How to avoid:**
- **Study Ableton Looper's exact parameter layout** before designing your UI. Load a Looper device, enter Configure Mode, and document every parameter it exposes and its range/type.
- Map your controller's UI actions to the Looper's **actual parameters**, not to conceptual actions. For example, the Looper's main transport parameter may be a discrete "State" parameter with values like 0=Stop, 1=Record, 2=Play, 3=Overdub.
- Test with the actual Looper device in Ableton Live — don't assume you know the parameter names from reading the manual alone.
- Design your controller UI to match the Looper's state machine, not the user's mental model. A single "Transport" control that cycles through states may be more appropriate than separate Record/Play/Overdub/Stop buttons.
- For third-party loopers (Mobius, SooperLooper), the parameter layout will be different — plan for adapter/strategy patterns.

**Warning signs:**
- UI has four separate toggle buttons (Record, Overdub, Play, Stop) but Looper has one discrete "State" parameter
- MIDI CCs sent to looper parameters produce no effect or unexpected behavior
- The controller works in one state but breaks after state transitions
- Documentation references "Looper buttons" without specifying the actual parameter mapping

**Phase to address:** Phase 2 (first Ableton integration) — must verify parameter mapping before finalizing UI

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Using `CriticalSection` instead of lock-free queue | Faster to implement, simpler code | Priority inversion causing audio dropouts under load | Only in non-audio-thread code paths (editor, state loading) |
| Hardcoding Looper parameter names/indices | Quick integration, no abstraction layer | Breaks when Looper updates, doesn't support 3rd-party loopers | v1 only — must be replaced with parameter discovery before adding other looper types |
| Running IPC on the audio thread instead of message thread | Simpler code, no thread marshaling | Guaranteed audio dropouts, DAW freezes, potential crashes | **Never** — this must be message-thread-only from day one |
| Skipping state serialization (`getStateInformation`/`setStateInformation`) | Faster initial dev, less code to test | Plugin state lost on project save/load, no recall | Never for production — acceptable only in the very first proof-of-concept |
| Using string parameter IDs without `ParameterID` version hints | Slightly less verbose code | Automation breaks on format change, state recall fails across versions | Never — use `ParameterID` from day one |
| Single plugin format (VST3 only) for v1 | Cuts testing surface in half, faster to ship | macOS AU users can't use it; some DAWs prefer AU | Acceptable for v1 if documented as v1 limitation |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| Ableton Live VST3 | Assuming all plugin parameters auto-appear in Live's device panel | Keep parameters under 64; test in Configure Mode; verify parameter exposure |
| Ableton Live AU vs VST3 | Shipping both formats without cross-format state recall testing | Pick VST3 as primary for v1; test AU separately before shipping both |
| Ableton Looper parameters | Mapping UI buttons directly to conceptual actions (Record/Play) rather than actual Looper parameter values | Load Looper, enter Configure Mode, enumerate actual parameters with types and ranges before designing UI |
| JUCE AudioProcessorValueTreeState | Calling `getRawParameterValue()` result without `.load()` (it's `std::atomic<float>*`) | Always use `.load()` for thread-safe access; never dereference as raw `float*` |
| JUCE parameter listeners | Updating UI directly from `parameterValueChanged()` callback (may be on audio thread) | Use `AsyncUpdater` or `ChangeBroadcaster` to marshal updates to the message thread |
| IPC between plugin instances | Using TCP/UDP on audio thread for cross-instance communication | All networking/IPC must happen on the message thread; use atomic flags or lock-free queues to pass data between threads |
| Pluginval validation | Skipping plugin validation, assuming "it works in Live" is sufficient | Run Pluginval as part of CI; many crashes only appear under automated validation edge cases |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Allocating in `processBlock()` | Audio dropouts, CPU spikes, Ableton CPU meter red-lining | Pre-allocate all buffers in `prepareToPlay()`; use object pools for dynamic needs | As low as 2-3 tracks of loopers updating state simultaneously |
| String operations on audio thread | Intermittent clicks, pop sounds, CPU spikes | Never format strings, concatenate, or convert numbers in `processBlock()`; pre-format on message thread | Any string processing at audio rate (even logging) |
| Polling DAW state in `processBlock()` | Growing CPU load proportional to number of monitored loopers | Use event-driven model: only process when state changes (via parameter listeners), not every block | Above 4-5 looper instances being actively monitored |
| IPC message processing in audio callback | Glitches that get worse with network traffic | IPC reader/writer on message thread only; audio thread reads from pre-populated atomic/lock-free buffer | Immediately when cross-track communication is added |
| Timer-driven UI repaints at 60fps | Excessive CPU when no state is changing | Only repaint when state changes (use `repaint()` triggered by `AsyncUpdater`, not a timer) | Visible CPU usage on older laptops in live performance scenarios |
| Unbounded parameter listener cascades | Brief infinite loops or stack overflows when parameter A changes parameter B which changes parameter A | Design parameter dependency graph as DAG; use `ValueChangedCallback` guards to prevent re-entrant updates | When implementing looper state machine transitions |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Unauthenticated IPC between plugin instances | Malicious local process could inject fake looper state commands, causing unexpected audio behavior during live performance | Use loopback-only sockets (127.0.0.1) with session tokens generated at plugin load time |
| Sending MIDI CC values without validation | Out-of-range values could crash connected looper devices or cause unexpected state in Ableton Live | Validate all parameter values before sending; clamp to known ranges; handle edge cases (NaN, infinity) |
| Crashing plugin crashing the entire DAW | An unhandled exception in `processBlock()` takes down Ableton Live, losing all unsaved work | Use `try/catch` blocks around all non-trivial code in the audio callback (though C++ best practice is to not throw at all); validate inputs before audio thread |
| Storing session state in plaintext | Looper state data visible to other local users | Not a major concern for a local performance tool, but don't write API keys or credentials alongside state data |
| Bundling JUCE under AGPLv3 while selling commercially | Legal risk of forced source code disclosure via AGPLv3's copyleft provision | Obtain JUCE commercial license before any commercial distribution |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Requiring MIDI mapping for every looper manually | Users spend 20+ minutes setting up before each performance; mapping lost on track changes | Auto-detect loopers on tracks where the plugin is placed; provide clear one-time MIDI routing setup |
| Showing all loopers in a flat list regardless of track count | Overwhelming visual clutter with 8+ looper tracks; can't quickly find the right looper in performance | Group by track; show only active/armed loopers by default; allow filtering |
| No visual feedback on looper state transitions | Performer doesn't know if their command was received; double-taps commands | Immediate visual confirmation on state change; color-coded states (red=recording, green=playing, blue=overdubbing, gray=stopped) |
| Latency between button press and looper response | Commands arrive too late for musical rhythm; looper starts on wrong beat | Minimize round-trip: direct parameter write on audio thread for same-instance, lowest-latency IPC path for cross-instance |
| Controller assumes a specific looper parameter layout | Works with Ableton Looper but breaks with SooperLooper, Mobius, etc. | Implementation-agnostic adapter pattern: define `LooperAdapter` interface; ship with Ableton Looper adapter; plan for 3rd-party adapters |
| No offline/bus-offline UI state | When plugin window is closed, there's no way to see looper states | Ensure all critical state is in the AudioProcessor (visible via Ableton's parameter display in the device panel even with window closed) |

## "Looks Done But Isn't" Checklist

- [ ] **Cross-track communication:** Often missing in early prototypes — one plugin instance on one track can see its own looper but not others. Verify: can Track 1's plugin see Track 5's looper state?
- [ ] **Thread safety audit:** All shared state between audio thread and message thread uses atomics or lock-free queues, not mutexes. Verify: run with Pluginval's thread-safety validator.
- [ ] **State serialization:** `getStateInformation`/`setStateInformation` round-trips correctly. Verify: save Ableton set, close, reopen — all looper states restored?
- [ ] **Parameter format consistency:** Same `.als` project loads correctly with both VST3 and AU versions. Verify: create project with VST3, reload with AU (on macOS).
- [ ] **Editor destruction resilience:** Close plugin window, reopen — all state preserved. Verify: programmatic test that destroys/recreates editor.
- [ ] **Ableton Looper parameter mapping:** All Looper transport commands (play, record, overdub, stop, undo, clear) map correctly to actual Looper parameters. Verify: manual test in Ableton Live with a Looper device.
- [ ] **Denormal handling:** `juce::ScopedNoDenormals` used in `processBlock()`. Verify: no CPU spikes when looper state is idle (near-zero values).
- [ ] **Plugin format validation:** Plugin passes Pluginval (or at least Ableton's internal validation). Verify: run Pluginval against both VST3 and AU builds.
- [ ] **IPC latency measurement:** Cross-instance communication adds <5ms latency. Verify: instrumented test with timing markers.
- [ ] **Multi-instance conflict resolution:** Two plugin instances don't send conflicting commands to the same looper. Verify: place controller plugins on two tracks that both target the same looper.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Cross-track communication architecture wrong | **HIGH** — near-total rewrite of the plugin's communication model | Redesign with IPC bridge; each instance becomes a thin satellite; may need a companion "hub" standalone app |
| Thread safety violations in audio callback | **MEDIUM** — requires audit and refactoring of all audio-thread code paths | Identify all audio-thread code; replace mutexes with atomics/lock-free queues; add `ScopedNoDenormals`; test with Pluginval |
| Parameter IDs not future-proof | **LOW-MEDIUM** — requires state format migration but no architectural change | Add version parameter to `ParameterID`; implement migration in `setStateInformation`; ship both old and new parameter IDs temporarily |
| Editor state destruction issues | **LOW** — move state from editor to processor; straightforward refactoring | Create `ValueTree`-based state in processor; editor reads from it; destroy/recreate editor to verify |
|IPC protocol needs redesign | MEDIUM — requires protocol versioning and backward compatibility | Add version header to IPC messages; implement old→new migration; test with mixed-version instances |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Cross-track device discovery impossible via plugin API | Phase 1 (Architecture) | Write architecture doc that explicitly specifies communication mechanism (IPC, MIDI routing, or standalone hub); prove feasibility with spike |
| Audio thread blocking | Phase 1 (Architecture) | Code review of all audio-thread code paths; static analysis for allocations/locks; Pluginval thread-safety test |
| Parameter ID format mismatch | Phase 1 (when creating parameters) | Automated test: create parameters with `ParameterID`; save state as VST3; load as AU; verify values match |
| Ableton Configure Mode parameter visibility | Phase 2 (first Ableton integration) | Manual test: load plugin in Ableton; verify all parameters appear in device panel; verify MIDI mapping works |
| Editor lifecycle state loss | Phase 2 (UI implementation) | Automated test: destroy editor, recreate, assert all state preserved |
| Looper state machine mismatch | Phase 2 (Ableton integration) | Manual test: for each Looper state, verify controller triggers correct transition; test edge cases (empty→overdub, stopped→record) |
| JUCE AGPLv3 licensing | Phase 0 (pre-development) | Document licensing decision in project README; obtain JUCE commercial license if needed before first distribution |
| IPC on audio thread | Phase 3 (cross-instance communication) | Code review: verify all IPC/networking code runs on message thread only; never in `processBlock()` |
| Parameter listener cascade | Phase 2 (state machine implementation) | Unit test: trigger state transitions; assert no re-entrant listener calls; add guard flag |
| String operations on audio thread | Phase 1-3 (ongoing) | Static analysis / code review: grep `processBlock` for any string operations; move to message thread |
| Polling DAW state in audio callback | Phase 3 (state monitoring) | Profiler test: monitor CPU usage with 8+ looper instances; verify no per-block DAW queries |

## Sources

- JUCE AudioProcessorValueTreeState documentation and thread-safety notes (Context7, docs.juce.com)
- JUCE BREAKING_CHANGES.md — parameter ID hashing, VST3/AU compatibility issues (github.com/juce-framework/juce)
- JUCE `ParameterChangeForwarder` and `AudioProcessorListener` API — synchronous callback thread-safety requirements (docs.juce.com)
- Ableton Live 12 Reference Manual — Looper device behavior, plugin Configure Mode, VST/AU parameter exposure (ableton.com/en/live-manual)
- JUCE CMake plugin configuration — `IS_MIDI_EFFECT` flag, `FORMATS`, and `AU_MAIN_TYPE` (Context7, juce-framework/juce)
- Pamplejuce JUCE template project — build system best practices (github.com/sudara/pamplejuce)
- Ableton Live 12 MIDI Remote Scripts — transport controls, device parameter mapping, mixer control (github.com/gluon/abletonlive12_midiremotescripts)
- JUCE `AudioProcessor::isMidiEffect()` — MIDI-only plugin type handling (docs.juce.com)

---
*Pitfalls research for: Live Looper Controller DAW Plugin*
*Researched: 2026-04-15*