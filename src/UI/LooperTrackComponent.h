#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Model/LooperState.h"

namespace looper {

class TransportButton;
class ExpandableDetailSection;

class LooperTrackComponent : public juce::Component
{
public:
    LooperTrackComponent();
    ~LooperTrackComponent() override;

    // Update all visual state from LooperState data (per D-10, D-04)
    void setState(const LooperState& state);

    // Toggle detail section expansion (per D-08)
    void setExpanded(bool expanded);
    bool isExpanded() const { return expanded_; }

    // Callbacks — wired by LooperListView to BridgeClient methods
    std::function<void(const juce::String& trackId, const juce::String& targetState)> onTransportClick;
    std::function<void(const juce::String& trackId)> onUndoClick;
    std::function<void(const juce::String& trackId)> onRedoClick;
    std::function<void(const juce::String& trackId, float feedbackValue)> onFeedbackChange;

    // Per D-02: ~44px compact row height
    static constexpr float compactHeight() { return 44.0f; }
    // Per D-11: expandable detail adds ~40px when opened
    static constexpr float detailHeight() { return 40.0f; }

private:
    LooperState state_;
    bool expanded_ = false;

    // Child components
    std::unique_ptr<TransportButton> transportButton_;
    std::unique_ptr<ExpandableDetailSection> detailSection_;
    juce::Label trackNameLabel_;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Helper for state color tint (per D-04)
    static juce::Colour stateTintColor(LooperState::State state);
};

} // namespace looper
