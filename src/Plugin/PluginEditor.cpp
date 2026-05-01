#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace looper {

LiveLooperEditor::LiveLooperEditor(LiveLooperProcessor& processor)
    : AudioProcessorEditor(&processor), processor_(processor)
{
    // Per plan: 400px wide x 500px tall (good default for 5+ loopers)
    setSize(400, 500);

    // Create LooperListView (per D-03: scrollable viewport with fixed header)
    looperListView_ = std::make_unique<LooperListView>();
    addAndMakeVisible(*looperListView_);

    // Wire callbacks to BridgeClient commands (per CTRL-01, CTRL-02, CTRL-04)
    looperListView_->onTransportClick = [this](const juce::String& trackId,
                                               const juce::String& targetState) {
        if (auto* bridge = processor_.getBridgeClient())
            bridge->sendSetState(trackId, targetState);
    };

    looperListView_->onUndoClick = [this](const juce::String& trackId) {
        if (auto* bridge = processor_.getBridgeClient())
            bridge->sendUndo(trackId);
    };

    looperListView_->onRedoClick = [this](const juce::String& trackId) {
        if (auto* bridge = processor_.getBridgeClient())
            bridge->sendRedo(trackId);
    };

    looperListView_->onFeedbackChange = [this](const juce::String& trackId, float value) {
        if (auto* bridge = processor_.getBridgeClient())
            bridge->sendSetFeedback(trackId, value);
    };

    // Register LooperTracker state change callback for reactive updates
    // (per VIS-03: <100ms latency via 50ms Timer + callback combo)
    processor_.getLooperTracker().onStateChange([this]() {
        stateChangePending_ = true;
    });

    // Start Timer with 50ms interval (per VIS-03: <100ms updates)
    startTimer(50);
}

LiveLooperEditor::~LiveLooperEditor()
{
    stopTimer();
}

void LiveLooperEditor::paint(juce::Graphics& g)
{
    // Per theme: dark background
    g.fillAll(juce::Colours::darkgrey);
}

void LiveLooperEditor::resized()
{
    looperListView_->setBounds(getLocalBounds());
}

void LiveLooperEditor::timerCallback()
{
    // Per PITFALL 5: all persistent state in processor, editor just reads it
    auto& tracker = processor_.getLooperTracker();

    // Update connection status
    looperListView_->setConnected(tracker.isConnected());

    // Refresh all looper track states
    looperListView_->refreshFromTracker(tracker);

    // Clear pending flag
    stateChangePending_ = false;
}

void LiveLooperEditor::handleStateChange()
{
    // This is called both by Timer and by the LooperTracker callback
    // The callback sets stateChangePending_, Timer does the actual refresh
    // This ensures we always refresh on timer even if callback missed
    stateChangePending_ = true;
}

} // namespace looper
