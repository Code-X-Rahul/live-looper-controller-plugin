#include "BridgeClient.h"

namespace looper {

BridgeClient::BridgeClient() = default;

BridgeClient::~BridgeClient()
{
    disconnect();
}

bool BridgeClient::connect(int preferredPort)
{
    // Try the preferred port first, then range fallback (D-05)
    for (int port = preferredPort; port <= 7019; ++port)
    {
        if (receiver.connect(port))
        {
            boundPort = port;
            receiver.addListener(this);
            return true;
        }
    }
    return false;
}

void BridgeClient::disconnect()
{
    receiver.disconnect();
    sender.disconnect();
    boundPort = 0;
}

void BridgeClient::oscMessageReceived(const juce::OSCMessage& message)
{
    // OSC message handling will be implemented in Phase 1 Plan 2
}

} // namespace looper
