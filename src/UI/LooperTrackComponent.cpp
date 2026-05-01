#include "LooperTrackComponent.h"
#include "TransportButton.h"
#include "ExpandableDetailSection.h"

namespace looper {

LooperTrackComponent::LooperTrackComponent()
{
    // Track name label
    addAndMakeVisible(trackNameLabel_);
    trackNameLabel_.setFont(juce::Font(14.0f, juce::Font::bold));
    trackNameLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

    // Transport button (per D-07: single multi-purpose transport button)
    transportButton_ = std::make_unique<TransportButton>();
    addAndMakeVisible(*transportButton_);

    // Expandable detail section (per D-08: hidden by default)
    detailSection_ = std::make_unique<ExpandableDetailSection>();
    addChildComponent(*detailSection_);

    setSize(400, static_cast<int>(compactHeight()));
}

LooperTrackComponent::~LooperTrackComponent() = default;

void LooperTrackComponent::setState(const LooperState& state)
{
    state_ = state;

    // Update track name
    trackNameLabel_.setText(state_.trackName + " — " + state_.deviceName,
                             juce::dontSendNotification);

    // Update transport button
    transportButton_->setState(state_.state);
    transportButton_->setTrackId(state_.trackId);

    // Wire transport button callback
    transportButton_->onClick = [this](const juce::String& trackId,
                                        const juce::String& targetState) {
        if (onTransportClick)
            onTransportClick(trackId, targetState);
    };

    // Update detail section
    detailSection_->setState(state_);
    detailSection_->setTrackId(state_.trackId);

    // Wire detail section callbacks
    detailSection_->onUndo = [this](const juce::String& trackId) {
        if (onUndoClick)
            onUndoClick(trackId);
    };

    detailSection_->onRedo = [this](const juce::String& trackId) {
        if (onRedoClick)
            onRedoClick(trackId);
    };

    detailSection_->onFeedbackChange = [this](const juce::String& trackId, float value) {
        if (onFeedbackChange)
            onFeedbackChange(trackId, value);
    };

    // Per D-05: instant repaint, no animation
    repaint();
}

void LooperTrackComponent::setExpanded(bool expanded)
{
    if (expanded_ != expanded)
    {
        expanded_ = expanded;

        if (expanded_)
        {
            detailSection_->setVisible(true);
            detailSection_->expand();
        }
        else
        {
            detailSection_->collapse();
            detailSection_->setVisible(false);
        }

        // Update total height
        auto newHeight = expanded_
            ? compactHeight() + detailHeight()
            : compactHeight();
        setSize(getWidth(), static_cast<int>(newHeight));

        // Notify parent to update layout
        if (auto* parent = getParentComponent())
            parent->resized();
    }
}

juce::Colour LooperTrackComponent::stateTintColor(LooperState::State state)
{
    // Per D-04: full-row color tint
    // Recording: red (0.15 alpha), Overdubbing: orange (0.15),
    // Playing: green (0.15), Stopped: grey (0.1)
    using State = LooperState::State;
    switch (state)
    {
        case State::Recording:  return juce::Colours::red.withAlpha(0.15f);
        case State::Overdubbing: return juce::Colours::orange.withAlpha(0.15f);
        case State::Playing:    return juce::Colours::green.withAlpha(0.15f);
        case State::Stopped:    return juce::Colours::grey.withAlpha(0.1f);
        default:                return juce::Colours::transparentBlack;
    }
}

void LooperTrackComponent::paint(juce::Graphics& g)
{
    // Per D-04: full-row background color tint
    g.fillAll(stateTintColor(state_.state));
}

void LooperTrackComponent::resized()
{
    if (expanded_)
    {
        // Compact row
        auto compactBounds = getLocalBounds().removeFromTop(static_cast<int>(compactHeight()));

        // Track name: left 60%
        auto nameWidth = static_cast<int>(compactBounds.getWidth() * 0.6f);
        trackNameLabel_.setBounds(compactBounds.removeFromLeft(nameWidth));

        // Transport button: right ~36px, vertically centered
        transportButton_->setBounds(compactBounds.removeFromRight(36).reduced(4));

        // Detail section below compact row
        detailSection_->setBounds(getLocalBounds().removeFromTop(static_cast<int>(compactHeight()))
                                    .withHeight(static_cast<int>(detailHeight())));
    }
    else
    {
        // Compact row only
        auto compactBounds = getLocalBounds();

        // Track name: left 60%
        auto nameWidth = static_cast<int>(compactBounds.getWidth() * 0.6f);
        trackNameLabel_.setBounds(compactBounds.removeFromLeft(nameWidth));

        // Transport button: right ~36px, vertically centered
        transportButton_->setBounds(compactBounds.removeFromRight(36).reduced(4));
    }
}

void LooperTrackComponent::mouseDown(const juce::MouseEvent& e)
{
    // Per D-08: clicking on the track name area toggles expand/collapse
    // (NOT the transport button)
    if (e.y < compactHeight())
    {
        // Check if NOT clicking on transport button
        auto transportBounds = transportButton_->getBounds();
        if (!transportBounds.contains(e.x, e.y))
        {
            setExpanded(!expanded_);
        }
    }
}

} // namespace looper
