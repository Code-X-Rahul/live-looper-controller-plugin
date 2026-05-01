#include <catch2/catch_test_macros.hpp>
#include <Bridge/MessageProtocol.h>
#include <Shared/ProtocolDefs.h>

using namespace looper;
using namespace looper::protocol;

TEST_CASE("MessageProtocol::createHello generates valid message", "[protocol]")
{
    auto msg = MessageProtocol::createHello(7010);

    REQUIRE(msg.uuid.isNotEmpty());
    REQUIRE(msg.ns == "system");
    REQUIRE(msg.name == "hello");
    REQUIRE(msg.version == 1);

    // Verify args contains port number
    REQUIRE(msg.args.isObject());
    auto* obj = msg.args.getDynamicObject();
    REQUIRE(obj != nullptr);
    REQUIRE(static_cast<int>(obj->getProperty("port")) == 7010);
}

TEST_CASE("MessageProtocol::createDiscover generates valid message", "[protocol]")
{
    auto msg = MessageProtocol::createDiscover();

    REQUIRE(msg.uuid.isNotEmpty());
    REQUIRE(msg.ns == "looper");
    REQUIRE(msg.name == "discover");
    REQUIRE(msg.version == 1);
}

TEST_CASE("MessageProtocol round-trip: Message -> JSON -> Message", "[protocol]")
{
    Message original;
    original.uuid = "test-uuid-1234";
    original.ns = "looper";
    original.nsid = "track-1";
    original.name = "set_state";
    original.version = 1;

    auto json = MessageProtocol::messageToJson(original);
    auto restored = MessageProtocol::jsonToMessage(json);

    REQUIRE(restored.uuid == original.uuid);
    REQUIRE(restored.ns == original.ns);
    REQUIRE(restored.nsid == original.nsid);
    REQUIRE(restored.name == original.name);
    REQUIRE(restored.version == original.version);
}

TEST_CASE("MessageProtocol round-trip: Event -> JSON -> Event", "[protocol]")
{
    Event original;
    original.uuid = "test-uuid-5678";
    original.event = "looper_state_changed";
    original.version = 1;

    auto json = MessageProtocol::eventToJson(original);
    auto restored = MessageProtocol::jsonToEvent(json);

    REQUIRE(restored.uuid == original.uuid);
    REQUIRE(restored.event == original.event);
    REQUIRE(restored.version == original.version);
}

TEST_CASE("MessageProtocol JSON contains version field", "[protocol]")
{
    auto msg = MessageProtocol::createHello(7010);
    auto json = MessageProtocol::messageToJson(msg);

    REQUIRE(json.contains("\"version\""));
    REQUIRE(json.contains("1"));
}

TEST_CASE("MessageProtocol generates unique UUIDs", "[protocol]")
{
    auto msg1 = MessageProtocol::createHello(7010);
    auto msg2 = MessageProtocol::createHello(7010);

    REQUIRE(msg1.uuid != msg2.uuid);
}

TEST_CASE("MessageProtocol hello message includes port", "[protocol]")
{
    auto msg = MessageProtocol::createHello(7010);

    REQUIRE(msg.args.isObject());
    auto* obj = msg.args.getDynamicObject();
    REQUIRE(obj != nullptr);
    REQUIRE(static_cast<int>(obj->getProperty("port")) == 7010);
}

TEST_CASE("MessageProtocol handles looper_discovered event deserialization", "[protocol]")
{
    // Simulate a JSON event from the Remote Script
    juce::String jsonStr = R"({
        "uuid": null,
        "event": "looper_discovered",
        "data": {
            "track_id": "1",
            "track_name": "Guitar",
            "device_id": "0",
            "device_name": "Looper",
            "state": "Stopped",
            "feedback": 0.5
        },
        "version": 1
    })";

    auto evt = MessageProtocol::jsonToEvent(jsonStr);

    REQUIRE(evt.event == "looper_discovered");
    REQUIRE(evt.version == 1);
    REQUIRE(evt.data.isObject());

    auto* data = evt.data.getDynamicObject();
    REQUIRE(data != nullptr);
    REQUIRE(data->getProperty("track_id").toString() == "1");
    REQUIRE(data->getProperty("track_name").toString() == "Guitar");
    REQUIRE(data->getProperty("device_id").toString() == "0");
    REQUIRE(data->getProperty("state").toString() == "Stopped");
}

TEST_CASE("Protocol constants are correct", "[protocol]")
{
    REQUIRE(PROTOCOL_VERSION == 1);
    REQUIRE(PLUGIN_PORT_START == 7010);
    REQUIRE(SCRIPT_PORT_START == 7011);
    REQUIRE(OSC_PREFIX == "/loopercontrol");

    REQUIRE(STATE_STOPPED == "Stopped");
    REQUIRE(STATE_RECORDING == "Recording");
    REQUIRE(STATE_PLAYING == "Playing");
    REQUIRE(STATE_OVERDUBBING == "Overdubbing");
}

TEST_CASE("Protocol command constants for control commands", "[protocol]")
{
    REQUIRE(CMD_SET_STATE == "set_state");
    REQUIRE(CMD_UNDO == "undo");
    REQUIRE(CMD_REDO == "redo");
    REQUIRE(CMD_SET_FEEDBACK == "set_feedback");
}

// --- Command Message Tests ---

TEST_CASE("MessageProtocol::createSetState generates valid message", "[protocol][commands]")
{
    auto msg = MessageProtocol::createSetState("track1", "Recording");

    REQUIRE(msg.uuid.isNotEmpty());
    REQUIRE(msg.ns == NS_LOOPER);
    REQUIRE(msg.nsid == "track1");
    REQUIRE(msg.name == CMD_SET_STATE);
    REQUIRE(msg.version == PROTOCOL_VERSION);

    REQUIRE(msg.args.isObject());
    auto* obj = msg.args.getDynamicObject();
    REQUIRE(obj != nullptr);
    REQUIRE(obj->getProperty("track_id").toString() == "track1");
    REQUIRE(obj->getProperty("target_state").toString() == "Recording");
}

TEST_CASE("MessageProtocol::createUndo generates valid message", "[protocol][commands]")
{
    auto msg = MessageProtocol::createUndo("track1");

    REQUIRE(msg.uuid.isNotEmpty());
    REQUIRE(msg.ns == NS_LOOPER);
    REQUIRE(msg.nsid == "track1");
    REQUIRE(msg.name == CMD_UNDO);
    REQUIRE(msg.version == PROTOCOL_VERSION);

    REQUIRE(msg.args.isObject());
    auto* obj = msg.args.getDynamicObject();
    REQUIRE(obj != nullptr);
    REQUIRE(obj->getProperty("track_id").toString() == "track1");
}

TEST_CASE("MessageProtocol::createRedo generates valid message", "[protocol][commands]")
{
    auto msg = MessageProtocol::createRedo("track1");

    REQUIRE(msg.uuid.isNotEmpty());
    REQUIRE(msg.ns == NS_LOOPER);
    REQUIRE(msg.nsid == "track1");
    REQUIRE(msg.name == CMD_REDO);
    REQUIRE(msg.version == PROTOCOL_VERSION);

    REQUIRE(msg.args.isObject());
    auto* obj = msg.args.getDynamicObject();
    REQUIRE(obj != nullptr);
    REQUIRE(obj->getProperty("track_id").toString() == "track1");
}

TEST_CASE("MessageProtocol::createSetFeedback generates valid message", "[protocol][commands]")
{
    auto msg = MessageProtocol::createSetFeedback("track1", 0.75f);

    REQUIRE(msg.uuid.isNotEmpty());
    REQUIRE(msg.ns == NS_LOOPER);
    REQUIRE(msg.nsid == "track1");
    REQUIRE(msg.name == CMD_SET_FEEDBACK);
    REQUIRE(msg.version == PROTOCOL_VERSION);

    REQUIRE(msg.args.isObject());
    auto* obj = msg.args.getDynamicObject();
    REQUIRE(obj != nullptr);
    REQUIRE(obj->getProperty("track_id").toString() == "track1");
    REQUIRE(std::abs(static_cast<float>(obj->getProperty("feedback")) - 0.75f) < 0.001f);
}

TEST_CASE("MessageProtocol command messages round-trip through JSON", "[protocol][commands]")
{
    // Test createSetState round-trip
    auto setStateMsg = MessageProtocol::createSetState("track2", "Playing");
    auto setStateJson = MessageProtocol::messageToJson(setStateMsg);
    auto setStateRestored = MessageProtocol::jsonToMessage(setStateJson);
    REQUIRE(setStateRestored.ns == NS_LOOPER);
    REQUIRE(setStateRestored.nsid == "track2");
    REQUIRE(setStateRestored.name == CMD_SET_STATE);
    REQUIRE(setStateRestored.args.getDynamicObject()->getProperty("target_state").toString() == "Playing");

    // Test createUndo round-trip
    auto undoMsg = MessageProtocol::createUndo("track3");
    auto undoJson = MessageProtocol::messageToJson(undoMsg);
    auto undoRestored = MessageProtocol::jsonToMessage(undoJson);
    REQUIRE(undoRestored.name == CMD_UNDO);
    REQUIRE(undoRestored.args.getDynamicObject()->getProperty("track_id").toString() == "track3");

    // Test createRedo round-trip
    auto redoMsg = MessageProtocol::createRedo("track4");
    auto redoJson = MessageProtocol::messageToJson(redoMsg);
    auto redoRestored = MessageProtocol::jsonToMessage(redoJson);
    REQUIRE(redoRestored.name == CMD_REDO);
    REQUIRE(redoRestored.args.getDynamicObject()->getProperty("track_id").toString() == "track4");

    // Test createSetFeedback round-trip
    auto feedbackMsg = MessageProtocol::createSetFeedback("track5", 0.3f);
    auto feedbackJson = MessageProtocol::messageToJson(feedbackMsg);
    auto feedbackRestored = MessageProtocol::jsonToMessage(feedbackJson);
    REQUIRE(feedbackRestored.name == CMD_SET_FEEDBACK);
    REQUIRE(std::abs(static_cast<float>(feedbackRestored.args.getDynamicObject()->getProperty("feedback")) - 0.3f) < 0.001f);
}
