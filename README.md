# Live Looper Controller

A minimal VST3 control surface plugin that gives musicians a single panel to see the state of and control all looper devices across multiple tracks in Ableton Live.

**Core Value:** Perform a complete live looping set without touching the native DAW UI — see every looper's state and control record, overdub, play, and stop in real time from a single surface.

## Architecture

The Live Looper Controller uses a **two-component hybrid architecture**:

```
┌─────────────────────────────────────────────────────────────────────┐
│                     JUCE Plugin Process                              │
│  ┌─────────────┐  ┌─────────────────┐  ┌──────────────────────┐  │
│  │ LiveLooper  │  │ LooperTracker    │  │ BridgeClient        │  │
│  │ Processor   │  │ (Shadow State)   │  │ (OSC Sender/       │  │
│  │ (Audio     │  │ - loopers map    │  │  Receiver)         │  │
│  │  Processor) │  │ - change callback│  │ - send/receive OSC │  │
│  └─────────────┘  └────────┬────────┘  └──────────┬───────────┘  │
└─────────────────────────────┼───────────────────────┼───────────────┘
                              │                       │
                          UDP localhost:7010 ←→ 7011
                          (OSC over UDP)
┌─────────────────────────────┼───────────────────────┼───────────────┐
│                     Ableton Live Process           │               │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │  Remote Script: LooperControlSurface (Python)               │  │
│  │  ┌──────────────────┐  ┌─────────────────┐  ┌──────────┐ │  │
│  │  │ BridgeServer     │  │ LooperDiscovery  │  │ LiveAPI  │ │  │
│  │  │ (python-osc     │  │ (Track/Device    │  │ Wrapper  │ │  │
│  │  │  UDP Server)    │  │  Enumeration)    │  │          │ │  │
│  │  └──────────────────┘  └──────────────────┘  └──────────┘ │  │
│  └─────────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  Ableton Live Internal: Track 1 [Looper] Track 2 [Looper] │  │
│  └──────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────┘
```

### Components

1. **JUCE VST3 Plugin** (`src/`): Runs inside Ableton Live as an audio effect. Maintains shadow state of all discovered loopers and communicates with the Remote Script over OSC.

2. **Python Remote Script** (`remote-script/`): Runs inside Ableton Live as a Control Surface. Discovers looper devices across all tracks using Live's Python API and pushes state changes to the plugin over UDP/OSC.

### Communication Protocol

All communication uses **JSON over OSC** with the following message format:

```json
// Command (plugin → script)
{"uuid": "...", "ns": "looper", "nsid": "", "name": "discover", "args": {}, "version": 1}

// Event (script → plugin)
{"uuid": null, "event": "looper_state_changed", "data": {...}, "version": 1}
```

- **D-01**: JSON with namespace/action message format
- **D-02**: UUID per request for request/response correlation
- **D-03**: Full state push on change
- **D-04**: Version field in every message
- **D-05**: Known ports with range fallback (7010-7019 for plugin, 7011-7020 for script)
- **D-06**: Immediate handshake on load
- **D-07**: Auto-reconnect with exponential backoff

## Requirements

- Ableton Live 11+ (Standard or Suite)
- macOS 12+ or Windows 10+
- JUCE 8.0.12 (included as submodule)
- CMake 3.22+
- C++17 compiler (Xcode/clang++ on macOS, MSVC on Windows)

## Building

### macOS

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/your-repo/live-looper-controller.git
cd live-looper-controller

# Build VST3 plugin
cmake -B build -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build --config Release

# The VST3 will be at: ~/Library/Audio/Plug-Ins/VST3/LiveLooperController.vst3
```

### Windows

```powershell
# Clone with submodules
git clone --recurse-submodules https://github.com/your-repo/live-looper-controller.git
cd live-looper-controller

# Build VST3 plugin
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release

# The VST3 will be at: build/VST3/Release/LiveLooperController.vst3
```

### Running Tests

```bash
# C++ tests (Linux/macOS/Windows)
cmake -B build-tests -S tests -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tests
./build-tests/LiveLooperControllerTests

# Python tests (all platforms)
python3 -m pytest tests/ -v
```

## Installing the Remote Script

After building the plugin:

### macOS

```bash
# Run the install script
./scripts/install_remote_script.sh

# Or manually:
# Copy remote-script/ to ~/Music/Ableton/User Library/Remote Scripts/LooperControl/
```

### Windows

```batch
# Run the install script
scripts\install_remote_script.bat

# Or manually:
# Copy remote-script/ to %USERPROFILE%\Music\Ableton\User Library\Remote Scripts\LooperControl\
```

Then:
1. Restart Ableton Live (mandatory for script discovery)
2. Open Live > Preferences > Link/Tempo/MIDI > Control Surfaces
3. Select "LooperControl" from the dropdown
4. Load the VST3 plugin on any audio track

## Testing in Ableton Live

See [tests/manual/test_in_ableton.md](tests/manual/test_in_ableton.md) for step-by-step validation procedures.

### Quick Test Checklist

1. **Plugin Loading (PLUG-01)**: VST3 loads without errors, editor shows "Connected"
2. **Looper Discovery (PLUG-02)**: Script discovers all Looper devices across tracks
3. **State Updates**: Looper record/play/stop in Live UI updates plugin display
4. **Pattern Detection (PLUG-03)**: Both Ableton Looper and third-party loopers discovered

## Project Structure

```
live-looper-controller/
├── CMakeLists.txt                    # Root build config with JUCE submodule
├── src/
│   ├── Plugin/
│   │   ├── PluginProcessor.h/cpp    # AudioProcessor (audio passthrough)
│   │   └── PluginEditor.h/cpp      # Minimal editor (Phase 1)
│   ├── Model/
│   │   ├── LooperState.h/cpp       # Looper state data model
│   │   └── LooperTracker.h/cpp     # Shadow state manager
│   ├── Bridge/
│   │   ├── BridgeClient.h/cpp      # OSC client (plugin ↔ script)
│   │   └── MessageProtocol.h/cpp   # JSON encode/decode
│   └── Shared/
│       └── ProtocolDefs.h/cpp       # Protocol constants
├── remote-script/
│   ├── __init__.py                 # Ableton entry point
│   ├── LooperControlSurface.py     # Main ControlSurface class
│   ├── LooperDiscovery.py          # Pattern-based device discovery
│   ├── BridgeServer.py             # python-osc UDP server
│   ├── LiveAPIWrapper.py           # Live API helpers
│   └── python_osc/                 # Bundled python-osc library
├── tests/
│   ├── CMakeLists.txt             # Catch2 test config
│   ├── TestLooperState.cpp        # C++ unit tests
│   ├── TestBridgeClient.cpp        # C++ bridge tests
│   ├── test_*.py                   # Python unit tests
│   ├── mock_live_api.py            # Mock Live API for testing
│   └── manual/
│       ├── test_in_ableton.md      # Manual test procedure
│       └── test_results.md         # Test result template
└── scripts/
    ├── install_remote_script.sh     # macOS install
    └── install_remote_script.bat    # Windows install
```

## Key Decisions

| ID | Decision | Rationale |
|----|----------|-----------|
| D-01 | JSON with namespace/action format | Human-readable, debuggable, versionable |
| D-02 | UUID per request | Request/response correlation without ordering assumptions |
| D-03 | Full state push on change | Simple implementation, no state reconciliation needed |
| D-04 | Version field in every message | Forward compatibility |
| D-05 | Port range fallback | Handle port conflicts gracefully |
| D-06 | Immediate handshake on load | Quick connection establishment |
| D-07 | Auto-reconnect with backoff | Survive Live restarts and script reloads |
| D-08 | Audio passthrough plugin | No DSP, just control surface |
| D-09 | VST3 only for v1 | Simpler testing, AU added later |
| D-10 | Ableton Live 11+ | Stable VST3 and Python 3 support |

## Limitations

- **Phase 1**: No UI beyond connection status and looper list (Phase 2 adds full control surface)
- **Phase 1**: Single DAW (Ableton Live) — architecture supports future DAWs
- **Phase 1**: VST3 only — AU support deferred
- **Linux**: JUCE GUI and plugin host require macOS/Windows for full testing

## Contributing

See `.planning/` for project planning artifacts including requirements, research, and phase plans.

## License

[License placeholder - AGPLv3 or commercial]
