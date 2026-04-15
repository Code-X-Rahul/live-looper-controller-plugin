# Architecture Research

**Domain:** DAW control surface plugin (multi-track looper controller)
**Researched:** 2026-04-15
**Confidence:** MEDIUM

## Standard Architecture

### System Overview

A live looper controller plugin faces a fundamental constraint: **VST/AU plugins are sandboxed to a single track** and cannot directly access devices on other tracks. This means a naive plugin architecture cannot fulfill the core requirement of controlling looper devices across multiple tracks. The solution requires a **two-component hybrid architecture**: a JUCE plugin for the UI plus an Ableton Live Remote Script for DAW API access.

```
┌─────────────────────────────────────────────────────────────────┐
│                        UI Layer (JUCE)                          │
│  ┌──────────────┐ ┌──────────────┐ ┌────────────────────────┐  │
│  │ LooperTrack  │ │ LooperTrack  │ │  TransportControls     │  │
│  │   View 1     │ │   View N     │ │  (Record/Play/Stop)    │  │
│  └──────┬───────┘ └──────┬───────┘ └──────────┬─────────────┘  │
│         │                │                     │                 │
├─────────┴────────────────┴─────────────────────┴────────────────┤
│                    State Manager (in-process)                    │
│  ┌───────────────────────────────────────────────────────────┐   │
│  │  LooperModel: tracks[{id, name, state, parameters}]       │   │
│  │  - Maintains shadow copy of all looper states             │   │
│  │  - Diff-based UI updates (only repaint changed tracks)    │   │
│  └────────────────────────┬──────────────────────────────────┘   │
├─────────────────────────────┴───────────────────────────────────┤
│                  Communication Layer (UDP)                       │
│  ┌───────────────────────────────────────────────────────────┐   │
│  │  BridgeClient: JSON over UDP ↔ Remote Script Bridge       │   │
│  │  - Sends commands (record/overdub/play/stop)               │   │
│  │  - Receives state updates (looper state changes)           │   │
│  │  - Handles connection/disconnection/reconnection           │   │
│  └────────────────────────┬──────────────────────────────────┘   │
└─────────────────────────────┼───────────────────────────────────┘
                              │ UDP (localhost)
                              │
┌─────────────────────────────┼───────────────────────────────────┐
│           Ableton Live Process                                   │
│  ┌────────────────────────┴──────────────────────────────────┐  │
│  │  Remote Script (Python) — "LooperControlSurface"          │  │
│  │  - UDP server for plugin communication                     │  │
│  │  - Live API: enumerate tracks → find Looper devices        │  │
│  │  - Live API: read/set device parameters                   │  │
│  │  - Live API: listen for parameter/state changes            │  │
│  └────────────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  Ableton Live Internal (Song, Tracks, Devices)             │  │
│  │  Track 1: [Looper]  Track 2: [Looper]  Track N: [Looper]  │  │
│  └────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Implementation |
|-----------|----------------|----------------|
| PluginProcessor | Audio passthrough, plugin lifecycle, parameter hosting | JUCE `AudioProcessor` subclass, `isMidiEffect()=true` or audio passthrough |
| PluginEditor | Visual UI for all looper states and controls | JUCE `AudioProcessorEditor` subclass with custom components |
| LooperModel | Shadow state of all discovered loopers, diff-based updates | Plain C++ data model with change broadcasting |
| BridgeClient | Bidirectional UDP communication with Remote Script | JUCE `DatagramSocket` + `Thread` for async IO |
| Remote Script | Live API access, track/device enumeration, parameter control | Python, installed in Ableton's Remote Scripts folder |
| Command Protocol | Structured JSON messages for plugin ↔ script communication | JSON over UDP with UUID request/response pattern |

## Recommended Project Structure

```
live-looper-controller/
├── src/
│   ├── Plugin/                        # JUCE plugin entry point
│   │   ├── PluginProcessor.h/.cpp      # AudioProcessor subclass
│   │   ├── PluginEditor.h/.cpp         # UI editor
│   │   └── CMakeLists.txt              # Plugin build config
│   ├── UI/                             # JUCE UI components
│   │   ├── LooperTrackComponent.h/.cpp # Single looper track UI
│   │   ├── LooperGridView.h/.cpp        # Grid layout of all tracks
│   │   ├── TransportButton.h/.cpp       # Record/Play/Stop button
│   │   └── StatusIndicator.h/.cpp       # State color indicator
│   ├── Model/                          # State management
│   │   ├── LooperState.h               # Single looper data model
│   │   ├── LooperTracker.h/.cpp         # Collection + discovery state
│   │   └── CommandTypes.h              # Command enums, message types
│   ├── Bridge/                          # Communication layer
│   │   ├── BridgeClient.h/.cpp          # UDP client for plugin→script
│   │   ├── MessageProtocol.h/.cpp       # JSON message encode/decode
│   │   └── ConnectionManager.h/.cpp     # Reconnection, heartbeat
│   └── Shared/                           # Shared constants
│       └── ProtocolDefs.h               # Port numbers, version, msg IDs
├── remote-script/                        # Ableton Live Remote Script
│   ├── __init__.py                       # Script registration
│   ├── LooperControlSurface.py           # Main ControlSurface class
│   ├── LooperDiscovery.py                # Track/device enumeration
│   ├── BridgeServer.py                   # UDP server for plugin comm
│   └── LiveAPIWrapper.py                # Live API parameter access
├── tests/                                # Unit/integration tests
│   ├── Model/                            # Model tests
│   ├── Bridge/                           # Communication tests
│   └── remote-script/                    # Script tests (Python)
├── CMakeLists.txt                        # Root build config
└── README.md
```

### Structure Rationale

- **Plugin/**: Standard JUCE plugin structure, minimal processor logic since this is a control surface (no DSP)
- **UI/**: Separate UI components for a composable, testable interface. Each looper track gets its own component, reused in the grid layout
- **Model/**: Pure data layer, no JUCE UI dependencies. Can be tested without a GUI. The `LooperTracker` manages discovery state and broadcasts changes
- **Bridge/**: Isolated communication layer. If UDP doesn't work, swap to TCP/WebSocket without touching Model or UI
- **remote-script/**: Python, separate from C++ build. Installed manually by user in Ableton's Remote Scripts directory
- **Shared/**: Protocol definitions that both C++ and Python use, ensuring message format consistency

## Architectural Patterns

### Pattern 1: Shadow State (Plugin mirrors DAW state)

**What:** The plugin maintains a local copy ("shadow") of every looper's state, updated by the Remote Script. UI reads only from shadow state, never directly from DAW.

**When to use:** Always — this is the core pattern for any DAW controller plugin.

**Trade-offs:**
- ✅ Decouples UI rendering from network latency
- ✅ Enables diff-based updates (only repaint changed tracks)
- ✅ Plugin works offline (shows last known state if connection drops)
- ❌ Requires synchronization logic (eventual consistency)
- ❌ Stale data risk if DAW changes aren't propagated quickly

**Example:**
```cpp
struct LooperState {
    String trackId;       // Live API track index
    String trackName;     // Human-readable track name
    String deviceId;      // Live API device index
    enum State { Stopped, Recording, Overdubbing, Playing } state;
    float feedback;        // 0-1 normalized
    bool reverse;
    // ... other looper parameters
};

class LooperTracker {
    std::map<String, LooperState> loopers;  // key: trackId
   listeners::Broadcast broadcaster;
    
    void updateState(const String& trackId, const LooperState& newState) {
        if (loopers[trackId] != newState) {
            loopers[trackId] = newState;
            broadcaster.notify(trackId, newState);
        }
    }
};
```

### Pattern 2: Request-Response over UDP (JSON Protocol)

**What:** The plugin and Remote Script communicate via JSON over UDP, with UUID-tagged messages for request/response correlation. Based on the proven ableton.js pattern.

**When to use:** For all structured communication between plugin and Remote Script.

**Trade-offs:**
- ✅ UDP is low-latency (critical for live performance control)
- ✅ JSON is debuggable, versionable, and language-agnostic
- ✅ UUID correlation enables async request-response without blocking
- ✅ Works on macOS (localhost UDP) and Windows
- ❌ UDP is unreliable — requires retransmission logic for critical commands
- ❌ No built-in ordering — need sequence numbers for state updates

**Example:**
```cpp
// Plugin sends command
nlohmann::json msg = {
    {"uuid", generateUUID()},
    {"ns", "looper"},
    {"nsid", trackId},
    {"name", "set_state"},
    {"args", {{"state", "Recording"}}}
};
bridgeClient.send(msg);

// Remote Script responds
{
    "uuid": "same-uuid",
    "event": "result", 
    "data": {"success": true}
}

// Remote Script pushes state update (event)
{
    "uuid": null,
    "event": "looper_state_changed",
    "data": {"track_id": "1", "state": "Recording"}
}
```

### Pattern 3: DAW Abstraction Layer (Future Multi-DAW Support)

**What:** The `BridgeClient` communicates via a text protocol (JSON over UDP) rather than DAW-specific APIs. DAW-specific logic is confined to the Remote Script. Adding a new DAW means writing a new Remote Script, not changing the plugin.

**When to use:** Always — this is the extensibility pattern required by "DAW-agnostic architecture, Ableton-first."

**Trade-offs:**
- ✅ Plugin code is DAW-agnostic — same binary for all DAWs
- ✅ New DAW support = new Remote Script only
- ✅ Protocol can evolve independently of DAW APIs
- ❌ Each DAW needs its own bridge script (Python, Lua, etc.)
- ❌ DAW-specific features need protocol extensions

**Example:**
```
Plugin doesn't know about:
  - Live API track indices
  - Ableton Looper device parameter names
  - Remote Script implementation details

Plugin only knows about:
  - Track ID (string)
  - Looper state (enum)
  - Looper parameters (normalized floats)
  - Commands (record, overdub, play, stop)
```

## Data Flow

### Command Flow (User clicks "Record" on track 3)

```
User clicks "Record" on Track 3
    ↓
PluginEditor::onRecordClicked(trackId="3")
    ↓
LooperTracker::sendCommand("3", Command::Record)
    ↓
BridgeClient::send({"uuid":"...", "ns":"looper", "nsid":"3", 
                     "name":"set_state", "args":{"state":"Recording"}})
    ↓ [UDP localhost:portA → localhost:portB]
Remote Script receives JSON
    ↓
LooperControlSurface handles command
    ↓
Live API: track[3].devices[looperIdx].setParameter("State", 1.0)
    ↓
Ableton Live: Looper enters Record mode
    ↓ (parameter change listener fires)
Remote Script detects state change
    ↓
Remote Script sends: {"event":"looper_state_changed", 
                       "data":{"track_id":"3", "state":"Recording"}}
    ↓ [UDP localhost:portB → localhost:portA]
BridgeClient receives state update
    ↓
LooperTracker updates shadow state for track "3"
    ↓
LooperTracker broadcasts change to UI
    ↓
LooperTrackComponent for track 3 repaints (shows "Recording" indicator)
```

### Discovery Flow (Plugin loads, finds all loopers)

```
PluginProcessor::prepareToPlay()
    ↓
BridgeClient::connect()
    ↓ [UDP to Remote Script]
BridgeClient::send({"uuid":"...", "ns":"looper", "name":"discover"})
    ↓
Remote Script enumerates tracks:
    for track in self.song.tracks:
        for device in track.devices:
            if device.class_name == "Looper":
                register_looper(track, device)
    ↓
Remote Script sends each looper:
    {"event":"looper_discovered", 
     "data":{"track_id":"1", "track_name":"Guitar", 
             "device_id":"0", "state":"Stopped"}}
    ↓
BridgeClient receives discovery messages
    ↓
LooperTracker adds each looper to shadow state
    ↓
UI rebuilds grid with discovered loopers
```

### State Synchronization (Continuous updates)

```
Remote Script continuously monitors:
    - Looper state changes (recording/playing/overdubbing/stopped)
    - Looper parameter changes (feedback, reverse, speed)
    
    On any change:
    {"event":"looper_state_changed", 
     "data":{"track_id":"2", "state":"Overdubbing", "feedback":0.75}}
    
    ↓
    BridgeClient → LooperTracker → UI
```

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|------------------------|
| 1-8 loopers | Default architecture — single UDP connection, full state refresh on connect, event-driven updates |
| 8-32 loopers | Add state compression (gzip like ableton.js), batch updates per frame, throttle rapid parameter changes |
| 32+ loopers | Consider WebSocket instead of UDP for reliability, add delta-only updates, implement request batching |

### Scaling Priorities

1. **First bottleneck:** UDP packet loss at high update rates. Mitigation: Sequence numbers + retransmission for commands, idempotent state updates (newest wins)
2. **Second bottleneck:** UI repaint performance with many tracks. Mitigation: Only repaint changed components, use JUCE's `repaint()` with dirty regions, not full grid repaints

## Anti-Patterns

### Anti-Pattern 1: Direct Plugin-to-DAW API Access

**What people do:** Try to use Live API / OS-level inter-process communication directly from the plugin to enumerate tracks and devices.

**Why it's wrong:** VST/AU plugins are sandboxed — they cannot access the host DAW's internal API. The VST3/AU spec provides no cross-track access. Attempting this leads to platform-specific hacks that break across DAW versions.

**Do this instead:** Use a Remote Script running inside the DAW process (which has full API access) and communicate with the plugin via a local network protocol.

### Anti-Pattern 2: Polling for State Changes

**What people do:** Periodically query all looper states (e.g., every 100ms) to detect changes.

**Why it's wrong:** Wastes CPU, introduces latency (up to poll interval), can flood UDP with redundant data, and creates visible lag between the DAW state and the plugin UI.

**Do this instead:** Use Live API parameter listeners on the Remote Script side. When any looper state changes, push the update immediately. Only poll on initial connection (discovery) and after reconnection.

### Anti-Pattern 3: Coupling Protocol to DAW Internals

**What people do:** Use Live API indices (track number, device index) directly in the protocol between plugin and script.

**Why it's wrong:** Track numbers change if tracks are reordered. Device indices change if devices are moved. Hardcoding these creates fragile coupling that breaks when the user rearranges their set.

**Do this instead:** Use stable identifiers (track name + device name combination, or Live API's canonical path). The Remote Script should resolve these to API indices internally and re-resolve when the set changes.

### Anti-Pattern 4: Blocking Audio Thread for Communication

**What people do:** Send UDP messages or process incoming state updates on the audio thread in `processBlock()`.

**Why it's wrong:** Network IO is non-deterministic. Blocking the audio thread causes audio glitches, which is unacceptable in a live performance tool.

**Do this instead:** All network IO happens on a dedicated `juce::Thread`. State updates from the network thread are posted to the model via `juce::MessageManager::callAsync()` or a lock-free queue. The audio thread (`processBlock()`) is completely empty or just passes audio through.

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| Ableton Live | Remote Script (Python) via UDP localhost | Remote Script must be installed in `~/Music/Ableton/User Library/Remote Scripts/LooperControl/` |
| macOS IAC Driver | Virtual MIDI port (future: direct control) | Can be used as fallback communication channel; setup required by user |
| Windows loopMIDI | Virtual MIDI port (future: direct control) | Windows equivalent of IAC; requires user installation |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Plugin UI ↔ Model | Direct method calls in same process | Model broadcasts changes via JUCE listeners; UI subscribes |
| Model ↔ BridgeClient | Method calls + async callbacks | BridgeClient runs network thread; posts state updates to Model via MessageManager |
| BridgeClient ↔ Remote Script | JSON over UDP (localhost) | Two socket pairs: plugin→script and script→plugin; each side binds a port |
| Remote Script ↔ Live API | Python Live API calls | Direct in-process access; synchronous within Live's Python interpreter |
| PluginProcessor ↔ PluginEditor | JUCE AudioProcessorValueTreeState | Standard JUCE pattern — shared parameter state tree |

### Looper Device Parameters (Ableton Live Looper)

The Ableton Live Looper device exposes these key automatable parameters:

| Parameter | Type | Values | Notes |
|-----------|------|--------|-------|
| State | Integer (0-3) | 0=Stop, 1=Record, 2=Play, 3=Overdub | Primary control parameter — this is what we set |
| Feedback | Float | 0.0-1.0 | Controls how much previous loop content is retained during overdub |
| Reverse | Toggle | On/Off | Reverses loop playback |
| Speed | Choice | Various speed/pitch settings | Speed affects pitch — octave up/down |
| Quantize | Choice | Bar lengths, None | Quantization of recording start/end |
| Tempo Control | Choice | None/Follow/Set & Follow | How Looper syncs with song tempo |
| Input -> Output | Choice | Always/Never/Rec OVR/Rec OVR Stop | Input monitoring behavior |
| Record Length | Choice | Song running, x bars | Length of recorded material |

## Build Order Implications

Based on the component dependencies, the recommended build order is:

1. **Message Protocol + Shared Definitions** — No dependencies, required by both C++ and Python
2. **Model Layer** — Depends on protocol types only; can be developed and tested independently
3. **BridgeClient** — Depends on protocol + networking; can be tested with mock Remote Script
4. **Remote Script** — Depends on protocol; can be tested with Live API🐍 and mock plugin client
5. **Plugin Processor (minimal)** — Depends on Model + Bridge; skeleton with no UI
6. **UI Components** — Depends on Model; can be developed once Model API is stable
7. **PluginEditor** — Wires Model ↔ UI ↔ Bridge together
8. **Integration Testing** — Full system with Live running

**Key dependency chain:** Protocol → Model → Bridge → (Processor | Script) → Integration

**Parallel development paths:**
- C++ side: Model + UI can be developed in parallel with Bridge
- Python side: Remote Script can be developed independently, tested with ableton.js or a mock UDP client
- The Protocol is the contract — get it right first since both sides depend on it

## Confidence Assessment

| Area | Confidence | Reason |
|------|------------|--------|
| Two-component hybrid architecture (Plugin + Remote Script) | HIGH | Fundamental VST/AU limitation makes this the only viable approach; proven by ableton.js |
| UDP communication protocol | HIGH | ableton.js has used this pattern in production for years |
| JUCE plugin structure (Processor + APVTS + Editor) | HIGH | Well-documented JUCE patterns, standard approach |
| Remote Script Live API access | HIGH | Proven by ableton.js, APC40 scripts, Push scripts |
| Looper device parameter mapping | MEDIUM | Based on Live manual and automation docs; need to verify exact parameter IDs at runtime |
| Discovery mechanism (track/device enumeration) | MEDIUM | Live API supports this, but track reordering and dynamic sets need edge case testing |
| Future DAW extensibility of protocol | MEDIUM | Protocol design is DAW-agnostic, but each DAW needs a custom bridge script — untested with other DAWs |

## Sources

- JUCE AudioProcessor documentation — Context7, HIGH confidence
- JUCE AudioProcessorValueTreeState patterns — Context7, HIGH confidence
- Ableton Live 12 Looper parameters — Official Ableton manual, HIGH confidence
- Ableton Live Remote Script architecture — Context7 (abletonlive12_midiremotescripts), HIGH confidence
- ableton.js (UDP + WebSocket communication with Live) — GitHub, MEDIUM confidence (well-established project, 505 stars)
- Ableton Live MIDI port virtual bus setup — Official Ableton docs, HIGH confidence
- JUCE UMP/MIDI virtual endpoint support — Context7 (JUCE source), HIGH confidence
- JUCE InterprocessConnection networking — Context7 (JUCE source), HIGH confidence

---
*Architecture research for: live looper controller DAW plugin*
*Researched: 2026-04-15*