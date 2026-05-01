# Roadmap: Live Looper Controller

## Overview

Build a two-component hybrid control surface — a JUCE AU/VST3 plugin for the visual panel and a Python Remote Script bridge for DAW API access — that lets live performers see and control every looper device across all tracks from a single surface. The journey starts by validating the hardest technical risk (cross-track IPC architecture), then delivers the core visual control surface, and finally hardens it into a reliable performance tool with sync, presets, and MIDI mapping.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Foundation & Protocol** - Validate the two-component hybrid architecture end-to-end: plugin loads in Ableton, Remote Script discovers loopers, bidirectional UDP communication works
- [ ] **Phase 2: Visual Control Surface** - Users see every looper's state at a glance and control record/overdub/play/stop from a single panel with real-time updates
- [ ] **Phase 3: Sync, Presets & Performance** - Users perform complete live sets with timing sync, saved configurations, and MIDI controller mapping

## Phase Details

### Phase 1: Foundation & Protocol
**Goal**: The two-component hybrid architecture works end-to-end — plugin loads in Ableton Live, the Remote Script discovers looper devices across all tracks, and bidirectional communication over UDP is proven
**Depends on**: Nothing (first phase)
**Requirements**: PLUG-01, PLUG-02, PLUG-03
**Success Criteria** (what must be TRUE):
  1. Plugin loads as AU/VST3 in Ableton Live without errors and passes Pluginval validation
  2. Remote Script discovers tracks containing looper devices via Ableton Live API and reports them over UDP
  3. Plugin receives looper device state from Remote Script and logs it (debug output or console — no UI required yet)
  4. Looper devices are identified by parameter patterns, not just Ableton Looper by name — a third-party looper with matching parameter signatures is also discovered
**Plans**: TBD

### Phase 2: Visual Control Surface
**Goal**: Users can see every looper's state at a glance and trigger record/overdub/play/stop from a single panel with real-time updates
**Depends on**: Phase 1
**Requirements**: VIS-01, VIS-02, VIS-03, VIS-04, CTRL-01, CTRL-02, CTRL-03, CTRL-04
**Success Criteria** (what must be TRUE):
  1. All looper devices across tracks are visible in one panel with distinguishable state (recording, overdubbing, playing, stopped)
  2. Each looper's state is color-coded for at-a-glance recognition (red=recording, orange=overdubbing, green=playing, gray=stopped)
  3. User can trigger record, overdub, play, and stop for any looper from the panel, including multi-purpose transport button cycling
  4. Looper state changes in the DAW appear in the plugin UI within 100ms
  5. User can see loop timing details (length, bar position, cycle count), trigger undo/redo, and adjust feedback amount per looper
**Plans**: 2 plans
- [ ] 02-01-PLAN.md — Protocol, model, and bridge command extensions for looper control
- [ ] 02-02-PLAN.md — JUCE UI components, looper panel, and editor rebuild with reactive updates
**UI hint**: yes

### Phase 3: Sync, Presets & Performance
**Goal**: Users can perform a complete live looping set with timing precision, recall saved configurations between songs, and map hardware MIDI controllers
**Depends on**: Phase 2
**Requirements**: SYNC-01, SYNC-02, SYNC-03, PRESET-01, CTRL-05
**Success Criteria** (what must be TRUE):
  1. User can see current BPM and playback status from the DAW in the plugin panel
  2. User can trigger quantized looper actions that snap to beat/bar boundaries
  3. User can start and stop multiple loopers in a coordinated sync from the panel
  4. User can save a looper configuration and recall it later, restoring track mappings and parameter states
  5. User can assign MIDI CCs to looper controls from the plugin UI
**Plans**: TBD
**UI hint**: yes

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Foundation & Protocol | 4/4 | ✓ Complete | 2026-04-16 |
| 2. Visual Control Surface | 0/2 | Not started | - |
| 3. Sync, Presets & Performance | 0/? | Not started | - |
