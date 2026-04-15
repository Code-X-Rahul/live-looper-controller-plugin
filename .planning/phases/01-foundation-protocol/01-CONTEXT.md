# Phase 1: Foundation & Protocol - Context

**Gathered:** 2026-04-15
**Status:** Ready for planning

<domain>
## Phase Boundary

Validate the two-component hybrid architecture end-to-end: plugin loads as VST3 in Ableton Live, the Python Remote Script discovers looper devices across all tracks via the Live API, and bidirectional UDP communication between the two components is proven with real looper state data flowing. No UI is required — debug logging or console output confirming received state is sufficient.
</domain>

<decisions>
## Implementation Decisions

### Communication Protocol
- **D-01:** JSON with namespace/action message format — each message is a JSON object with `{ns, nsid, name, args}` structure, following the ableton.js pattern. Human-readable, debuggable, versionable.
- **D-02:** UUID per request for request/response correlation — each request gets a unique UUID echoed back in the response. Removes ordering assumptions; proven by ableton.js.
- **D-03:** Full state push on change — the Remote Script sends the complete looper state (all fields) whenever anything changes. Simpler implementation, no state reconciliation needed, negligible overhead (~10 fields per looper).
- **D-04:** Version field in every message — `"version": 1` included in every message for forward compatibility. Both sides check version on handshake.

### Connection Handshake
- **D-05:** Known ports with range fallback — plugin binds port 7010, Remote Script binds port 7011. If a port is taken, try the next in a range (7010-7019). Simple, predictable, debuggable.
- **D-06:** Immediate handshake on load — plugin sends a "hello" message to the Remote Script on startup. Remote Script responds with its protocol version and triggers looper discovery. Plugin retries with exponential backoff if no response.
- **D-07:** Auto-reconnect with exponential backoff on disconnect — if the connection to the Remote Script is lost (Live restarts, script reloads), plugin automatically re-enters the connect/reconnect cycle. User sees "reconnecting" state in UI (Phase 2). Stale shadow state is preserved and shown, refreshed when connection restores.

### Plugin Configuration
- **D-08:** Audio passthrough plugin — plugin is an audio effect that passes audio through unchanged (`processBlock` is a no-op for audio). Can be placed on any audio or MIDI track in Ableton. More flexible placement than MIDI effect.
- **D-09:** VST3 only for v1 — ships VST3 format on both macOS and Windows. Halves format-related testing and avoids VST3/AU parameter ID mismatch issues. AU support can be added later.
- **D-10:** Minimum Ableton Live 11+ — Live 11 has stable VST3 support and Python 3. Remote Script uses Live 11+ APIs. Live 12 is preferred but not required.

### Agent's Discretion
- Looper discovery intelligence level — whether Phase 1 validates pattern-based discovery (PLUG-03) or starts with Ableton Looper class-name detection first
- Exact JSON message field names and type definitions — the schema details beyond the namespace/action pattern
- Error code structure for protocol errors
- Heartbeat/ping interval during active connection (if any)
- Whether the plugin exposes any parameters to Ableton's device panel in Phase 1
- JUCE module selection (beyond the required set for plugin + OSC + networking)
- C++ project structure and directory layout specifics
- Python Remote Script code organization and module split

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Architecture & Protocol
- `.planning/research/ARCHITECTURE.md` — Two-component hybrid architecture, shadow state model, request-response pattern, data flow diagrams, anti-patterns, build order dependencies
- `.planning/research/FEATURES.md` — Feature dependencies, MVP definition, Transport control per looper, Real-time state updates requirements
- `.planning/research/PITFALLS.md` — Cross-track pitfall, audio thread blocking, parameter ID mismatch, thread safety patterns, "looks done but isn't" checklist
- `.planning/research/SUMMARY.md` — Research synthesis, architecture approach, gaps to address (including UDP port discovery handshake)

### Project Requirements
- `.planning/REQUIREMENTS.md` — PLUG-01 (AU/VST3 load), PLUG-02 (Remote Script enumerates tracks), PLUG-03 (pattern-based looper discovery)
- `.planning/PROJECT.md` — Core value, constraints (must be AU/VST3, no audio DSP, v1 targets Ableton Live)

### Stack
- `.planning/research/STACK.md` — JUCE 8, C++17, CMake 3.22+, Python 3.9+, python-osc, Catch2, fmt

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- No existing source code — this is a greenfield project. Only planning artifacts and research documents exist.

### Established Patterns
- Research defines the architectural pattern: shadow state model, push-based updates (no polling), all IPC on message thread
- ableton.js project provides a proven reference for the JSON-over-UDP communication pattern with UUID correlation
- JUCE AudioProcessorValueTreeState pattern for parameter management (well-documented)
- JUCE DatagramSocket + Thread pattern for async UDP I/O (referenced in architecture research)

### Integration Points
- Live API (Python) — Remote Script accesses `Live.Application`, `Live.Song`, `Live.Track`, `Live.Device` objects to enumerate tracks, find looper devices, and listen for parameter changes
- JUCE OSC/UDP — Plugin uses JUCE's networking modules for localhost UDP communication
- Ableton Remote Scripts directory — Script must be installed in `~/Music/Ableton/User Library/Remote Scripts/LooperControl/`

</code_context>

<specifics>
## Specific Ideas

- The ableton.js project's message format (`{uuid, ns, nsid, name, args}`) is the reference implementation for the protocol pattern
- Port numbers 7010/7011 are suggested starting points (can be overridden in range)
- Phase 1 success criterion: plugin loads, Remote Script discovers loopers, state data flows bidirectionally — no visual UI required

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 01-foundation-protocol*
*Context gathered: 2026-04-15*