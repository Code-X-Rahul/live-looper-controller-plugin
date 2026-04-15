#include <catch2/catch_test_macros.hpp>
#include "Bridge/MessageProtocol.h"
#include "Model/LooperState.h"
#include "Model/LooperTracker.h"
#include "Shared/ProtocolDefs.h"

using namespace looper;

// --- BridgeClient message format tests (no OSC dependency) ---

TEST_CASE("BridgeClient hello message format", "[BridgeClient]")
{
    // Verify that MessageProtocol::createHello produces a message
    // with the correct format including local port (D-06)
    auto msg = MessageProtocol::createHello(7010);

    REQUIRE(msg.uuid.isNotEmpty());
    REQUIRE(msg.ns == protocol::NS_SYSTEM);
    REQUIRE(msg.name == protocol::CMD_HELLO);
    REQUIRE(msg.version == protocol::PROTOCOL_VERSION);

    // The args should contain the port number
    REQUIRE(msg.args.isObject());
    auto* argsObj = msg.args.getDynamicObject();
    REQUIRE(argsObj != nullptr);
    REQUIRE(argsObj->getProperty("port").isInt());
    REQUIRE(static_cast<int>(argsObj->getProperty("port")) == 7010);
}

TEST_CASE("BridgeClient handles looper_discovered event", "[BridgeClient]")
{
    // Create a looper_discovered event JSON and parse it through MessageProtocol,
    // then verify that LooperState is created correctly
    LooperTracker tracker;

    // Create the event JSON representing a detected looper
    juce::String eventJson = R"({
        "uuid": "",
        "event": "looper_discovered",
        "data": {
            "track_id": "1",
            "track_name": "Guitar",
            "device_id": "0",
            "device_name": "Looper 1",
            "class_name": "Looper",
            "state": "Recording",
            "feedback": 0.7,
            "reverse": false,
            "loop_length_bars": 4
        },
        "version": 1
    })";

    auto event = MessageProtocol::jsonToEvent(eventJson);
    REQUIRE(event.event == protocol::EVENT_LOOPER_DISCOVERED);

    // Parse the data into a LooperState (mirroring BridgeClient::handleLooperDiscovered)
    LooperState state;
    auto* data = event.data.getDynamicObject();
    REQUIRE(data != nullptr);

    state.trackId     = data->getProperty("track_id").toString();
    state.trackName   = data->getProperty("track_name").toString();
    state.deviceId    = data->getProperty("device_id").toString();
    state.deviceName  = data->getProperty("device_name").toString();
    state.className   = data->getProperty("class_name").toString();
    state.state       = LooperState::stringToState(data->getProperty("state").toString());
    state.feedback    = static_cast<float>(data->getProperty("feedback"));

    REQUIRE(state.trackId == "1");
    REQUIRE(state.trackName == "Guitar");
    REQUIRE(state.deviceId == "0");
    REQUIRE(state.deviceName == "Looper 1");
    REQUIRE(state.className == "Looper");
    REQUIRE(state.state == LooperState::Recording);
    REQUIRE(std::abs(state.feedback - 0.7f) < 0.01f);

    // Add to tracker and verify
    tracker.addLooper(state);
    REQUIRE(tracker.getLooperCount() == 1);
    auto retrieved = tracker.getLooper("1");
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->state == LooperState::Recording);
}

TEST_CASE("BridgeClient handles looper_state_changed event", "[BridgeClient]")
{
    // Create a state change event JSON and verify LooperTracker is updated
    LooperTracker tracker;

    // First add a looper in Stopped state
    LooperState initial;
    initial.trackId = "2";
    initial.trackName = "Bass";
    initial.deviceId = "0";
    initial.deviceName = "Looper";
    initial.className = "Looper";
    initial.state = LooperState::Stopped;
    initial.feedback = 0.5f;

    tracker.addLooper(initial);
    REQUIRE(tracker.getLooperCount() == 1);

    // Now create a state change event
    juce::String eventJson = R"({
        "uuid": "",
        "event": "looper_state_changed",
        "data": {
            "track_id": "2",
            "track_name": "Bass",
            "device_id": "0",
            "device_name": "Looper",
            "class_name": "Looper",
            "state": "Playing",
            "feedback": 0.6,
            "reverse": false,
            "loop_length_bars": 0
        },
        "version": 1
    })";

    auto event = MessageProtocol::jsonToEvent(eventJson);
    REQUIRE(event.event == protocol::EVENT_LOOPER_STATE_CHANGED);

    // Parse data and update tracker (mirroring BridgeClient::handleLooperStateChanged)
    LooperState updated;
    auto* data = event.data.getDynamicObject();
    REQUIRE(data != nullptr);

    updated.trackId     = data->getProperty("track_id").toString();
    updated.trackName   = data->getProperty("track_name").toString();
    updated.deviceId    = data->getProperty("device_id").toString();
    updated.deviceName  = data->getProperty("device_name").toString();
    updated.className   = data->getProperty("class_name").toString();
    updated.state       = LooperState::stringToState(data->getProperty("state").toString());
    updated.feedback    = static_cast<float>(data->getProperty("feedback"));

    tracker.updateState(updated.trackId, updated);

    // Verify the state was updated
    auto retrieved = tracker.getLooper("2");
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->state == LooperState::Playing);
    REQUIRE(std::abs(retrieved->feedback - 0.6f) < 0.01f);
}