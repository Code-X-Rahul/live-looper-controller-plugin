#pragma once

#include "LooperState.h"
#include <juce_core/juce_core.h>
#include <map>
#include <vector>
#include <optional>
#include <functional>

namespace looper {

class LooperTracker
{
public:
    LooperTracker() = default;

    // Change callback type — called when state changes
    using ChangeCallback = std::function<void()>;

    // Add a looper (from discovery)
    void addLooper(const LooperState& state);

    // Remove a looper by trackId
    void removeLooper(const juce::String& trackId);

    // Update state (from Remote Script push, D-03: full state push)
    // Only notifies listeners if state actually changed (diff-based)
    void updateState(const juce::String& trackId, const LooperState& newState);

    // Get state
    std::vector<LooperState> getAllLoopers() const;
    std::optional<LooperState> getLooper(const juce::String& trackId) const;
    int getLooperCount() const;

    // Connection status
    void setConnected(bool connected);
    bool isConnected() const;

    // Change callback subscription
    void onStateChange(ChangeCallback callback);

private:
    void notifyChange();

    std::map<juce::String, LooperState> loopers_;  // key: trackId
    bool connected_ = false;
    ChangeCallback onChange_;
};

} // namespace looper