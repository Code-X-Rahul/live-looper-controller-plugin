"""Main ControlSurface class for Ableton Live Remote Script.

This is the entry point that Ableton Live loads when the Remote Script is enabled.
It coordinates the LooperDiscovery and BridgeServer components.
"""

import logging

logger = logging.getLogger(__name__)

# Try to import Ableton's ControlSurface base class
# This will only work when running inside Ableton Live
try:
    from ableton.v2.control_surface import ControlSurface
    _HAS_ABETON_API = True
except ImportError:
    # For testing without Ableton, we'll create a mock base
    ControlSurface = object
    _HAS_ABETON_API = False

from .BridgeServer import BridgeServer
from .LooperDiscovery import LooperDiscovery


class LooperControlSurface(ControlSurface):
    """Main ControlSurface class for looper control.

    This class is instantiated by Ableton Live when the Remote Script is enabled.
    It discovers looper devices across all tracks and communicates with the
    JUCE plugin via OSC UDP.
    """

    def __init__(self, c_instance):
        """Initialize the ControlSurface.

        Args:
            c_instance: The ControlSurface instance from Ableton Live
        """
        super().__init__(c_instance)
        self._instance = c_instance
        self._log_message("LooperControlSurface: Initializing...")

        # Initialize discovery
        self._discovery = LooperDiscovery(self._song())

        # Initialize bridge server (OSC UDP)
        self._server = BridgeServer(
            host="127.0.0.1",
            port=7011,
            on_command=self._handle_command
        )
        self._server.start()
        self._log_message(f"LooperControlSurface: BridgeServer started on port {self._server.port}")

        # Discover loopers on startup
        self._discovered_loopers = {}
        self._full_scan()

        # Listen for track changes (new tracks added, tracks removed)
        try:
            self._song().add_tracks_listener(self._on_tracks_changed)
        except Exception as e:
            self._log_message(f"LooperControlSurface: Could not add tracks listener: {e}")

    def _song(self):
        """Get the Live API Song object.

        Returns:
            Live API Song object or mock for testing
        """
        # In Ableton: self.song() is provided by ControlSurface base class
        # For testing: we use the song passed to LooperDiscovery
        if _HAS_ABETON_API:
            return self.song()
        else:
            # Return None for non-Ableton environment - use discovery's song directly
            return None

    def _log_message(self, message):
        """Log a message to Ableton's log file.

        Args:
            message: String message to log
        """
        if hasattr(self, 'log_message'):
            self.log_message(message)
        else:
            # Fallback for testing without Ableton
            logger.info(message)

    def _full_scan(self):
        """Discover all looper devices and push full state to plugin."""
        self._discovered_loopers = self._discovery.scan_all_tracks()
        self._log_message(f"LooperControlSurface: Found {len(self._discovered_loopers)} looper(s)")

        # Push full state to plugin (D-03)
        for track_id, looper_info in self._discovered_loopers.items():
            self._server.send_event("looper_discovered", looper_info)

        # Start listening for parameter changes
        self._discovery.start_listening(self._on_looper_param_changed)

    def _on_tracks_changed(self):
        """Called when tracks are added or removed. Rescan for loopers."""
        self._log_message("LooperControlSurface: Track list changed, rescanning...")
        self._discovery.stop_listening()
        self._full_scan()

    def _on_looper_param_changed(self, track, device, param, value):
        """Called when a looper parameter changes. Push full state (D-03).

        Args:
            track: The track containing the changed device
            device: The device whose parameter changed
            param: The parameter that changed
            value: The new value
        """
        # Re-read the full state of this looper
        song = self._song()
        if song is None:
            # Testing mode - use discovery's internal song
            song = self._discovery._song

        for track_idx, t in enumerate(song.tracks):
            if t.name == track.name:
                for device_idx, d in enumerate(t.devices):
                    if d.name == device.name:
                        if self._discovery._is_looper_device(d):
                            looper_info = self._discovery._read_looper_state(t, track_idx, d, device_idx)
                            self._server.send_event("looper_state_changed", looper_info)
                            return

    def _handle_command(self, ns, nsid, name, args):
        """Handle incoming commands from the plugin.

        Args:
            ns: Namespace string (e.g., 'looper', 'system')
            nsid: Namespace ID (usually empty for our protocol)
            name: Command name (e.g., 'discover', 'set_state')
            args: Command arguments dictionary

        Returns:
            Response dictionary or None
        """
        if ns == "looper":
            if name == "discover":
                # Re-scan and send all loopers
                self._discovery.stop_listening()
                self._full_scan()
                return {"success": True, "loopers_found": len(self._discovered_loopers)}
        elif ns == "system":
            if name == "ping":
                return {"success": True, "pong": True}

        return {"success": False, "error": "unknown_command"}

    def disconnect(self):
        """Clean up when Live closes or script is unloaded."""
        self._log_message("LooperControlSurface: Disconnecting...")
        self._discovery.stop_listening()
        self._server.stop()

        try:
            self._song().remove_tracks_listener(self._on_tracks_changed)
        except Exception:
            pass

        self._log_message("LooperControlSurface: Disconnected")
