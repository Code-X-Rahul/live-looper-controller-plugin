# Live Looper Controller

## What This Is

A minimal AU/VST3 control surface plugin that gives musicians a single panel to see the state of and control all looper devices across multiple tracks in their DAW. Designed DAW-agnostic but shipping Ableton Live first, it eliminates the need to navigate scattered looper devices during a live performance — one interface, full control.

## Core Value

Perform a complete live looping set without touching the native DAW UI — see every looper's state and control record, overdub, play, and stop in real time from a single surface.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] User can see the state of all looper devices across multiple tracks in one panel (recording, overdubbing, playing, stopped)
- [ ] User can trigger record/overdub/play/stop for any looper from the panel
- [ ] User can control loopers regardless of looper type (native or third-party) on each track
- [ ] Plugin loads as AU/VST3 in Ableton Live
- [ ] Multiple looper states are visible and distinguishable at a glance (color/status indicators)
- [ ] Looper timing and sync across tracks is controllable from the panel
- [ ] Plugin UI is minimal — only essential controls, no unnecessary complexity

### Out of Scope

- DAW support beyond Ableton Live in v1 — architecture designed for extensibility, but only Ableton shipping
- Audio processing or DSP — this is a control surface, not an audio effect
- Built-in looper functionality — controls existing looper devices, doesn't replace them
- MIDI hardware controller mapping — software-only in v1

## Context

- Ableton Live ships with a built-in Looper device, but controlling multiple loopers across tracks requires navigating each track individually — impractical during live performance
- Third-party loopers (Mobius, SooperLooper, etc.) also scatter their controls across tracks
- Live looping performers need to see and control loop state in real time without breaking flow
- The plugin communicates with the DAW to discover and control looper parameters — no audio signal path needed
- AU/VST3 format ensures compatibility with Ableton Live (which supports VST3 and AU depending on platform)

## Constraints

- **Tech format**: Must be AU/VST3 plugin (C++ with JUCE framework is the standard approach)
- **DAW compatibility**: v1 targets Ableton Live only; architecture should not prevent future DAW support
- **Platform**: macOS and Windows (Ableton Live runs on both)
- **No audio DSP**: Plugin acts as a controller/MIDI effect, not an audio processor
- **Ableton Live 1.0+**: Must work with Ableton Live Intro (not just Suite)

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| AU/VST3 plugin format | Industry standard, works in Ableton Live and other DAWs | — Pending |
| DAW-agnostic architecture, Ableton-first | Future-proof design without overbuilding for v1 | — Pending |
| Control surface only (no DSP) | Looper devices already exist — this controls them, doesn't replace them | — Pending |
| Works with any looper on track | Maximizes flexibility — users aren't locked to a specific looper | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-15 after initialization*