---
phase: 1
plan: 2
type: execute
wave: 2
depends_on:
  - 01-PLAN-1
files_modified:
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
  - tests/TestLooperState.cpp
  - tests/TestBridgeClient.cpp
autonomous: true
requirements:
  - PLUG-01
  - PLUG-02
  - PLUG-03
---

# Phase 1 Plan 2: Plugin Scaffold, Model & Bridge

<objective>
Build the JUCE plugin processor (audio passthrough with connection status parameter), the LooperState/LooperTracker model (shadow state management), and the BridgeClient (OSC send/receive with handshake and reconnect). This makes the plugin loadable in Ableton Live with connection infrastructure ready.
</objective>

<tasks>

## Task 2.1: Implement LooperState and LooperTracker Model

<read_first>
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (Shadow State pattern, LooperState struct, LooperTracker class)
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (D-03: full state push on change)
- src/Shared/ProtocolDefs.h (protocol constants and state values)
- .planning/research/ARCHITECTURE.md (Shadow State model details, component responsibilities)
</read_first>

<action>
1. Create `src/Model/LooperState.h` with a `LooperState` struct:
   ```cpp
   #pragma once
   #include <juce_core/juce_core.h>

   namespace looper {

   struct LooperState {
       juce::String trackId;        // Stable identifier (track name for v1)
       juce::String trackName;      // Human-readable track name
       juce::String deviceId;       // Device index within track as string
       juce::String deviceName;      // Device display name
       juce::String className;       // Device class_name (e.g. "Looper")
       enum State { Stopped, Recording, Playing, Overdubbing } state = Stopped;
       float feedback = 0.5f;        // 0.0-1.0 normalized
       bool reverse = false;
       int loopLengthBars = 0;       // 0 = free, N = N bars

       // Comparison for diff-based updates
       bool operator==(const LooperState& other) const;
       bool operator!=(const LooperState& other) const { return !(*this == other); }

       // Convert state enum to string (matching protocol constants)
       static juce::String stateToString(State s);
       static State stringToState(const juce::String& s);
   };

   } // namespace looper
   ```

2. Create `src/Model/LooperState.cpp` implementing comparison and conversion methods. State string values must match `ProtocolDefs.h`: "Stopped", "Recording", "Playing", "Overdubbing".

3. Create `src/Model/LooperTracker.h` with a `LooperTracker` class:
   ```cpp
   #pragma once
   #include "LooperState.h"
   #include <juce_core/juce_core.h>
   #include <juce_data_structures/juce_data_structures.h>

   namespace looper {

   class LooperTracker {
   public:
       LooperTracker() = default;

       // Add/remove loopers (from discovery)
       void addLooper(const LooperState& state);
       void removeLooper(const juce::String& trackId);

       // Update state (from Remote Script push, D-03)
       void updateState(const juce::String& trackId, const LooperState& newState);

       // Get state
       std::vector<LooperState> getAllLoopers() const;
       std::optional<LooperState> getLooper(const juce::String& trackId) const;
       int getLooperCount() const;

       // Connection status
       void setConnected(bool connected);
       bool isConnected() const;

       // Change broadcasting
       juce::ChangeListener* getChangeListener() const;

   private:
       std::map<juce::String, LooperState> loopers_;  // key: trackId
       bool connected_ = false;
       juce::ChangeBroadcaster broadcaster_;
   };

   } // namespace looper
   ```

4. Create `src/Model/LooperTracker.cpp` implementing all methods. `updateState()` must only notify listeners if the state actually changed (diff-based updates).

5. Create `tests/TestLooperState.cpp` with Catch2 test cases:
   - `TEST_CASE("LooperState comparison: equal states")` — two identical states compare equal
   - `TEST_CASE("LooperState comparison: different states")` — states with different values compare unequal
   - `TEST_CASE("LooperState stateToString conversion")` — verify Stopped→"Stopped", Recording→"Recording", Playing→"Playing", Overdubbing→"Overdubbing"
   - `TEST_CASE("LooperState stringToState conversion")` — verify reverse mapping
   - `TEST_CASE("LooperTracker add and retrieve looper")` — add a looper, retrieve by trackId
   - `TEST_CASE("LooperTracker update state fires change")` — verify updateState with different state triggers change
   - `TEST_CASE("LooperTracker update state no change")` — verify updateState with same state does NOT trigger change
   - `TEST_CASE("LooperTracker remove looper")` — add then remove, verify getLooper returns nullopt

6. Update `tests/CMakeLists.txt` to include `TestLooperState.cpp`.
</action>

<acceptance_criteria>
- `src/Model/LooperState.h` exists with `LooperState` struct containing `trackId`, `trackName`, `deviceId`, `deviceName`, `className`, `State` enum, `feedback`, `reverse`, `loopLengthBars` fields
- `src/Model/LooperTracker.h` exists with `LooperTracker` class containing `addLooper`, `removeLooper`, `updateState`, `getAllLoopers`, `getLooper`, `setConnected`, `isConnected` methods
- `updateState` only triggers change when state actually differs (diff-based, no redundant notifications)
- State string values map exactly to ProtocolDefs.h constants: "Stopped", "Recording", "Playing", "Overdubbing"
- `tests/TestLooperState.cpp` exists with at least 8 test cases covering comparison, conversion, and tracker operations
- All tests pass: `ctest --test-dir build --output-on-failure` exits 0
</acceptance_criteria>

## Task 2.2: Implement BridgeClient (OSC Send/Receive with Handshake)

<read_first>
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (D-05: port range fallback, D-06: immediate handshake, D-07: auto-reconnect)
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (BridgeClient pattern, OSCSender/OSCReceiver, thread safety)
- src/Shared/ProtocolDefs.h (port constants, OSC addresses)
- src/Bridge/MessageProtocol.h (message format)
- .planning/research/PITFALLS.md (pitfall 4: audio thread contamination, pitfall 3: port conflicts)
</read_first>

<action>
1. Create `src/Bridge/BridgeClient.h` with a `BridgeClient` class:
   ```cpp
   #pragma once
   #include <juce_osc/juce_osc.h>
   #include <juce_core/juce_core.h>
   #include "MessageProtocol.h"
   #include "../Model/LooperTracker.h"

   namespace looper {

   class BridgeClient : public juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback> {
   public:
       BridgeClient(LooperTracker& tracker);
       ~BridgeClient();

       // Connection lifecycle (D-06: immediate handshake, D-07: auto-reconnect)
       bool connect();          // Bind to port range (D-05), start handshake
       void disconnect();       // Clean disconnect
       bool isConnected() const;

       // Send commands to Remote Script
       void sendHello();        // D-06: immediate handshake on load
       void sendDiscover();     // Request looper discovery
       void sendCommand(const protocol::Message& msg);  // Send any command

       // OSCReceiver::Listener callback (runs on message thread)
       void oscMessageReceived(const juce::OSCMessage& message) override;
       void osCSocketClosed(const juce::OSCReceiver::RemoteEndpointDetails&) {}

   private:
       // Port binding with range fallback (D-05)
       bool bindToPortRange(int startPort, int endPort);

       // Message handling
       void handleEvent(const protocol::Event& event);
       void handleLooperDiscovered(const protocol::Event& event);   // D-03: full state push
       void handleLooperStateChanged(const protocol::Event& event); // D-03: full state push
       void handleResult(const protocol::Event& event);

       // Auto-reconnect with exponential backoff (D-07)
       void startReconnectTimer();
       void stopReconnectTimer();
       void onReconnectTimer();

       LooperTracker& tracker_;
       juce::OSCSender sender_;
       juce::OSCReceiver receiver_;
       int localPort_ = 0;       // Port we bound to
       int remotePort_ = 0;     // Remote Script's port (discovered via handshake)
       bool connected_ = false;

       // Reconnect state
       std::unique_ptr<juce::Timer> reconnectTimer_;
       int reconnectAttempt_ = 0;
       static constexpr int MAX_RECONNECT_ATTEMPTS = 50;
       static constexpr int INITIAL_RECONNECT_DELAY_MS = 500;
   };

   } // namespace looper
   ```

2. Create `src/Bridge/BridgeClient.cpp` implementing all methods:
   - `connect()` — tries to bind `OSCReceiver` to ports 7010-7019 in sequence. On success, stores the bound port. Then calls `sendHello()`.
   - `bindToPortRange()` — iterates through port range, tries `receiver.connect(port)` for each, returns true on first success, logs the bound port.
   - `sendHello()` — creates a hello message via `MessageProtocol::createHello(localPort_)`, serializes to JSON, sends via `OSCSender` to `127.0.0.1:remotePort_`.
   - `oscMessageReceived()` — receives incoming OSC message, extracts JSON payload, parses via `MessageProtocol::jsonToEvent`, dispatches to `handleEvent()`.
   - `handleEvent()` — switches on event type (looper_discovered, looper_state_changed, result) and calls appropriate handler.
   - `handleLooperDiscovered()` — creates `LooperState` from event data, calls `tracker_.addLooper()`.
   - `handleLooperStateChanged()` — creates `LooperState` from event data, calls `tracker_.updateLooper()` (D-03: full state push).
   - `startReconnectTimer()` — implements exponential backoff (500ms initial, doubled each attempt, max 30s between attempts, max 50 attempts).
   - Use `juce::MessageManager::callAsync()` if any OSC processing needs to be marshaled to the message thread.

3. Create `tests/TestBridgeClient.cpp` with unit tests (using mock tracker or tracker with no actual OSC network dependency):
   - `TEST_CASE("BridgeClient bind to port range succeeds for first available port")` — mock test or skip if no network
   - `TEST_CASE("BridgeClient hello message format")` — verify hello message has correct format with port
   - `TEST_CASE("BridgeClient handles looper_discovered event")` — create a test event JSON, feed through MessageProtocol, verify LooperState is created
   - `TEST_CASE("BridgeClient handles looper_state_changed event")` — create state change event JSON, verify LooperTracker is updated

4. Update `tests/CMakeLists.txt` to include `TestBridgeClient.cpp`.
</action>

<acceptance_criteria>
- `src/Bridge/BridgeClient.h` exists with `BridgeClient` class
- `connect()` attempts to bind to ports 7010-7019 in sequence (D-05)
- `sendHello()` creates a hello message with local port and version field (D-06)
- `oscMessageReceived()` callback uses `MessageLoopCallback` template (runs on message thread, not audio thread)
- `handleLooperDiscovered()` and `handleLooperStateChanged()` both update `LooperTracker` with full state (D-03)
- Exponential backoff reconnect is implemented with INITIAL_RECONNECT_DELAY_MS = 500ms and MAX_RECONNECT_ATTEMPTS = 50 (D-07)
- `tests/TestBridgeClient.cpp` exists with at least 4 test cases
- All tests pass: `ctest --test-dir build --output-on-failure` exits 0
</acceptance_criteria>

## Task 2.3: Implement PluginProcessor (Audio Passthrough with Connection Status)

<read_first>
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (D-08: audio passthrough, D-09: VST3 only)
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (PluginProcessor skeleton, APVTS pattern)
- .planning/research/PITFALLS.md (Pitfall 2: blocking audio thread, Pitfall 5: editor lifecycle)
- src/Model/LooperTracker.h (tracker that BridgeClient updates)
- src/Bridge/BridgeClient.h (connection manager)
</read_first>

<action>
1. Create `src/Plugin/PluginProcessor.h` with a `LiveLooperProcessor` class:
   ```cpp
   #pragma once
   #include <juce_audio_processors/juce_audio_processors.h>
   #include "../Model/LooperTracker.h"
   #include "../Bridge/BridgeClient.h"

   namespace looper {

   class LiveLooperProcessor : public juce::AudioProcessor {
   public:
       LiveLooperProcessor();
       ~LiveLooperProcessor() override;

       // AudioProcessor overrides
       void prepareToPlay(double sampleRate, int samplesPerBlock) override;
       void releaseResources() override;
       void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

       // State persistence
       void getStateInformation(juce::MemoryBlock& destData) override;
       void setStateInformation(const void* data, int sizeInBytes) override;

       // Required overrides
       const juce::String getName() const override { return "Live Looper Controller"; }
       bool acceptsMidi() const override { return true; }
       bool producesMidi() const override { return false; }
       double getTailLengthSeconds() const override { return 0.0; }
       int getNumPrograms() override { return 1; }
       int getCurrentProgram() override { return 0; }
       void setCurrentProgram(int) override {}
       const juce::String getProgramName(int) override { return {}; }
       void changeProgramName(int, const juce::String&) override {}
       bool hasEditor() const override { return true; }
       juce::AudioProcessorEditor* createEditor() override;

       // APVTS access
       juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }

   private:
       static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

       juce::AudioProcessorValueTreeState apvts_;
       LooperTracker looperTracker_;
       std::unique_ptr<BridgeClient> bridgeClient_;

       JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LiveLooperProcessor)
   };

   } // namespace looper
   ```

2. Create `src/Plugin/PluginProcessor.cpp` implementing all methods:
   - Constructor: initialize `apvts_` with parameter layout containing one `AudioParameterBool` called "connected" with `ParameterID("connected", 1)` (version hint!).
   - `prepareToPlay()`: initialize `BridgeClient` and call `bridgeClient_->connect()`. This triggers the handshake (D-06).
   - `processBlock()`: audio passthrough — DO NOTHING with the audio buffer. Add `juce::ScopedNoDenormals noDenormals;` at the top. This fulfills D-08.
   - `getStateInformation()`/`setStateInformation()`: serialize/deserialize APVTS state using `copyState()`/`replaceState()`.
   - `releaseResources()`: call `bridgeClient_->disconnect()`.
   - `createEditor()`: return new `LiveLooperEditor(*this)`.

3. Create `src/Plugin/PluginEditor.h` and `PluginEditor.cpp` with minimal editor:
   ```cpp
   class LiveLooperEditor : public juce::AudioProcessorEditor {
   public:
       explicit LiveLooperEditor(LiveLooperProcessor& p)
           : AudioProcessorEditor(&p), processor_(p) {
           setSize(400, 300);
           // Phase 1: minimal editor showing connection status
           addAndMakeVisible(statusLabel_);
           statusLabel_.setText("Connecting...", juce::dontSendNotification);
           statusLabel_.setJustificationType(juce::Justification::centred);
       }
       void paint(juce::Graphics& g) override {
           g.fillAll(juce::Colours::darkgrey);
       }
       void resized() override {
           statusLabel_.setBounds(getLocalBounds());
       }
   private:
       LiveLooperProcessor& processor_;
       juce::Label statusLabel_;
   };
   ```
   - The editor shows a simple status label: "Connecting...", "Connected", or "Disconnected — reconnecting..."
   - It reads the "connected" parameter from APVTS to update the label.
   - All persistent state is in the processor, NOT in the editor (Pitfall 5 from RESEARCH).

4. Update `CMakeLists.txt` `target_sources` to include all new .cpp files if not already listed.

5. Build and verify the plugin loads in JUCE AudioPluginHost:
   ```bash
   cmake --build build --config Debug
   # Open the built VST3 in AudioPluginHost to verify it loads
   ```
</action>

<acceptance_criteria>
- `src/Plugin/PluginProcessor.h` exists with `LiveLooperProcessor` class extending `juce::AudioProcessor`
- `processBlock()` contains `juce::ScopedNoDenormals noDenormals;` and does NOT modify the audio buffer (audio passthrough per D-08)
- `createParameterLayout()` includes `AudioParameterBool` with `ParameterID("connected", 1)` — version hint included (per PITFALLS)
- `prepareToPlay()` calls `BridgeClient::connect()` which triggers the handshake (D-06)
- `releaseResources()` calls `BridgeClient::disconnect()`
- `getStateInformation()`/`setStateInformation()` persist APVTS state (plugin state save/load works)
- `PluginEditor.cpp` contains a `statusLabel_` that shows "Connecting...", "Connected", or "Disconnected"
- No persistent state stored in the editor class — all state is in the processor/APVTS
- Plugin compiles and builds without errors
- VST3 binary exists in the build output directory
</acceptance_criteria>

</tasks>

<verification>
1. All C++ unit tests pass: `ctest --test-dir build --output-on-failure` exits 0
2. Plugin compiles without errors: `cmake --build build --config Debug` exits 0
3. VST3 plugin binary exists in build output
4. Plugin loads in AudioPluginHost without crashing
5. `processBlock()` is a no-op for audio (passthrough verified)
6. BridgeClient attempts to bind to port 7010-7019 range
7. Handshake message format includes version field and local port
8. LooperTracker correctly adds/updates/removes loopers
9. Diff-based update: updating with same state does not trigger notification
</verification>

<must_haves>
- Plugin loads as VST3 without crashing (PLUG-01 partial — loads but not yet tested in Ableton)
- Audio passes through unmodified (D-08)
- LooperTracker models shadow state correctly with diff-based updates (D-03)
- BridgeClient connects to port range 7010-7019 (D-05)
- Handshake on load with exponential backoff retry (D-06, D-07)
- All state in processor/APVTS, not in editor (Pitfall 5)
- No audio thread blocking (all network/OSC on message thread)
</must_haves>

<threat_model>
## Threat Model — Plan 2: Plugin Scaffold & Bridge

| Threat | STRIDE | Mitigation |
|--------|--------|------------|
| Malformed OSC message crashes plugin | Denial of Service | MessageProtocol validates all incoming JSON before processing; BridgeClient catches exceptions in oscMessageReceived |
| Audio thread contamination from OSC callbacks | Denial of Service | OSCReceiver uses MessageLoopCallback template ensuring callbacks run on message thread; processBlock does zero network I/O |
| Port exhaustion (all 7010-7019 ports taken) | Denial of Service | BridgeClient logs error and enters reconnect cycle; status parameter shows "Disconnected"; no crash |
</threat_model>