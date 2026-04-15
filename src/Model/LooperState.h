#pragma once

#include <juce_core/juce_core.h>
#include "Shared/ProtocolDefs.h"

namespace looper {

struct LooperState
{
    juce::String trackId;        // Stable identifier (track name for v1)
    juce::String trackName;      // Human-readable track name
    juce::String deviceId;       // Device index within track as string
    juce::String deviceName;      // Device display name
    juce::String className;       // Device class_name (e.g. "Looper")
    enum State { Stopped, Recording, Playing, Overdubbing } state = Stopped;
    float feedback = 0.5f;        // 0.0-1.0 normalized
    bool reverse = false;
    int loopLengthBars = 0;       // 0 = free, N = N bars

    // Comparison for diff-based updates (D-03)
    bool operator==(const LooperState& other) const;
    bool operator!=(const LooperState& other) const { return !(*this == other); }

    // Convert state enum to string (matching protocol constants)
    static juce::String stateToString(State s);
    static State stringToState(const juce::String& s);
};

} // namespace looper