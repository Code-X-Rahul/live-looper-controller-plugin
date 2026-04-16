"""
Mock Live API objects for testing the Remote Script.
These simulate Ableton Live's API for unit testing without Live running.
"""

from typing import List, Callable, Any, Optional
import logging

logger = logging.getLogger(__name__)


class MockParameter:
    """Simulates a Live API device parameter."""

    def __init__(self, name: str, value: float = 0.0):
        self._name = name
        self._value = value
        self._listeners: List[Callable] = []

    @property
    def name(self) -> str:
        return self._name

    @property
    def value(self) -> float:
        return self._value

    @value.setter
    def value(self, v: float):
        self._value = v
        # Notify all listeners
        for listener in self._listeners:
            try:
                listener(v)
            except Exception as e:
                logger.warning(f"Parameter listener error: {e}")

    def add_value_listener(self, listener: Callable):
        """Register a callback for value changes."""
        self._listeners.append(listener)

    def remove_value_listener(self, listener: Callable):
        """Remove a value change callback."""
        if listener in self._listeners:
            self._listeners.remove(listener)


class MockDevice:
    """Simulates a Live API device."""

    def __init__(self, name: str, class_name: str = ""):
        self._name = name
        self._class_name = class_name
        self._parameters: List[MockParameter] = []

    @property
    def name(self) -> str:
        return self._name

    @property
    def class_name(self) -> str:
        return self._class_name

    @property
    def parameters(self) -> List[MockParameter]:
        return self._parameters

    def add_parameter(self, name: str, value: float = 0.0) -> MockParameter:
        """Add a parameter to this device."""
        param = MockParameter(name, value)
        self._parameters.append(param)
        return param


class MockLooperDevice(MockDevice):
    """Simulates an Ableton Looper device with preset parameters."""

    def __init__(self, name: str = "Looper"):
        super().__init__(name, "Looper")
        # Ableton Looper parameters
        # State: 0=Stopped, 1=Recording, 2=Playing, 3=Overdubbing
        self.add_parameter("State", 0.0)
        self.add_parameter("Feedback", 0.5)
        self.add_parameter("Reverse", 0.0)
        self.add_parameter("Speed", 0.0)
        self.add_parameter("Quantize", 0.0)
        self.add_parameter("Tempo Control", 0.0)
        self.add_parameter("Input -> Output", 0.0)
        self.add_parameter("Record Length", 0.0)


class MockTrack:
    """Simulates a Live API track."""

    def __init__(self, name: str):
        self._name = name
        self._devices: List[MockDevice] = []
        self._devices_listeners: List[Callable] = []

    @property
    def name(self) -> str:
        return self._name

    @name.setter
    def name(self, v: str):
        self._name = v

    @property
    def devices(self) -> List[MockDevice]:
        return self._devices

    def add_device(self, device: MockDevice):
        """Add a device to this track."""
        self._devices.append(device)

    def add_devices_listener(self, listener: Callable):
        """Register a callback for device list changes."""
        self._devices_listeners.append(listener)

    def remove_devices_listener(self, listener: Callable):
        """Remove a devices change callback."""
        if listener in self._devices_listeners:
            self._devices_listeners.remove(listener)


class MockSong:
    """Simulates the Live API Song object."""

    def __init__(self):
        self._tracks: List[MockTrack] = []
        self._tracks_listeners: List[Callable] = []

    @property
    def tracks(self) -> List[MockTrack]:
        return self._tracks

    def add_track(self, track: MockTrack):
        """Add a track to the song."""
        self._tracks.append(track)
        # Notify listeners
        for listener in self._tracks_listeners:
            try:
                listener()
            except Exception as e:
                logger.warning(f"Tracks listener error: {e}")

    def add_tracks_listener(self, listener: Callable):
        """Register a callback for track list changes."""
        self._tracks_listeners.append(listener)

    def remove_tracks_listener(self, listener: Callable):
        """Remove a tracks change callback."""
        if listener in self._tracks_listeners:
            self._tracks_listeners.remove(listener)
