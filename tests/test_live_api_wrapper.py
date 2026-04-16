"""Tests for LiveAPIWrapper helper functions."""

import sys
import os

# Add the remote-script directory to the path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'remote-script'))

import pytest
from mock_live_api import MockSong, MockTrack, MockDevice, MockLooperDevice
from LiveAPIWrapper import (
    get_track_by_index,
    get_device_parameter_by_name,
    set_device_parameter_by_name,
    get_all_track_names,
    count_looper_devices,
)
from LooperDiscovery import LooperDiscovery


class TestGetTrackByIndex:
    """Tests for get_track_by_index()."""

    def test_get_track_by_index_valid(self, mock_song_with_multiple_loopers):
        """Get track by valid index returns track."""
        track = get_track_by_index(mock_song_with_multiple_loopers, 0)
        assert track is not None
        assert track.name == "Guitar"

    def test_get_track_by_index_out_of_range(self, mock_song_with_multiple_loopers):
        """Get track by out-of-range index returns None."""
        track = get_track_by_index(mock_song_with_multiple_loopers, 100)
        assert track is None

    def test_get_track_by_index_negative(self, mock_song_with_multiple_loopers):
        """Get track by negative index returns None."""
        track = get_track_by_index(mock_song_with_multiple_loopers, -1)
        assert track is None


class TestGetDeviceParameterByName:
    """Tests for get_device_parameter_by_name()."""

    def test_get_existing_parameter(self, mock_track_with_looper):
        """Get parameter that exists returns (param, index)."""
        device = mock_track_with_looper.devices[0]
        result = get_device_parameter_by_name(device, "State")
        assert result is not None
        param, idx = result
        assert param.name == "State"
        assert idx == 0

    def test_get_nonexistent_parameter(self, mock_track_with_looper):
        """Get parameter that doesn't exist returns None."""
        device = mock_track_with_looper.devices[0]
        result = get_device_parameter_by_name(device, "NonExistent")
        assert result is None

    def test_get_feedback_parameter(self, mock_track_with_looper):
        """Get Feedback parameter returns correct index."""
        device = mock_track_with_looper.devices[0]
        result = get_device_parameter_by_name(device, "Feedback")
        assert result is not None
        param, idx = result
        assert param.name == "Feedback"
        assert idx == 1


class TestSetDeviceParameterByName:
    """Tests for set_device_parameter_by_name()."""

    def test_set_existing_parameter(self, mock_track_with_looper):
        """Set parameter that exists returns True and changes value."""
        device = mock_track_with_looper.devices[0]

        # Get the State parameter
        result = get_device_parameter_by_name(device, "State")
        param, _ = result
        initial_value = param.value

        # Set to new value
        success = set_device_parameter_by_name(device, "State", 1.0)
        assert success is True
        assert param.value == 1.0

    def test_set_nonexistent_parameter(self, mock_track_with_looper):
        """Set parameter that doesn't exist returns False."""
        device = mock_track_with_looper.devices[0]
        success = set_device_parameter_by_name(device, "NonExistent", 1.0)
        assert success is False

    def test_set_parameter_preserves_other_params(self, mock_track_with_looper):
        """Setting one parameter doesn't affect others."""
        device = mock_track_with_looper.devices[0]

        # Get initial values
        feedback_param, _ = get_device_parameter_by_name(device, "Feedback")
        initial_feedback = feedback_param.value

        # Set State
        set_device_parameter_by_name(device, "State", 2.0)

        # Feedback should be unchanged
        assert feedback_param.value == initial_feedback


class TestGetAllTrackNames:
    """Tests for get_all_track_names()."""

    def test_get_all_track_names(self, mock_song_with_multiple_loopers):
        """Get all track names returns list of names."""
        names = get_all_track_names(mock_song_with_multiple_loopers)
        assert len(names) == 4
        assert "Guitar" in names
        assert "Bass" in names
        assert "Drums" in names
        assert "Synth" in names

    def test_get_all_track_names_empty_song(self):
        """Empty song returns empty list."""
        song = MockSong()
        names = get_all_track_names(song)
        assert len(names) == 0


class TestCountLooperDevices:
    """Tests for count_looper_devices()."""

    def test_count_looper_devices(self, mock_song_with_multiple_loopers):
        """Count looper devices returns correct count."""
        from LooperDiscovery import LooperDiscovery
        discovery = LooperDiscovery(mock_song_with_multiple_loopers)
        count = count_looper_devices(mock_song_with_multiple_loopers, discovery._is_looper_device)
        assert count == 3  # Guitar, Bass, Drums have loopers; Synth doesn't

    def test_count_looper_devices_no_loopers(self):
        """Song with no loopers returns 0."""
        song = MockSong()
        track = MockTrack("Audio")
        track.add_device(MockDevice("Volume", "Volume"))
        song.add_track(track)

        discovery = LooperDiscovery(song)
        count = count_looper_devices(song, discovery._is_looper_device)
        assert count == 0
