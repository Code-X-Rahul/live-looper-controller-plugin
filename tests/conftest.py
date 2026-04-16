"""
Pytest configuration and fixtures for Remote Script tests.
"""

import pytest
import sys
import os

# Add the remote-script directory to the path for imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'remote-script'))

from mock_live_api import (
    MockSong,
    MockTrack,
    MockDevice,
    MockLooperDevice,
    MockParameter,
)


@pytest.fixture
def mock_song():
    """Mock Live API Song object for testing."""
    return MockSong()


@pytest.fixture
def mock_track_with_looper():
    """Mock track containing an Ableton Looper device."""
    track = MockTrack("Guitar")
    track.add_device(MockLooperDevice("Looper"))
    return track


@pytest.fixture
def mock_track_with_generic_looper():
    """Mock track containing a third-party looper device."""
    track = MockTrack("Bass")
    device = MockDevice("SuperLooper", "GenericLooper")
    device.add_parameter("Record", 0.0)
    device.add_parameter("Play", 0.0)
    device.add_parameter("Stop", 0.0)
    track.add_device(device)
    return track


@pytest.fixture
def mock_song_with_multiple_loopers():
    """Mock song with multiple tracks containing various looper devices."""
    song = MockSong()

    # Track 1: Guitar with Ableton Looper
    guitar_track = MockTrack("Guitar")
    guitar_track.add_device(MockLooperDevice("Looper"))
    song.add_track(guitar_track)

    # Track 2: Bass with generic looper
    bass_track = MockTrack("Bass")
    generic_looper = MockDevice("SuperLooper", "GenericLooper")
    generic_looper.add_parameter("Record", 0.0)
    generic_looper.add_parameter("Play", 0.0)
    generic_looper.add_parameter("Stop", 0.0)
    bass_track.add_device(generic_looper)
    song.add_track(bass_track)

    # Track 3: Drums with overdub-style looper
    drums_track = MockTrack("Drums")
    overdub_looper = MockDevice("OverdubLooper", "CustomLooper")
    overdub_looper.add_parameter("Record", 0.0)
    overdub_looper.add_parameter("Overdub", 0.0)
    overdub_looper.add_parameter("Play", 0.0)
    drums_track.add_device(overdub_looper)
    song.add_track(drums_track)

    # Track 4: Synth with non-looper device
    synth_track = MockTrack("Synth")
    synth_track.add_device(MockDevice("Volume", "Volume"))
    synth_track.add_device(MockDevice("Pan", "Pan"))
    song.add_track(synth_track)

    return song
