#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Model/LooperTracker.h"
#include "../Bridge/BridgeClient.h"

namespace looper {

class LiveLooperEditor;

class LiveLooperProcessor : public juce::AudioProcessor
{
public:
    LiveLooperProcessor();
    ~LiveLooperProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // State persistence
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Required overrides
    const juce::String getName() const override { return "Live Looper Controller"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    // APVTS access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }
    LooperTracker& getLooperTracker() { return looperTracker_; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts_;
    LooperTracker looperTracker_;
    std::unique_ptr<BridgeClient> bridgeClient_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveLooperProcessor)
};

} // namespace looper