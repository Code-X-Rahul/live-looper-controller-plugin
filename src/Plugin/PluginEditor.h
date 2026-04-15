#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class LiveLooperProcessor;

class LiveLooperEditor : public juce::AudioProcessorEditor
{
public:
    explicit LiveLooperEditor(LiveLooperProcessor& processor);
    ~LiveLooperEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    LiveLooperProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveLooperEditor)
};
