---
phase: 1
plan: 1
type: execute
wave: 1
depends_on: []
files_modified:
  - CMakeLists.txt
  - third_party/JUCE/
  - src/Shared/ProtocolDefs.h
  - src/Shared/ProtocolDefs.cpp
  - src/Bridge/MessageProtocol.h
  - src/Bridge/MessageProtocol.cpp
  - tests/CMakeLists.txt
  - tests/TestMessageProtocol.cpp
  - tests/catch2_main.cpp
autonomous: true
requirements:
  - PLUG-01
  - PLUG-02
  - PLUG-03
---

# Phase 1 Plan 1: Project Scaffold & Protocol Definition

<objective>
Set up the JUCE CMake project scaffold, define the shared protocol message format (the contract between C++ plugin and Python Remote Script), and create the message encode/decode infrastructure. This is the foundation that both C++ and Python sides depend on, so it must be defined first.
</objective>

<tasks>

## Task 1.1: Initialize JUCE Project with CMake

<read_first>
- .planning/research/STACK.md (JUCE build configuration, CMake patterns)
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (D-09: VST3 only, D-08: audio passthrough)
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (project structure, JUCE CMake patterns)
</read_first>

<action>
1. Initialize git submodules: add JUCE 8.0.12 as `third_party/JUCE`
   ```bash
   git submodule add https://github.com/juce-framework/JUCE.git third_party/JUCE
   ```

2. Create root `CMakeLists.txt` with the following configuration:
   ```cmake
   cmake_minimum_required(VERSION 3.22)
   project(LiveLooperController VERSION 0.1.0 LANGUAGES CXX)

   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   add_subdirectory(third_party/JUCE)

   juce_add_plugin(LiveLooperController
       VERSION 0.1.0
       COMPANY_NAME "LiveLooperController"
       PLUGIN_MANUFACTURER_CODE Llct
       PLUGIN_CODE Llcr
       FORMATS VST3
       PRODUCT_NAME "Live Looper Controller"
       IS_SYNTH FALSE
       NEEDS_MIDI_INPUT TRUE
       NEEDS_MIDI_OUTPUT FALSE
       IS_MIDI_EFFECT FALSE
       COPY_PLUGIN_AFTER_BUILD TRUE
       VST3_CATEGORIES "Fx" "Instrument"
   )

   target_sources(LiveLooperController PRIVATE
       src/Plugin/PluginProcessor.cpp
       src/Plugin/PluginEditor.cpp
       src/Model/LooperState.h
       src/Model/LooperTracker.cpp
       src/Bridge/BridgeClient.cpp
       src/Bridge/MessageProtocol.cpp
       src/Shared/ProtocolDefs.cpp
   )

   target_link_libraries(LiveLooperController PRIVATE
       juce::juce_audio_processors
       juce::juce_osc
       juce::juce_data_structures
       juce::juce_gui_basics
       PUBLIC
       juce::juce_recommended_config_flags
       juce::juce_recommended_warning_flags
   )

   target_compile_definitions(LiveLooperController PUBLIC
       JUCE_WEB_BROWSER=0
       JUCE_USE_CURL=0
       JUCE_VST3_CAN_REPLACE_VST2=0
   )

   # Do NOT link juce_dsp — no audio processing needed

   add_subdirectory(tests)
   ```

3. Create the source directory structure:
   ```bash
   mkdir -p src/Plugin src/Model src/Bridge src/Shared remote-script tests
   ```

4. Create minimal placeholder files for all source files listed in `target_sources`:
   - `src/Plugin/PluginProcessor.h` — empty class declaration extending `juce::AudioProcessor`
   - `src/Plugin/PluginProcessor.cpp` — minimal passthrough implementation with `processBlock` that does nothing
   - `src/Plugin/PluginEditor.h` — minimal editor declaration
   - `src/Plugin/PluginEditor.cpp` — minimal editor (empty window)
   - `src/Model/LooperState.h` — empty struct placeholder
   - `src/Model/LooperTracker.h` — empty class placeholder
   - `src/Model/LooperTracker.cpp` — empty implementation
   - `src/Bridge/BridgeClient.h` — empty class declaration
   - `src/Bridge/BridgeClient.cpp` — empty implementation
   - `src/Bridge/MessageProtocol.h` — empty namespace/class declaration
   - `src/Bridge/MessageProtocol.cpp` — empty implementation
   - `src/Shared/ProtocolDefs.h` — empty namespace with port constants
   - `src/Shared/ProtocolDefs.cpp` — empty implementation

5. Create `.gitignore` at project root with standard C++/CMake ignores:
   ```
   build/
   cmake-build-*/
   .cache/
   *.o
   *.obj
   *.exe
   *.dll
   *.so
   *.dylib
   *.a
   *.lib
   .DS_Store
   *.swp
   *.swo
   VST3/
   ```

6. Verify the project builds:
   ```bash
   cmake -Bbuild -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
   cmake --build build --config Debug
   ```
</action>

<acceptance_criteria>
- `CMakeLists.txt` exists at project root with `cmake_minimum_required(VERSION 3.22)`
- `juce_add_plugin(LiveLooperController ...)` line contains `FORMATS VST3`, `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT TRUE`, `IS_MIDI_EFFECT FALSE`
- `juce::juce_dsp` is NOT in `target_link_libraries`
- `COPY_PLUGIN_AFTER_BUILD TRUE` is set
- `JUCE_WEB_BROWSER=0` and `JUCE_USE_CURL=0` are in compile definitions
- Directory structure exists: `src/Plugin/`, `src/Model/`, `src/Bridge/`, `src/Shared/`, `remote-script/`, `tests/`
- `third_party/JUCE/` exists as a git submodule
- `cmake -Bbuild` succeeds without error
- At least one source file compiles (minimal placeholder)
</acceptance_criteria>

## Task 1.2: Define Protocol Constants and Message Format

<read_first>
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (D-01 through D-07: protocol decisions)
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (Protocol message format, JSON structure)
- .planning/research/ARCHITECTURE.md (Communication patterns)
</read_first>

<action>
1. Create `src/Shared/ProtocolDefs.h` with the following content:
   ```cpp
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

   // OSC address prefixes
   const juce::String OSC_PREFIX = "/loopercontrol";

   // Namespace constants (D-01)
   const juce::String NS_SYSTEM = "system";
   const juce::String NS_LOOPER = "looper";

   // Command names
   const juce::String CMD_HELLO = "hello";
   const juce::String CMD_DISCOVER = "discover";
   const juce::String CMD_GET_STATE = "get_state";
   const juce::String CMD_SET_STATE = "set_state";

   // Event names
   const juce::String EVENT_RESULT = "result";
   const juce::String EVENT_ERROR = "error";
   const juce::String EVENT_LOOPER_DISCOVERED = "looper_discovered";
   const juce::String EVENT_LOOPER_STATE_CHANGED = "looper_state_changed";
   const juce::String EVENT_LOOPER_REMOVED = "looper_removed";

   // Looper state values (matching Ableton Looper State parameter)
   const juce::String STATE_STOPPED = "Stopped";
   const juce::String STATE_RECORDING = "Recording";
   const juce::String STATE_PLAYING = "Playing";
   const juce::String STATE_OVERDUBBING = "Overdubbing";

   // Error codes
   const juce::String ERR_UNKNOWN = "unknown";
   const juce::String ERR_VERSION_MISMATCH = "version_mismatch";
   const juce::String ERR_INVALID_MESSAGE = "invalid_message";
   const juce::String ERR_LOOPER_NOT_FOUND = "looper_not_found";
   const juce::String ERR_COMMAND_FAILED = "command_failed";

   }} // namespace looper::protocol
   ```

2. Create `src/Shared/ProtocolDefs.cpp` — implementation file (empty, all constants are header-only for now).

3. Create `src/Bridge/MessageProtocol.h` with a MessageProtocol class/namespace that:
   - Has a `struct Message` with fields: `uuid` (String), `ns` (String), `nsid` (String), `name` (String), `args` (var), `version` (int)
   - Has a `struct Event` with fields: `uuid` (String, nullable), `event` (String), `data` (var), `version` (int)
   - Has `static Message createHello(int localPort)` — creates handshake message with a fresh UUID, namespace "system", name "hello", args containing local port number, version 1
   - Has `static Message createDiscover(const juce::String& nsid = "")` — creates discover message
   - Has `static juce::String messageToJson(const Message& msg)` — serializes Message to JSON string (D-01 format)
   - Has `static Message jsonToMessage(const juce::String& json)` — deserializes JSON string to Message
   - Has `static juce::String eventToJson(const Event& evt)` — serializes Event to JSON string
   - Has `static Event jsonToEvent(const juce::String& json)` — deserializes JSON string to Event
   - Has `static juce::Uuid generateUuid()` — generates a new UUID for request correlation (D-02)

4. Create `src/Bridge/MessageProtocol.cpp` — implement all serialization/deserialization methods. Use `juce::JSON` for parsing, `juce::Uuid` for UUID generation. All JSON output must include the `version` field (D-04).
</action>

<acceptance_criteria>
- `src/Shared/ProtocolDefs.h` exists and contains namespace `looper::protocol`
- Protocol version constant is `1` (D-04)
- Port range constants start at `7010` for plugin, `7011` for script (D-05)
- OSC address prefix is `/loopercontrol`
- All four looper state string constants exist: "Stopped", "Recording", "Playing", "Overdubbing"
- `MessageProtocol.h` declares `Message` struct with `uuid`, `ns`, `nsid`, `name`, `args`, `version` fields
- `MessageProtocol.h` declares `Event` struct with `uuid`, `event`, `data`, `version` fields
- `MessageProtocol.h` declares `createHello`, `createDiscover`, `messageToJson`, `jsonToMessage`, `eventToJson`, `jsonToEvent` methods
- `MessageProtocol.cpp` implements all methods using `juce::JSON` and `juce::Uuid`
- Every JSON output includes `"version": 1` field (D-04)
- The hello message includes the local port number in args (D-06)
</acceptance_criteria>

## Task 1.3: Create Catch2 Test Infrastructure and Protocol Tests

<read_first>
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (test framework, validation architecture)
- .planning/research/STACK.md (Catch2 as test framework)
</read_first>

<action>
1. Create `tests/CMakeLists.txt` that:
   - Uses `FetchContent` to download Catch2 v3.x from GitHub
   - Creates a test executable `LiveLooperControllerTests`
   - Links it against Catch2::Catch2WithMain
   - Sources: `tests/catch2_main.cpp`, `tests/TestMessageProtocol.cpp`
   - Links against the main project's source files (or creates a test-specific target)

2. Create `tests/catch2_main.cpp`:
   ```cpp
   #define CATCH_CONFIG_MAIN
   #include <catch2/catch_test_macros.hpp>
   ```

3. Create `tests/TestMessageProtocol.cpp` with the following test cases:
   - `TEST_CASE("MessageProtocol::createHello generates valid message")` — verify UUID is non-empty, ns is "system", name is "hello", version is 1, args contains a port number
   - `TEST_CASE("MessageProtocol::createDiscover generates valid message")` — verify ns is "looper", name is "discover", version is 1
   - `TEST_CASE("MessageProtocol round-trip: Message → JSON → Message")` — create a message, serialize to JSON, deserialize back, verify all fields match
   - `TEST_CASE("MessageProtocol round-trip: Event → JSON → Event")` — create an event, serialize to JSON, deserialize back, verify all fields match
   - `TEST_CASE("MessageProtocol JSON contains version field")` — verify serialized JSON string contains `"version": 1`
   - `TEST_CASE("MessageProtocol generates unique UUIDs")` — create two messages, verify UUIDs differ
   - `TEST_CASE("MessageProtocol hello message includes port")` — verify createHello(7010) includes port 7010 in args
   - `TEST_CASE("MessageProtocol handles looper_discovered event deserialization")` — create a JSON string representing a looper_discovered event with track data, deserialize it, verify fields

4. Build and run the tests:
   ```bash
   cmake --build build --config Debug
   ctest --test-dir build --output-on-failure
   ```
</action>

<acceptance_criteria>
- `tests/CMakeLists.txt` exists and uses FetchContent for Catch2 v3
- `tests/catch2_main.cpp` exists with `#define CATCH_CONFIG_MAIN`
- `tests/TestMessageProtocol.cpp` exists with at least 8 test cases
- Test for message round-trip serialization/deserialization exists
- Test for version field in JSON exists
- Test for UUID uniqueness exists
- Test for hello message port field exists
- `ctest --test-dir build --output-on-failure` exits with code 0 (all tests pass)
</acceptance_criteria>

</tasks>

<verification>
1. CMake configure succeeds: `cmake -Bbuild -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0` exits 0
2. Full build succeeds: `cmake --build build --config Debug` exits 0
3. All Catch2 tests pass: `ctest --test-dir build --output-on-failure` exits 0
4. VST3 plugin binary exists in build output directory
5. Protocol constants match CONTEXT.md decisions (D-01 through D-07)
6. All JSON messages include version field (D-04)
</verification>

<must_haves>
- JUCE project builds and produces a VST3 plugin binary
- Protocol definitions are concrete and code-verified (not just documentation)
- Message format matches the ableton.js pattern with {ns, nsid, name, args, version}
- Port constants start at 7010/7011 as decided (D-05)
- UUID correlation is implemented (D-02)
- Version field is in every message (D-04)
</must_haves>

<threat_model>
## Threat Model — Plan 1: Project Scaffold & Protocol

| Threat | STRIDE | Mitigation |
|--------|--------|------------|
| Malicious JSON payloads injected via OSC | Tampering | MessageProtocol validates JSON structure and types before processing; reject malformed messages |
| Port scanning on localhost | Information Disclosure | Bind to 127.0.0.1 only; no external interfaces |
| Unbounded JSON payloads | Denial of Service | Implement reasonable maximum message size (64KB) during deserialization |
</threat_model>