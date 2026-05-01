#include "StatusHeaderBar.h"

namespace looper {

StatusHeaderBar::StatusHeaderBar()
{
    setSize(400, 32);  // Per D-06: ~32px fixed height
}

StatusHeaderBar::~StatusHeaderBar() = default;

void StatusHeaderBar::setConnected(bool connected)
{
    if (connected_ != connected)
    {
        connected_ = connected;
        repaint();  // Per D-05: instant snap transition
    }
}

void StatusHeaderBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dark background bar
    g.setColour(juce::Colours::darkgrey.withAlpha(0.95f));
    g.fillRect(bounds);

    // Status text and indicator dot
    juce::Font statusFont(14.0f, juce::Font::bold);
    g.setFont(statusFont);

    if (connected_)
    {
        // Green dot + "Connected"
        g.setColour(juce::Colours::green);
        g.fillEllipse(getLocalBounds().getX() + 12.0f,
                      getLocalBounds().getCentreY() - 4.0f,
                      8.0f, 8.0f);
        g.setColour(juce::Colours::white);
        g.drawText("Connected", getLocalBounds().getX() + 28.0f,
                   getLocalBounds().getY(),
                   getLocalBounds().getWidth() - 28.0f,
                   getLocalBounds().getHeight(),
                   juce::Justification::left);
    }
    else
    {
        // Red dot + "Reconnecting..."
        g.setColour(juce::Colours::red);
        g.fillEllipse(getLocalBounds().getX() + 12.0f,
                      getLocalBounds().getCentreY() - 4.0f,
                      8.0f, 8.0f);
        g.setColour(juce::Colours::white);
        g.drawText("Reconnecting...", getLocalBounds().getX() + 28.0f,
                   getLocalBounds().getY(),
                   getLocalBounds().getWidth() - 28.0f,
                   getLocalBounds().getHeight(),
                   juce::Justification::left);
    }
}

} // namespace looper
