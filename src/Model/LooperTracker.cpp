#include "LooperTracker.h"

namespace looper {

void LooperTracker::updateState(const juce::String& trackId, const LooperState& newState)
{
    loopers[trackId] = newState;
}

const LooperState* LooperTracker::getState(const juce::String& trackId) const
{
    auto it = loopers.find(trackId);
    if (it != loopers.end())
        return &(it->second);
    return nullptr;
}

const std::map<juce::String, LooperState>& LooperTracker::getAllStates() const
{
    return loopers;
}

void LooperTracker::removeLooper(const juce::String& trackId)
{
    loopers.erase(trackId);
}

void LooperTracker::clear()
{
    loopers.clear();
}

} // namespace looper
