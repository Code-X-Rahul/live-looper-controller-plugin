#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "StatusHeaderBar.h"
#include "LooperTrackComponent.h"
#include "../Model/LooperTracker.h"

namespace looper {

class LooperListView : public juce::Component
{
public:
    LooperListView();
    ~LooperListView() override;

    // Rebuild/update from current LooperTracker state
    void refreshFromTracker(LooperTracker& tracker);

    // Set connection status on header bar
    void setConnected(bool connected);

    // Callbacks for transport, undo, redo, feedback
    // Wired by PluginEditor to BridgeClient commands
    std::function<void(const juce::String& trackId, const juce::String& targetState)> onTransportClick;
    std::function<void(const juce::String& trackId)> onUndoClick;
    std::function<void(const juce::String& trackId)> onRedoClick;
    std::function<void(const juce::String& trackId, float feedbackValue)> onFeedbackChange;

private:
    std::unique_ptr<StatusHeaderBar> headerBar_;
    std::unique_ptr<juce::Viewport> viewport_;
    std::unique_ptr<juce::Component> contentContainer_;
    std::map<juce::String, std::unique_ptr<LooperTrackComponent>> trackComponents_;

    void resized() override;
    void paint(juce::Graphics& g) override;
};

} // namespace looper
