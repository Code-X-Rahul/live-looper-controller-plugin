# Ableton Live Remote Script entry point
# This script is loaded by Ableton Live when the ControlSurface is enabled

from .LooperControlSurface import LooperControlSurface


def create_instance(c_instance):
    """Called by Live when the ControlSurface is selected.

    Args:
        c_instance: The ControlSurface instance provided by Ableton Live

    Returns:
        LooperControlSurface instance
    """
    return LooperControlSurface(c_instance)
