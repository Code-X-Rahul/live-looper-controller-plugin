"""Looper device discovery via pattern matching (PLUG-03)."""

# Looper-like parameter patterns for device identification
# Order matters: more specific patterns first
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

# Reverse map for setting looper state from string to integer value
ABLETON_LOOPER_STATE_REVERSE_MAP = {
    "Stopped": 0,
    "Recording": 1,
    "Playing": 2,
    "Overdubbing": 3,
}


class LooperDiscovery:
    """Discovers looper-like devices by parameter pattern matching."""

    def __init__(self, song):
        """Initialize the discovery engine.

        Args:
            song: MockSong or Live API Song object with tracks and devices
        """
        self._song = song
        self._loopers = {}  # track_name -> looper info dict
        self._param_listeners = {}  # device_id -> list of (param, listener) tuples

    def scan_all_tracks(self):
        """Scan all tracks for looper-like devices.

        Returns:
            dict: Discovered loopers keyed by track_id, each containing:
                - track_id: str - Track index as string
                - track_name: str - Human-readable track name
                - device_id: str - Device index as string
                - device_name: str - Device name
                - class_name: str - Device class name
                - state: str - Current looper state
                - feedback: float - Feedback value (0.0-1.0)
                - reverse: bool - Reverse playback state
        """
        discovered = {}
        for track_idx, track in enumerate(self._song.tracks):
            for device_idx, device in enumerate(track.devices):
                if self._is_looper_device(device):
                    looper_info = self._read_looper_state(track, track_idx, device, device_idx)
                    discovered[looper_info["track_id"]] = looper_info
                    self._loopers[track.name] = looper_info
        return discovered

    def _is_looper_device(self, device):
        """PLUG-03: Identify looper-like devices by parameter patterns, not just class name.

        The primary detection method is parameter pattern matching.
        Class name match is a fallback for Ableton Looper devices that may not
        have been detected by pattern matching.

        Args:
            device: MockDevice or Live API Device object

        Returns:
            bool: True if the device is looper-like
        """
        param_names = set()
        try:
            param_names = {p.name for p in device.parameters}
        except Exception:
            pass

        # Primary method: pattern matching on parameter names
        for pattern in LOOPER_PARAMETER_PATTERNS:
            if pattern["required_names"].issubset(param_names):
                return True

        # Fallback: class name match (Ableton Looper specifically)
        if getattr(device, 'class_name', '') == 'Looper':
            return True

        return False

    def _read_looper_state(self, track, track_idx, device, device_idx):
        """Read current state from a discovered looper device.

        Args:
            track: MockTrack or Live API Track object
            track_idx: Index of the track
            device: MockDevice or Live API Device object
            device_idx: Index of the device within the track

        Returns:
            dict: Looper state information
        """
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
        """Register Live API parameter listeners for all discovered loopers.

        Args:
            on_state_changed: Callback function(track, device, param, value)
                called when a looper parameter changes
        """
        for track in self._song.tracks:
            for device in track.devices:
                if self._is_looper_device(device):
                    for param in device.parameters:
                        if param.name in ("State", "Feedback", "Reverse", "Record", "Play", "Stop", "Overdub"):
                            try:
                                # Use default argument to capture loop variables correctly
                                listener = lambda v, t=track, d=device, p=param, cb=on_state_changed: cb(t, d, p, v)
                                param.add_value_listener(listener)
                                self._param_listeners.setdefault(id(device), []).append((param, listener))
                            except Exception:
                                pass

    def stop_listening(self):
        """Remove all parameter listeners registered by start_listening."""
        for device_id, listeners in self._param_listeners.items():
            for param, listener in listeners:
                try:
                    param.remove_value_listener(listener)
                except Exception:
                    pass
        self._param_listeners.clear()

    def get_discovered_loopers(self):
        """Get the currently discovered loopers from the last scan.

        Returns:
            dict: Currently discovered loopers keyed by track_id
        """
        return self._loopers.copy()

    def _set_looper_param(self, track_idx, device_idx, param_name, value):
        """Set a parameter on a looper device at the given track/device indices.

        Args:
            track_idx: Index of the track
            device_idx: Index of the device within the track
            param_name: Name of the parameter to set
            value: Value to set the parameter to

        Returns:
            bool: True if the parameter was set successfully
        """
        try:
            track = self._song.tracks[track_idx]
            device = track.devices[device_idx]

            if not self._is_looper_device(device):
                return False

            for param in device.parameters:
                if param.name == param_name:
                    param.value = value
                    return True
            return False
        except Exception:
            return False

    def _get_looper_params(self, track_idx, device_idx):
        """Get all parameters for a looper device at the given track/device indices.

        Args:
            track_idx: Index of the track
            device_idx: Index of the device within the track

        Returns:
            list: List of parameter objects, or empty list if device not found
        """
        try:
            track = self._song.tracks[track_idx]
            device = track.devices[device_idx]

            if not self._is_looper_device(device):
                return []

            return [p for p in device.parameters]
        except Exception:
            return []
