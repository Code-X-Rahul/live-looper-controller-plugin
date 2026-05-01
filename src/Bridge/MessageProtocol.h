#pragma once

#include <juce_core/juce_core.h>
#include "Shared/ProtocolDefs.h"

namespace looper {

struct Message
{
    juce::String uuid;
    juce::String ns;
    juce::String nsid;
    juce::String name;
    juce::var args;
    int version = protocol::PROTOCOL_VERSION;
};

struct Event
{
    juce::String uuid;  // nullable — null for unsolicited events
    juce::String event;
    juce::var data;
    int version = protocol::PROTOCOL_VERSION;
};

class MessageProtocol
{
public:
    // --- Message creation ---

    /** Create a handshake hello message with the local port number (D-06). */
    static Message createHello(int localPort);

    /** Create a discover message to enumerate looper devices. */
    static Message createDiscover(const juce::String& nsid = "");

    /** Create a set_state message to change a track's looper state. */
    static Message createSetState(const juce::String& trackId, const juce::String& targetState);

    /** Create an undo message for a track's looper. */
    static Message createUndo(const juce::String& trackId);

    /** Create a redo message for a track's looper. */
    static Message createRedo(const juce::String& trackId);

    /** Create a set_feedback message to adjust feedback for a track's looper. */
    static Message createSetFeedback(const juce::String& trackId, float feedbackValue);

    // --- Serialization ---

    /** Serialize a Message to a JSON string (D-01 format). */
    static juce::String messageToJson(const Message& msg);

    /** Deserialize a JSON string to a Message. */
    static Message jsonToMessage(const juce::String& json);

    /** Serialize an Event to a JSON string. */
    static juce::String eventToJson(const Event& evt);

    /** Deserialize a JSON string to an Event. */
    static Event jsonToEvent(const juce::String& json);

    // --- Utility ---

    /** Generate a new UUID for request correlation (D-02). */
    static juce::Uuid generateUuid();
};

} // namespace looper
