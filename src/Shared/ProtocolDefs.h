#pragma once

#include <juce_core/juce_core.h>

namespace looper {
namespace protocol {

// Protocol version (D-04)
constexpr int PROTOCOL_VERSION = 1;

// Port configuration (D-05)
constexpr int PLUGIN_PORT_START = 7010;
constexpr int PLUGIN_PORT_END = 7019;
constexpr int SCRIPT_PORT_START = 7011;
constexpr int SCRIPT_PORT_END = 7020;
constexpr int PORT_RANGE_SIZE = 10;

// Maximum message size for safety (64KB)
constexpr size_t MAX_MESSAGE_SIZE = 65536;

// OSC address prefixes
static const juce::String OSC_PREFIX = "/loopercontrol";

// Namespace constants (D-01)
static const juce::String NS_SYSTEM = "system";
static const juce::String NS_LOOPER = "looper";

// Command names
static const juce::String CMD_HELLO = "hello";
static const juce::String CMD_DISCOVER = "discover";
static const juce::String CMD_GET_STATE = "get_state";
static const juce::String CMD_SET_STATE = "set_state";

// Event names
static const juce::String EVENT_RESULT = "result";
static const juce::String EVENT_ERROR = "error";
static const juce::String EVENT_LOOPER_DISCOVERED = "looper_discovered";
static const juce::String EVENT_LOOPER_STATE_CHANGED = "looper_state_changed";
static const juce::String EVENT_LOOPER_REMOVED = "looper_removed";

// Looper state values (matching Ableton Looper State parameter)
static const juce::String STATE_STOPPED = "Stopped";
static const juce::String STATE_RECORDING = "Recording";
static const juce::String STATE_PLAYING = "Playing";
static const juce::String STATE_OVERDUBBING = "Overdubbing";

// Error codes
static const juce::String ERR_UNKNOWN = "unknown";
static const juce::String ERR_VERSION_MISMATCH = "version_mismatch";
static const juce::String ERR_INVALID_MESSAGE = "invalid_message";
static const juce::String ERR_LOOPER_NOT_FOUND = "looper_not_found";
static const juce::String ERR_COMMAND_FAILED = "command_failed";

}} // namespace looper::protocol
