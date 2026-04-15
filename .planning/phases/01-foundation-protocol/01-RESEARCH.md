# Phase 1: Foundation & Protocol - Research

**Researched:** 2026-04-15
**Domain:** JUCE plugin scaffold, OSC protocol design, Ableton Remote Script integration, end-to-end architecture validation
**Confidence:** HIGH

## Summary

Phase 1 validates the two-component hybrid architecture end-to-end: a JUCE VST3 plugin that loads in Ableton Live and a Python Remote Script that discovers looper devices across tracks via the Live API, communicating over UDP. This is the highest-risk phase — if cross-track IPC doesn't work, the entire project architecture fails.

The JUCE plugin scaffold is straightforward using `juce_add_plugin()` in CMake with `IS_MIDI_EFFECT FALSE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT TRUE`, and `FORMATS VST3` (v0.1 targets VST3 only per D-09). The plugin must be an audio passthrough (processBlock is a no-op for audio) per D-08. JUCE's `juce_osc` module provides `OSCSender` and `OSCReceiver` for localhost UDP communication, verified for this use case.

The Ableton Remote Script must follow Ableton's `ControlSurface` base class pattern, registering as a MIDI Remote Script in `__init__.py` with proper `define_class()` and `create_instance()` entry points. The script accesses Live's Song/Track/Device hierarchy through the `self.song()` API, registers parameter listeners for state change detection, and communicates with the JUCE plugin via python-osc's `ThreadingOSCUDPServer`.

The protocol follows the ableton.js pattern (JSON with `{uuid, ns, nsid, name, args}` structure) over OSC addresses, with UUID for request/response correlation and version field for forward compatibility per D-01/D-02/D-04. The handshake uses known ports with range fallback (7010-7019) per D-05, with immediate handshake on load per D-06 and auto-reconnect per D-07.

**Primary recommendation:** Build the protocol definition first as it is the contract shared by both C++ and Python sides, then develop the JUCE plugin scaffold and Remote Script skeleton in parallel, testing end-to-end communication with a minimal message before adding looper discovery.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** JSON with namespace/action message format — each message is a JSON object with `{ns, nsid, name, args}` structure, following the ableton.js pattern. Human-readable, debuggable, versionable.
- **D-02:** UUID per request for request/response correlation — each request gets a unique UUID echoed back in the response. Removes ordering assumptions; proven by ableton.js.
- **D-03:** Full state push on change — the Remote Script sends the complete looper state (all fields) whenever anything changes. Simpler implementation, no state reconciliation needed, negligible overhead (~10 fields per looper).
- **D-04:** Version field in every message — `"version": 1` included in every message for forward compatibility. Both sides check version on handshake.
- **D-05:** Known ports with range fallback — plugin binds port 7010, Remote Script binds port 7011. If a port is taken, try the next in a range (7010-7019). Simple, predictable, debuggable.
- **D-06:** Immediate handshake on load — plugin sends a "hello" message to the Remote Script on startup. Remote Script responds with its protocol version and triggers looper discovery. Plugin retries with exponential backoff if no response.
- **D-07:** Auto-reconnect with exponential backoff on disconnect — if the connection to the Remote Script is lost (Live restarts, script reloads), plugin automatically re-enters the connect/reconnect cycle. User sees "reconnecting" state in UI (Phase 2). Stale shadow state is preserved and shown, refreshed when connection restores.
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

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PLUG-01 | Plugin loads as AU/VST3 in Ableton Live on macOS and Windows | JUCE `juce_add_plugin()` CMake configuration, VST3 format, audio passthrough processor |
| PLUG-02 | Ableton Live Remote Script bridge enumerates tracks with looper devices and exposes parameters via IPC | ControlSurface base class, Live API track/device enumeration, python-osc UDP server |
| PLUG-03 | Looper device discovery identifies looper-like devices by parameter patterns (not just Ableton Looper by name) | Pattern-based parameter match: check for Record + Play + Stop or equivalent parameter signatures |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|-----------|-------------|----------------|-----------|
| Plugin lifecycle (load, initialize, state) | JUCE Plugin (C++) | — | Plugin is the host-loaded component; it manages its own lifecycle |
| Looper discovery & enumeration | Remote Script (Python) | — | Only the Remote Script has access to Live's track/device API; plugin cannot see other tracks |
| Bidirectional communication | Both (shared protocol) | — | Protocol is the contract; C++ uses JUCE OSC, Python uses python-osc |
| State push on change | Remote Script (Python) | — | Remote Script subscribes to Live API parameter listeners and pushes changes to plugin |
| Shadow state management | JUCE Plugin (C++) | — | Plugin maintains local mirror of all looper states; UI reads from this (Phase 2) |
| Audio passthrough | JUCE Plugin (C++) | — | processBlock is a no-op for audio; plugin passes through unmodified |
| Connection handshake & reconnect | JUCE Plugin (C++) | — | Plugin initiates connection; Remote Script responds. Plugin handles retry logic. |
| Looper parameter mapping | Remote Script (Python) | — | Script resolves abstract parameter names to Live API parameter indices; plugin only sees string IDs |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.12 | AU/VST3 plugin framework | De facto standard. Verified as latest stable release (Dec 2025). Provides AudioProcessor, APVTS, OSC, GUI — everything needed. [VERIFIED: GitHub releases] |
| C++ | 17 | Plugin language | JUCE 8 minimum. Provides `std::optional`, `std::variant`, structured bindings. [CITED: JUCE docs] |
| CMake | 3.22+ | Build system | JUCE 8 official build system. `juce_add_plugin()` provides first-class plugin format support. [CITED: JUCE CMake API docs] |
| Python 3 | 3.9+ | Remote Script | Ableton Live 12 embeds Python 3.9. This is mandatory — no alternative exists. [ASSUMED: Live 12 embeds 3.9, verify with Ableton docs] |
| python-osc | 1.9+ | OSC in Remote Script | Pure Python OSC library. Only viable option for Ableton's embedded Python (no C extensions). Works with `ThreadingOSCUDPServer`. [VERIFIED: PyPI] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| JUCE `juce_osc` module | (included in JUCE 8) | OSC send/receive for plugin ↔ Remote Script | Always — this is the primary IPC channel |
| JUCE `juce_audio_processors` module | (included in JUCE 8) | AudioProcessorValueTreeState for parameter management | Always — standard JUCE parameter state management |
| JUCE `juce_data_structures` module | (included in JUCE 8) | ValueTree for shadow state and serialization | Always — LooperModel needs structured state storage |
| Catch2 | 3.x | C++ unit testing | During development — for Model and Bridge unit tests |
| fmt | 11.x | C++ string formatting and logging | During development — better than `DBG()` macro for debug output in Phase 1 |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|-----------|-----------|----------|
| JUCE OSC module | Raw UDP sockets (juce::DatagramSocket) | OSC adds address pattern matching and type tagging. Raw sockets are simpler but you'd hand-roll message framing. Stick with JUCE OSC — it's already linked and provides structure. |
| python-osc | Raw UDP sockets in Python | python-osc handles OSC message encoding/decoding, dispatch, and threading. Re-implementing this adds no value. Always use python-osc. |
| JSON over OSC | Binary protocol over UDP | Binary would be more compact but JSON is debuggable and human-readable (per D-01 decision). The overhead is negligible for the message sizes we're sending. |
| Ableton ControlSurface | Ableton Live API via MIDI Remote Scripts (new API in Live 11+) | Live 11+ introduced a new Python MIDI Remote Script API that's more structured. However, the classic `ControlSurface` pattern is better documented and more widely used. For Phase 1, use the established pattern; evaluate the new API later. [ASSUMED: classic API is sufficient, needs verification] |

**Installation:**
```bash
# JUCE as submodule
git submodule add https://github.com/juce-framework/JUCE.git third_party/JUCE

# python-osc bundled in Remote Script directory
# (Ableton's embedded Python doesn't support pip — copy source directly)
cp -r python_osc/osc/ remote-script/python_osc/osc/

# CMake build (macOS)
cmake -Bbuild -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build --config Debug

# CMake build (Windows)
cmake -Bbuild -G "Visual Studio 17 2022"
cmake --build build --config Debug
```

**Version verification:**
- JUCE 8.0.12 — confirmed latest stable release (December 2025) [VERIFIED: GitHub releases page]
- python-osc 1.9.3 — latest stable on PyPI [VERIFIED: PyPI]
- CMake 3.22 minimum required by JUCE 8 [CITED: JUCE CMake API docs]

## Architecture Patterns

### System Architecture Diagram

```
┌───────────────────────────────────────────────────────────────────┐
│                    JUCE Plugin Process                            │
│  ┌─────────────┐  ┌─────────────────┐  ┌──────────────────────┐  │
│  │ PluginProcessor │  │ LooperModel      │  │ BridgeClient         │  │
│  │ (AudioProcessor)│  │ (Shadow State)   │  │ (OSC Sender/Receiver)│  │
│  │ - processBlock  │  │ - loopers map    │  │ - send command       │  │
│  │   (passthrough) │  │ - updateState()  │  │ - receive state      │  │
│  │ - APVTS         │  │ - change broadcast│  │ - handshake          │  │
│  └─────────────────┘  └────────┬────────┘  └──────────┬──────────┘  │
│                                │                       │             │
│  ┌─────────────────────────────┘                       │             │
│  │  Message Thread (JUCE)                              │             │
│  │  ┌──────────────────────────────────────────────────┐│             │
│  │  │ OSCReceiver::addListener → handleIncomingOSC()  ││             │
│  │  │ OSCSender::send → outgoing OSC messages         ││             │
│  │  └──────────────────────────────────────────────────┘│             │
└────────┼────────────────────────────────────────────────┼─────────────┘
         │  UDP localhost:7010 ←→ 7011                    │
         │  (OSC over UDP)                                │
┌────────┼────────────────────────────────────────────────┼─────────────┐
│        │      Ableton Live Process                      │             │
│  ┌─────┴────────────────────────────────────────────────┴──────────┐  │
│  │  Remote Script: LooperControlSurface (Python)                   │  │
│  │  ┌──────────────────┐  ┌─────────────────┐  ┌────────────────┐ │  │
│  │  │ BridgeServer      │  │ LooperDiscovery  │  │ LiveAPIWrapper │ │  │
│  │  │ (OSC Server)      │  │ (Track/Device    │  │ (Parameter     │ │  │
│  │  │ python-osc        │  │  Enumeration)    │  │  Read/Write)   │ │  │
│  │  └──────────────────┘  └──────────────────┘  └────────────────┘ │  │
│  │           │                      │                      │         │  │
│  │           └──────────────────────┴──────────────────────┘         │  │
│  │                    Live API: self.song()                          │  │
│  │                    .tracks → .devices → .parameters               │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  Ableton Live Internal: Track 1 [Looper] Track 2 [Looper] ...    │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────────────┘
```

### Recommended Project Structure

```
live-looper-controller/
├── CMakeLists.txt                    # Root build config with JUCE submodule
├── src/
│   ├── Plugin/
│   │   ├── PluginProcessor.h         # AudioProcessor subclass (passthrough)
│   │   ├── PluginProcessor.cpp       # processBlock (no-op), state save/load
│   │   ├── PluginEditor.h            # Minimal editor (Phase 1: debug output)
│   │   └── PluginEditor.cpp          # Editor skeleton
│   ├── Model/
│   │   ├── LooperState.h             # Data model for single looper state
│   │   ├── LooperTracker.h           # Collection + discovery state manager
│   │   └── LooperTracker.cpp         # Shadow state update, change broadcast
│   ├── Bridge/
│   │   ├── BridgeClient.h            # OSC client for plugin → script comm
│   │   ├── BridgeClient.cpp          # Connection, handshake, send/receive
│   │   ├── MessageProtocol.h         # Message format definitions (shared)
│   │   └── MessageProtocol.cpp       # JSON encode/decode, version handling
│   └── Shared/
│       └── ProtocolDefs.h            # Port numbers, version, namespace constants
├── remote-script/
│   ├── __init__.py                   # define_class() / create_instance() entry
│   ├── LooperControlSurface.py       # Main ControlSurface class (OSC server)
│   ├── LooperDiscovery.py             # Track/device enumeration + pattern match
│   ├── BridgeServer.py               # python-osc UDP server
│   └── LiveAPIWrapper.py             # Live API parameter read/write/listen
├── tests/
│   ├── CMakeLists.txt                # Catch2 test configuration
│   ├── TestLooperState.cpp           # Model unit tests
│   ├── TestMessageProtocol.cpp       # Protocol encode/decode tests
│   └── test_bridge_server.py          # Python bridge server tests
├── third_party/
│   └── JUCE/                          # Git submodule
└── README.md
```

### Pattern 1: Shadow State Model

**What:** Plugin maintains a local copy of every looper's state, updated only by Remote Script push events. UI (Phase 2) reads from shadow state exclusively.

**When to use:** Always — this is the core pattern for DAW controller plugins.

**Example:**
```cpp
// Source: JUCE AudioProcessorValueTreeState pattern [CITED: JUCE docs]
struct LooperState {
    juce::String trackId;       // Stable identifier from Live API
    juce::String trackName;     // Human-readable track name
    juce::String deviceId;      // Device index within track
    enum State { Stopped, Recording, Playing, Overdubbing } state;
    float feedback;             // 0.0-1.0 normalized
    bool reverse;
    int loopLengthBars;         // 0 = free, N = N bars
};

class LooperTracker {
    std::map<juce::String, LooperState> loopers;  // key: trackId
    juce::Listeners<juce::ChangeListener> listeners;
    
    void updateState(const juce::String& trackId, const LooperState& newState) {
        if (loopers[trackId] != newState) {
            loopers[trackId] = newState;
            listeners.call(&juce::ChangeListener::changeListenerCallback, nullptr);
        }
    }
};
```

### Pattern 2: JSON-over-OSC Protocol (ableton.js Pattern)

**What:** All messages follow `{ns, nsid, name, args, uuid, version}` structure, sent as OSC bundles with JSON payload. UUID enables request/response correlation without ordering assumptions.

**When to use:** All communication between plugin and Remote Script.

**Example:**
```cpp
// Plugin sends discovery command
nlohmann::json msg = {
    {"uuid", generateUUID()},
    {"ns", "looper"},
    {"nsid", ""},               // empty = all loopers
    {"name", "discover"},
    {"args", {}},
    {"version", 1}
};
// Sent via OSCSender to address "/loopercontrol"
```

```python
# Remote Script responds
{
    "uuid": "a20f25a0-83e2-11e9-bbe1-bd3a580ef903",
    "event": "result",
    "data": {"success": True, "loopers_found": 3}
}

# Remote Script pushes state update (no UUID for events)
{
    "uuid": None,
    "event": "looper_state_changed",
    "data": {
        "track_id": "1",
        "track_name": "Guitar",
        "device_id": "0",
        "state": "Recording",
        "feedback": 0.7
    },
    "version": 1
}
```

### Pattern 3: Ableton ControlSurface Remote Script

**What:** Python script installed in Ableton's User Library Remote Scripts directory. Extends `ControlSurface` base class, registers as a MIDI Remote Script that Live loads automatically on startup.

**When to use:** The only way to access Live's internal API from an external process.

**Example:**
```python
# __init__.py — Entry point for Ableton Live
from .LooperControlSurface import LooperControlSurface

def create_instance(c_instance):
    """Called by Live when the ControlSurface is selected."""
    return LooperControlSurface(c_instance)

# LooperControlSurface.py
from ableton.v2.control_surface import ControlSurface
from .BridgeServer import BridgeServer
from .LooperDiscovery import LooperDiscovery

class LooperControlSurface(ControlSurface):
    def __init__(self, c_instance):
        super().__init__(c_instance)
        self._instance = c_instance
        self.server = BridgeServer(host="127.0.0.1", port=7011)
        self.discovery = LooperDiscovery(self.song())
        self.server.start()
        self._register_script_tasks()
```

### Pattern 4: Pattern-Based Looper Discovery (PLUG-03)

**What:** Instead of matching on `device.class_name == "Looper"`, scan all devices on all tracks and identify looper-like devices by their parameter signatures. A device is "looper-like" if it has a set of parameters matching the looper pattern.

**When to use:** Phase 1 requirement — PLUG-03 mandates this approach.

**Example:**
```python
LOOPER_PARAMETER_PATTERNS = [
    # Pattern 1: Ableton Looper has "State" (discrete 0-3) + "Feedback" (float)
    {"required": ["State", "Feedback"], "type_checks": {"State": "discrete"}},
    # Pattern 2: Generic looper has "Record" + "Play" + "Stop" boolean/toggle params
    {"required": ["Record", "Play", "Stop"], "type_checks": {}},
    # Pattern 3: Looper with overdub capability
    {"required": ["Record", "Overdub", "Play"], "type_checks": {}},
]

def is_looper_device(device):
    """Check if a device matches any looper parameter pattern."""
    param_names = {p.name for p in device.parameters}
    for pattern in LOOPER_PARAMETER_PATTERNS:
        required = set(pattern["required"])
        if required.issubset(param_names):
            return True
    # Also try class_name match as fallback (Ableton Looper specifically)
    if device.class_name == "Looper":
        return True
    return False
```

### Anti-Patterns to Avoid

- **Blocking the audio thread in processBlock():** All network I/O, string formatting, and memory allocation must happen on the message thread. `processBlock()` should only pass audio through and read atomic state flags. [CITED: PITFALLS.md]
- **Hardcoding Ableton Looper as the only discovery target:** PLUG-03 requires pattern-based discovery. Hardcoding `class_name == "Looper"` fails the requirement. [CITED: CONTEXT.md D-03 implies full state push but PLUG-03 is explicit]
- **Polling for state changes:** Use Live API `add_value_listener` callbacks on parameter changes, not periodic queries. [CITED: PITFALLS.md]
- **Putting persistent state in PluginEditor:** The editor can be destroyed at any time. All persistent state must live in the AudioProcessor or APVTS. [CITED: PITFALLS.md]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| OSC message encoding/decoding | Custom binary protocol or raw UDP framing | JUCE `juce_osc` + python-osc | Both handle OSC address patterns, type tagging, and bundle encoding. Re-implementing is error-prone and gains nothing. |
| JSON serialization in C++ | Manual string concatenation or sprintf formatting | nlohmann/json or juce::JSON | JSON serialization has edge cases (unicode escaping, number formatting) that hand-rolled code always gets wrong. |
| UUID generation | Custom timestamp-based IDs | `juce::Uuid()` or `uuid.uuid4()` | UUIDs must be globally unique with no coordination. Both JUCE and Python have built-in generators. |
| Thread-safe state updates | Mutex between audio and message thread | `juce::AsyncUpdater` + `std::atomic` flags | Mutex on audio thread causes priority inversion. AsyncUpdater marshals updates to message thread safely. |
| Ableton Remote Script registration | Writing custom script discovery/loading | `ControlSurface` base class + `__init__.py` + `create_instance()` | Ableton's script loading is specific and fragile. The ControlSurface pattern is well-documented and proven. |
| Parameter management | Raw `AudioProcessorParameter` array | `AudioProcessorValueTreeState` (APVTS) | APVTS handles thread safety, state save/load, undo/redo, and host communication automatically. |

**Key insight:** The two riskiest components — cross-track IPC and Remote Script integration — both have proven reference implementations (ableton.js for the UDP/OSC pattern, Ableton's own Push/APC40 scripts for the ControlSurface pattern). Phase 1 is about validating these patterns work for our specific use case, not inventing new patterns.

## Common Pitfalls

### Pitfall 1: OSC Address Format Mismatch Between JUCE and Python

**What goes wrong:** JUCE's `OSCMessage` uses OSC address patterns like `/loopercontrol/discover`, while python-osc's `Dispatcher.map()` matches these patterns. If the address format doesn't match exactly between sender and receiver, messages silently disappear.

**Why it happens:** OSC addresses are strings that must match byte-for-byte. A typo or inconsistent naming convention (e.g., `/loopercontrol` vs `/looper_control`) causes messages to be routed to the wrong handler or dropped entirely.

**How to avoid:** Define all OSC addresses as constants in a shared protocol specification document. Use a single `ProtocolDefs.h` in C++ that maps to string constants used in Python. Test round-trip message delivery in the very first integration test.

**Warning signs:** Messages appear in JUCE debug output but Remote Script never receives them, or vice versa.

### Pitfall 2: Remote Script Not Loading in Ableton

**What goes wrong:** The script doesn't appear in Ableton's Control Surfaces list, or crashes on load. Ableton silently swallows Remote Script errors in production mode.

**Why it happens:** The `__init__.py` must be in the correct directory with the exact name matching Ableton's expected format. The script folder must be in `~/Music/Ableton/User Library/Remote Scripts/LooperControl/` (macOS) or the equivalent on Windows. Missing `__init__.py`, incorrect `create_instance` signature, or import errors will cause silent failure.

**How to avoid:** Follow the exact `ControlSurface` pattern from Ableton's published scripts. Enable Live's debug log (`Options.txt` with `-debug` flag) to see Python errors. Test script loading before adding any OSC functionality.

**Warning signs:** Script doesn't appear in Live preferences. No Log.txt output. Script works in stand-alone Python test but not inside Live.

### Pitfall 3: JUCE OSC Module Bind Port Conflicts

**What goes wrong:** `OSCReceiver::connect(port)` fails silently if the port is already in use. Multiple plugin instances or previous runs leave stale sockets.

**Why it happens:** UDP sockets may not release immediately on close, especially on macOS where SO_REUSEADDR behavior differs. The range fallback (D-05) mitigates this but must be implemented correctly.

**How to avoid:** After calling `OSCReceiver::connect(port)`, check the return value explicitly. If false, try the next port in the range (7010-7019). Log the bound port. On plugin shutdown, call `disconnect()` explicitly.

**Warning signs:** `OSCReceiver::connect()` returns false. Plugin can't receive messages. Previously working plugin stops receiving after Live restart.

### Pitfall 4: Audio Thread Contamination from OSC Callbacks

**What goes wrong:** JUCE's `OSCReceiver::addListener()` callbacks fire on an internal JUCE thread, NOT the message thread. If you update `LooperTracker` state directly from these callbacks, you may cause data races with the UI thread or, worse, block the OSC thread.

**Why it happens:** JUCE's OSC implementation receives on a background thread. The listener callback runs on that same thread. Updating shared state without synchronization is undefined behavior.

**How to avoid:** Use `juce::MessageManager::callAsync()` or `juce::AsyncUpdater` to marshal OSC callback data to the message thread before updating `LooperTracker`. Never access the looper state map directly from the OSC callback thread.

**Warning signs:** Intermittent crashes. State updates appearing in wrong order. Audio glitches when many looper state changes happen simultaneously.

### Pitfall 5: Ableton Looper Parameter Indices Not Matching Between Sessions

**What goes wrong:** Looper device parameters are accessed by index (`device.parameters[i]`), but parameter indices can change if the user rearranges devices on a track. The same looper device might have its State parameter at a different index in different sessions.

**Why it happens:** Parameter indices are not stable across sessions or device chain reorderings. A device loaded before the Looper shifts its indices.

**How to avoid:** Access parameters by NAME, not by index. After discovering a Looper device, iterate `device.parameters` and match on `parameter.name` to find the State, Feedback, etc. parameters. Store the canonical parameter name, not the index. Re-resolve indices on every device chain change.

**Warning signs:** Looper state reads as wrong values after user rearranges devices on a track. Works in testing but breaks in real use.

### Pitfall 6: Pluginval Fails on Parameter-less Plugin

**What goes wrong:** Pluginval rejects the plugin because it has no automatable parameters. While Phase 1 doesn't need user-facing parameters, Pluginval expects at least minimal parameter compliance.

**Why it happens:** A plugin with zero parameters may fail some Pluginval validation checks. Even a control surface plugin should have at least one parameter (or mark itself appropriately).

**How to avoid:** Add a minimal "connection status" parameter to APVTS. This serves dual purpose: passes Pluginval and provides connection state to the host. In Phase 2, more parameters will be added.

**Warning signs:** Pluginval reports "no parameters found" or similar warnings. Plugin loads in Ableton but device panel shows no parameters at all.

## Runtime State Inventory

This is a greenfield project — no existing runtime state.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — greenfield | N/A |
| Live service config | None — greenfield | N/A |
| OS-registered state | None — greenfield | N/A |
| Secrets/env vars | None — greenfield | N/A |
| Build artifacts | None — greenfield | N/A |

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|-------------|-----------------|--------------|--------|
| Projucer (.jucer files) | CMake (`juce_add_plugin()`) | JUCE 7+, deprecated in JUCE 8 | Must use CMake. Projucer is EOL. |
| VST2 format | VST3 only (VST2 license terminated 2018) | Steinberg, 2018 | Cannot ship VST2. JUCE 8 doesn't include VST2 SDK by default. |
| Ableton Live Remote Script (old API) | Ableton Live 11+ MIDI Remote Script API | Live 11, 2021 | New API is more structured but classic ControlSurface pattern is better documented and more widely used. |
| Raw MIDI CC for feedback | OSC over localhost for feedback | Community pattern, 2020+ | OSC carries richer data, no 7-bit range limitation, more debuggable. |
| JUCE `ParameterID` auto-generated | `juce::ParameterID("id", version_hint)` | JUCE 7+ | Must use version hints to prevent VST3/AU parameter ID mismatches. |

**Deprecated/outdated:**
- Projucer for builds: Use CMake exclusively
- VST2 format: Terminated license, cannot use
- String-only parameter IDs in JUCE: Use `ParameterID` with version hint
- Ableton Live 10 and earlier: v1 targets Live 11+ only per D-10

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake 3.22+ | Build system | ✗ (to verify) | — | Install via cmake.org or brew |
| C++17 compiler | Plugin build | ✗ (to verify) | — | Xcode (macOS) / VS 2022 (Windows) |
| Git | Source control | ✓ | — | — |
| Python 3.9+ | Remote Script development | — | — | Ableton embeds Python; local Python not needed |
| JUCE 8.0.12 | Plugin framework | ✗ (git submodule) | 8.0.12 | git submodule add |
| Ableton Live 12 | Integration testing | ✗ (to verify) | — | Requires Standard or Suite license |
| Pluginval | Plugin validation | ✗ | — | Free download from tracktion.com |

**Missing dependencies with no fallback:**
- Ableton Live is required for integration testing of the Remote Script and plugin loading. Without it, only unit tests and localhost OSC communication can be verified. Must have Live 11+ (preferably 12 Suite) for Phase 1 validation.

**Missing dependencies with fallback:**
- JUCE AudioPluginHost (bundled with JUCE) can be used for basic plugin loading tests instead of loading directly into Ableton. Faster iteration for the plugin scaffold.

## Code Examples

### JUCE Plugin CMakeLists.txt (VST3 Audio Passthrough)

```cmake
# Source: JUCE CMake API docs [CITED: docs.juce.com]
cmake_minimum_required(VERSION 3.22)
project(LiveLooperController VERSION 0.1.0)

add_subdirectory(third_party/JUCE)

juce_add_plugin(LiveLooperController
    VERSION 0.1.0
    COMPANY_NAME "LiveLooperController"
    PLUGIN_MANUFACTURER_CODE Llct
    PLUGIN_CODE Llcr
    FORMATS VST3
    PRODUCT_NAME "Live Looper Controller"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(LiveLooperController PRIVATE
    src/Plugin/PluginProcessor.cpp
    src/Plugin/PluginEditor.cpp
    src/Model/LooperTracker.cpp
    src/Bridge/BridgeClient.cpp
    src/Bridge/MessageProtocol.cpp
)

target_link_libraries(LiveLooperController PRIVATE
    juce::juce_audio_processors
    juce::juce_osc
    juce::juce_data_structures
    juce::juce_gui_basics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

# Do NOT link juce_dsp — no audio processing needed
```

### JUCE AudioProcessor Skeleton (Passthrough)

```cpp
// Source: JUCE AudioProcessor docs [CITED: docs.juce.com]
class LiveLooperProcessor : public juce::AudioProcessor {
public:
    LiveLooperProcessor()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "Parameters", createParameterLayout())
    {
        // Initialize BridgeClient, register OSC listeners
    }

    // --- Audio passthrough (no DSP) ---
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals noDenormals;
        // Do nothing — audio passes through unchanged
        // All state updates happen on message thread via AsyncUpdater
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
    void releaseResources() override {}

    // --- State persistence ---
    void getStateInformation(juce::MemoryBlock& destData) override {
        auto state = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }

    void setStateInformation(const void* data, int sizeInBytes) override {
        std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
        if (xml && xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }

    // --- Required overrides ---
    const juce::String getName() const override { return "Live Looper Controller"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        // Phase 1: minimal parameter for connection status
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID("connected", 1), "Connected", false));
        return { params.begin(), params.end() };
    }

private:
    juce::AudioProcessorValueTreeState apvts;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveLooperProcessor)
};
```

### JUCE OSC Communication (BridgeClient skeleton)

```cpp
// Source: JUCE OSC module [CITED: docs.juce.com]
class BridgeClient : public juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback> {
public:
    BridgeClient() : sender() {
        // Bind receiver to known port range (D-05)
        for (int port = 7010; port <= 7019; ++port) {
            if (receiver.connect(port)) {
                boundPort = port;
                break;
            }
        }
        receiver.addListener(this);
    }

    void sendHello() {
        // D-06: Immediate handshake on load
        auto msg = juce::OSCMessage("/loopercontrol/hello");
        msg.addString(generateUUID());
        msg.addInt32(1);  // version (D-04)
        sender.send("127.0.0.1", remotePort, msg);
    }

    void oscMessageReceived(const juce::OSCMessage& message) override {
        // This runs on the message thread due to MessageLoopCallback template
        // Parse incoming OSC and update LooperTracker
        if (message.getAddressPattern().matches("/loopercontrol/state")) {
            // Parse state update, dispatch to LooperTracker
        }
    }

private:
    juce::OSCSender sender;
    juce::OSCReceiver receiver;
    int boundPort = 0;
    int remotePort = 7011;  // Default Remote Script port
};
```

### Python Remote Script (LooperControlSurface skeleton)

```python
# Source: Ableton Live Remote Scripts pattern [CITED: gluon/AbletonLive12_MidiRemoteScripts]
# LooperControlSurface.py
from ableton.v2.control_surface import ControlSurface
from .BridgeServer import BridgeServer
from .LooperDiscovery import LooperDiscovery

class LooperControlSurface(ControlSurface):
    def __init__(self, c_instance):
        super().__init__(c_instance)
        self._instance = c_instance
        self.log_message("LooperControlSurface initialized")
        
        # Start UDP OSC server
        self.server = BridgeServer(
            host="127.0.0.1",
            port=7011,
            callback=self._handle_command
        )
        self.server.start()
        
        # Discover loopers
        self._discovery = LooperDiscovery(self.song())
        self._discovered_loopers = {}
        self._setup_track_listeners()
    
    def _setup_track_listeners(self):
        """Listen for track/device changes."""
        self.song().add_tracks_listener(self._on_tracks_changed)
        self._scan_all_tracks()
    
    def _on_tracks_changed(self):
        """Track list changed — rescan for loopers."""
        self._discovered_loopers = self._discovery.scan_all_tracks()
        # Push full state to plugin (D-03)
        self._send_full_state()
```

### Python Looper Discovery (Pattern-Based, PLUG-03)

```python
# Source: Ableton Live API [CITED: nsuspray.github.io/Live_API_Doc]
# LooperDiscovery.py

# Looper-like parameter patterns (PLUG-03: pattern-based, not name-based)
LOOPER_PARAMETER_PATTERNS = [
    # Ableton Looper: State (discrete 0-3) + Feedback (float 0-1)
    {"required_names": {"State", "Feedback"}, "class_hint": "Looper"},
    # Generic looper: Record + Play + Stop toggles
    {"required_names": {"Record", "Play", "Stop"}, "class_hint": None},
    # Looper with overdub: Record + Overdub + Play
    {"required_names": {"Record", "Overdub", "Play"}, "class_hint": None},
]

class LooperDiscovery:
    def __init__(self, song):
        self._song = song
    
    def scan_all_tracks(self):
        """Scan all tracks for looper-like devices."""
        loopers = {}
        for track in self._song.tracks:
            for device_idx, device in enumerate(track.devices):
                if self._is_looper_device(device):
                    loopers[track.name] = {
                        "track_id": str(track.name),
                        "track_name": track.name,
                        "device_id": str(device_idx),
                        "device_name": device.name,
                        "class_name": getattr(device, 'class_name', ''),
                        "state": self._read_looper_state(device),
                    }
        return loopers
    
    def _is_looper_device(self, device):
        """PLUG-03: Match by parameter pattern, not just class name."""
        param_names = {p.name for p in device.parameters}
        for pattern in LOOPER_PARAMETER_PATTERNS:
            if pattern["required_names"].issubset(param_names):
                return True
        # Fallback: class name match for devices without standard params
        if getattr(device, 'class_name', '') == 'Looper':
            return True
        return False
    
    def _read_looper_state(self, device):
        """Read current state from a discovered looper device."""
        state = {"state": "Stopped", "feedback": 0.0}
        for param in device.parameters:
            if param.name == "State":
                # Ableton Looper: 0=Stop, 1=Record, 2=Play, 3=Overdub
                state_values = {0: "Stopped", 1: "Recording", 2: "Playing", 3: "Overdubbing"}
                state["state"] = state_values.get(int(param.value), "Unknown")
            elif param.name == "Feedback":
                state["feedback"] = param.value
        return state
```

### python-osc BridgeServer (ThreadingOSCUDPServer)

```python
# Source: python-osc library [VERIFIED: PyPI/python-osc]
# BridgeServer.py
from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import ThreadingOSCUDPServer
from pythonosc.osc_message_builder import OscMessageBuilder
import threading

class BridgeServer:
    def __init__(self, host, port, callback):
        self._host = host
        self._port = port
        self._callback = callback
        self._dispatcher = Dispatcher()
        self._dispatcher.map("/loopercontrol/*", self._on_osc_message)
        self._server = None
        self._thread = None
    
    def start(self):
        self._server = ThreadingOSCUDPServer(
            (self._host, self._port),
            self._dispatcher
        )
        self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        self._thread.start()
    
    def send_to_plugin(self, address, *args):
        """Send OSC message to plugin's bound port."""
        # Plugin port discovered via handshake (D-05/D-06)
        builder = OscMessageBuilder(address=address)
        for arg in args:
            builder.add_arg(arg)
        msg = builder.build()
        # sender.send() to plugin's port
```

### Protocol Message Format (Shared Contract)

```json
// Discovery request (plugin → script)
{
    "uuid": "a20f25a0-83e2-11e9-bbe1-bd3a580ef903",
    "ns": "looper",
    "nsid": "",
    "name": "discover",
    "args": {},
    "version": 1
}

// Discovery response (script → plugin)
{
    "uuid": "a20f25a0-83e2-11e9-bbe1-bd3a580ef903",
    "event": "result",
    "data": {
        "success": true,
        "loopers": [
            {
                "track_id": "1",
                "track_name": "Guitar",
                "device_id": "0",
                "device_name": "Looper",
                "class_name": "Looper",
                "state": "Stopped",
                "feedback": 0.5
            }
        ]
    },
    "version": 1
}

// State push event (script → plugin, D-03: full state push)
{
    "uuid": null,
    "event": "looper_state_changed",
    "data": {
        "track_id": "1",
        "track_name": "Guitar",
        "device_id": "0",
        "state": "Recording",
        "feedback": 0.5
    },
    "version": 1
}

// Handshake (plugin → script, D-06)
{
    "uuid": "fresh-uuid-here",
    "ns": "system",
    "nsid": "",
    "name": "hello",
    "args": {"port": 7010},
    "version": 1
}

// Handshake response (script → plugin)
{
    "uuid": "fresh-uuid-here",
    "event": "result",
    "data": {"version": 1, "port": 7011},
    "version": 1
}
```

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.x (C++) + pytest (Python) |
| Config file | `tests/CMakeLists.txt` (C++), `tests/conftest.py` (Python) |
| Quick run command | `ctest --test-dir build -R LooperState --output-on-failure` (C++ unit), `pytest tests/test_bridge_server.py -x` (Python unit) |
| Full suite command | `ctest --test-dir build --output-on-failure` (C++ full), `pytest tests/` (Python full) |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PLUG-01 | Plugin loads as VST3 in Ableton Live | manual | Load plugin in Ableton Live, verify no errors | ❌ Wave 0 |
| PLUG-01 | Plugin passes Pluginval validation | manual | `pluginval --validate-in-builds-dir` | ❌ Wave 0 |
| PLUG-01 | Plugin processBlock passes audio unchanged | unit | Catch2: verify output buffer equals input buffer | ❌ Wave 0 |
| PLUG-02 | Remote Script discovers tracks with looper devices | unit | pytest: mock Live API, verify looper enumeration | ❌ Wave 0 |
| PLUG-02 | Remote Script reports discovery over UDP | integration | python-osc client receives discovery response | ❌ Wave 0 |
| PLUG-03 | Pattern-based discovery identifies Ableton Looper by params | unit | pytest: device with State+Feedback params is identified | ❌ Wave 0 |
| PLUG-03 | Pattern-based discovery identifies generic looper by params | unit | pytest: device with Record+Play+Stop params is identified | ❌ Wave 0 |
| PLUG-03 | Class name match is fallback, not primary | unit | pytest: class_name match alone still detects Looper | ❌ Wave 0 |
| Protocol | JSON messages encode/decode correctly | unit | Catch2: round-trip JSON → OSC → JSON | ❌ Wave 0 |
| Protocol | UUID correlation works for request/response | unit | Send command with UUID, verify response has same UUID | ❌ Wave 0 |
| Protocol | Version field present in all messages | unit | Verify all message constructors include version=1 | ❌ Wave 0 |
| Protocol | Port range fallback works | unit | Bind first available port in 7010-7019 range | ❌ Wave 0 |
| Protocol | Handshake and auto-reconnect | integration | Kill Remote Script, verify plugin retries with backoff | ❌ Wave 0 |
| Protocol | End-to-end message flow | integration | Plugin sends hello → Script responds → Plugin logs state | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build -R <test> --output-on-failure`
- **Per wave merge:** `ctest --test-dir build --output-on-failure && pytest tests/`
- **Phase gate:** Full suite green + manual integration test in Ableton Live

### Wave 0 Gaps
- [ ] `tests/CMakeLists.txt` — Catch2 integration via FetchContent
- [ ] `tests/TestLooperState.cpp` — LooperState model unit tests
- [ ] `tests/TestMessageProtocol.cpp` — Protocol encode/decode tests
- [ ] `tests/test_bridge_server.py` — Python BridgeServer unit tests
- [ ] `tests/test_looper_discovery.py` — LooperDiscovery pattern-matching tests
- [ ] `tests/conftest.py` — Shared pytest fixtures for mock Live API objects

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | N/A — localhost-only communication, no user authentication |
| V3 Session Management | partial | Session token for plugin-script handshake (D-06/D-07) |
| V4 Access Control | no | N/A — single-user local performance tool |
| V5 Input Validation | yes | Validate all incoming OSC messages: check JSON structure, type constraints, field ranges before processing |
| V6 Cryptography | no | N/A — no encryption needed for localhost UDP |
| V7 Error Handling | yes | Graceful error handling for port conflicts, malformed messages, disconnections |
| V8 Data Protection | no | N/A — no persistent sensitive data |
| V9 Logging | yes | Debug logging of protocol messages for development (disable in release) |

### Known Threat Patterns for Live Looper Controller

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malicious local process injects fake OSC messages | Tampering | Bind to localhost only (127.0.0.1). Session handshake validates peer. [ASSUMED: localhost-only is sufficient for v1] |
| Malformed OSC message crashes plugin or script | Denial of Service | Validate all incoming messages against schema before processing. Never access fields without type checking. |
| Port scan discovers and floods the UDP port | Denial of Service | Accept connections only from localhost. Rate-limit message processing. Ignore unrecognized OSC addresses. |
| Plugin receives state updates from stale/restarted Remote Script | Repudiation | Version field in handshake (D-04). Full state push on reconnect (D-03) overwrites any stale data. |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Ableton Live 12 embeds Python 3.9 | Standard Stack | Remote Script code must be 3.9-compatible. If Live uses older Python, some language features may not work. |
| A2 | python-osc works inside Ableton's embedded Python | Standard Stack | If python-osc has C extension dependencies, it won't work. Pure Python requirement must be verified. |
| A3 | classic ControlSurface API pattern is sufficient for Phase 1 | Architecture Patterns | If Live 11+ new MIDI Remote Script API is required, Remote Script code will need significant restructuring. |
| A4 | localhost-only UDP is sufficient for Phase 1 security | Security Domain | A malicious local process could inject fake looper state. Acceptable for v1, but Phase 3 should consider session tokens. |
| A5 | JUCE OSCReceiver listener callbacks run on message thread with MessageLoopCallback template | Code Examples | If callbacks run on a different thread, thread-safety model needs adjustment. Verified via JUCE docs. |
| A6 | Ableton Looper State parameter values 0-3 map to Stop/Record/Play/Overdub | Code Examples | If mapping differs (e.g., 0=Stop, 1=Play, 2=Record, 3=Overdub), state display will be incorrect. Runtime verification needed. |

## Open Questions

1. **Ableton Looper exact State parameter value mapping**
   - What we know: Research indicates State is a discrete parameter with values 0-3
   - What's unclear: The exact mapping (0=Stop vs 0=Stopped, 1=Record vs 1=Playing)
   - Recommendation: Runtime verification — load a Looper in Ableton Live, use Configure Mode to enumerate all parameters and their current values at each state. Document findings in code comments.

2. **Remote Script loading behavior in Ableton Live**
   - What we know: Scripts go in `~/Music/Ableton/User Library/Remote Scripts/LooperControl/`
   - What's unclear: Whether users need to manually select the script in Live's preferences, or it auto-detects
   - Recommendation: Test the full installation flow. Document user-facing steps clearly.

3. **python-osc in Ableton's embedded Python**
   - What we know: python-osc is pure Python with no C extensions
   - What's unclear: Whether Ableton's embedded Python has all standard library modules that python-osc depends on (e.g., `threading`, `socket`)
   - Recommendation: Verify early by bundling python-osc source and testing import inside Ableton's Python environment.

4. **JUCE OSC vs raw DatagramSocket performance**
   - What we know: JUCE OSC adds OSC protocol overhead (address pattern matching, type tagging)
   - What's unclear: Whether this overhead is measurable for our use case (a few messages per second with small payloads)
   - Recommendation: Start with JUCE OSC for structured messaging. Profile latency only if performance issues appear. The overhead is negligible for looper state updates.

## Sources

### Primary (HIGH confidence)
- JUCE 8.0.12 CMake API — verified `juce_add_plugin()` with `IS_MIDI_EFFECT`, `FORMATS`, `NEEDS_MIDI_INPUT` options [CITED: Context7 + GitHub releases]
- JUCE AudioProcessor and AudioProcessorValueTreeState documentation — verified parameter management, state save/load, thread safety [CITED: Context7]
- JUCE OSC module — verified `OSCSender`/`OSCReceiver` API for localhost UDP communication [CITED: Context7]
- python-osc (PyPI) — verified ThreadingOSCUDPServer, Dispatcher, message encoding [CITED: Context7 + PyPI]
- Ableton Live 12 MIDI Remote Scripts (gluon/AbletonLive12_MidiRemoteScripts) — verified ControlSurface architecture, INI configuration [CITED: Context7]

### Secondary (MEDIUM confidence)
- ableton.js (leolabs/ableton-js) — confirmed JSON-over-UDP protocol pattern with UUID correlation, namespace/action message format [CITED: Context7]
- JUCE CMake API examples — verified plugin build configuration, module linking, format targets [CITED: Context7]
- Live API documentation (nsuspray.github.io/Live_API_Doc) — verified Python API for track/device/parameter access [CITED: WebSearch, verified with Context7]

### Tertiary (LOW confidence)
- Ableton Looper State parameter exact value mapping — research indicates 0-3 but exact mapping needs runtime verification [ASSUMED: see A6]
- python-osc threading behavior inside Ableton's embedded Python — assumed compatible but needs testing [ASSUMED: see A3]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — JUCE 8, C++17, CMake, python-osc are well-documented and verified
- Architecture: HIGH — two-component hybrid is the only viable approach; ableton.js proves the pattern
- Pitfalls: HIGH — well-documented JUCE pitfalls and Ableton integration gotchas
- Protocol: HIGH — ableton.js provides proven reference implementation
- Remote Script: MEDIUM — ControlSurface pattern is well-documented but Ableton's embedded Python environment quirks need runtime verification

**Research date:** 2026-04-15
**Valid until:** 2026-05-15 (30 days — stable stack, but verify JUCE version and Ableton Live version compatibility before starting)