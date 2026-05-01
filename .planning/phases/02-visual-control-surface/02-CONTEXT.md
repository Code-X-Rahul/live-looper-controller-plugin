# Phase 2: Visual Control Surface - Context

**Gathered:** 2026-05-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can see every looper's state at a glance and trigger record/overdub/play/stop from a single panel with real-time updates. Looper states are color-coded. Loop length, cycle count, undo/redo, and feedback are accessible per looper. State changes propagate from DAW to UI within 100ms.
</domain>

<decisions>
## Implementation Decisions

### Panel Layout & Density
- **D-01:** Vertical list layout — each looper is a horizontal row showing track name, state color tint, and transport button. Scales vertically with scrolling. Matches the mental model of a track mixer.
- **D-02:** Compact row design (~40-50px per row) — track name + state indicator + transport button in one row. 5 loopers fit in ~250px. Timing, feedback, and undo/redo controls are hidden by default and revealed by expanding a looper's detail section.
- **D-03:** Scrollable viewport with fixed connection status bar at the top. All loopers visible in a scrollable list. Status bar always pinned. Low looper count = no scroll needed. High looper count = natural vertical scroll.

### State Visualization & Color
- **D-04:** Full-row color tint — the entire looper row gets a subtle background tint matching its state (red=recording, orange=overdubbing, green=playing, gray=stopped). State text label reinforces the color. Unmistakable at a glance, even in peripheral vision.
- **D-05:** Snap color change on state transitions — no animation. Immediate transition between state colors. Performers need instant feedback; animation can mask real changes or feel like lag.
- **D-06:** Connection status shown in a fixed header bar at the top (green dot + "Connected" / red dot + "Reconnecting..."). Continues the pattern established in Phase 1 editor. No per-looper stale state indicators in v1.

### Transport Controls & Interaction
- **D-07:** Single multi-purpose transport button per looper row — cycles through states following Ableton Looper's native behavior: Stopped→Record→Play, tap to toggle overdub while playing, double-tap to stop. Matches existing Ableton muscle memory (CTRL-03 requirement).
- **D-08:** Undo/Redo and Feedback controls hidden by default, revealed by expanding the looper row's detail section. These are secondary controls used less frequently during performance. Keeps the compact layout clean.
- **D-09:** Ableton-native state cycling behavior — tap cycles through states mirroring Ableton Looper's exact transport button behavior. Zero learning curve for Ableton users. The Remote Script sends state-change commands that map directly to the Looper device's State parameter.

### Timing Display & Detail Depth
- **D-10:** Compact row shows only track name + state color tint + transport button. No timing info in the compact row. Maximizes readability at a glance.
- **D-11:** Expanded detail section shows loop length (bars), cycle count, undo/redo buttons, and feedback slider. Rich information available on demand without cluttering the default view.
- **D-12:** No real-time bar position tracking in v1 — showing loop length (bars) and cycle count is sufficient. Live position tracking requires continuous DAW position sync which is Phase 3 scope (SYNC requirements). Position tracking deferred.
- **D-13:** Show cycle count in expanded detail — count how many times a loop has played through by tracking play-state entry events. Useful for performers tracking loop repetitions.

### Agent's Discretion
- Exact pixel sizes, font choices, border radii, and spacing values
- Color hex codes for state tints (specific shades of red/orange/green/gray)
- Dark theme vs light theme default (JUCE LookAndFeel)
- Plugin default size and resize constraints
- How expand/collapse animation works (instant vs animated)
- Feedback slider styling and range labels
- Undo/redo button styling (icon vs text)
- Scrollbar styling and behavior
- Track name truncation for long names
- How to handle zero loopers (empty state message)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Architecture & Protocol
- `.planning/research/ARCHITECTURE.md` — Two-component hybrid architecture, shadow state model, UI layer component responsibilities, recommended project structure (includes UI/ directory with LooperTrackComponent, LooperGridView, TransportButton, StatusIndicator)
- `.planning/research/FEATURES.md` — Feature dependencies, transport control per looper, undo/redo per looper, feedback control per looper, multi-purpose transport button emulation
- `.planning/research/PITFALLS.md` — Pitfall 2 (blocking audio thread — all UI updates must be on message thread), Pitfall 3 (parameter ID mismatch — use ParameterID with version hint), Pitfall 5 (editor reads state from processor via Timer — never cache in editor)

### Project Requirements
- `.planning/REQUIREMENTS.md` — VIS-01 through VIS-04 (visibility requirements), CTRL-01 through CTRL-04 (control requirements)
- `.planning/PROJECT.md` — Core value (single surface for all loopers), constraints (no DSP, minimal UI only)

### Phase 1 Context (Locked Decisions)
- `.planning/phases/01-foundation-protocol/01-CONTEXT.md` — Protocol decisions (D-01 to D-10), shadow state model, BridgeClient OSC on message thread, all state in processor/editor reads via Timer/APVTS

### Stack
- `.planning/research/STACK.md` — JUCE 8, C++17, juce_gui_basics, juce_gui_extra for UI components

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/Model/LooperState.h` — Core data model with State enum (Stopped, Recording, Playing, Overdubbing), stateToString/stringToState converters, comparison operators for diff-based updates. Directly used by UI to render state.
- `src/Model/LooperTracker.h/.cpp` — Shadow state collection with map<trackId, LooperState>, change callback via std::function, getAllLoopers(), getLooper(), connection status. The UI will subscribe to changes via onStateChange().
- `src/Plugin/PluginProcessor.h/.cpp` — Holds APVTS and LooperTracker. Editor accesses both via processor reference. Current parameter layout only has "connected" bool — needs expansion for looper control parameters.
- `src/Plugin/PluginEditor.h/.cpp` — Current minimal editor with Timer-based polling (500ms), statusLabel_, looperCountLabel_, looperListLabel_. Entirely replaced in this phase.
- `src/Bridge/BridgeClient.h/.cpp` — OSC sender/receiver on message thread. sendCommand() sends JSON-over-OSC to Remote Script. Needs new command methods for state-change commands (record, overdub, play, stop, undo, redo, set_feedback).
- `src/Bridge/MessageProtocol.h/.cpp` — Message/Event structs with JSON serialization. createHello(), createDiscover() exist. Needs createCommand() methods for looper control actions.
- `src/Shared/ProtocolDefs.h` — Protocol constants (namespaces, command names, event names, state strings). Needs new command constants for looper actions.

### Established Patterns
- Shadow state model with diff-based updates — LooperTracker only notifies on actual state change
- All IPC on message thread — BridgeClient uses OSCReceiver::MessageLoopCallback, ensuring UI updates are safe
- Editor reads from processor — PluginEditor gets tracker via processor_.getLooperTracker(), timer refreshes (to be replaced with more responsive pattern)
- JSON-over-OSC protocol — All plugin↔script communication uses juce::OSCMessage with JSON string payload on /loopercontrol address
- Audio passthrough — processBlock is a no-op for audio

### Integration Points
- PluginEditor will be completely rewritten — rebuild from scratch using LooperTracker as data source
- LooperTracker needs new methods: cycle count tracking, state-change event dispatch for commands
- BridgeClient needs new command methods: sendLooperCommand(trackId, action) for record/play/stop/etc.
- MessageProtocol needs new message factories for looper control actions
- ProtocolDefs needs new command/event constants for looper actions
- APVTS parameter layout needs expansion if parameters are exposed to host (but per Phase 1 decision, most state lives in LooperTracker not APVTS)

</code_context>

<specifics>
## Specific Ideas

- The vertical list of horizontal rows matches how Ableton Live's own Session View shows clips — familiar mental model for the target user
- The multi-purpose transport button mirrors Ableton Looper's native single-button behavior, so users don't have to learn a new interaction pattern
- Expandable detail section follows the pattern of many DAW plugins where advanced controls are one click away
- Cycle count is practically useful for live looping performers to track loop repetitions without counting manually
</specifics>

<deferred>
## Deferred Ideas

- Real-time bar position animation — requires continuous DAW position sync (Phase 3 SYN-01/SYN-02)
- Per-looper stale state indicators when disconnected — useful but adds UI complexity for an edge case
- MIDI CC mapping from plugin UI (CTRL-05) — Phase 3 scope
- Quantized action triggering (SYN-02) — Phase 3 scope
- Coordinated multi-looper start/stop (SYN-03) — Phase 3 scope
- Preset/scene management — Phase 3 scope
- Dark/light theme switching — agent's discretion for default, but a theme toggle is out of scope for v1
</deferred>

---

*Phase: 02-visual-control-surface*
*Context gathered: 2026-05-01*