#include "PluginEditor.h"
#include "PluginProcessor.h"

LiveLooperEditor::LiveLooperEditor(LiveLooperProcessor& processor)
    : AudioProcessorEditor(&processor), processorRef(processor)
{
    setSize(400, 300);
}

LiveLooperEditor::~LiveLooperEditor()
{
}

void LiveLooperEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("Live Looper Controller", getLocalBounds(), juce::Justification::centred, 1);
}

void LiveLooperEditor::resized()
{
    // Layout will be implemented in Phase 2 (UI)
}
