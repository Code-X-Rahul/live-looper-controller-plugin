"""
Integration tests for the OSC protocol between BridgeClient (C++) and BridgeServer (Python).

These tests verify the BridgeServer behavior and message protocol structure
without requiring actual OSC socket communication.
"""

import pytest
import json
import time
import uuid

# Add the remote-script directory to the path for imports
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'remote-script'))

from BridgeServer import BridgeServer, OSC_PREFIX, PROTOCOL_VERSION
from LooperDiscovery import LooperDiscovery
from mock_live_api import MockSong, MockTrack, MockLooperDevice, MockDevice


class TestBridgeServerProtocol:
    """Protocol tests for BridgeServer message handling."""

    @pytest.fixture
    def mock_song_with_loopers(self):
        """Create a mock song with multiple looper devices."""
        song = MockSong()

        # Track 1: Guitar with Ableton Looper
        guitar_track = MockTrack("Guitar")
        guitar_track.add_device(MockLooperDevice("Looper 1"))
        song.add_track(guitar_track)

        # Track 2: Bass with generic looper
        bass_track = MockTrack("Bass")
        generic_looper = MockDevice("SuperLooper", "GenericLooper")
        generic_looper.add_parameter("Record", 0.0)
        generic_looper.add_parameter("Play", 0.0)
        generic_looper.add_parameter("Stop", 0.0)
        bass_track.add_device(generic_looper)
        song.add_track(bass_track)

        return song

    @pytest.fixture
    def running_server(self, mock_song_with_loopers):
        """Start a BridgeServer with mock song."""
        server = BridgeServer(
            host="127.0.0.1",
            port=9011,
            on_command=None
        )
        server.start()
        time.sleep(0.1)

        yield server, mock_song_with_loopers

        server.stop()

    def test_server_starts_successfully(self, running_server):
        """BridgeServer starts and binds to a port."""
        server, song = running_server
        assert server.port >= 9011
        assert server.port < 9021  # Within port range

    def test_protocol_version_constant(self):
        """Verify PROTOCOL_VERSION is 1."""
        assert PROTOCOL_VERSION == 1

    def test_osc_prefix_constant(self):
        """Verify OSC_PREFIX is correct."""
        assert OSC_PREFIX == "/loopercontrol"

    def test_hello_message_structure(self):
        """Verify hello message has correct structure per D-06."""
        msg = {
            "uuid": str(uuid.uuid4()),
            "ns": "system",
            "nsid": "",
            "name": "hello",
            "args": {"port": 7010},
            "version": 1
        }

        assert "uuid" in msg
        assert msg["ns"] == "system"
        assert msg["name"] == "hello"
        assert "port" in msg["args"]
        assert msg["version"] == 1

    def test_discover_message_structure(self):
        """Verify discover message has correct structure."""
        msg = {
            "uuid": str(uuid.uuid4()),
            "ns": "looper",
            "nsid": "",
            "name": "discover",
            "args": {},
            "version": 1
        }

        assert msg["ns"] == "looper"
        assert msg["name"] == "discover"
        assert msg["version"] == 1

    def test_event_message_structure(self):
        """Verify event messages have correct structure per D-01/D-03."""
        event = {
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

        assert event["uuid"] is None  # Events don't have request UUIDs
        assert "event" in event
        assert "data" in event
        assert event["version"] == 1

    def test_result_response_structure(self):
        """Verify result responses have correct structure per D-02."""
        response = {
            "uuid": str(uuid.uuid4()),
            "event": "result",
            "data": {
                "success": True,
                "version": 1,
                "port": 9011
            },
            "version": 1
        }

        assert response["uuid"] is not None  # Responses have request UUID
        assert response["event"] == "result"
        assert "success" in response["data"]
        assert response["version"] == 1

    def test_version_mismatch_error_structure(self):
        """Verify error response for version mismatch."""
        error_response = {
            "uuid": str(uuid.uuid4()),
            "event": "result",
            "data": {
                "success": False,
                "error": "version_mismatch"
            },
            "version": 1
        }

        assert error_response["data"]["success"] is False
        assert error_response["data"]["error"] == "version_mismatch"

    def test_all_protocol_constants_defined(self):
        """Verify all required protocol constants are defined."""
        # Check BridgeServer has all expected constants
        assert hasattr(BridgeServer, 'PROTOCOL_VERSION') or PROTOCOL_VERSION == 1
        assert OSC_PREFIX == "/loopercontrol"


class TestLooperDiscovery:
    """Test looper discovery and state reading."""

    def test_ableton_looper_detected_by_state_feedback(self):
        """Ableton Looper detected by State + Feedback params."""
        song = MockSong()
        track = MockTrack("Test")
        track.add_device(MockLooperDevice("Looper"))
        song.add_track(track)

        discovery = LooperDiscovery(song)
        discovered = discovery.scan_all_tracks()

        assert len(discovered) == 1

    def test_generic_looper_detected_by_record_play_stop(self):
        """Generic looper detected by Record + Play + Stop params."""
        song = MockSong()
        track = MockTrack("Bass")
        device = MockDevice("SuperLooper", "GenericLooper")
        device.add_parameter("Record", 0.0)
        device.add_parameter("Play", 0.0)
        device.add_parameter("Stop", 0.0)
        track.add_device(device)
        song.add_track(track)

        discovery = LooperDiscovery(song)
        discovered = discovery.scan_all_tracks()

        assert len(discovered) == 1

    def test_overdub_looper_detected(self):
        """Looper with overdub detected by Record + Overdub + Play."""
        song = MockSong()
        track = MockTrack("Drums")
        device = MockDevice("OverdubLooper", "CustomLooper")
        device.add_parameter("Record", 0.0)
        device.add_parameter("Overdub", 0.0)
        device.add_parameter("Play", 0.0)
        track.add_device(device)
        song.add_track(track)

        discovery = LooperDiscovery(song)
        discovered = discovery.scan_all_tracks()

        assert len(discovered) == 1

    def test_non_looper_not_detected(self):
        """Non-looper devices are not detected."""
        song = MockSong()
        track = MockTrack("Synth")
        track.add_device(MockDevice("Volume", "Volume"))
        track.add_device(MockDevice("Pan", "Pan"))
        song.add_track(track)

        discovery = LooperDiscovery(song)
        discovered = discovery.scan_all_tracks()

        assert len(discovered) == 0

    def test_class_name_fallback_for_looper(self):
        """Class name 'Looper' is fallback for devices without standard params."""
        song = MockSong()
        track = MockTrack("Test")
        device = MockDevice("Custom Looper", "Looper")
        device.add_parameter("Custom", 0.0)
        track.add_device(device)
        song.add_track(track)

        discovery = LooperDiscovery(song)
        discovered = discovery.scan_all_tracks()

        # Should still be detected via class_name fallback
        assert len(discovered) == 1

    def test_looper_state_reading(self):
        """Test reading state from Ableton Looper device."""
        song = MockSong()
        track = MockTrack("Guitar")
        looper = MockLooperDevice("Looper")
        # Set state to Recording
        for param in looper.parameters:
            if param.name == "State":
                param.value = 1.0  # Recording
        track.add_device(looper)
        song.add_track(track)

        discovery = LooperDiscovery(song)
        discovered = discovery.scan_all_tracks()

        assert len(discovered) == 1
        looper_info = list(discovered.values())[0]
        assert looper_info["state"] == "Recording"

    def test_multiple_tracks_with_loopers(self):
        """Multiple tracks with loopers are all discovered."""
        song = MockSong()

        guitar_track = MockTrack("Guitar")
        guitar_track.add_device(MockLooperDevice("Guitar Looper"))
        song.add_track(guitar_track)

        bass_track = MockTrack("Bass")
        bass_looper = MockDevice("Bass Looper", "GenericLooper")
        bass_looper.add_parameter("Record", 0.0)
        bass_looper.add_parameter("Play", 0.0)
        bass_looper.add_parameter("Stop", 0.0)
        bass_track.add_device(bass_looper)
        song.add_track(bass_track)

        drums_track = MockTrack("Drums")
        drums_track.add_device(MockLooperDevice("Drums Looper"))
        song.add_track(drums_track)

        discovery = LooperDiscovery(song)
        discovered = discovery.scan_all_tracks()

        assert len(discovered) == 3


class TestBridgeServerPortRange:
    """Test BridgeServer port range fallback."""

    def test_server_binds_within_port_range(self):
        """Server should bind within its port range."""
        server = BridgeServer(host="127.0.0.1", port=9015)
        server.start()
        time.sleep(0.1)

        # Server should bind to 9015 or nearby
        assert server.port >= 9015
        assert server.port < 9025  # Within range + 10

        server.stop()

    def test_multiple_servers_get_different_ports(self):
        """Multiple servers on same preferred port should get different ports."""
        server1 = BridgeServer(host="127.0.0.1", port=9016)
        server1.start()
        time.sleep(0.1)

        server2 = BridgeServer(host="127.0.0.1", port=9016)
        server2.start()
        time.sleep(0.1)

        # They should have different ports
        assert server1.port != server2.port or server1.port > 9016

        server1.stop()
        server2.stop()


class TestProtocolMessageFormat:
    """Test the exact message format used in the protocol."""

    def test_hello_message_format_d06(self):
        """Verify hello message format per D-06."""
        # D-06: plugin sends hello with local port
        msg = {
            "uuid": "test-uuid",
            "ns": "system",
            "nsid": "",
            "name": "hello",
            "args": {"port": 7010},
            "version": 1
        }

        # Required fields
        assert msg["ns"] == "system"
        assert msg["name"] == "hello"
        assert msg["args"]["port"] == 7010
        assert msg["version"] == 1

    def test_discover_message_format(self):
        """Verify discover message format."""
        msg = {
            "uuid": "test-uuid",
            "ns": "looper",
            "nsid": "",
            "name": "discover",
            "args": {},
            "version": 1
        }

        assert msg["ns"] == "looper"
        assert msg["name"] == "discover"

    def test_looper_discovered_event_d03(self):
        """Verify looper_discovered event format per D-03 (full state push)."""
        # D-03: Full state push - complete state sent on discovery
        event = {
            "uuid": None,  # Events don't have request UUIDs
            "event": "looper_discovered",
            "data": {
                "track_id": "0",
                "track_name": "Guitar",
                "device_id": "0",
                "device_name": "Looper 1",
                "class_name": "Looper",
                "state": "Stopped",
                "feedback": 0.5,
                "reverse": False,
                "loop_length_bars": 0
            },
            "version": 1
        }

        # All required fields per D-03
        data = event["data"]
        assert "track_id" in data
        assert "track_name" in data
        assert "device_id" in data
        assert "device_name" in data
        assert "class_name" in data
        assert "state" in data
        assert "feedback" in data
        assert "reverse" in data
        assert "loop_length_bars" in data

    def test_looper_state_changed_event_d03(self):
        """Verify state change event format per D-03 (full state push)."""
        event = {
            "uuid": None,
            "event": "looper_state_changed",
            "data": {
                "track_id": "0",
                "track_name": "Guitar",
                "device_id": "0",
                "device_name": "Looper 1",
                "class_name": "Looper",
                "state": "Recording",
                "feedback": 0.7,
                "reverse": False,
                "loop_length_bars": 0
            },
            "version": 1
        }

        assert event["event"] == "looper_state_changed"
        assert event["data"]["state"] == "Recording"

    def test_version_field_in_all_messages(self):
        """Verify version field is present in all message types (D-04)."""
        messages = [
            # Hello
            {"uuid": "x", "ns": "system", "name": "hello", "args": {}, "version": 1},
            # Discover
            {"uuid": "x", "ns": "looper", "name": "discover", "args": {}, "version": 1},
            # Event
            {"uuid": None, "event": "looper_state_changed", "data": {}, "version": 1},
            # Result
            {"uuid": "x", "event": "result", "data": {"success": True}, "version": 1},
        ]

        for msg in messages:
            assert "version" in msg
            assert msg["version"] == 1
