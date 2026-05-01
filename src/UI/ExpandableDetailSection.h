#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Model/LooperState.h"

namespace looper {

class ExpandableDetailSection : public juce::Component
{
public:
    ExpandableDetailSection();
    ~ExpandableDetailSection() override;

    // Update from looper state (per D-11: shows loop length, cycle count, undo/redo, feedback)
    void setState(const LooperState& state);

    // Expand/collapse controls (per D-08: hidden by default)
    void expand();
    void collapse();
    void toggle();
    bool isExpanded() const { return expanded_; }

    // Callbacks — wired by LooperListView to BridgeClient methods
    std::function<void(const juce::String& trackId)> onUndo;
    std::function<void(const juce::String& trackId)> onRedo;
    std::function<void(const juce::String& trackId, float value)> onFeedbackChange;

    void setTrackId(const juce::String& trackId) { trackId_ = trackId; }

    // Return ideal height for layout calculations
    static constexpr float expandedHeight() { return 40.0f; }
    static constexpr float collapsedHeight() { return 0.0f; }

protected:
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::String trackId_;
    LooperState state_;
    bool expanded_ = false;

    juce::Label loopLengthLabel_;
    juce::Label cycleCountLabel_;
    juce::TextButton undoButton_;
    juce::TextButton redoButton_;
    juce::Slider feedbackSlider_;
    juce::Label feedbackLabel_;

    void updateVisibility();
};

} // namespace looper
