# Requirements: Live Looper Controller

**Defined:** 2026-04-15
**Core Value:** Perform a complete live looping set without touching the native DAW UI — see every looper's state and control record, overdub, play, and stop in real time from a single surface.

## v1 Requirements

### Plugin Infrastructure

- [ ] **PLUG-01**: Plugin loads as AU/VST3 in Ableton Live on macOS and Windows
- [ ] **PLUG-02**: Ableton Live Remote Script bridge enumerates tracks with looper devices and exposes parameters via IPC
- [ ] **PLUG-03**: Looper device discovery identifies looper-like devices by parameter patterns (not just Ableton Looper by name)

### Visibility

- [ ] **VIS-01**: User can see state of all looper devices across multiple tracks in one panel (recording, overdubbing, playing, stopped)
- [ ] **VIS-02**: Looper states are color-coded for at-a-glance awareness during performance (red=recording, orange=overdubbing, green=playing, gray=stopped)
- [ ] **VIS-03**: Looper state changes propagate from DAW to UI in real time (<100ms latency)
- [ ] **VIS-04**: User can see loop length, bar position, and cycle count per looper

### Control

- [ ] **CTRL-01**: User can trigger record, overdub, play, and stop for any looper from the panel
- [ ] **CTRL-02**: User can undo/redo per looper from the panel
- [ ] **CTRL-03**: Multi-purpose transport button emulates Ableton Looper's single-button state cycling
- [ ] **CTRL-04**: User can adjust feedback/decay amount per looper from the panel
- [ ] **CTRL-05**: User can assign MIDI CCs to looper controls from the plugin UI

### Timing & Sync

- [ ] **SYNC-01**: User can see current BPM and playback status from DAW
- [ ] **SYNC-02**: User can trigger quantized looper actions snapped to beat/bar boundaries
- [ ] **SYNC-03**: User can coordinate start/stop across multiple loopers in sync

### Presets

- [ ] **PRESET-01**: User can save and recall looper configurations (track mappings, parameter states) per song

## v2 Requirements

### DAW Extensibility

- **DAW-02**: Plugin works in Logic Pro (AU plugin with Logic Remote Script bridge)
- **DAW-03**: Plugin works in REAPER and Bitwig (VST3 with respective bridge implementations)

### Extended Features

- **CTRL-06**: Mobile companion app for looper control via OSC/MIDI
- **AUTO-01**: Scriptable state change sequences for automated looper arrangements

## Out of Scope

| Feature | Reason |
|---------|--------|
| Built-in looper DSP/audio processing | Controller only — not replacing existing loopers |
| DAW-agnostic cross-track discovery via plugin API | Impossible by plugin sandbox design; requires DAW-specific bridge |
| Complex MIDI routing | DAWs already handle this; focus on the visual surface they don't provide |
| Audio recording/archival | DAWs handle audio recording; this is a real-time control surface |
| Automatic looper device creation/deletion | Users manage their own DAW setup; discovery-only approach |
| Pattern/arranger view | Real-time performance tool, not a sequencer |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| PLUG-01 | Phase 1 | Pending |
| PLUG-02 | Phase 1 | Pending |
| PLUG-03 | Phase 1 | Pending |
| VIS-01 | Phase 2 | Pending |
| VIS-02 | Phase 2 | Pending |
| VIS-03 | Phase 2 | Pending |
| VIS-04 | Phase 2 | Pending |
| CTRL-01 | Phase 2 | Pending |
| CTRL-02 | Phase 2 | Pending |
| CTRL-03 | Phase 2 | Pending |
| CTRL-04 | Phase 2 | Pending |
| CTRL-05 | Phase 3 | Pending |
| SYNC-01 | Phase 3 | Pending |
| SYNC-02 | Phase 3 | Pending |
| SYNC-03 | Phase 3 | Pending |
| PRESET-01 | Phase 3 | Pending |

**Coverage:**
- v1 requirements: 16 total
- Mapped to phases: 16
- Unmapped: 0 ✓

---
*Requirements defined: 2026-04-15*
*Last updated: 2026-04-15 after initial definition*