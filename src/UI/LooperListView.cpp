#include "LooperListView.h"

namespace looper {

LooperListView::LooperListView()
{
    // Status header bar (fixed at top, per D-03)
    headerBar_ = std::make_unique<StatusHeaderBar>();
    addAndMakeVisible(*headerBar_);

    // Viewport for scrollable looper tracks (per D-03)
    viewport_ = std::make_unique<juce::Viewport>();
    addAndMakeVisible(*viewport_);

    // Content container holding vertically stacked LooperTrackComponents
    contentContainer_ = std::make_unique<juce::Component>();
    viewport_->setViewedComponent(contentContainer_.get(), false);

    // Empty state label (hidden by default)
    emptyStateLabel_ = std::make_unique<juce::Label>();
    emptyStateLabel_->setText("No loopers discovered", juce::dontSendNotification);
    emptyStateLabel_->setColour(juce::Label::textColourId, juce::Colours::grey);
    emptyStateLabel_->setFont(juce::Font(14.0f, juce::Font::plain));
    emptyStateLabel_->setJustificationType(juce::Justification::centred);
    emptyStateLabel_->setVisible(false);
    contentContainer_->addAndMakeVisible(*emptyStateLabel_);

    setSize(400, 500);  // Default size (per plan: 400x500 for 5+ loopers)
}

LooperListView::~LooperListView() = default;

void LooperListView::refreshFromTracker(LooperTracker& tracker)
{
    auto loopers = tracker.getAllLoopers();

    // Track which trackIds still exist
    std::set<juce::String> currentTrackIds;
    for (const auto& looper : loopers)
        currentTrackIds.insert(looper.trackId);

    // Remove track components for loopers that no longer exist
    for (auto it = trackComponents_.begin(); it != trackComponents_.end(); )
    {
        if (currentTrackIds.find(it->first) == currentTrackIds.end())
        {
            contentContainer_->removeChildComponent(it->second.get());
            it = trackComponents_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Add or update track components
    float yOffset = 0.0f;
    for (const auto& looper : loopers)
    {
        auto it = trackComponents_.find(looper.trackId);
        if (it == trackComponents_.end())
        {
            // New looper: create component
            auto trackComp = std::make_unique<LooperTrackComponent>();
            trackComp->setState(looper);

            // Wire callbacks
            trackComp->onTransportClick = onTransportClick;
            trackComp->onUndoClick = onUndoClick;
            trackComp->onRedoClick = onRedoClick;
            trackComp->onFeedbackChange = onFeedbackChange;

            contentContainer_->addAndMakeVisible(trackComp.get());
            trackComponents_[looper.trackId] = std::move(trackComp);
            it = trackComponents_.find(looper.trackId);
        }
        else
        {
            // Existing looper: update state
            it->second->setState(looper);
        }

        // Position this track component
        auto height = it->second->isExpanded()
            ? LooperTrackComponent::compactHeight() + LooperTrackComponent::detailHeight()
            : LooperTrackComponent::compactHeight();
        it->second->setBounds(0, static_cast<int>(yOffset),
                               contentContainer_->getWidth(), static_cast<int>(height));
        yOffset += height;
    }

    // Resize content container to fit all tracks
    contentContainer_->setSize(getWidth() - 20, static_cast<int>(yOffset));

    // If no loopers, show empty state message
    if (loopers.empty())
    {
        contentContainer_->setSize(getWidth() - 20, 100);
        emptyStateLabel_->setBounds(0, 0, contentContainer_->getWidth(), 100);
        emptyStateLabel_->setVisible(true);
    }
    else
    {
        emptyStateLabel_->setVisible(false);
    }
}

void LooperListView::setConnected(bool connected)
{
    headerBar_->setConnected(connected);
}

void LooperListView::resized()
{
    // Header bar: 32px fixed at top
    headerBar_->setBounds(getLocalBounds().removeFromTop(32));

    // Viewport fills remaining space
    viewport_->setBounds(getLocalBounds());
}

void LooperListView::paint(juce::Graphics& g)
{
    // Dark background (per theme)
    g.fillAll(juce::Colours::darkgrey);
}

} // namespace looper
