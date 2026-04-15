#pragma once

#include "LooperState.h"
#include <map>
#include <juce_core/juce_core.h>

namespace looper {

class LooperTracker
{
public:
    LooperTracker() = default;
    ~LooperTracker() = default;

    void updateState(const juce::String& trackId, const LooperState& newState);
    const LooperState* getState(const juce::String& trackId) const;
    const std::map<juce::String, LooperState>& getAllStates() const;
    void removeLooper(const juce::String& trackId);
    void clear();

private:
    std::map<juce::String, LooperState> loopers;
};

} // namespace looper
