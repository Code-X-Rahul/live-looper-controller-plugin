# Feature Research

**Domain:** Live looper controller DAW plugin
**Researched:** 2026-04-15
**Confidence:** MEDIUM-HIGH

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist. Missing these = product feels incomplete or unusable for its intended purpose.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **Multi-looper state visibility** | The entire point of the plugin is seeing all loopers at once; without this, there's no reason to use it | HIGH | Requires discovering looper devices across tracks and polling/observing their state parameters in real time. Architecture-agnostic discovery layer is critical. |
| **Transport control per looper** (record, overdub, play, stop) | Controlling looper state is the other half of the value proposition; a viewer without controls has limited utility | MEDIUM | Mapping to looper device parameters (Record, Overdub, Play, Stop buttons). Ableton Looper uses a single multi-purpose transport button; the plugin needs to decode/represent this as discrete actions. |
| **Clear visual status indicators** (color-coded states) | Live performers need at-a-glance awareness without reading text; this is how all looper hardware and software works | LOW | JUCE provides rich UI drawing capabilities. Color mapping for states (red=recording, orange=overdubbing, green=playing, gray=stopped) is straightforward. |
| **Real-time state updates** | During live performance, latency between looper state change and UI update is unacceptable; stale state = wrong decisions | MEDIUM | Requires efficient observation mechanism. Ableton's parameter listeners fire on change — need to propagate changes to plugin UI with minimal latency. Target: <50ms update latency. |
| **Tempo/sync awareness** | Loopers are tempo-dependent; performers need to see whether loopers are synced, how long loops are, and trigger quantized operations | MEDIUM | Get tempo from DAW host via AudioPlayHead. Display loop length in bars/beats. Show sync status per looper. |
| **Plugin loads as AU/VST3** | Must work in Ableton Live on both macOS (AU) and Windows (VST3) | LOW | Standard JUCE plugin build. Single codebase targets both formats. |

### Differentiators (Competitive Advantage)

Features that set the product apart. Not required, but valuable.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Single-panel aggregated view of ALL loopers** | No other tool provides a unified control surface for multiple looper instances across tracks. This IS the core differentiator. | HIGH | Requires inter-plugin communication or DAW API bridge. No VST3/AU plugin standard allows cross-track device access — must use a companion process. |
| **Works with ANY looper device on track** | Not locked to Ableton's built-in Looper — can control SooperLooper, Mobius, or any device that exposes record/play/stop parameters | MEDIUM | Requires a parameter mapping/discovery system that identifies looper-like devices by their parameter structure rather than by device name. Makes the tool adaptable. |
| **Quantized trigger timing** | Operations (record start, overdub toggle, etc.) snap to musical time — prevents timing errors during live performance | MEDIUM | Requires DAW tempo synchronization (available via JUCE AudioPlayHead). Ableton Looper has native quantize; plugin should expose quantize settings per-looper. |
| **Undo/Redo per looper from panel** | Undo/Redo is essential for live looping recovery; Ableton Looper supports it (hold 2 seconds on transport button) but it's hard to trigger reliably live | LOW | Map to the looper's Undo/Redo parameter. Ableton Looper exposes these as automatable parameters. |
| **Multi-purpose transport button emulation** | Ableton Looper uses a single button for state cycling (tap to switch record→play→overdub, double-tap to stop). A smart UI button that mimics this behavior is more intuitive than separate buttons. | LOW | Discrete state-change buttons are clearer for beginners; single smart button is faster for experienced users. Offer both. |
| **Looper timing display** (loop length, bar position, cycle count) | Seeing how long each loop is and where you are in the cycle helps performers time their actions | MEDIUM | Requires tracking DAW position relative to each looper's loop start. Ableton Looper doesn't expose position directly — need to compute from DAW position + loop length metadata. |
| **Cross-looper sync management** | Start/stop multiple loopers in sync, or see which loopers are in sync vs. free-running | HIGH | Complex sync logic. Requires understanding each looper's sync mode and coordinating start/stop across tracks. Defer to v1.x. |
| **Preset/scene management** | Save configurations of looper states for different songs in a setlist | MEDIUM | Store track-to-looper mappings and preferred parameter states. Persist with DAW project or external file. |
| **Feedback control per looper** | Adjusting the feedback/decay amount is how performers fade loops in real time | LOW | Map to each looper's Feedback parameter. Simple parameter pass-through. High impact for overdub workflow. |
| **MIDI CC mapping from plugin UI** | Let users assign MIDI CCs to looper controls directly from the plugin panel — foot pedal integration without DAW MIDI mapping | MEDIUM | JUCE handles MIDI input/output. Requires MIDI learn UI flow. Defer to v1.x but design parameter architecture to support it. |

### Anti-Features (Commonly Requested, Often Problematic)

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **Built-in looper DSP/audio processing** | "Why not just build a looper?" | Completely different product scope. Changes plugin from controller to instrument. Massively increases complexity (latency-sensitive audio buffering, crossfading, tempo engine). Duplicates what existing loopers do well. | Control existing looper devices — make their controls accessible, don't replace them. |
| **DAW-agnostic cross-track device discovery via plugin API** | "Shouldn't a VST3/AU plugin be able to enumerate all tracks?" | The VST3/AU plugin sandbox prohibits cross-track access. A plugin instance only knows its own parameters and audio. No plugin standard exposes the DAW's track/device tree. Attempting this via hacks (digging into host window handles, memory scanning) is fragile and version-dependent. | Use a DAW-specific companion (Python Remote Script for Ableton) that bridges DAW internals to the plugin via IPC. Design the bridge interface to be swappable per DAW. |
| **Full MIDI mapping UI with complex routing** | Power users want extensive MIDI routing, conditional triggers, etc. | Turns a performance tool into a mapping utility. Feature creep that distracts from core value. MIDI mapping is already available in DAWs and hardware controllers. | Support simple 1:1 MIDI CC mapping in v1. Let DAWs handle complex routing. Focus on the visual aggregation surface that DAWs don't provide. |
| **Audio recording/archival** | "Record my live looping session" | DAWs already record audio. Adding recording creates format lock-in and raises storage/latency concerns. | DAW's own recording handles this. Focus purely on real-time control and visibility. |
| **Automatic looper device creation/deletion** | "Add/remove looper devices from tracks" | Modifying DAW structure from a plugin is error-prone and semantically deep (what track? where in the chain?). Users should set up their sets manually for predictability. | Discovery-only approach: detect existing looper devices, let users manage their DAW setup. Only add devices the user explicitly configured. |
| **Pattern/arranger view** | "Sequence looper state changes" | This is a DAW arrangement feature, not a controller feature. Sequencing state changes requires timeline awareness and is a different product (like Ableton's Session view). | Real-time performance control only. Arrangement stays in the DAW. |

## Feature Dependencies

```
Multi-looper state visibility
    └──requires──> Looper device discovery (Ableton Remote Script / companion bridge)
                        └──requires──> Inter-process communication layer
                                             └──requires──> Plugin ↔ bridge protocol definition

Transport control per looper
    └──requires──> Multi-looper state visibility (must see loopers to control them)
    └──requires──> Looper device discovery

Clear visual status indicators
    └──requires──> Multi-looper state visibility (need state data to display)
    └──independent──> Can be built as simple JUCE component mock

Real-time state updates
    └──requires──> Inter-process communication layer (event stream from DAW bridge)
    └──requires──> JUCE async message thread handling

Tempo/sync awareness
    └──requires──> JUCE AudioPlayHead access (DAW tempo/position info)

Quantized trigger timing
    └──requires──> Tempo/sync awareness
    └──requires──> Transport control per looper

Works with ANY looper device
    └──requires──> Looper device discovery
    └──enhances──> Multi-looper state visibility (more useful when not Ableton-locked)

MIDI CC mapping from plugin UI
    └──requires──> Transport control per looper (midi triggers actions)
    └──conflicts──> DAW's own MIDI mapping (avoid fighting for same CCs)

Cross-looper sync management
    └──requires──> Quantized trigger timing
    └──requires──> Transport control per looper
    └──requires──> Tempo/sync awareness

Preset/scene management
    └──requires──> Multi-looper state visibility
    └──requires──> Transport control per looper
    └──independent──> Can be file/I/O only, doesn't need DAW bridge

Looper timing display
    └──requires──> Tempo/sync awareness
    └──requires──> Real-time state updates (for position tracking)
```

### Dependency Notes

- **Multi-looper state visibility requires looper device discovery:** The plugin cannot discover looper devices across tracks natively. An Ableton Live Remote Script (Python) must enumerate tracks → devices → parameters and communicate findings to the plugin via IPC (WebSocket, named pipe, or shared memory).
- **Inter-process communication layer is foundational:** Every feature that reads or writes DAW state depends on this bridge. It must be reliable, low-latency, and handle DAW project load/unload gracefully. The ableton-js project provides a proven pattern (Python script ↔ Node.js via UDP socket).
- **Real-time state updates require event stream:** Rather than polling, the bridge should push parameter change events to the plugin. Ableton's parameter listener system (add_value_listener) fires callbacks on change — the bridge should forward these events.
- **Quantized trigger timing depends on DAW tempo awareness:** JUCE's AudioPlayHead provides tempo and playback position, enabling time-quantized actions. The plugin must decide: quantize in the bridge (Python, using DAW position) or in the plugin (using AudioPlayHead data). In-plugin quantization is more accurate but must coordinate with bridge timing.
- **MIDI CC mapping conflicts with DAW mapping:** If the user has already mapped MIDI CCs via Ableton's MIDI Map mode, the plugin receiving the same CCs could trigger double-actions. The plugin should NOT intercept MIDI that's already mapped by the DAW, or at minimum should surface a conflict warning.
- **Works with ANY looper enhances visibility:** A parameter-based discovery system (detecting devices with "Record" + "Overdub" + "Play" parameter patterns) makes the tool universally useful, not locked to Ableton's Looper device class name.

## MVP Definition

### Launch With (v1)

Minimum viable product — what's needed to validate the concept.

- [x] **Plugin loads as AU/VST3 in Ableton Live** — No plugin = no product. JUCE scaffolding.
- [x] **Ableton Live Remote Script bridge** — Companion script that enumerates tracks with looper devices and exposes their parameters to the plugin via IPC. This is the hardest technical problem and must be validated early.
- [x] **Multi-looper state visibility** — Show all discovered loopers with their current state (recording, overdubbing, playing, stopped) in one panel. Color-coded, at-a-glance.
- [x] **Transport control per looper** — Record, overdub, play, stop buttons per looper. Bidirectional: pressing a button sends command to DAW; DAW state changes update the UI.
- [x] **Clear visual status indicators** — Color and icon differentiation for each looper state. No text-reading required during performance.
- [x] **Real-time state updates** — Parameter change events propagate from DAW → bridge → plugin UI with <100ms latency.
- [x] **Tempo display from DAW** — Show current BPM and playback status from AudioPlayHead.

### Add After Validation (v1.x)

Features to add once core is working.

- [ ] **Works with ANY looper device** — Pattern-based device discovery (not just Ableton Looper by name). Trigger: once Ableton Looper control is validated, expand discovery.
- [ ] **Feedback control per looper** — Expose each looper's Feedback parameter as a knob. Trigger: users request more control beyond transport.
- [ ] **Undo/Redo per looper** — Map to looper's Undo/Redo parameters. Trigger: live performers need recovery mechanisms.
- [ ] **Quantized trigger timing** — Snap actions to beat/bar boundaries. Trigger: users report timing inconsistency.
- [ ] **Looper timing display** — Show loop length in bars, current position. Trigger: users need visual timing reference.
- [ ] **MIDI learn in plugin UI** — Simple 1:1 MIDI CC assignment for looper controls. Trigger: hardware controller users want direct mapping.

### Future Consideration (v2+)

Features to defer until product-market fit is established.

- [ ] **Cross-looper sync management** — Coordinated start/stop across multiple loopers. Complex tempo/phase math. Defer because v1 validates single-looper control first.
- [ ] **Preset/scene management** — Save/recall looper configurations. Defer because the manual setup workflow needs validation before automating.
- [ ] **DAW support beyond Ableton** — Logic, REAPER, Bitwig. Requires per-DAW bridge implementations. Defer because v1 must nail Ableton first.
- [ ] **Mobile companion app** — Control surface on phone/tablet via OSC/MIDI. Defer because JUCE plugin UI on desktop is sufficient for v1.
- [ ] **Scriptable automation** — Let users define state change sequences. Defer because it's a different product direction (more arranger than controller).

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Plugin AU/VST3 load + JUCE scaffold | HIGH | LOW | P1 |
| Ableton Remote Script bridge (IPC layer) | HIGH | HIGH | P1 |
| Multi-looper state visibility (aggregated view) | HIGH | MEDIUM | P1 |
| Transport control per looper (record/overdub/play/stop) | HIGH | MEDIUM | P1 |
| Clear visual status indicators | HIGH | LOW | P1 |
| Real-time state updates | HIGH | HIGH | P1 |
| Tempo/sync awareness from DAW | MEDIUM | LOW | P2 |
| Feedback control per looper | MEDIUM | LOW | P2 |
| Undo/Redo per looper | MEDIUM | LOW | P2 |
| Works with ANY looper device | HIGH | MEDIUM | P2 |
| Quantized trigger timing | MEDIUM | MEDIUM | P2 |
| Looper timing display | MEDIUM | MEDIUM | P3 |
| MIDI learn in plugin UI | MEDIUM | MEDIUM | P3 |
| Cross-looper sync management | MEDIUM | HIGH | P3 |
| Preset/scene management | LOW | MEDIUM | P3 |

**Priority key:**
- P1: Must have for launch
- P2: Should have, add when possible
- P3: Nice to have, future consideration

## Competitor Feature Analysis

| Feature | Ableton Live Built-in Looper | SooperLooper | Mobius | Our Approach |
|---------|-------------------------------|-------------|--------|-------------|
| Multi-track aggregated view | ❌ — per-track only, one Looper per track | ✅ — can manage multiple loops from single GUI | ✅ — scriptable multi-loop management | ✅ — this IS our product; aggregated view of any looper on any track |
| State visibility at glance | ⚠️ — need to click each track to see Looper state | ✅ — GUI shows all loop states | ✅ — GUI shows all loop states | ✅ — single panel showing all loopers with color-coded states |
| Transport controls | ✅ — record/overdub/play/stop per Looper | ✅ — extensive transport + multiply/insert/replace | ✅ — record/overdub/multiply/insert/replace | ✅ — per-looper transport; leverages existing looper's capabilities |
| Undo/Redo | ⚠️ — hold transport button 2 sec (hard to trigger live) | ✅ — unlimited undo/redo | ✅ — extensive undo history | ✅ — map to looper's own undo/redo; make it one-click |
| Feedback control | ✅ — feedback knob on each Looper | ✅ — feedback per loop | ✅ — feedback per track | ✅ — expose and aggregate per-looper feedback |
| Tempo sync | ✅ — syncs to song tempo | ✅ — MIDI/JACK/tap tempo sync | ✅ — MIDI sync, quantize | ✅ — reads DAW tempo; lets each looper's native sync handle timing |
| Multi-channel loops | ❌ — one stereo Looper per track | ✅ — multi-channel loops per instance | ✅ — multi-channel loops | N/A — we don't process audio; we control existing looper instances |
| OSC support | ❌ | ✅ — full OSC control | ⚠️ — limited | ⚠️ — v2 consideration; bridge uses IPC internally |
| Hardware MIDI control | ⚠️ — via DAW MIDI mapping (cumbersome per-track) | ✅ — fully MIDI mappable | ✅ — fully MIDI mappable | ✅ — per-looper controls accessible via simple MIDI mapping |
| DAW integration | ✅ — native Ableton device | ⚠️ — JACK/AU, requires routing setup | ⚠️ — VST/AU, not tightly integrated | ✅ — deep Ableton integration via Remote Script; designed for the DAW workflow |
| Cross-loop sync | ❌ — each Looper syncs to song independently | ✅ — loops can sync to each other | ✅ — sync modes between loops | ⚠️ — v2; observe each looper's sync state and coordinate start/stops |

**Key competitive insight:** No existing tool provides an aggregated multi-track looper control surface. SooperLooper and Mobius both manage multiple loops, but only within their own looper instances. Ableton's built-in Looper has no cross-track awareness. Our product occupies a unique niche: **observing and controlling any looper on any track from one panel.**

## Sources

- Ableton Live 12 Reference Manual — Looper device documentation (HIGH confidence)
- SooperLooper README and feature list (http://essej.net/sooperlooper/) (HIGH confidence)
- Ableton Live 12 MIDI Remote Scripts (github.com/gluon/abletonlive12_midiremotescripts) — device parameter enumeration, MIDI CC mapping (HIGH confidence)
- ableton-js (github.com/leolabs/ableton-js) — proven pattern for DAW ↔ external control via Python Remote Script + UDP socket (HIGH confidence)
- JUCE AudioPlayHead and AudioProcessorValueTreeState documentation (juce.com) (HIGH confidence)
- Live API documentation (nsuspray.github.io/Live_API_Doc) — Python API for track/device/parameter access (HIGH confidence)
- Mobius looper — website unavailable during research; feature list from established knowledge (LOW confidence for specific details, MEDIUM for general feature set)
- VST3/AU plugin sandbox constraint — well-documented limitation; plugin cannot access other tracks' devices (HIGH confidence)

---
*Feature research for: live looper controller DAW plugin*
*Researched: 2026-04-15*