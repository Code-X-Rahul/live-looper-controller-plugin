# Project Research Summary

**Project:** Live Looper Controller Plugin
**Domain:** DAW control surface plugin (AU/VST3) for multi-track looper management
**Researched:** 2026-04-15
**Confidence:** HIGH

## Executive Summary

This project is a DAW control surface plugin that provides a unified, aggregated view of all looper devices across multiple tracks in Ableton Live — something no existing tool offers. The core innovation is a single panel where a live performer can see and control every looper instance at a glance. Because VST/AU plugins are sandboxed to their own track and cannot discover or control devices on other tracks, the architecture requires a **two-component hybrid design**: a JUCE AU/VST3 plugin for the visual control surface, paired with an Ableton Live Remote Script (Python) that bridges DAW API access to the plugin via OSC over UDP localhost. This is the same pattern used by hardware controllers like Push and APC40, and is the only viable approach given fundamental plugin API constraints.

The recommended approach builds the JUCE plugin and Python Remote Script in parallel, connected by a JSON-over-UDP protocol defined first as a shared contract. The plugin maintains a shadow state of all looper devices (updated by the Remote Script) and renders the UI from local state only — never reaching across processes during rendering. All network I/O happens on a dedicated thread, never on the audio thread. The Remote Script uses Ableton's Live API to discover looper devices, listen for parameter changes via event listeners, and set parameters in response to plugin commands. V1 targets Ableton Live 12 on macOS (AU + VST3) with Windows (VST3) as a secondary format, using the Ableton Looper device as the sole supported looper type initially.

Key risks include: (1) the cross-track communication architecture is the hardest technical problem and must be validated early; (2) Ableton Looper's state machine is complex and doesn't map to simple toggle buttons — the UI must match actual device parameters; (3) thread safety violations on the audio thread will cause crashes that are hard to diagnose; (4) the JUCE 8 AGPLv3 license requires an early open-source vs. commercial decision; (5) parameter format mismatches between VST3 and AU require careful use of `juce::ParameterID` with version hints from day one.

## Key Findings

### Recommended Stack

The stack centers on JUCE 8 for the plugin framework, C++17 for the language, CMake for builds, and Python 3.9+ for the Ableton Remote Script. JUCE is the de facto standard for audio plugin development — there is no credible alternative for a cross-format AU/VST3 plugin with built-in OSC, GUI, and parameter management. The Remote Script must be Python because Ableton Live ships with an embedded Python interpreter and that is the only way to access its internal track/device API. Communication between the two components uses OSC over UDP localhost (via JUCE's `juce_osc` module and Python's `python-osc` library), proven fast enough (<1ms latency) by the ableton.js project.

**Core technologies:**
- **JUCE 8.0.12** — AU/VST3 plugin framework, provides GUI, parameter management, OSC, MIDI — no alternative comes close in maturity
- **C++17** — minimum required by JUCE 8; provides modern language features without limiting compiler support
- **CMake 3.22+** — officially recommended build system for JUCE 8; Projucer is deprecated
- **Python 3.9+** — required for Ableton Remote Script; Live 12 embeds Python 3.9, no choice here
- **python-osc** — OSC library for Python; pure Python, works in Ableton's embedded interpreter
- **JSON over UDP localhost** — communication protocol between plugin and Remote Script; debuggable, low-latency, language-agnostic

### Expected Features

**Must have (table stakes):**
- **Multi-looper state visibility** — aggregated view of all loopers across tracks; this IS the core product
- **Transport control per looper** — record, overdub, play, stop buttons per looper instance
- **Clear visual status indicators** — color-coded states for at-a-glance awareness during performance
- **Real-time state updates** — <100ms latency between DAW state change and UI update
- **Tempo/sync awareness** — display DAW BPM and playback status via AudioPlayHead
- **Plugin loads as AU/VST3** — must work in Ableton Live on macOS and Windows

**Should have (competitive):**
- **Single-panel aggregated view** — the core differentiator; no other tool offers this
- **Works with ANY looper device** — pattern-based discovery beyond Ableton's Looper (v1.x)
- **Quantized trigger timing** — snap actions to beat/bar boundaries
- **Undo/Redo per looper** — one-click recovery instead of Ableton's 2-second hold
- **Feedback control per looper** — expose the Feedback parameter for each looper

**Defer (v2+):**
- **Cross-looper sync management** — coordinated start/stop across multiple loopers
- **Preset/scene management** — save/recall looper configurations
- **DAW support beyond Ableton** — Logic, REAPER, Bitwig require custom bridge scripts
- **MIDI learn in plugin UI** — hardware controller mapping

### Architecture Approach

The architecture is a two-component hybrid: a JUCE plugin providing the control surface UI and a Python Remote Script providing DAW API access, communicating via JSON over UDP localhost. This is necessitated by the fundamental constraint that VST/AU plugins cannot access other tracks' devices. The plugin maintains a shadow state model of all discovered loopers, updated by the Remote Script's push events, and the UI reads only from this local state. The Remote Script uses Ableton's parameter listeners to detect state changes and pushes them to the plugin immediately (no polling). The protocol is designed DAW-agnostic so that future DAW support only requires a new bridge script, not changes to the plugin.

**Major components:**
1. **PluginProcessor/PluginEditor** (C++/JUCE) — audio passthrough, parameter hosting, visual control surface UI
2. **LooperModel/LooperTracker** (C++) — shadow state management, diff-based UI updates, change broadcasting
3. **BridgeClient + MessageProtocol** (C++) — UDP communication with Remote Script, connection management, JSON encode/decode
4. **Remote Script** (Python) — Live API access, track/device enumeration, parameter read/write, event listeners, UDP server

### Critical Pitfalls

1. **Cross-track device discovery impossible via plugin API** — Must use the Remote Script bridge architecture from day one; a naive plugin-only approach cannot work. Validate with a spike in Phase 1.
2. **Blocking the audio thread** — All IPC, networking, and string operations must happen on the message thread, never in `processBlock()`. Use `AsyncUpdater` and lock-free queues to pass data between threads.
3. **Parameter ID mismatch across VST3/AU formats** — Always use `juce::ParameterID` with version hints. Consider VST3-only for v1 to halve format-related bugs.
4. **Ableton Looper state machine doesn't match simple toggles** — The Looper's State parameter is a discrete integer (0=Stop, 1=Record, 2=Play, 3=Overdub), not separate boolean buttons. Study actual parameter layout in Configure Mode before finalizing UI.
5. **Editor lifecycle: state lives in the processor, not the editor** — All persistent state must be in `AudioProcessor`/`AudioProcessorValueTreeState`, not in editor member variables. The editor is destroyed when the plugin window closes.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Foundation & Protocol
**Rationale:** The JSON-over-UDP protocol is the contract shared between C++ and Python. Both sides depend on it, so it must be defined first. The JUCE plugin scaffold and Remote Script skeleton need to validate that the two-component architecture actually works end-to-end.
**Delivers:** Message protocol spec, JUCE plugin shell (loads in Ableton, no UI yet), Python Remote Script that can discover loopers and send state over UDP, proof-of-concept communication between the two.
**Addresses:** Multi-looper state visibility (discovery), plugin loads as AU/VST3, real-time state updates (communication layer)
**Avoids:** Cross-track architecture pitfall (validates the approach before building on it)

### Phase 2: State Management & Core UI
**Rationale:** With the protocol validated and communication working, build the shadow state model and the visual UI that renders it. This brings the product to its first demonstrable form: seeing all loopers at a glance with live state updates.
**Delivers:** LooperModel/LooperTracker (shadow state), LooperTrackComponent (single looper UI), LooperGridView (aggregated panel), status indicators with color coding, bidirectional communication (commands from UI → DAW, state updates from DAW → UI).
**Uses:** JUCE GUI modules, AudioProcessorValueTreeState, OSC
**Implements:** Shadow State pattern, UI components, transport controls
**Avoids:** Editor lifecycle pitfall (state in processor, not editor), Looper state machine mismatch (study parameters before building UI), Configure Mode parameter limits (keep under 64 params)

### Phase 3: Polish & Ableton Integration
**Rationale:** With core functionality working, harden the integration: state serialization (save/load), tempo display, thread safety audit, edge cases, and testing. This phase turns a prototype into a reliable performance tool.
**Delivers:** Full Ableton Live integration (save/recall), tempo/sync display, undo/redo per looper, feedback control per looper, Pluginval validation passed, thread safety audit complete, connection/reconnection handling.
**Avoids:** Audio thread blocking (audit all code paths), parameter ID format issues (verify cross-format), polling pitfalls (event-driven only), state serialization bugs

### Phase 4: Extended Features & Hardening
**Rationale:** After a solid v1, add features that differentiate the product and prepare for broader support. These are P2/P3 items from the feature matrix.
**Delivers:** Works with ANY looper device (pattern-based discovery), quantized trigger timing, looper timing display, MIDI learn in plugin UI, Windows VST3 testing, installer/package for Remote Script.
**Uses:** Multi-looper adapters, AudioPlayHead tempo sync

### Phase Ordering Rationale

- **Protocol first** because both C++ and Python sides depend on it — it's the integration contract that enables parallel development and must be validated before building anything on top of it.
- **State model and UI second** because the core value proposition (seeing all loopers at a glance) depends on the model being correct and the UI rendering it. This is where the shadow state pattern and the visual indicators come together.
- **Integration hardening third** because save/load, edge cases, and thread safety are critical for a live performance tool but can only be tested once the core loop works end-to-end.
- **Extended features fourth** because pattern-based looper discovery, quantized timing, and MIDI learn are valuable but not needed to validate the concept.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 1:** Remote Script bridge architecture — complex integration with Ableton's Live API; needs spike to validate OSC communication, device discovery, and parameter listener callbacks. The exact parameter IDs and state machine for Ableton's Looper device need runtime verification.
- **Phase 2:** Ableton Looper parameter mapping — the discrete State parameter (0-3) and multi-function transport button need careful study in Configure Mode to design correct UI controls.
- **Phase 4:** Multi-looper adapter pattern — each looper type (SooperLooper, Mobius) has different parameter layouts; needs research per looper type.

Phases with standard patterns (skip research-phase):
- **Phase 2:** JUCE UI components — well-documented, standard patterns from JUCE examples and tutorials.
- **Phase 3:** Thread safety and state serialization — established JUCE patterns (APVTS, AsyncUpdater, lock-free queues).
- **Phase 3:** Pluginval validation — standard tool with well-known checks.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | JUCE is the industry standard for plugin dev; no credible alternatives. Python for Remote Script is mandatory. All technologies verified with official docs. |
| Features | MEDIUM-HIGH | Feature set is clear; prioritization is well-supported by competitor analysis. Some uncertainty about Ableton Looper parameter behavior at runtime. |
| Architecture | MEDIUM | Two-component pattern is proven (ableton.js, hardware controllers). UDP/OSC communication is standard. Medium confidence in discovery mechanism (track reordering edge cases) and Looper parameter mapping (needs runtime verification). |
| Pitfalls | HIGH | Well-documented pitfalls from JUCE community and plugin development best practices. Cross-track limitation is a fundamental API constraint, not an assumption. |

**Overall confidence:** HIGH

### Gaps to Address

- **Ableton Looper exact parameter IDs and state transitions:** The research identified that the Looper's State parameter uses integer values 0-3, but the exact behavior of transitions (e.g., can you go directly from Stopped to Overdubbing?) and the multi-function transport button mapping need runtime verification. Address during Phase 1 spike by loading a Looper device in Live and enumerating all parameters via Configure Mode.
- **Remote Script installation UX:** The research describes installing the Remote Script in Ableton's User Library, but the exact user flow (how does a user install and activate the script? does it auto-connect to the plugin?) needs design work. Address during Phase 3.
- **UDP port conflict resolution:** The plugin and Remote Script both need to bind UDP ports on localhost. The research doesn't specify how port conflicts are handled or how the plugin and script discover each other's ports. Design a handshake/discovery protocol. Address in Phase 1 protocol definition.
- **Multi-instance conflict:** If two controller plugins are loaded on different tracks, both trying to control the same loopers could create conflicts. The research mentions this as a checklist item but doesn't prescribe a solution (primary/secondary instance, or only one controller per set). Address in Phase 3.

## Sources

### Primary (HIGH confidence)
- JUCE 8 official documentation — AudioProcessor, AudioProcessorValueTreeState, OSC module, CMake API
- Ableton Live 12 Reference Manual — Looper device behavior, Configure Mode, VST/AU parameter exposure
- Ableton Live 12 MIDI Remote Scripts (github.com/gluon/abletonlive12_midiremotescripts) — ControlSurface architecture, device parameter mapping
- Live API documentation (nsuspray.github.io/Live_API_Doc) — Python API for track/device/parameter access

### Secondary (MEDIUM confidence)
- ableton.js (github.com/leolabs/ableton-js) — production-proven UDP communication pattern with Live Remote Scripts
- JUCE BREAKING_CHANGES.md — parameter ID format issues between VST3 and AU
- Pamplejuce JUCE template project — build system best practices

### Tertiary (LOW confidence)
- Mobius looper — website unavailable during research; general feature knowledge from established community knowledge
- DAWs beyond Ableton — architecture is designed for extensibility but not tested with other DAWs

---
*Research completed: 2026-04-15*
*Ready for roadmap: yes*