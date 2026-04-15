#include "PluginProcessor.h"
#include "PluginEditor.h"

LiveLooperProcessor::LiveLooperProcessor()
    : AudioProcessor(juce::AudioProcessor::BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

LiveLooperProcessor::~LiveLooperProcessor()
{
}

void LiveLooperProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // No preparation needed — audio passthrough
}

void LiveLooperProcessor::releaseResources()
{
    // No resources to release
}

void LiveLooperProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Audio passthrough — do nothing to the buffer
    // All state updates happen on the message thread via AsyncUpdater
    // MIDI input is accepted but not processed in Phase 1
}

juce::AudioProcessorEditor* LiveLooperProcessor::createEditor()
{
    return new LiveLooperEditor(*this);
}

void LiveLooperProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LiveLooperProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout LiveLooperProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Phase 1: minimal parameter for connection status
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("connected", 1), "Connected", false));

    return { params.begin(), params.end() };
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LiveLooperProcessor();
}
