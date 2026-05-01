#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Model/LooperState.h"

namespace looper {

class TransportButton : public juce::Component
{
public:
    TransportButton();
    ~TransportButton() override;

    // Update display based on looper state (per D-05: snap color change, no animation)
    void setState(LooperState::State newState);

    // Callback for click events — wired by parent to BridgeClient::sendSetState
    std::function<void(const juce::String& trackId, const juce::String& targetState)> onClick;

    void setTrackId(const juce::String& trackId) { trackId_ = trackId; }

protected:
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    juce::String trackId_;
    LooperState::State state_ = LooperState::Stopped;

    // Double-click detection
    juce::Time lastClickTime_;
    static constexpr int doubleClickMs_ = 400;

    // Cycling state machine per D-09:
    // Stopped → Recording → Playing → Overdubbing (toggle) → Playing (toggle)
    // Double-click always goes to Stopped
    juce::String getNextState(LooperState::State current) const;
    juce::String getStateSymbol(LooperState::State state) const;
    juce::Colour getStateColour(LooperState::State state) const;
};

} // namespace looper
