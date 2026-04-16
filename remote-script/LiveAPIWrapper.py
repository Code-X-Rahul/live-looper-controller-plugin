"""Helper functions for the Live API."""

from typing import Optional, Tuple


def get_track_by_index(song, index: int):
    """Get a track by its index, handling audio/MIDI/return/master tracks.

    Args:
        song: MockSong or Live API Song object
        index: Track index

    Returns:
        Track object or None if index out of range
    """
    tracks = list(song.tracks)
    if hasattr(song, 'return_tracks'):
        tracks.extend(list(song.return_tracks))
    if hasattr(song, 'master_track'):
        tracks.append(song.master_track)

    if 0 <= index < len(tracks):
        return tracks[index]
    return None


def get_device_parameter_by_name(device, param_name: str) -> Optional[Tuple]:
    """Find a parameter by name on a device.

    Args:
        device: MockDevice or Live API Device object
        param_name: Name of the parameter to find

    Returns:
        Tuple of (parameter, index) or None if not found
    """
    for i, param in enumerate(device.parameters):
        if param.name == param_name:
            return (param, i)
    return None


def set_device_parameter_by_name(device, param_name: str, value: float) -> bool:
    """Set a device parameter by name.

    Args:
        device: MockDevice or Live API Device object
        param_name: Name of the parameter to set
        value: New value for the parameter

    Returns:
        True if parameter was found and set, False otherwise
    """
    result = get_device_parameter_by_name(device, param_name)
    if result:
        param, _ = result
        param.value = value
        return True
    return False


def get_all_track_names(song) -> list:
    """Get names of all tracks in the song.

    Args:
        song: MockSong or Live API Song object

    Returns:
        List of track names
    """
    return [track.name for track in song.tracks]


def count_looper_devices(song, discovery_func) -> int:
    """Count the number of looper-like devices in the song.

    Args:
        song: MockSong or Live API Song object
        discovery_func: Function to check if a device is a looper

    Returns:
        Number of looper devices found
    """
    count = 0
    for track in song.tracks:
        for device in track.devices:
            if discovery_func(device):
                count += 1
    return count
