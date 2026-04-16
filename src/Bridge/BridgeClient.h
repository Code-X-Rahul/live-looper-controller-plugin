#pragma once

#include <juce_osc/juce_osc.h>
#include <juce_core/juce_core.h>
#include "MessageProtocol.h"
#include "../Model/LooperTracker.h"

namespace looper {

class BridgeClient : public juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    BridgeClient(LooperTracker& tracker);
    ~BridgeClient() override;

    // Connection lifecycle (D-06: immediate handshake, D-07: auto-reconnect)
    bool connect();          // Bind to port range (D-05), start handshake
    void disconnect();       // Clean disconnect
    bool isConnected() const;

    // Send commands to Remote Script
    void sendHello();        // D-06: immediate handshake on load
    void sendDiscover();     // Request looper discovery
    void sendCommand(const protocol::Message& msg);  // Send any command

    // OSCReceiver::Listener callback (runs on message thread per MessageLoopCallback)
    void oscMessageReceived(const juce::OSCMessage& message) override;

private:
    // Port binding with range fallback (D-05)
    bool bindToPortRange(int startPort, int endPort);

    // Message handling
    void handleEvent(const protocol::Event& event);
    void handleLooperDiscovered(const protocol::Event& event);   // D-03: full state push
    void handleLooperStateChanged(const protocol::Event& event); // D-03: full state push
    void handleLooperRemoved(const protocol::Event& event);     // looper removed from DAW
    void handleResult(const protocol::Event& event);

    // Auto-reconnect with exponential backoff (D-07)
    void startReconnectTimer();
    void stopReconnectTimer();
    void onReconnectTimer();

    LooperTracker& tracker_;
    juce::OSCSender sender_;
    juce::OSCReceiver receiver_;
    int localPort_ = 0;       // Port we bound to
    int remotePort_ = 0;      // Remote Script's port (discovered via handshake)
    bool connected_ = false;

    // Reconnect state
    std::unique_ptr<juce::Timer> reconnectTimer_;
    int reconnectAttempt_ = 0;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 50;
    static constexpr int INITIAL_RECONNECT_DELAY_MS = 500;
};

} // namespace looper