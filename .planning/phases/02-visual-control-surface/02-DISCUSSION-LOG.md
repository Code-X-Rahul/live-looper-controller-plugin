# Phase 2: Visual Control Surface - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-01
**Phase:** 02-visual-control-surface
**Areas discussed:** Panel Layout & Density, State Visualization & Color, Transport Controls & Interaction, Timing Display & Detail Depth

---

## Panel Layout & Density

| Option | Description | Selected |
|--------|-------------|----------|
| Vertical list | Each looper is a horizontal row. Simple, familiar, scrollable. Matches track mixer mental model. | ✓ |
| Compact grid | Loopers in grid tiles. Max 4-6 visible. More density, less room for controls. | |
| Adaptive layout | Switches between grid (few) and list (many). Complex. | |

**User's choice:** Vertical list
**Notes:** Familiar mental model, works with any looper count, scrolls naturally.

| Option | Description | Selected |
|--------|-------------|----------|
| Compact (~40-50px per row) | Track name + state + transport in one row. Expandable detail for advanced controls. | ✓ |
| Full info (~80-100px per row) | Everything visible. Needs more height. | |
| Two-tier | Essential always visible, tap to expand. Compromise. | |

**User's choice:** Compact row design
**Notes:** Keeps the panel clean. 5 loopers fit in ~250px. Expand for detail.

| Option | Description | Selected |
|--------|-------------|----------|
| Always visible with scroll | Scrollable viewport, fixed header. Simple, predictable. | ✓ |
| Viewport with page up/down | Page-based navigation. More complex. | |
| You decide | Implementation detail. | |

**User's choice:** Always visible with scroll
**Notes:** Fixed status bar at top, looper rows scroll naturally.

---

## State Visualization & Color

| Option | Description | Selected |
|--------|-------------|----------|
| Full-row color tint | Entire row background tinted by state color. State text reinforces. Unmistakable at a glance. | ✓ |
| State dot/badge only | Small colored circle at left edge. More minimal, requires focused attention. | |
| State-colored left border | Thick left border stripe. Like task managers. | |

**User's choice:** Full-row color tint
**Notes:** Unmistakable in peripheral vision. Matches the "at-a-glance" requirement.

| Option | Description | Selected |
|--------|-------------|----------|
| Snap change | Immediate color change. No animation. | ✓ |
| Subtle pulse on transition | Brief flash on state change, then settle. | |
| You decide | Implementation detail. | |

**User's choice:** Snap change
**Notes:** Performers need instant feedback. Animation can mask real state changes.

| Option | Description | Selected |
|--------|-------------|----------|
| Status bar at top | Fixed header with connection indicator. Pattern established in Phase 1. | ✓ |
| Per-looper stale state indicator | Individual staleness icons per row when disconnected. | |
| Both — bar + stale indicator | Most informative, slightly more complexity. | |

**User's choice:** Status bar at top
**Notes:** Continues Phase 1 pattern. Stale per-looper indicators deferred.

---

## Transport Controls & Interaction

| Option | Description | Selected |
|--------|-------------|----------|
| Multi-purpose button | Single button cycling through states per Ableton Looper. Matches muscle memory. | ✓ |
| Separate buttons (Record, Overdub, Play, Stop) | Four explicit buttons. Clearer but more space. Doesn't match Ableton workflow. | |
| Multi-purpose + Stop override | Primary cycling button plus dedicated Stop. Best of both. | |

**User's choice:** Multi-purpose button
**Notes:** Matches Ableton Looper's native behavior. Zero learning curve. Meets CTRL-03 requirement.

| Option | Description | Selected |
|--------|-------------|----------|
| Expandable detail | Undo/Redo and Feedback hidden, revealed by expanding looper row. | ✓ |
| Always visible in row | Small buttons always present. Makes compact layout busy. | |
| Footer controls panel | Selected looper's detail in shared bottom panel. Adds selection step. | |

**User's choice:** Expandable detail
**Notes:** Keeps compact layout clean. Secondary controls on demand.

| Option | Description | Selected |
|--------|-------------|----------|
| Ableton-native cycling | Tap cycles states mirroring Looper's transport button. Zero learning curve. | ✓ |
| Direct state commands | Each tap sends explicit command. More flexible but doesn't match native workflow. | |
| You decide | Implementation detail. | |

**User's choice:** Ableton-native cycling
**Notes:** Must mirror Ableton Looper's exact transport behavior for muscle memory compatibility.

---

## Timing Display & Detail Depth

| Option | Description | Selected |
|--------|-------------|----------|
| Compact: state + track name only; Detail: everything | Maximizes compact readability. Expand for loop length, cycle count, feedback. | ✓ |
| Compact: state + track + loop length; Detail: position + cycle + feedback | Slightly more info at a glance, less compact. | |
| Compact: state + track + length + position; Detail: cycle + feedback | Most info-dense, risks readability. | |

**User's choice:** Compact shows state + track name only; Detail shows everything
**Notes:** Clean compact rows. Loop length, cycle count, undo/redo, feedback all in expandable detail.

| Option | Description | Selected |
|--------|-------------|----------|
| No position tracking in v1 | Show loop length and cycle count only. Don't animate position. | ✓ |
| Static position (snapshot) | Show position at moment of state change. | |
| Real-time position tracking | Animate position continuously. High complexity, Phase 3 territory. | |

**User's choice:** No position tracking in v1
**Notes:** Position tracking requires continuous DAW sync — Phase 3 scope. Loop length (bars) and cycle count are sufficient.

| Option | Description | Selected |
|--------|-------------|----------|
| Show cycle count | Display how many times loop has played through. Track via state change events. | ✓ |
| Skip cycle count for now | Not critical for v1. Add in Phase 3. | |
| You decide | Low priority, implement if natural. | |

**User's choice:** Show cycle count
**Notes:** Useful for performers. Can be tracked from existing state change events.

---

## Agent's Discretion

- Exact pixel sizes, font choices, border radii, spacing values
- Color hex codes for state tints (specific shades)
- Dark vs light theme default
- Plugin default size and resize constraints
- Expand/collapse animation style
- Feedback slider styling and range labels
- Undo/redo button styling (icon vs text)
- Scrollbar styling and behavior
- Track name truncation for long names
- Empty state message when no loopers discovered

## Deferred Ideas

- Real-time bar position animation — Phase 3 (SYN-01/SYN-02)
- Per-looper stale state indicators — adds UI complexity for edge case
- MIDI CC mapping (CTRL-05) — Phase 3 scope
- Quantized action triggering (SYN-02) — Phase 3 scope
- Coordinated multi-looper start/stop (SYN-03) — Phase 3 scope
- Theme toggle — dark/light switching is out of scope for v1