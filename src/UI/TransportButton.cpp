#include "TransportButton.h"

namespace looper {

TransportButton::TransportButton()
{
    setSize(36, 36);  // Per D-02: ~36x36px compact button
}

TransportButton::~TransportButton() = default;

void TransportButton::setState(LooperState::State newState)
{
    if (state_ != newState)
    {
        state_ = newState;
        repaint();  // Per D-05: instant repaint, no animation
    }
}

juce::String TransportButton::getNextState(LooperState::State current) const
{
    // Per D-09: Ableton-native state cycling behavior
    // Stopped → Recording → Playing → Overdubbing (toggle) → Playing (toggle)
    using State = LooperState::State;
    switch (current)
    {
        case State::Stopped:    return LooperState::stateToString(State::Recording);
        case State::Recording: return LooperState::stateToString(State::Playing);
        case State::Playing:   return LooperState::stateToString(State::Overdubbing);
        case State::Overdubbing: return LooperState::stateToString(State::Playing);  // toggle
        default:               return LooperState::stateToString(State::Stopped);
    }
}

juce::String TransportButton::getStateSymbol(LooperState::State state) const
{
    using State = LooperState::State;
    switch (state)
    {
        case State::Stopped:    return "\xE2\x96\xB9";   // ⏹ (stop square)
        case State::Recording: return "\xE2\x97\x89";   // ⏺ (record circle)
        case State::Playing:    return "\xE2\x96\xB6";   // ▶ (play triangle)
        case State::Overdubbing: return "\xE2\x8A\x95";  // ⊕ (circled plus)
        default:                return "?";
    }
}

juce::Colour TransportButton::getStateColour(LooperState::State state) const
{
    using State = LooperState::State;
    switch (state)
    {
        case State::Stopped:    return juce::Colours::grey;
        case State::Recording: return juce::Colours::red;
        case State::Playing:    return juce::Colours::green;
        case State::Overdubbing: return juce::Colours::orange;
        default:                return juce::Colours::grey;
    }
}

void TransportButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto cornerRadius = 4.0f;

    // Draw rounded rectangle button with state-specific fill
    g.setColour(getStateColour(state_).withAlpha(0.3f));
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Border
    g.setColour(getStateColour(state_));
    g.drawRoundedRectangle(bounds, cornerRadius, 2.0f);

    // Symbol text centered
    g.setColour(getStateColour(state_));
    g.setFont(juce::Font(18.0f, juce::Font::plain));
    g.drawText(getStateSymbol(state_), bounds, juce::Justification::centred);
}

void TransportButton::mouseDown(const juce::MouseEvent& e)
{
    auto now = juce::Time::getCurrentTime();
    auto elapsed = now - lastClickTime_;

    if (elapsed.inMilliseconds() < doubleClickMs_)
    {
        // Double-click: always go to Stopped (per D-09)
        if (onClick)
            onClick(trackId_, LooperState::stateToString(LooperState::Stopped));
        lastClickTime_ = juce::Time();  // Reset to avoid triple-click detection
    }
    else
    {
        // Single-click: cycle to next state per D-09
        auto nextState = getNextState(state_);
        if (onClick)
            onClick(trackId_, nextState);
        lastClickTime_ = now;
    }
}

} // namespace looper
