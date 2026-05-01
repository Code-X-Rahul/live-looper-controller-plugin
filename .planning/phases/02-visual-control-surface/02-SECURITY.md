---
phase: 02
slug: visual-control-surface
status: verified
threats_open: 0
asvs_level: 1
created: 2026-05-01
---

# Phase 02 — Security

> Per-phase security contract: threat register, accepted risks, and audit trail.

---

## Trust Boundaries

| Boundary | Description | Data Crossing |
|----------|-------------|---------------|
| Plugin ↔ Remote Script (localhost UDP) | OSC messages between plugin and Remote Script on localhost | JSON-serialized looper state, commands |
| User click → OSC command → Remote Script → Live API | User triggers DAW parameter changes via button clicks | State-change commands (record/play/stop/overdub), undo/redo, feedback |

---

## Threat Register

| Threat ID | Category | Component | Disposition | Mitigation | Status |
|-----------|----------|-----------|-------------|------------|--------|
| T-02-01 | Tampering | BridgeClient OSC input | mitigate | JSON structure validated in oscMessageReceived: message size check, parse fallback to handleResult if jsonToEvent fails, defaults for missing fields | closed |
| T-02-02 | Tampering | Remote Script command input | mitigate | Command handlers validate looper_info is not None before access, return error response for unknown track_id, device_idx cast protected by try/except | closed |
| T-02-03 | Information Disclosure | localhost UDP listener | accept | Plugin binds to 127.0.0.1 only — remote network access impossible. UDP messages carry only looper state (no credentials, no sensitive data) | closed |
| T-02-04 | Tampering | TransportButton click → BridgeClient command | accept | Click is user-initiated and valid by definition. trackId originates from LooperTracker data (validated from Remote Script discovery) | closed |
| T-02-05 | Denial of Service | Rapid state updates flooding UI repaint | mitigate | LooperTracker diff-based update only notifies on actual state change. 50ms Timer throttle prevents update flooding. State change callback triggers repaint only when needed | closed |
| T-02-06 | Elevation of Privilege | Feedback slider out-of-range value | mitigate | juce::Slider constrains range to 0.0-1.0 at UI level. BridgeClient::sendSetFeedback sends clamped value. Remote Script _set_looper_param uses float cast with exception handling | closed |

*Status: open · closed*
*Disposition: mitigate (implementation required) · accept (documented risk) · transfer (third-party)*

---

## Accepted Risks Log

| Risk ID | Threat Ref | Rationale | Accepted By | Date |
|---------|------------|-----------|-------------|------|
| R-02-01 | T-02-03 | localhost UDP listener acceptable for local-only plugin↔script IPC. No credentials or sensitive data transmitted. | system | 2026-05-01 |
| R-02-02 | T-02-04 | TransportButton clicks are user-initiated, trackId is validated looper identifier from discovery protocol | system | 2026-05-01 |

*Accepted risks do not resurface in future audit runs.*

---

## Security Audit Trail

| Audit Date | Threats Total | Closed | Open | Run By |
|------------|---------------|--------|------|--------|
| 2026-05-01 | 6 | 6 | 0 | orchestrator |

---

## Sign-Off

- [x] All threats have a disposition (mitigate / accept / transfer)
- [x] Accepted risks documented in Accepted Risks Log
- [x] `threats_open: 0` confirmed
- [x] `status: verified` set in frontmatter

**Approval:** verified 2026-05-01