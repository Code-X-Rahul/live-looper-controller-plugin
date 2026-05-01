#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "UI/LooperListView.h"

namespace looper {

class LiveLooperProcessor;

class LiveLooperEditor : public juce::AudioProcessorEditor,
                         public juce::Timer
{
public:
    explicit LiveLooperEditor(LiveLooperProcessor& processor);
    ~LiveLooperEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    LiveLooperProcessor& processor_;
    std::unique_ptr<LooperListView> looperListView_;

    // Per PITFALL 5: Timer-based refresh reads state from processor_.getLooperTracker()
    // LooperTracker::onStateChange triggers more responsive updates
    bool stateChangePending_ = false;
    void handleStateChange();  // Called by Timer and by LooperTracker callback

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveLooperEditor)
};

} // namespace looper
