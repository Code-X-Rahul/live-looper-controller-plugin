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
