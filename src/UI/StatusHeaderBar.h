#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace looper {

class StatusHeaderBar : public juce::Component
{
public:
    StatusHeaderBar();
    ~StatusHeaderBar() override;

    // Per D-06: connection status display
    // true = green dot + "Connected", false = red dot + "Reconnecting..."
    void setConnected(bool connected);

protected:
    void paint(juce::Graphics& g) override;

private:
    bool connected_ = false;
};

} // namespace looper
