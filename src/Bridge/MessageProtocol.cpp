#include "MessageProtocol.h"

namespace looper {

Message MessageProtocol::createHello(int localPort)
{
    Message msg;
    msg.uuid = generateUuid().toString();
    msg.ns = protocol::NS_SYSTEM;
    msg.nsid = "";
    msg.name = protocol::CMD_HELLO;
    msg.args = juce::var(juce::JSON::parse("{\"port\": " + juce::String(localPort) + "}"));
    msg.version = protocol::PROTOCOL_VERSION;
    return msg;
}

Message MessageProtocol::createDiscover(const juce::String& nsid)
{
    Message msg;
    msg.uuid = generateUuid().toString();
    msg.ns = protocol::NS_LOOPER;
    msg.nsid = nsid;
    msg.name = protocol::CMD_DISCOVER;
    msg.args = juce::var();
    msg.version = protocol::PROTOCOL_VERSION;
    return msg;
}

juce::String MessageProtocol::messageToJson(const Message& msg)
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();

    obj->setProperty("uuid", msg.uuid);
    obj->setProperty("ns", msg.ns);
    obj->setProperty("nsid", msg.nsid);
    obj->setProperty("name", msg.name);
    obj->setProperty("args", msg.args);
    obj->setProperty("version", msg.version);

    return juce::JSON::toString(juce::var(obj.get()));
}

Message MessageProtocol::jsonToMessage(const juce::String& json)
{
    Message msg;
    auto parsed = juce::JSON::parse(json);

    if (parsed.isObject())
    {
        auto* obj = parsed.getDynamicObject();
        if (obj != nullptr)
        {
            msg.uuid = obj->getProperty("uuid").toString();
            msg.ns = obj->getProperty("ns").toString();
            msg.nsid = obj->getProperty("nsid").toString();
            msg.name = obj->getProperty("name").toString();
            msg.args = obj->getProperty("args");
            msg.version = static_cast<int>(obj->getProperty("version"));
        }
    }

    return msg;
}

juce::String MessageProtocol::eventToJson(const Event& evt)
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();

    if (evt.uuid.isNotEmpty())
        obj->setProperty("uuid", evt.uuid);

    obj->setProperty("event", evt.event);
    obj->setProperty("data", evt.data);
    obj->setProperty("version", evt.version);

    return juce::JSON::toString(juce::var(obj.get()));
}

Event MessageProtocol::jsonToEvent(const juce::String& json)
{
    Event evt;
    auto parsed = juce::JSON::parse(json);

    if (parsed.isObject())
    {
        auto* obj = parsed.getDynamicObject();
        if (obj != nullptr)
        {
            evt.uuid = obj->getProperty("uuid").toString();
            evt.event = obj->getProperty("event").toString();
            evt.data = obj->getProperty("data");
            evt.version = static_cast<int>(obj->getProperty("version"));
        }
    }

    return evt;
}

juce::Uuid MessageProtocol::generateUuid()
{
    return juce::Uuid();
}

} // namespace looper
