#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Bridge/BridgeClient.h"

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

    // Timer callback for updating connection status display
    void timerCallback() override;

private:
    LiveLooperProcessor& processor_;
    juce::Label statusLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveLooperEditor)
};

} // namespace looper