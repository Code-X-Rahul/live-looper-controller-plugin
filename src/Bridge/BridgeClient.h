#pragma once

#include <juce_osc/juce_osc.h>

namespace looper {

class BridgeClient : private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    BridgeClient();
    ~BridgeClient() override;

    bool connect(int preferredPort = 7010);
    void disconnect();
    int getBoundPort() const { return boundPort; }

private:
    void oscMessageReceived(const juce::OSCMessage& message) override;

    juce::OSCSender sender;
    juce::OSCReceiver receiver;
    int boundPort = 0;
};

} // namespace looper
