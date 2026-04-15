#pragma once

#include <juce_core/juce_core.h>

namespace looper {

struct LooperState
{
    juce::String trackId;
    juce::String trackName;
    juce::String deviceId;
    enum State { Stopped, Recording, Playing, Overdubbing } state = Stopped;
    float feedback = 0.0f;
    bool reverse = false;
    int loopLengthBars = 0;
};

} // namespace looper
