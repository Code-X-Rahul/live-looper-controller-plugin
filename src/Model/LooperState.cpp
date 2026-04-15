#include "LooperState.h"

namespace looper {

bool LooperState::operator==(const LooperState& other) const
{
    return trackId == other.trackId
        && trackName == other.trackName
        && deviceId == other.deviceId
        && deviceName == other.deviceName
        && className == other.className
        && state == other.state
        && std::abs(feedback - other.feedback) < 0.001f
        && reverse == other.reverse
        && loopLengthBars == other.loopLengthBars;
}

juce::String LooperState::stateToString(State s)
{
    switch (s)
    {
        case Stopped:     return protocol::STATE_STOPPED;
        case Recording:   return protocol::STATE_RECORDING;
        case Playing:     return protocol::STATE_PLAYING;
        case Overdubbing: return protocol::STATE_OVERDUBBING;
        default:          return protocol::STATE_STOPPED;
    }
}

LooperState::State LooperState::stringToState(const juce::String& s)
{
    if (s == protocol::STATE_RECORDING)   return Recording;
    if (s == protocol::STATE_PLAYING)    return Playing;
    if (s == protocol::STATE_OVERDUBBING) return Overdubbing;
    return Stopped;
}

} // namespace looper