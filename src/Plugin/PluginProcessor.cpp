#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace looper {

LiveLooperProcessor::LiveLooperProcessor()
    : AudioProcessor(juce::AudioProcessor::BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "Parameters", createParameterLayout())
{
}

LiveLooperProcessor::~LiveLooperProcessor()
{
}

void LiveLooperProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialize BridgeClient and connect (D-06: immediate handshake on load)
    bridgeClient_ = std::make_unique<BridgeClient>(looperTracker_);
    bridgeClient_->connect();
}

void LiveLooperProcessor::releaseResources()
{
    // Clean disconnect from Remote Script
    if (bridgeClient_)
    {
        bridgeClient_->disconnect();
        bridgeClient_.reset();
    }
}

void LiveLooperProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Audio passthrough — do nothing to the buffer (D-08)
    // All state updates happen on the message thread via BridgeClient
    // MIDI input is accepted but not processed in Phase 1
    (void)midiMessages;
}

juce::AudioProcessorEditor* LiveLooperProcessor::createEditor()
{
    return new LiveLooperEditor(*this);
}

void LiveLooperProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LiveLooperProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts_.state.getType()))
        apvts_.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout LiveLooperProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Phase 1: minimal parameter for connection status (Pitfall 6: at least one parameter)
    // Using ParameterID with version hint (Pitfall 3: prevents VST3/AU ID mismatch)
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("connected", 1), "Connected", false));

    return { params.begin(), params.end() };
}

} // namespace looper

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new looper::LiveLooperProcessor();
}