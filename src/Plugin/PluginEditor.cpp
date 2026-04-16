#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace looper {

LiveLooperEditor::LiveLooperEditor(LiveLooperProcessor& processor)
    : AudioProcessorEditor(&processor), processor_(processor)
{
    setSize(500, 350);

    // Connection status label at top
    addAndMakeVisible(statusLabel_);
    statusLabel_.setText("Connecting...", juce::dontSendNotification);
    statusLabel_.setJustificationType(juce::Justification::centred);
    statusLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    statusLabel_.setFont(juce::Font(16.0f, juce::Font::bold));

    // Looper count label
    addAndMakeVisible(looperCountLabel_);
    looperCountLabel_.setText("Loopers: 0", juce::dontSendNotification);
    looperCountLabel_.setJustificationType(juce::Justification::topLeft);
    looperCountLabel_.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Looper list label - shows each looper's track name and state
    addAndMakeVisible(looperListLabel_);
    looperListLabel_.setText("No loopers discovered", juce::dontSendNotification);
    looperListLabel_.setJustificationType(juce::Justification::topLeft);
    looperListLabel_.setColour(juce::Label::textColourId, juce::Colours::grey);

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
    auto bounds = getLocalBounds();

    // Status label at top
    statusLabel_.setBounds(bounds.removeFromTop(40));

    // Looper count below status
    looperCountLabel_.setBounds(bounds.removeFromTop(30));

    // Looper list takes remaining space
    looperListLabel_.setBounds(bounds);
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

    // Update looper count and list from tracker
    auto& tracker = processor_.getLooperTracker();
    auto loopers = tracker.getAllLoopers();

    looperCountLabel_.setText(
        juce::String::formatted("Loopers: %d", loopers.size()),
        juce::dontSendNotification);

    // Build looper list string
    if (loopers.empty())
    {
        looperListLabel_.setText("No loopers discovered", juce::dontSendNotification);
    }
    else
    {
        juce::StringArray lines;
        for (const auto& looper : loopers)
        {
            lines.add(juce::String::formatted("  [%s] %s: %s",
                looper.deviceName,
                looper.trackName,
                LooperState::stateToString(looper.state)));
        }
        looperListLabel_.setText(lines.joinIntoString("\n"), juce::dontSendNotification);
    }
}

} // namespace looper
