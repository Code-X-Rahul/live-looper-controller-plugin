---
phase: 1
plan: 3
type: execute
wave: 3
depends_on:
  - 01-PLAN-1
files_modified:
  - remote-script/__init__.py
  - remote-script/LooperControlSurface.py
  - remote-script/LooperDiscovery.py
  - remote-script/BridgeServer.py
  - remote-script/LiveAPIWrapper.py
  - tests/test_looper_discovery.py
  - tests/test_bridge_server.py
  - tests/conftest.py
  - tests/mock_live_api.py
autonomous: true
requirements:
  - PLUG-02
  - PLUG-03
---

# Phase 1 Plan 3: Ableton Live Remote Script (Python)

<objective>
Build the Python Remote Script that runs inside Ableton Live's embedded Python interpreter. The script discovers looper devices across all tracks via the Live API (pattern-based, not name-based per PLUG-03), listens for parameter changes, and communicates with the JUCE plugin via UDP/OSC using python-osc. This validates the entire cross-process architecture.
</objective>

<tasks>

## Task 3.1: Create Remote Script Package Structure and Entry Points

<read_first>
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (Ableton ControlSurface pattern, __init__.py entry point)
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (D-05: port 7011 default, D-06: handshake)
- .planning/research/ARCHITECTURE.md (Remote Script component responsibilities, data flow)
</read_first>

<action>
1. Create `remote-script/__init__.py` with Ableton Remote Script entry points:
   ```python
   # Ableton Live Remote Script entry point
   from .LooperControlSurface import LooperControlSurface

   def create_instance(c_instance):
       """Called by Live when the ControlSurface is selected."""
       return LooperControlSurface(c_instance)
   ```

2. Create `remote-script/python_osc/` directory by copying the python-osc library source files into the project. The directory structure must be:
   ```
   remote-script/
   ├── __init__.py
   ├── LooperControlSurface.py
   ├── LooperDiscovery.py
   ├── BridgeServer.py
   ├── LiveAPIWrapper.py
   └── python_osc/
       ├── __init__.py
       ├── dispatcher.py
       ├── osc_server.py
       ├── osc_message_builder.py
       ├── osc_message.py
       └── ... (all python-osc source files)
   ```
   This bundling is necessary because Ableton's embedded Python doesn't support pip installation.

3. Create `tests/conftest.py` with pytest configuration and mock Live API fixtures:
   ```python
   import pytest

   @pytest.fixture
   def mock_song():
       """Mock Live API Song object for testing."""
       return MockSong()

   @pytest.fixture
   def mock_track_with_looper():
       """Mock track containing an Ableton Looper device."""
       track = MockTrack("Guitar")
       track.add_device(MockLooperDevice("Looper", is_looper=True))
       return track

   @pytest.fixture
   def mock_track_with_generic_looper():
       """Mock track containing a third-party looper device."""
       track = MockTrack("Bass")
       device = MockDevice("SuperLooper")
       device.add_parameter("Record", 0.0)
       device.add_parameter("Play", 0.0)
       device.add_parameter("Stop", 0.0)
       track.add_device(device)
       return track
   ```

4. Create `tests/mock_live_api.py` with mock objects that simulate Ableton Live's API:
   - `MockSong` — has `tracks` property returning a list of `MockTrack` objects, `add_tracks_listener` method
   - `MockTrack` — has `name`, `devices` property (list of `MockDevice`), `add_devices_listener` method
   - `MockDevice` — has `name`, `class_name`, `parameters` property (list of `MockParameter`), `add_parameter` helper method
   - `MockParameter` — has `name`, `value` (float 0.0-1.0), `add_value_listener` method
   - `MockLooperDevice(MockDevice)` — preset with Ableton Looper parameters: State (0-3), Feedback (0-1), Reverse, etc.
</action>

<acceptance_criteria>
- `remote-script/__init__.py` exists with `create_instance` function that returns `LooperControlSurface`
- `remote-script/python_osc/` directory exists with python-osc source files (at minimum: `dispatcher.py`, `osc_server.py`, `osc_message_builder.py`)
- `tests/conftest.py` exists with `mock_song`, `mock_track_with_looper`, and `mock_track_with_generic_looper` fixtures
- `tests/mock_live_api.py` exists with `MockSong`, `MockTrack`, `MockDevice`, `MockParameter`, `MockLooperDevice` classes
- `MockLooperDevice` has parameters named "State" and "Feedback" matching Ableton Looper
</acceptance_criteria>

## Task 3.2: Implement LooperDiscovery (Pattern-Based Device Discovery)

<read_first>
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (Pattern-based looper discovery, PLUG-03)
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (D-03: full state push)
- .planning/research/ARCHITECTURE.md (Discovery flow diagram)
- tests/mock_live_api.py (mock objects for testing)
</read_first>

<action>
1. Create `remote-script/LooperDiscovery.py` with a `LooperDiscovery` class:
   ```python
   """Looper device discovery via pattern matching (PLUG-03)."""

   # Looper-like parameter patterns for device identification
   LOOPER_PARAMETER_PATTERNS = [
       # Pattern 1: Ableton Looper — has discrete State param + continuous Feedback
       {"required_names": {"State", "Feedback"}, "class_hint": "Looper"},
       # Pattern 2: Generic looper — has Record + Play + Stop toggle params
       {"required_names": {"Record", "Play", "Stop"}, "class_hint": None},
       # Pattern 3: Looper with overdub — has Record + Overdub + Play
       {"required_names": {"Record", "Overdub", "Play"}, "class_hint": None},
   ]

   # State value mapping for Ableton Looper device
   ABLETON_LOOPER_STATE_MAP = {
       0: "Stopped",
       1: "Recording",
       2: "Playing",
       3: "Overdubbing",
   }


   class LooperDiscovery:
       def __init__(self, song):
           self._song = song
           self._loopers = {}  # track_name -> looper info dict
           self._param_listeners = {}  # device_id -> list of listener objects

       def scan_all_tracks(self):
           """Scan all tracks for looper-like devices. Returns dict of discovered loopers."""
           discovered = {}
           for track_idx, track in enumerate(self._song.tracks):
               for device_idx, device in enumerate(track.devices):
                   if self._is_looper_device(device):
                       looper_info = self._read_looper_state(track, track_idx, device, device_idx)
                       discovered[looper_info["track_id"]] = looper_info
           return discovered

       def _is_looper_device(self, device):
           """PLUG-03: Identify looper-like devices by parameter patterns, not just class name."""
           param_names = set()
           try:
               param_names = {p.name for p in device.parameters}
           except Exception:
               pass

           for pattern in LOOPER_PARAMETER_PATTERNS:
               if pattern["required_names"].issubset(param_names):
                   return True

           # Fallback: class name match (Ableton Looper specifically)
           if getattr(device, 'class_name', '') == 'Looper':
               return True

           return False

       def _read_looper_state(self, track, track_idx, device, device_idx):
           """Read current state from a discovered looper device."""
           state = {
               "track_id": f"{track_idx}",
               "track_name": track.name,
               "device_id": f"{device_idx}",
               "device_name": device.name,
               "class_name": getattr(device, 'class_name', ''),
               "state": "Stopped",
               "feedback": 0.5,
               "reverse": False,
           }

           for param in device.parameters:
               if param.name == "State":
                   # Ableton Looper: 0=Stop, 1=Record, 2=Play, 3=Overdub
                   state_val = int(param.value) if hasattr(param, 'value') else 0
                   state["state"] = ABLETON_LOOPER_STATE_MAP.get(state_val, "Stopped")
               elif param.name == "Feedback":
                   state["feedback"] = float(param.value) if hasattr(param, 'value') else 0.5
               elif param.name == "Reverse":
                   state["reverse"] = bool(param.value > 0.5) if hasattr(param, 'value') else False

           return state

       def start_listening(self, on_state_changed):
           """Register Live API parameter listeners for all discovered loopers."""
           for track in self._song.tracks:
               for device in track.devices:
                   if self._is_looper_device(device):
                       for param in device.parameters:
                           if param.name in ("State", "Feedback", "Reverse", "Record", "Play", "Stop", "Overdub"):
                               try:
                                   listener = lambda value, t=track, d=device, p=param, cb=on_state_changed: cb(t, d, p, value)
                                   param.add_value_listener(listener)
                                   self._param_listeners.setdefault(id(device), []).append((param, listener))
                               except Exception:
                                   pass

       def stop_listening(self):
           """Remove all parameter listeners."""
           for device_id, listeners in self._param_listeners.items():
               for param, listener in listeners:
                   try:
                       param.remove_value_listener(listener)
                   except Exception:
                       pass
           self._param_listeners.clear()
   ```

2. Create `tests/test_looper_discovery.py` with the following test cases:
   - `test_ableton_looper_detected_by_state_feedback_params` — MockDevice with "State" and "Feedback" parameters is identified as looper
   - `test_generic_looper_detected_by_record_play_stop` — MockDevice with "Record", "Play", "Stop" is identified
   - `test_looper_with_overdub_detected` — MockDevice with "Record", "Overdub", "Play" is identified
   - `test_non_looper_device_not_detected` — MockDevice with "Volume" and "Pan" parameters is NOT identified
   - `test_class_name_fallback_detects_looper` — Device with class_name "Looper" but no matching parameters still detected (fallback)
   - `test_read_state_maps_ableton_looper_values` — Ableton Looper state parameter value 1 maps to "Recording", 2 to "Playing"
   - `test_scan_all_tracks_finds_multiple_loopers` — Multiple tracks with loopers all discovered
   - `test_pattern_match_is_primary_class_name_is_fallback` — Verify that pattern match takes precedence over class_name
</action>

<acceptance_criteria>
- `remote-script/LooperDiscovery.py` exists with `LooperDiscovery` class
- `_is_looper_device()` checks parameter patterns FIRST (3 patterns defined), then falls back to `class_name == "Looper"`
- `LOOPER_PARAMETER_PATTERNS` contains at least 3 patterns: State+Feedback, Record+Play+Stop, Record+Overdub+Play
- `ABLETON_LOOPER_STATE_MAP` maps 0→"Stopped", 1→"Recording", 2→"Playing", 3→"Overdubbing"
- `_read_looper_state()` reads State, Feedback, and Reverse parameters by name (not by index)
- `start_listening()` registers parameter listeners for state-relevant parameters
- `stop_listening()` removes all registered listeners
- `tests/test_looper_discovery.py` exists with at least 8 test cases
- Tests pass: `pytest tests/test_looper_discovery.py` exits 0
- PLUG-03 verification: `_is_looper_device()` with "Record", "Play", "Stop" params returns True (NOT restricted to class_name "Looper")
</acceptance_criteria>

## Task 3.3: Implement BridgeServer (python-osc UDP Server)

<read_first>
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (D-01: JSON message format, D-04: version field, D-05: port 7011)
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (python-osc ThreadingOSCUDPServer, BridgeServer pattern)
- src/Shared/ProtocolDefs.h (OSC address prefix, port constants)
</read_first>

<action>
1. Create `remote-script/BridgeServer.py` with a `BridgeServer` class:
   ```python
   """UDP OSC server for communication with JUCE plugin."""

   import json
   import uuid
   import threading
   import logging

   from pythonosc.dispatcher import Dispatcher
   from pythonosc.osc_server import ThreadingOSCUDPServer
   from pythonosc osc_message_builder import OscMessageBuilder

   logger = logging.getLogger(__name__)

   PROTOCOL_VERSION = 1
   OSC_PREFIX = "/loopercontrol"
   DEFAULT_HOST = "127.0.0.1"
   DEFAULT_PORT = 7011


   class BridgeServer:
       def __init__(self, host=DEFAULT_HOST, port=DEFAULT_PORT, on_command=None):
           self._host = host
           self._port = port
           self._on_command = on_command  # Callback: (ns, nsid, name, args) -> response dict
           self._dispatcher = Dispatcher()
           self._dispatcher.map(f"{OSC_PREFIX}/*", self._on_osc_message)
           self._server = None
           self._thread = None
           self._plugin_port = None  # Discovered via handshake (D-06)

       def start(self):
           """Start the OSC server on a separate thread."""
           # Try port range (D-05: range fallback for script port 7011-7020)
           for port in range(DEFAULT_PORT, DEFAULT_PORT + 10):
               try:
                   self._server = ThreadingOSCUDPServer(
                       (self._host, port),
                       self._dispatcher
                   )
                   self._port = port
                   break
               except OSError:
                   continue

           if self._server is None:
               logger.error("Could not bind to any port in range 7011-7020")
               return

           self._thread = threading.Thread(target=self._server.serve_forever, daemon=True)
           self._thread.start()
           logger.info(f"BridgeServer started on {self._host}:{self._port}")

       def stop(self):
           """Stop the OSC server."""
           if self._server:
               self._server.shutdown()
               logger.info("BridgeServer stopped")

       def send_to_plugin(self, address, data_dict):
           """Send a message to the plugin's bound port."""
           if self._plugin_port is None:
               logger.warning("Plugin port not known yet, cannot send message")
               return

           builder = OscMessageBuilder(address=address)
           # Serialize the entire data dict as a JSON string in one OSC string argument
           builder.add_arg(json.dumps(data_dict))
           msg = builder.build()
           # Send via UDP to plugin
           self._server.socket.sendto(
               msg.dgram,
               (self._host, self._plugin_port)
           )

       def send_event(self, event_type, data):
           """Send an event to the plugin (D-01: JSON with namespace format, D-03: full state push)."""
           message = {
               "uuid": None,  # Events don't have request UUIDs
               "event": event_type,
               "data": data,
               "version": PROTOCOL_VERSION  # D-04
           }
           self.send_to_plugin(f"{OSC_PREFIX}/event", message)

       def send_response(self, request_uuid, data):
           """Send a response correlating to a request UUID (D-02)."""
           message = {
               "uuid": request_uuid,
               "event": "result",
               "data": data,
               "version": PROTOCOL_VERSION  # D-04
           }
           self.send_to_plugin(f"{OSC_PREFIX}/response", message)

       def _on_osc_message(self, address, *args):
           """Handle incoming OSC message from plugin."""
           try:
               if len(args) < 1:
                   logger.warning(f"No args in message to {address}")
                   return

               message_json = args[0]
               message = json.loads(message_json)

               # D-04: Version check
               if message.get("version", 0) != PROTOCOL_VERSION:
                   logger.warning(f"Version mismatch: got {message.get('version')}, expected {PROTOCOL_VERSION}")
                   # Send error response
                   self.send_response(message.get("uuid"), {"success": False, "error": "version_mismatch"})
                   return

               ns = message.get("ns", "")
               nsid = message.get("nsid", "")
               name = message.get("name", "")
               msg_args = message.get("args", {})

               # D-06: Hello/handshake handling
               if name == "hello" and ns == "system":
                   self._plugin_port = msg_args.get("port", 7010)
                   logger.info(f"Plugin connected on port {self._plugin_port}")
                   self.send_response(message["uuid"], {
                       "success": True,
                       "version": PROTOCOL_VERSION,
                       "port": self._port
                   })
                   if self._on_command:
                       self._on_command("system", "", "hello", msg_args)
                   return

               # Dispatch to command handler
               if self._on_command:
                   result = self._on_command(ns, nsid, name, msg_args)
                   self.send_response(message.get("uuid"), result)

           except json.JSONDecodeError as e:
               logger.error(f"Invalid JSON in message: {e}")
           except Exception as e:
               logger.error(f"Error handling message: {e}")
   ```

2. Create `tests/test_bridge_server.py` with pytest test cases:
   - `test_bridge_server_starts_on_default_port` — start server, verify it's listening on port 7011
   - `test_bridge_server_port_range_fallback` — start two servers, verify second gets port 7012
   - `test_bridge_server_handles_json_message` — send JSON message to /loopercontrol/command, verify parsing
   - `test_bridge_server_version_check_rejects_mismatch` — send message with version=99, verify error response
   - `test_bridge_server_hello_saves_plugin_port` — send hello with port 7010, verify plugin_port is stored
   - `test_bridge_server_sends_response_with_uuid` — send command with UUID, verify response has same UUID
   - `test_bridge_server_sends_event_with_version` — send event, verify version field is 1
</action>

<acceptance_criteria>
- `remote-script/BridgeServer.py` exists with `BridgeServer` class
- Server binds to port range 7011-7020 with fallback (D-05)
- `send_event()` includes `version: 1` in every message (D-04)
- `send_response()` echoes back the request UUID (D-02)
- Hello message handler stores plugin port and sends version in response (D-06)
- All incoming messages are validated for version before processing (D-04)
- JSON parsing errors are caught and logged, not crashing the server
- `tests/test_bridge_server.py` exists with at least 7 test cases
- Tests pass: `pytest tests/test_bridge_server.py` exits 0
</acceptance_criteria>

## Task 3.4: Implement LooperControlSurface (Main Remote Script Class)

<read_first>
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (Ableton ControlSurface pattern)
- remote-script/BridgeServer.py (OSC server)
- remote-script/LooperDiscovery.py (device discovery)
- .planning/research/ARCHITECTURE.md (Remote Script data flow)
</read_first>

<action>
1. Create `remote-script/LooperControlSurface.py` with a `LooperControlSurface` class:
   ```python
   """Main ControlSurface class for Ableton Live Remote Script."""

   from ableton.v2.control_surface import ControlSurface
   from .BridgeServer import BridgeServer
   from .LooperDiscovery import LooperDiscovery

   class LooperControlSurface(ControlSurface):
       def __init__(self, c_instance):
           super().__init__(c_instance)
           self._instance = c_instance
           self.log_message("LooperControlSurface: Initializing...")

           # Initialize discovery
           self._discovery = LooperDiscovery(self.song())

           # Initialize bridge server (OSC UDP)
           self._server = BridgeServer(
               host="127.0.0.1",
               port=7011,
               on_command=self._handle_command
           )
           self._server.start()
           self.log_message(f"LooperControlSurface: BridgeServer started on port {self._server._port}")

           # Discover loopers on startup
           self._discovered_loopers = {}
           self._full_scan()

           # Listen for track changes (new tracks added, tracks removed)
           try:
               self.song().add_tracks_listener(self._on_tracks_changed)
           except Exception as e:
               self.log_message(f"LooperControlSurface: Could not add tracks listener: {e}")

       def _full_scan(self):
           """Discover all looper devices and push full state to plugin."""
           self._discovered_loopers = self._discovery.scan_all_tracks()
           self.log_message(f"LooperControlSurface: Found {len(self._discovered_loopers)} looper(s)")

           # Push full state to plugin (D-03)
           for track_id, looper_info in self._discovered_loopers.items():
               self._server.send_event("looper_discovered", looper_info)

           # Start listening for parameter changes
           self._discovery.start_listening(self._on_looper_param_changed)

       def _on_tracks_changed(self):
           """Called when tracks are added or removed. Rescan for loopers."""
           self._discovery.stop_listening()
           self._full_scan()

       def _on_looper_param_changed(self, track, device, param, value):
           """Called when a looper parameter changes. Push full state (D-03)."""
           # Re-read the full state of this looper
           for track_idx, t in enumerate(self.song().tracks):
               if t.name == track.name:
                   for device_idx, d in enumerate(t.devices):
                       if d.name == device.name:
                           looper_info = self._discovery._read_looper_state(t, track_idx, d, device_idx)
                           self._server.send_event("looper_state_changed", looper_info)
                           return

       def _handle_command(self, ns, nsid, name, args):
           """Handle incoming commands from the plugin."""
           if ns == "looper" and name == "discover":
               # Re-scan and send all loopers
               self._full_scan()
               return {"success": True, "loopers_found": len(self._discovered_loopers)}
           return {"success": False, "error": "unknown_command"}

       def disconnect(self):
           """Clean up when Live closes or script is unloaded."""
           self._discovery.stop_listening()
           self._server.stop()
           try:
               self.song().remove_tracks_listener(self._on_tracks_changed)
           except Exception:
               pass
           self.log_message("LooperControlSurface: Disconnected")
   ```

2. Create `remote-script/LiveAPIWrapper.py` with helper functions for Live API access:
   ```python
   """Helper functions for the Live API."""

   def get_track_by_index(song, index):
       """Get a track by its index, handling audio/MIDI/return/master tracks."""
       tracks = list(song.tracks) + list(song.return_tracks) + [song.master_track]
       if 0 <= index < len(tracks):
           return tracks[index]
       return None

   def get_device_parameter_by_name(device, param_name):
       """Find a parameter by name on a device. Returns (parameter, index) or None."""
       for i, param in enumerate(device.parameters):
           if param.name == param_name:
               return (param, i)
       return None

   def set_device_parameter_by_name(device, param_name, value):
       """Set a device parameter by name."""
       result = get_device_parameter_by_name(device, param_name)
       if result:
           param, _ = result
           param.value = value
           return True
       return False
   ```

3. Create `tests/test_live_api_wrapper.py` with unit tests for the helper functions using mock objects.

4. Verify all Python tests pass:
   ```bash
   pytest tests/ -v
   ```
</action>

<acceptance_criteria>
- `remote-script/LooperControlSurface.py` exists with `LooperControlSurface(ControlSurface)` class
- `__init__` initializes `LooperDiscovery`, `BridgeServer`, and performs initial scan
- `_full_scan()` discovers all loopers and pushes full state to plugin (D-03)
- `_on_tracks_changed()` triggers rescan when track list changes
- `_on_looper_param_changed()` pushes full looper state on parameter change (D-03)
- `_handle_command()` processes "discover" command from plugin
- `disconnect()` cleans up listeners and stops server
- Track listener registered via `song().add_tracks_listener()` (not polling)
- `remote-script/LiveAPIWrapper.py` exists with `get_device_parameter_by_name` and `set_device_parameter_by_name` helpers
- All Python tests pass: `pytest tests/ -v` exits 0
</acceptance_criteria>

</tasks>

<verification>
1. Remote Script directory structure is correct for Ableton Live installation
2. `__init__.py` has `create_instance()` entry point matching Ableton's expected format
3. python-osc is bundled in the remote-script directory
4. All Python unit tests pass via pytest
5. `LooperDiscovery._is_looper_device()` searches by parameter patterns (PLUG-03)
6. Class name "Looper" is a FALLBACK in discovery, not the primary method
7. BridgeServer implements D-05 (port range), D-06 (handshake), D-04 (version field)
8. Full state push on change is implemented (D-03)
9. All messages include `version: 1` field (D-04)
10. UUID correlation in request/response implemented (D-02)
</verification>

<must_haves>
- Remote Script can be loaded by Ableton Live (correct `__init__.py` and `create_instance()`)
- Looper discovery finds devices by parameter patterns (PLUG-03)
- State updates pushed to plugin immediately on change, not polled (D-03, anti-pattern from PITFALLS)
- All messages follow D-01 JSON format with ns/nsid/name/args
- UUID correlation in request/response (D-02)
- Version field in every message (D-04)
- Port range fallback starting at 7011 (D-05)
- Handshake with version and port exchange (D-06)
</must_haves>

<threat_model>
## Threat Model — Plan 3: Remote Script

| Threat | STRIDE | Mitigation |
|--------|--------|------------|
| Malicious local process sends OSC to Remote Script | Tampering | Validate all incoming messages against schema; version check rejects incompatible messages |
| Live API exceptions crash the script | Denial of Service | All Live API calls wrapped in try/except; exceptions logged and script continues |
| python-osc import fails in embedded Python | Denial of Service | python-osc is bundled as source (no pip needed); tested in Live's Python environment |
| Parameter listener callbacks cause infinite loops | Denial of Service | _on_looper_param_changed uses full state push (idempotent); no re-entrant command sends |
</threat_model>