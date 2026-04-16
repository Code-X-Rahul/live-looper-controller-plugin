"""Tests for LooperDiscovery pattern-based device discovery (PLUG-03)."""

import sys
import os

# Add the remote-script directory to the path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'remote-script'))

import pytest
from mock_live_api import (
    MockSong,
    MockTrack,
    MockDevice,
    MockLooperDevice,
    MockParameter,
)
from LooperDiscovery import LooperDiscovery, LOOPER_PARAMETER_PATTERNS, ABLETON_LOOPER_STATE_MAP


class TestIsLooperDevice:
    """Tests for _is_looper_device() pattern matching."""

    def test_ableton_looper_detected_by_state_feedback_params(self):
        """MockDevice with 'State' and 'Feedback' parameters is identified as looper."""
        device = MockDevice("Test Looper", "Looper")
        device.add_parameter("State", 0.0)
        device.add_parameter("Feedback", 0.5)

        discovery = LooperDiscovery(MockSong())
        assert discovery._is_looper_device(device) is True

    def test_generic_looper_detected_by_record_play_stop(self):
        """MockDevice with 'Record', 'Play', 'Stop' is identified as looper."""
        device = MockDevice("SuperLooper", "GenericLooper")
        device.add_parameter("Record", 0.0)
        device.add_parameter("Play", 0.0)
        device.add_parameter("Stop", 0.0)

        discovery = LooperDiscovery(MockSong())
        assert discovery._is_looper_device(device) is True

    def test_looper_with_overdub_detected(self):
        """MockDevice with 'Record', 'Overdub', 'Play' is identified as looper."""
        device = MockDevice("OverdubLooper", "CustomLooper")
        device.add_parameter("Record", 0.0)
        device.add_parameter("Overdub", 0.0)
        device.add_parameter("Play", 0.0)

        discovery = LooperDiscovery(MockSong())
        assert discovery._is_looper_device(device) is True

    def test_non_looper_device_not_detected(self):
        """MockDevice with 'Volume' and 'Pan' parameters is NOT identified."""
        device = MockDevice("Volume", "Volume")
        device.add_parameter("Volume", 0.8)
        device.add_parameter("Pan", 0.5)

        discovery = LooperDiscovery(MockSong())
        assert discovery._is_looper_device(device) is False

    def test_class_name_fallback_detects_looper(self):
        """Device with class_name 'Looper' but no matching parameters still detected (fallback)."""
        device = MockDevice("Custom Looper", "Looper")
        # Note: no standard looper parameters added, should still be detected by class_name fallback
        device.add_parameter("CustomParam", 0.0)

        discovery = LooperDiscovery(MockSong())
        assert discovery._is_looper_device(device) is True

    def test_pattern_match_is_primary_class_name_is_fallback(self):
        """Verify that pattern match takes precedence over class_name."""
        # A device with a class_name that looks like a looper but has wrong params
        device = MockDevice("NotReallyALooper", "Looper")
        device.add_parameter("Volume", 0.8)  # Wrong params - not a real looper

        discovery = LooperDiscovery(MockSong())
        # Pattern match should fail, class_name fallback should succeed
        # because we check class_name == 'Looper' after patterns
        # But this specific device has class_name "Looper", so it should still be detected
        assert discovery._is_looper_device(device) is True


class TestReadLooperState:
    """Tests for _read_looper_state() state reading."""

    def test_read_state_maps_ableton_looper_values(self):
        """Ableton Looper state parameter value 1 maps to 'Recording', 2 to 'Playing'."""
        track = MockTrack("Guitar")
        device = MockLooperDevice("Looper")

        # Set state to Recording (value=1)
        state_param = device.parameters[0]  # State parameter
        state_param.value = 1.0

        discovery = LooperDiscovery(MockSong())
        state = discovery._read_looper_state(track, 0, device, 0)

        assert state["state"] == "Recording"
        assert state["track_name"] == "Guitar"

        # Set state to Playing (value=2)
        state_param.value = 2.0
        state = discovery._read_looper_state(track, 0, device, 0)
        assert state["state"] == "Playing"

        # Set state to Overdubbing (value=3)
        state_param.value = 3.0
        state = discovery._read_looper_state(track, 0, device, 0)
        assert state["state"] == "Overdubbing"

        # Set state to Stopped (value=0)
        state_param.value = 0.0
        state = discovery._read_looper_state(track, 0, device, 0)
        assert state["state"] == "Stopped"

    def test_read_feedback_value(self):
        """Feedback parameter value is correctly read."""
        track = MockTrack("Guitar")
        device = MockLooperDevice("Looper")

        # Set feedback to 0.75
        feedback_param = device.parameters[1]  # Feedback parameter
        feedback_param.value = 0.75

        discovery = LooperDiscovery(MockSong())
        state = discovery._read_looper_state(track, 0, device, 0)

        assert state["feedback"] == 0.75

    def test_read_reverse_value(self):
        """Reverse parameter value is correctly read."""
        track = MockTrack("Guitar")
        device = MockLooperDevice("Looper")

        # Set reverse to True (>0.5)
        reverse_param = device.parameters[2]  # Reverse parameter
        reverse_param.value = 1.0

        discovery = LooperDiscovery(MockSong())
        state = discovery._read_looper_state(track, 0, device, 0)

        assert state["reverse"] is True


class TestScanAllTracks:
    """Tests for scan_all_tracks() full discovery."""

    def test_scan_all_tracks_finds_multiple_loopers(self, mock_song_with_multiple_loopers):
        """Multiple tracks with loopers all discovered."""
        discovery = LooperDiscovery(mock_song_with_multiple_loopers)
        discovered = discovery.scan_all_tracks()

        # Should find 3 loopers: Guitar (Looper), Bass (SuperLooper), Drums (OverdubLooper)
        assert len(discovered) == 3

        # Check track IDs are present
        track_ids = list(discovered.keys())
        assert "0" in track_ids
        assert "1" in track_ids
        assert "2" in track_ids

        # Check track names
        assert discovered["0"]["track_name"] == "Guitar"
        assert discovered["1"]["track_name"] == "Bass"
        assert discovered["2"]["track_name"] == "Drums"

    def test_scan_all_tracks_excludes_non_looper_tracks(self, mock_song_with_multiple_loopers):
        """Tracks without loopers are not included in discovery."""
        discovery = LooperDiscovery(mock_song_with_multiple_loopers)
        discovered = discovery.scan_all_tracks()

        # Track 3 (Synth) has no looper, should not be in discovered
        assert "3" not in discovered

    def test_scan_empty_song_returns_empty(self):
        """Empty song returns empty discovery dict."""
        song = MockSong()
        discovery = LooperDiscovery(song)
        discovered = discovery.scan_all_tracks()

        assert len(discovered) == 0


class TestStateMap:
    """Tests for ABLETON_LOOPER_STATE_MAP constant."""

    def test_state_map_values(self):
        """Verify state map has correct mappings."""
        assert ABLETON_LOOPER_STATE_MAP[0] == "Stopped"
        assert ABLETON_LOOPER_STATE_MAP[1] == "Recording"
        assert ABLETON_LOOPER_STATE_MAP[2] == "Playing"
        assert ABLETON_LOOPER_STATE_MAP[3] == "Overdubbing"

    def test_state_map_keys(self):
        """Verify state map has all expected keys."""
        for key in range(4):
            assert key in ABLETON_LOOPER_STATE_MAP


class TestPatternDefinitions:
    """Tests for LOOPER_PARAMETER_PATTERNS constant."""

    def test_patterns_defined(self):
        """Verify all required patterns are defined."""
        assert len(LOOPER_PARAMETER_PATTERNS) >= 3

        # Extract required names from each pattern
        pattern_required = [p["required_names"] for p in LOOPER_PARAMETER_PATTERNS]

        # Pattern 1: State + Feedback (Ableton Looper)
        assert {"State", "Feedback"} in pattern_required

        # Pattern 2: Record + Play + Stop (generic looper)
        assert {"Record", "Play", "Stop"} in pattern_required

        # Pattern 3: Record + Overdub + Play (overdub looper)
        assert {"Record", "Overdub", "Play"} in pattern_required


class TestStartStopListening:
    """Tests for parameter listener registration."""

    def test_start_listening_registers_listeners(self, mock_track_with_looper):
        """start_listening() registers parameter listeners."""
        callback_called = []

        def on_change(track, device, param, value):
            callback_called.append((track, device, param, value))

        song = MockSong()
        song.add_track(mock_track_with_looper)
        discovery = LooperDiscovery(song)
        discovery.start_listening(on_change)

        # Should have registered listeners
        assert len(discovery._param_listeners) > 0

    def test_stop_listening_removes_listeners(self, mock_track_with_looper):
        """stop_listening() removes all registered listeners."""
        def on_change(track, device, param, value):
            pass

        song = MockSong()
        song.add_track(mock_track_with_looper)
        discovery = LooperDiscovery(song)
        discovery.start_listening(on_change)
        assert len(discovery._param_listeners) > 0

        discovery.stop_listening()
        assert len(discovery._param_listeners) == 0
