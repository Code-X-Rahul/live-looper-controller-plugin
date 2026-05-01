#include "BridgeClient.h"
#include "Shared/ProtocolDefs.h"

namespace looper {

// Helper timer class for exponential backoff reconnect (D-07)
class ReconnectTimer : public juce::Timer
{
public:
    explicit ReconnectTimer(std::function<void()> callback) : callback_(std::move(callback)) {}
    void timerCallback() override { callback_(); }
private:
    std::function<void()> callback_;
};

BridgeClient::BridgeClient(LooperTracker& tracker)
    : tracker_(tracker)
{
}

BridgeClient::~BridgeClient()
{
    disconnect();
}

bool BridgeClient::connect()
{
    // Try to bind receiver to port range (D-05)
    if (!bindToPortRange(protocol::PLUGIN_PORT_START, protocol::PLUGIN_PORT_END))
    {
        return false;
    }

    // Start listening for incoming OSC messages (runs on message thread)
    receiver_.addListener(this);

    connected_ = true;
    tracker_.setConnected(true);

    // D-06: Send immediate handshake hello
    sendHello();

    return true;
}

void BridgeClient::disconnect()
{
    stopReconnectTimer();
    receiver_.removeListener(this);
    receiver_.disconnect();
    sender_.disconnect();
    connected_ = false;
    localPort_ = 0;
    remotePort_ = 0;
    tracker_.setConnected(false);
}

bool BridgeClient::isConnected() const
{
    return connected_;
}

void BridgeClient::sendHello()
{
    auto msg = protocol::MessageProtocol::createHello(localPort_);

    // After sending hello, set remote port to script port range start
    // The remote script will respond with its actual port
    remotePort_ = protocol::SCRIPT_PORT_START;

    sendCommand(msg);
}

void BridgeClient::sendDiscover()
{
    auto msg = protocol::MessageProtocol::createDiscover();
    sendCommand(msg);
}

void BridgeClient::sendSetState(const juce::String& trackId, const juce::String& targetState)
{
    auto msg = protocol::MessageProtocol::createSetState(trackId, targetState);
    sendCommand(msg);
}

void BridgeClient::sendUndo(const juce::String& trackId)
{
    auto msg = protocol::MessageProtocol::createUndo(trackId);
    sendCommand(msg);
}

void BridgeClient::sendRedo(const juce::String& trackId)
{
    auto msg = protocol::MessageProtocol::createRedo(trackId);
    sendCommand(msg);
}

void BridgeClient::sendSetFeedback(const juce::String& trackId, float feedbackValue)
{
    auto msg = protocol::MessageProtocol::createSetFeedback(trackId, feedbackValue);
    sendCommand(msg);
}

void BridgeClient::sendCommand(const protocol::Message& msg)
{
    if (!connected_ || localPort_ == 0)
        return;

    auto jsonStr = protocol::MessageProtocol::messageToJson(msg);

    // Send via OSC — the JSON payload goes as a string argument
    // on the /loopercontrol address
    juce::OSCMessage oscMessage(protocol::OSC_PREFIX);
    oscMessage.addString(jsonStr);

    // Send to localhost on the remote script port
    if (!sender_.send("127.0.0.1", remotePort_, oscMessage))
    {
        // Send failed — may need reconnect
        // In a production build this would trigger reconnect timer (D-07)
    }
}

void BridgeClient::oscMessageReceived(const juce::OSCMessage& message)
{
    // This runs on the message thread (MessageLoopCallback template ensures this)
    // Extract JSON payload from OSC message
    if (message.size() == 0)
        return;

    // The first argument should be our JSON string
    auto jsonStr = message[0].getString();

    // Try to parse as an Event first (script → plugin messages are events)
    auto event = protocol::MessageProtocol::jsonToEvent(jsonStr);
    if (event.event.isNotEmpty())
    {
        handleEvent(event);
        return;
    }

    // Try to parse as a Message (for responses)
    auto responseMsg = protocol::MessageProtocol::jsonToMessage(jsonStr);
    if (responseMsg.name.isNotEmpty())
    {
        // Handle as a response message
        // For now, results are handled as events
        protocol::Event resultEvent;
        resultEvent.event = protocol::EVENT_RESULT;
        resultEvent.data = responseMsg.args;
        resultEvent.version = responseMsg.version;
        handleResult(resultEvent);
    }
}

bool BridgeClient::bindToPortRange(int startPort, int endPort)
{
    for (int port = startPort; port <= endPort; ++port)
    {
        if (receiver_.connect(port))
        {
            localPort_ = port;
            return true;
        }
    }
    return false;
}

void BridgeClient::handleEvent(const protocol::Event& event)
{
    if (event.event == protocol::EVENT_LOOPER_DISCOVERED)
    {
        handleLooperDiscovered(event);
    }
    else if (event.event == protocol::EVENT_LOOPER_STATE_CHANGED)
    {
        handleLooperStateChanged(event);
    }
    else if (event.event == protocol::EVENT_LOOPER_REMOVED)
    {
        handleLooperRemoved(event);
    }
    else if (event.event == protocol::EVENT_RESULT)
    {
        handleResult(event);
    }
}

void BridgeClient::handleLooperDiscovered(const protocol::Event& event)
{
    // D-03: Full state push — create LooperState from event data
    LooperState state;

    if (auto* data = event.data.getDynamicObject())
    {
        state.trackId      = data->getProperty("track_id").toString();
        state.trackName    = data->getProperty("track_name").toString();
        state.deviceId     = data->getProperty("device_id").toString();
        state.deviceName   = data->getProperty("device_name").toString();
        state.className    = data->getProperty("class_name").toString();

        // Parse state string to enum
        auto stateStr = data->getProperty("state").toString();
        state.state = LooperState::stringToState(stateStr);

        // Parse numeric fields with defaults
        state.feedback     = static_cast<float>(data->getProperty("feedback"));
        state.reverse      = static_cast<bool>(data->getProperty("reverse"));
        state.loopLengthBars = data->getProperty("loop_length_bars").isVoid()
                                ? 0
                                : static_cast<int>(data->getProperty("loop_length_bars"));
        state.cycleCount = data->getProperty("cycle_count").isVoid()
                            ? 0
                            : static_cast<int>(data->getProperty("cycle_count"));
    }

    tracker_.addLooper(state);
}

void BridgeClient::handleLooperStateChanged(const protocol::Event& event)
{
    // D-03: Full state push — update LooperState from event data
    LooperState state;

    if (auto* data = event.data.getDynamicObject())
    {
        state.trackId      = data->getProperty("track_id").toString();
        state.trackName    = data->getProperty("track_name").toString();
        state.deviceId     = data->getProperty("device_id").toString();
        state.deviceName   = data->getProperty("device_name").toString();
        state.className    = data->getProperty("class_name").toString();

        auto stateStr = data->getProperty("state").toString();
        state.state = LooperState::stringToState(stateStr);

        state.feedback     = data->getProperty("feedback").isVoid()
                                ? 0.5f
                                : static_cast<float>(data->getProperty("feedback"));
        state.reverse      = data->getProperty("reverse").isVoid()
                                ? false
                                : static_cast<bool>(data->getProperty("reverse"));
        state.loopLengthBars = data->getProperty("loop_length_bars").isVoid()
                                ? 0
                                : static_cast<int>(data->getProperty("loop_length_bars"));
        state.cycleCount = data->getProperty("cycle_count").isVoid()
                            ? 0
                            : static_cast<int>(data->getProperty("cycle_count"));
    }

    tracker_.updateState(state.trackId, state);
}

void BridgeClient::handleLooperRemoved(const protocol::Event& event)
{
    // Extract track_id from event data and remove from tracker
    if (auto* data = event.data.getDynamicObject())
    {
        auto trackId = data->getProperty("track_id").toString();
        tracker_.removeLooper(trackId);
    }
}

void BridgeClient::handleResult(const protocol::Event& event)
{
    // Handle result messages — check for success/failure
    // For Phase 1, just check connection and update state
    if (auto* data = event.data.getDynamicObject())
    {
        bool success = static_cast<bool>(data->getProperty("success"));

        if (!success)
        {
            // Error handling — could log, trigger reconnect, etc.
            // For Phase 1, we just mark not connected
            auto errorStr = data->getProperty("error").toString();
            (void)errorStr; // Suppress unused warning
        }

        // If result contains port info, update remote port
        if (data->getProperty("port").isInt())
        {
            remotePort_ = static_cast<int>(data->getProperty("port"));
        }
    }
}

void BridgeClient::startReconnectTimer()
{
    stopReconnectTimer();
    reconnectAttempt_ = 0;
    reconnectTimer_ = std::make_unique<ReconnectTimer>([this]() { onReconnectTimer(); });
    onReconnectTimer(); // Start first attempt immediately
}

void BridgeClient::stopReconnectTimer()
{
    reconnectTimer_.reset();
    reconnectAttempt_ = 0;
}

void BridgeClient::onReconnectTimer()
{
    if (reconnectAttempt_ >= MAX_RECONNECT_ATTEMPTS)
    {
        stopReconnectTimer();
        return;
    }

    // Exponential backoff: 500ms, 1000ms, 2000ms, ... up to 30s
    int delayMs = INITIAL_RECONNECT_DELAY_MS;
    for (int i = 0; i < reconnectAttempt_; ++i)
    {
        delayMs *= 2;
        if (delayMs > 30000)
        {
            delayMs = 30000;
            break;
        }
    }

    if (connect())
    {
        // Successfully reconnected
        stopReconnectTimer();
        return;
    }

    ++reconnectAttempt_;
    if (reconnectTimer_)
    {
        reconnectTimer_->startTimer(delayMs);
    }
}

} // namespace looper