#include "ExpandableDetailSection.h"

namespace looper {

ExpandableDetailSection::ExpandableDetailSection()
{
    // Loop length label
    addAndMakeVisible(loopLengthLabel_);
    loopLengthLabel_.setFont(juce::Font(12.0f));
    loopLengthLabel_.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Cycle count label (per D-13)
    addAndMakeVisible(cycleCountLabel_);
    cycleCountLabel_.setFont(juce::Font(12.0f));
    cycleCountLabel_.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Undo button (per D-08: hidden by default, visible when expanded)
    addAndMakeVisible(undoButton_);
    undoButton_.setButtonText("Undo");
    undoButton_.onClick = [this]() {
        if (onUndo)
            onUndo(trackId_);
    };

    // Redo button
    addAndMakeVisible(redoButton_);
    redoButton_.setButtonText("Redo");
    redoButton_.onClick = [this]() {
        if (onRedo)
            onRedo(trackId_);
    };

    // Feedback slider (per D-11: 0.0-1.0 range per CTRL-04)
    addAndMakeVisible(feedbackSlider_);
    feedbackSlider_.setRange(0.0f, 1.0f);
    feedbackSlider_.setValue(0.5f);
    feedbackSlider_.onValueChange = [this]() {
        if (onFeedbackChange)
            onFeedbackChange(trackId_, static_cast<float>(feedbackSlider_.getValue()));
    };

    // Feedback label
    addAndMakeVisible(feedbackLabel_);
    feedbackLabel_.setFont(juce::Font(11.0f));
    feedbackLabel_.setText("Feedback", juce::dontSendNotification);
    feedbackLabel_.setColour(juce::Label::textColourId, juce::Colours::grey);

    updateVisibility();
}

ExpandableDetailSection::~ExpandableDetailSection() = default;

void ExpandableDetailSection::setState(const LooperState& state)
{
    state_ = state;

    // Loop length
    if (state_.loopLengthBars > 0)
        loopLengthLabel_.setText(juce::String::formatted("%d bars", state_.loopLengthBars),
                                  juce::dontSendNotification);
    else
        loopLengthLabel_.setText("Free", juce::dontSendNotification);

    // Cycle count (per D-13)
    cycleCountLabel_.setText(juce::String::formatted("Cycle: %d", state_.cycleCount),
                              juce::dontSendNotification);

    // Feedback slider
    feedbackSlider_.setValue(state_.feedback, juce::dontSendNotification);
}

void ExpandableDetailSection::expand()
{
    if (!expanded_)
    {
        expanded_ = true;
        updateVisibility();
    }
}

void ExpandableDetailSection::collapse()
{
    if (expanded_)
    {
        expanded_ = false;
        updateVisibility();
    }
}

void ExpandableDetailSection::toggle()
{
    if (expanded_)
        collapse();
    else
        expand();
}

void ExpandableDetailSection::updateVisibility()
{
    // Per D-08: all detail controls hidden when collapsed, visible when expanded
    auto visible = expanded_ ? juce::VisibleWhenAlways : juce::VisibleWhenEmpty;

    loopLengthLabel_.setVisible(visible);
    cycleCountLabel_.setVisible(visible);
    undoButton_.setVisible(visible);
    redoButton_.setVisible(visible);
    feedbackSlider_.setVisible(visible);
    feedbackLabel_.setVisible(visible);

    // Notify parent to update layout
    if (auto* parent = getParentComponent())
        parent->resized();
}

void ExpandableDetailSection::paint(juce::Graphics& g)
{
    // Per D-05: no background paint for detail section
    // It inherits from parent background
}

void ExpandableDetailSection::resized()
{
    if (!expanded_)
        return;

    auto bounds = getLocalBounds();
    const float padding = 8.0f;
    const float buttonWidth = 50.0f;

    // Layout: loop length | cycle count | spacer | undo | redo | feedback label + slider
    auto y = bounds.getCentreY() - 12.0f;  // Vertically center controls in 40px

    // Loop length (left side)
    loopLengthLabel_.setBounds(bounds.getX() + padding, y, 60.0f, 24.0f);

    // Cycle count (left of center)
    cycleCountLabel_.setBounds(bounds.getX() + padding + 70.0f, y, 70.0f, 24.0f);

    // Undo button
    undoButton_.setBounds(bounds.getRight() - padding - buttonWidth * 2 - 120.0f, y, buttonWidth, 24.0f);

    // Redo button
    redoButton_.setBounds(bounds.getRight() - padding - buttonWidth - 60.0f, y, buttonWidth, 24.0f);

    // Feedback label + slider (right side)
    feedbackLabel_.setBounds(bounds.getRight() - padding - 130.0f, y, 50.0f, 24.0f);
    feedbackSlider_.setBounds(bounds.getRight() - padding - 75.0f, y + 4.0f, 70.0f, 16.0f);
}

} // namespace looper
