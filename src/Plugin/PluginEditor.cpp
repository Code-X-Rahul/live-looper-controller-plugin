#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace looper {

LiveLooperEditor::LiveLooperEditor(LiveLooperProcessor& processor)
    : AudioProcessorEditor(&processor), processor_(processor)
{
    setSize(400, 300);

    // Phase 1: minimal editor showing connection status (Pitfall 5: all state in processor)
    addAndMakeVisible(statusLabel_);
    statusLabel_.setText("Connecting...", juce::dontSendNotification);
    statusLabel_.setJustificationType(juce::Justification::centred);
    statusLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

    // Start timer to update connection status periodically
    startTimer(500);  // Update every 500ms
}

LiveLooperEditor::~LiveLooperEditor()
{
    stopTimer();
}

void LiveLooperEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void LiveLooperEditor::resized()
{
    statusLabel_.setBounds(getLocalBounds());
}

void LiveLooperEditor::timerCallback()
{
    // Read connection status from APVTS parameter (Pitfall 5: all state in processor)
    auto* connectedParam = dynamic_cast<juce::AudioParameterBool*>(
        processor_.getAPVTS().getParameter("connected"));

    if (connectedParam != nullptr)
    {
        bool isConnected = connectedParam->get();
        if (isConnected)
        {
            statusLabel_.setText("Connected", juce::dontSendNotification);
            statusLabel_.setColour(juce::Label::textColourId, juce::Colours::green);
        }
        else
        {
            statusLabel_.setText("Disconnected \xe2\x80\x94 reconnecting...", juce::dontSendNotification);
            statusLabel_.setColour(juce::Label::textColourId, juce::Colours::red);
        }
    }
    else
    {
        statusLabel_.setText("Connecting...", juce::dontSendNotification);
        statusLabel_.setColour(juce::Label::textColourId, juce::Colours::yellow);
    }
}

} // namespace looper