# Phase 1: Foundation & Protocol - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-15
**Phase:** 01-foundation-protocol
**Areas discussed:** Protocol Design, Connection Handshake, Plugin Type & Build

---

## Protocol Design

| Option | Description | Selected |
|--------|-------------|----------|
| JSON with namespace/action | {ns, nsid, name, args} pattern — debuggable, versionable. Based on ableton.js. | ✓ |
| OSC-style address patterns | OSC addresses like /looper/3/state/set with typed arguments | |
| Hybrid: OSC addresses + JSON body | OSC address for routing, JSON string as argument for payload | |

**User's choice:** JSON with namespace/action (Recommended)
**Notes:** Following the ableton.js proven pattern. Human-readable, easy to debug with any JSON tool.

### Correlation

| Option | Description | Selected |
|--------|-------------|----------|
| UUID per request | Each request gets a unique UUID, echoed back in response. Proven by ableton.js. | ✓ |
| Sequence numbers | Incrementing counter per request. Lighter but fragile if packets arrive out of order. | |
| Fire-and-forget | No correlation — commands sent without expecting response. No confirmation. | |

**User's choice:** UUID per request (Recommended)

### State push model

| Option | Description | Selected |
|--------|-------------|----------|
| Full state on change | Send complete looper state on every change. Simple, no reconciliation needed. | ✓ |
| Delta-only updates | Only send what changed. More efficient but requires reconciliation logic. | |

**User's choice:** Full state on change (Recommended)

### Protocol versioning

| Option | Description | Selected |
|--------|-------------|----------|
| Version field in every message | "version": 1 in each message. Forward-compatible, check at any point. | ✓ |
| Version in handshake only | Exchange versions once at connect. Saves bytes per message. | |
| Agent's discretion | Let planning decide | |

**User's choice:** Version field in every message (Recommended)

---

## Connection Handshake

### Port allocation

| Option | Description | Selected |
|--------|-------------|----------|
| Known ports with range fallback | Plugin on 7010, Remote Script on 7011, with range fallback. Simple, debuggable. | ✓ |
| Dynamic ports with broadcast discovery | Bind any port, broadcast on multicast. Flexible but complex. | |
| Plugin binds dynamic, tells Remote Script | Plugin binds dynamic, registers with Remote Script on known port. | |

**User's choice:** Known ports with range fallback (Recommended)

### Startup behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Immediate handshake on load | Plugin sends hello on startup, Remote Script responds with version and discovery. Retries with backoff if no response. | ✓ |
| Lazy connect on first command | Only connect when user acts. Connection failures surface late. | |
| Auto-connect with periodic heartbeat | Heartbeat pings every N seconds. More resilient but overhead. | |

**User's choice:** Immediate handshake on load (Recommended)

### Reconnection behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-reconnect with exponential backoff | Plugin detects disconnect, enters reconnect loop. Preserves stale state until refresh. | ✓ |
| Manual reconnect required | Shows disconnected state, waits for user action. Bad for live performance. | |

**User's choice:** Auto-reconnect with exponential backoff (Recommended)

---

## Plugin Type & Build

### Plugin type

| Option | Description | Selected |
|--------|-------------|----------|
| Audio passthrough | Audio effect that passes audio through unchanged. Placed on any track with audio. | ✓ |
| MIDI effect (isMidiEffect=true) | MIDI-only effect. Only on MIDI tracks. Cleaner conceptually but restrictive. | |
| Agent's discretion | Let planning decide | |

**User's choice:** Audio passthrough (Recommended)

### Plugin formats for v1

| Option | Description | Selected |
|--------|-------------|----------|
| VST3 only | Works on macOS and Windows. Halves format testing. AU added later. | ✓ |
| AU + VST3 | Both formats on macOS. More complete but doubles format testing and parameter ID risks. | |
| Agent's discretion | Let planning decide | |

**User's choice:** VST3 only for v1 (Recommended)

### Minimum Ableton Live version

| Option | Description | Selected |
|--------|-------------|----------|
| Ableton Live 11+ | Stable VST3 support, Python 3. Broad compatibility. | ✓ |
| Ableton Live 12 only | Latest APIs, best Remote Script support. Cuts out Live 11 users. | |
| Ableton Live 10+ | Maximum compatibility. VST3 issues in Live 10. | |

**User's choice:** Ableton Live 11+ (Recommended)

---

## Agent's Discretion

- Looper discovery intelligence level (pattern-based vs class-name detection in Phase 1)
- Exact JSON message field names and type definitions
- Error code structure for protocol errors
- Heartbeat/ping interval during active connection
- Whether the plugin exposes any parameters to Ableton's device panel in Phase 1
- JUCE module selection beyond required set
- C++ project structure and directory layout
- Python Remote Script code organization

## Deferred Ideas

None — discussion stayed within phase scope.