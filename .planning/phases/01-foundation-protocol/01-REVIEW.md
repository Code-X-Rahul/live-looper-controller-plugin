---
phase: 01-foundation-protocol
reviewed: 2026-04-16T12:00:00Z
depth: standard
files_reviewed: 17
files_reviewed_list:
  - src/Shared/ProtocolDefs.h
  - src/Shared/ProtocolDefs.cpp
  - src/Bridge/MessageProtocol.h
  - src/Bridge/MessageProtocol.cpp
  - src/Model/LooperState.h
  - src/Model/LooperState.cpp
  - src/Model/LooperTracker.h
  - src/Model/LooperTracker.cpp
  - src/Bridge/BridgeClient.h
  - src/Bridge/BridgeClient.cpp
  - src/Plugin/PluginProcessor.h
  - src/Plugin/PluginProcessor.cpp
  - src/Plugin/PluginEditor.h
  - src/Plugin/PluginEditor.cpp
  - remote-script/LooperControlSurface.py
  - remote-script/LooperDiscovery.py
  - remote-script/BridgeServer.py
findings:
  critical: 1
  warning: 5
  info: 4
  total: 10
status: issues_found
---

# Phase 1: Code Review Report

**Reviewed:** 2026-04-16
**Depth:** standard
**Files Reviewed:** 17
**Status:** issues_found

## Summary

Phase 1 implements the foundation protocol layer for the Live Looper Controller: JUCE VST3 plugin scaffold, OSC message protocol with JSON serialization, shadow state model (LooperState/LooperTracker), BridgeClient for OSC communication, and Python Remote Script with pattern-based looper discovery.

The implementation is structurally sound and tests pass (20 C++ tests, 66 Python tests). However, several issues were identified that should be addressed before Phase 2.

---

## Critical Issues

### CR-01: BridgeClient::sendCommand() allows sending when disconnected

**File:** `src/Bridge/BridgeClient.cpp:82`
**Issue:** The guard condition `if (!connected_ && localPort_ == 0)` allows `sendCommand()` to proceed when `connected_` is false but `localPort_` is non-zero. This means messages could be sent on a socket that was only partially initialized (bound receiver but no successful handshake). The intended logic appears to be "don't send if we have no port OR if we're not connected" — which should use OR, not AND.

**Fix:**
```cpp
void BridgeClient::sendCommand(const protocol::Message& msg)
{
    if (!connected_ || localPort_ == 0)
        return;
    // ...
}
```

---

## Warnings

### WR-01: No validation of OSC message argument type in BridgeClient

**File:** `src/Bridge/BridgeClient.cpp:108`
**Issue:** `oscMessageReceived()` calls `message[0].getString()` without checking that the first argument is actually a string type. If the Remote Script sends a non-string OSC argument (e.g., integer, float), `getString()` returns an empty string and parsing proceeds with no error indication.

**Fix:**
```cpp
void BridgeClient::oscMessageReceived(const juce::OSCMessage& message)
{
    if (message.size() == 0)
        return;

    // Validate first argument is a string
    if (!message[0].isString())
    {
        jassertfalse; // or log error
        return;
    }

    auto jsonStr = message[0].getString();
    // ...
}
```

### WR-02: LooperControlSurface crashes when `_song()` returns None

**File:** `remote-script/LooperControlSurface.py:44`
**Issue:** In test mode (`_HAS_ABETON_API = False`), `_song()` returns `None`. However, `LooperDiscovery.__init__` stores `song` directly, and `scan_all_tracks()` accesses `self._song.tracks` without a None check. This causes `AttributeError: 'NoneType' object has no attribute 'tracks'` when running in test mode.

**Fix:**
```python
# In LooperDiscovery.__init__:
if song is not None:
    self._song = song
else:
    self._song = None

# In scan_all_tracks:
if self._song is None:
    return discovered
```

### WR-03: BridgeClient port range fallback resets to wrong port on restart

**File:** `remote-script/BridgeServer.py:49-58`
**Issue:** When `start()` successfully binds to a port in the fallback range (e.g., 7012 instead of 7011), `self._port` is updated to 7012. If `stop()` is called and then `start()` is called again, the loop starts from `self._port` (7012) and tries 7012-7021, skipping 7011. If port 7011 became available again, it won't be used.

**Fix:**
```python
def start(self):
    # Try port range starting from our preferred port (D-05: range fallback for script port 7011-7020)
    preferred_port = self._port  # Save the originally requested port
    for port in range(preferred_port, preferred_port + 10):
        # ...
```

### WR-04: BridgeClient event handlers silently accept malformed data

**File:** `src/Bridge/BridgeClient.cpp:165-231`
**Issue:** `handleLooperDiscovered()` and `handleLooperStateChanged()` construct `LooperState` objects even when required fields are missing from the event data. Missing `track_id` becomes an empty string. No validation that required fields are present and non-empty before using them.

**Fix:**
```cpp
void BridgeClient::handleLooperDiscovered(const protocol::Event& event)
{
    LooperState state;

    if (auto* data = event.data.getDynamicObject())
    {
        auto trackId = data->getProperty("track_id").toString();
        if (trackId.isEmpty())
        {
            // Missing required field - log and ignore
            jassertfalse;
            return;
        }
        // ... proceed with validation
    }
}
```

### WR-05: MessageProtocol deserialization silently ignores parse errors

**File:** `src/Bridge/MessageProtocol.cpp:43-63, 79-97`
**Issue:** `jsonToMessage()` and `jsonToEvent()` return empty/default objects when JSON parsing fails (`parsed.isObject()` returns false) or when dynamic cast fails. There's no error indication to the caller — they cannot distinguish between a valid message with empty fields and a parse failure.

**Fix:** Consider returning `std::optional<Message>` / `std::optional<Event>` or adding a boolean out-parameter to indicate success:
```cpp
static std::optional<Message> jsonToMessage(const juce::String& json);
```

---

## Info

### IN-01: LooperTracker supports only single callback listener

**File:** `src/Model/LooperTracker.h:40, 77-79`
**Issue:** `onStateChange(callback)` replaces any previously registered callback rather than supporting multiple listeners. This is a design limitation for extensibility (e.g., if both the editor and a logging system want to subscribe).

**Advisory:** Consider using `std::vector<ChangeCallback>` or `juce::ListenerList` if multiple subscribers are needed in the future.

### IN-02: BridgeServer.send_to_plugin() has no error response to caller

**File:** `remote-script/BridgeServer.py:74-96`
**Issue:** `send_to_plugin()` silently returns if `self._plugin_port` is None. The caller (`send_event`, `send_response`) has no way to know the message wasn't delivered.

**Advisory:** Consider raising an exception or returning False to indicate delivery failure.

### IN-03: Lambda capture pattern in LooperDiscovery is correct but subtle

**File:** `remote-script/LooperDiscovery.py:137-139`
**Issue:** The lambda uses default arguments (`t=track, d=device, p=param`) to capture loop variables correctly. While this is the correct Python pattern, it's fragile — a future developer might "simplify" it and break the capture.

**Advisory:** Add a comment explaining why the default arguments are necessary:
```python
# Use default args to capture loop variables correctly (closure capture issue in Python 2/3)
listener = lambda v, t=track, d=device, p=param, cb=on_state_changed: cb(t, d, p, v)
```

### IN-04: PluginProcessor.processBlock casts MIDI buffer to void

**File:** `src/Plugin/PluginProcessor.cpp:53`
**Issue:** Uses C-style `(void)midiMessages` cast instead of modern C++ attribute `[[maybe_unused]]`.

**Advisory:** Replace with `[[maybe_unused]] auto& midiMessages` or use `(void)midiMessages` consistently (current pattern is fine, just not modern).

---

## Positive Findings

The following are correctly implemented and should be preserved:

1. **LooperState::operator== uses epsilon comparison** (`src/Model/LooperState.cpp:13`) — correctly uses `std::abs(feedback - other.feedback) < 0.001f` for float comparison.

2. **Diff-based notification in LooperTracker** (`src/Model/LooperTracker.cpp:24-29`) — only notifies listeners when state actually changes, avoiding unnecessary UI updates.

3. **Exponential backoff reconnect** (`src/Bridge/BridgeClient.cpp:271-303`) — correctly implements exponential backoff with 30s cap per D-07.

4. **MessageLoopCallback template** (`src/Bridge/BridgeClient.h:10`) — ensures OSC callbacks run on the message thread, not the audio thread, avoiding thread-safety issues.

5. **Port range fallback** (`src/Bridge/BridgeClient.cpp:132-143`) — correctly tries ports in range per D-05.

6. **Protocol version field** — version check in both C++ and Python (`BridgeServer.py:144-148`, `MessageProtocol.cpp:58`) ensures protocol compatibility per D-04.

7. **Python OSC library bundled** — correctly avoids pip dependency for Ableton's embedded Python.

---

## Recommendations

**Before Phase 2 begins:**

1. **Fix CR-01** (sendCommand guard condition) — this is a logic error that could cause unexpected behavior.

2. **Fix WR-02** (LooperControlSurface None song) — this prevents testing without Ableton.

3. **Fix WR-03** (port range restart) — prevents port starvation over time.

4. **Consider WR-04 and WR-05** — these are data validation gaps that could cause issues with malformed network messages.

---

_Reviewed: 2026-04-16_
_Reviewer: gsd-code-reviewer_
_Depth: standard_
