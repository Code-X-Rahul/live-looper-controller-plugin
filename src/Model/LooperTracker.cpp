#include "LooperTracker.h"

namespace looper {

void LooperTracker::addLooper(const LooperState& state)
{
    loopers_[state.trackId] = state;
    notifyChange();
}

void LooperTracker::removeLooper(const juce::String& trackId)
{
    if (loopers_.erase(trackId) > 0)
    {
        notifyChange();
    }
}

void LooperTracker::updateState(const juce::String& trackId, const LooperState& newState)
{
    auto it = loopers_.find(trackId);
    if (it != loopers_.end())
    {
        // D-03: Only notify listeners if state actually changed (diff-based)
        if (it->second != newState)
        {
            it->second = newState;
            notifyChange();
        }
    }
    else
    {
        // New looper — add it and notify
        loopers_[trackId] = newState;
        notifyChange();
    }
}

std::vector<LooperState> LooperTracker::getAllLoopers() const
{
    std::vector<LooperState> result;
    result.reserve(loopers_.size());
    for (const auto& pair : loopers_)
    {
        result.push_back(pair.second);
    }
    return result;
}

std::optional<LooperState> LooperTracker::getLooper(const juce::String& trackId) const
{
    auto it = loopers_.find(trackId);
    if (it != loopers_.end())
        return it->second;
    return std::nullopt;
}

int LooperTracker::getLooperCount() const
{
    return static_cast<int>(loopers_.size());
}

void LooperTracker::setConnected(bool connected)
{
    if (connected_ != connected)
    {
        connected_ = connected;
        notifyChange();
    }
}

bool LooperTracker::isConnected() const
{
    return connected_;
}

void LooperTracker::onStateChange(ChangeCallback callback)
{
    onChange_ = std::move(callback);
}

void LooperTracker::notifyChange()
{
    if (onChange_)
    {
        onChange_();
    }
}

} // namespace looper