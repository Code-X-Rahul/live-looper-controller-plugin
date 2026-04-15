---
phase: 1
plan: 4
type: execute
wave: 4
depends_on:
  - 01-PLAN-2
  - 01-PLAN-3
files_modified:
  - src/Plugin/PluginProcessor.cpp
  - src/Bridge/BridgeClient.cpp
  - remote-script/LooperControlSurface.py
  - remote-script/BridgeServer.py
  - tests/test_integration_protocol.py
  - tests/manual/test_in_ableton.md
autonomous: false
requirements:
  - PLUG-01
  - PLUG-02
  - PLUG-03
---

# Phase 1 Plan 4: End-to-End Integration & Validation

<objective>
Wire the JUCE plugin and Python Remote Script together for end-to-end communication. Validate that: (1) the plugin loads in Ableton Live as VST3, (2) the Remote Script discovers looper devices across tracks, (3) bidirectional UDP communication works, and (4) looper state data flows from Remote Script to plugin and is logged. This is the critical validation plan that proves the two-component architecture works.
</objective>

<tasks>

## Task 4.1: Wire End-to-End Communication and Test Protocol Round-Trip

<read_first>
- src/Bridge/BridgeClient.h (OSC send/receive)
- src/Bridge/MessageProtocol.h (message format)
- src/Plugin/PluginProcessor.h (plugin entry point)
- remote-script/BridgeServer.py (OSC server)
- remote-script/LooperControlSurface.py (main script)
- remote-script/LooperDiscovery.py (device discovery)
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (D-01 through D-07)
</read_first>

<action>
1. Update `src/Bridge/BridgeClient.cpp` to properly handle incoming OSC messages from the Remote Script:
   - In `oscMessageReceived()`, extract the JSON payload from the OSC message's first argument
   - Parse the JSON using `MessageProtocol::jsonToEvent()`
   - Dispatch to `handleEvent()` which routes based on `event` field:
     - "looper_discovered" → call `handleLooperDiscovered()`
     - "looper_state_changed" → call `handleLooperStateChanged()`
     - "result" → call `handleResult()` (correlate with pending request UUIDs)
     - "looper_removed" → call `handleLooperRemoved()`
   - Log all received events to `juce::Logger` for Phase 1 debugging

2. Update `src/Plugin/PluginProcessor.cpp` to connect the components:
   - In `prepareToPlay()`, after creating `BridgeClient`, call `bridgeClient_->connect()`
   - Register a `ChangeListener` on `LooperTracker` that logs state changes:
     ```cpp
     looperTracker_.getChangeListener()->addChangeListener([this]() {
         auto loopers = looperTracker_.getAllLoopers();
         juce::Logger::writeToLog(
             juce::String::formatted("Looper state update: %d loopers tracked", loopers.size()));
         for (const auto& l : loopers) {
             juce::Logger::writeToLog(
                 juce::String::formatted("  %s: %s", l.trackName, LooperState::stateToString(l.state)));
         }
     });
     ```
   - Update the "connected" APVTS parameter when connection status changes

3. Update `PluginEditor.cpp` to display connection status and discovered loopers:
   - Show "Connected" (green) when `connected` parameter is true
   - Show "Disconnected — reconnecting..." (red) when false
   - Show count of discovered loopers
   - Show each looper's track name and state (text output for Phase 1 — colors come in Phase 2)

4. Create `tests/test_integration_protocol.py` — an integration test that runs both the BridgeClient and BridgeServer on localhost and verifies:
   - Client sends hello, server responds with version and port
   - Client sends discover, server responds with looper list
   - Server pushes looper_discovered event, client receives and creates LooperState
   - Server pushes looper_state_changed event, client receives and updates LooperState
   - Server pushes looper_removed event, client receives and removes looper
   - Version mismatch results in error response
   - All messages contain `"version": 1` field

5. Run the integration test:
   ```bash
   python3 -m pytest tests/test_integration_protocol.py -v
   ```
</action>

<acceptance_criteria>
- `BridgeClient::oscMessageReceived()` parses JSON from OSC first argument and dispatches to handlers
- `handleLooperDiscovered()` creates a `LooperState` from event data and adds to `LooperTracker`
- `handleLooperStateChanged()` creates a `LooperState` from event data and updates `LooperTracker`
- `PluginProcessor::prepareToPlay()` creates `BridgeClient` and connects
- `ChangeListener` on `LooperTracker` logs all state changes via `juce::Logger`
- `PluginEditor` shows connection status ("Connected" / "Disconnected")
- `PluginEditor` shows discovered looper count and track names
- Integration test file `tests/test_integration_protocol.py` exists and tests the full message round-trip
- All integration tests pass: `pytest tests/test_integration_protocol.py -v` exits 0
</acceptance_criteria>

## Task 4.2: Install and Validate in Ableton Live

<read_first>
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (Remote Script installation, Ableton Live testing)
- .planning/research/PITFALLS.md (Pitfall 2: Remote Script not loading)
- remote-script/ directory (all Python files)
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (Success criteria)
</read_first>

<action>
1. Create installation script `scripts/install_remote_script.sh` (macOS) and `scripts/install_remote_script.bat` (Windows):
   ```bash
   #!/bin/bash
   # Install the Remote Script into Ableton Live's User Library
   SCRIPT_NAME="LooperControl"
   TARGET_DIR="$HOME/Music/Ableton/User Library/Remote Scripts/$SCRIPT_NAME"

   mkdir -p "$TARGET_DIR"

   # Copy all Python files
   cp remote-script/__init__.py "$TARGET_DIR/"
   cp remote-script/LooperControlSurface.py "$TARGET_DIR/"
   cp remote-script/LooperDiscovery.py "$TARGET_DIR/"
   cp remote-script/BridgeServer.py "$TARGET_DIR/"
   cp remote-script/LiveAPIWrapper.py "$TARGET_DIR/"

   # Copy python-osc bundled source
   cp -r remote-script/python_osc "$TARGET_DIR/"

   echo "Remote Script installed to: $TARGET_DIR"
   echo "Restart Ableton Live to load the script."
   echo "In Live, go to Preferences > Link/Tempo/MIDI > Control Surfaces and select '$SCRIPT_NAME'"
   ```

2. Create a manual test procedure document `tests/manual/test_in_ableton.md` with step-by-step instructions:
   ```markdown
   # Manual Test: Phase 1 Validation in Ableton Live

   ## Prerequisites
   - Ableton Live 11+ (Standard or Suite recommended)
   - Built VST3 plugin (in build output directory)
   - Remote Script installed via install script

   ## Test 1: Plugin Loads in Ableton Live (PLUG-01)
   1. Open Ableton Live
   2. Create a new project with at least 2 audio tracks
   3. Add a Looper device to Track 1
   4. Add a Looper device to Track 2
   5. On Track 1, add the "Live Looper Controller" VST3 plugin as an audio effect
   6. Verify: Plugin loads without error messages in Live
   7. Verify: Plugin editor window opens showing "Connecting..." or "Connected" status
   8. Verify: No audio processing — audio passes through unchanged

   ## Test 2: Remote Script Discovers Loopers (PLUG-02)
   1. In Live, go to Preferences > Link/Tempo/MIDI > Control Surfaces
   2. Select "LooperControl" from the Control Surfaces list
   3. Verify: Live's Log.txt shows "LooperControlSurface: Initializing..." and "Found N looper(s)"
   4. Verify: The plugin editor shows discovered looper count matching the number of Looper devices
   5. Verify: Each looper's track name appears in the plugin status area

   ## Test 3: Bidirectional Communication (PLUG-01/PLUG-02)
   1. With everything running, click "Record" on Track 1's Looper in Ableton's native UI
   2. Verify: The plugin's status area updates to show Track 1's looper state as "Recording"
   3. Click "Play" on Track 2's Looper in Ableton's native UI
   4. Verify: The plugin updates Track 2's looper state to "Playing"
   5. Verify: Round-trip latency feels immediate (< 100ms)

   ## Test 4: Pattern-Based Discovery (PLUG-03)
   1. Create a new audio track (Track 3)
   2. Add a third-party looper plugin OR a device with Record + Play + Stop parameters
   3. Verify: The LooperControl script discovers this device (check Log.txt)
   4. Verify: The plugin shows the third-party looper in its status area

   ## Test 5: Pluginval Validation (PLUG-01)
   1. Run Pluginval against the built VST3 plugin binary
   2. Verify: All tests pass (no crashes, no parameter format issues)

   ## Cleanup
   - Remove Control Surface assignment in Live preferences
   - Remove plugin from track
   - Delete the installed Remote Script if desired
   ```

3. Build the VST3 plugin in Release configuration:
   ```bash
   cmake --build build --config Release
   ```

4. Run Pluginval against the built VST3 binary:
   ```bash
   pluginval --validate-in-builds-dir build/VST3/LiveLooperController.vst3
   ```
   (Or locate the exact VST3 binary path from the build output)

5. Document any test results and adjustments needed in `tests/manual/test_results.md`.
</action>

<acceptance_criteria>
- `scripts/install_remote_script.sh` exists and correctly copies files to `~/Music/Ableton/User Library/Remote Scripts/LooperControl/`
- `tests/manual/test_in_ableton.md` exists with step-by-step validation procedure for all 4 success criteria
- VST3 plugin builds in Release configuration without errors
- Pluginval validation runs against the VST3 binary (results documented, even if some warnings)
- Manual test document covers: plugin loading, looper discovery, bidirectional communication, pattern-based discovery
</acceptance_criteria>

## Task 4.3: Final Integration Verification and Documentation

<read_first>
- All previous plans (01-PLAN-1 through 01-PLAN-3)
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (success criteria)
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (pitfalls, architecture)
</read_first>

<action>
1. Create `tests/manual/test_results.md` — a template for recording manual test results:
   ```markdown
   # Phase 1 Manual Test Results

   **Date:** [date]
   **Ableton Live Version:** [version]
   **OS:** [macOS/Windows version]

   ## PLUG-01: Plugin loads as VST3 in Ableton Live

   | Test | Result | Notes |
   |------|--------|-------|
   | Plugin loads without errors | PASS/FAIL | |
   | Plugin passes audio through unchanged | PASS/FAIL | |
   | Plugin passes Pluginval validation | PASS/FAIL | |
   | Editor window opens with connection status | PASS/FAIL | |

   ## PLUG-02: Remote Script discovers looper devices

   | Test | Result | Notes |
   |------|--------|-------|
   | Script loads in Live preferences | PASS/FAIL | |
   | Script discovers Ableton Looper devices | PASS/FAIL | |
   | Script reports discovery over UDP | PASS/FAIL | |
   | State changes update in plugin | PASS/FAIL | |
   | Round-trip latency < 100ms | PASS/FAIL | |

   ## PLUG-03: Pattern-based looper discovery

   | Test | Result | Notes |
   |------|--------|-------|
   | Ableton Looper detected by State+Feedback params | PASS/FAIL | |
   | Third-party looper detected by Record+Play+Stop params | PASS/FAIL | |
   | Class name "Looper" still detected as fallback | PASS/FAIL | |
   | Non-looper device (e.g., Reverb) NOT detected | PASS/FAIL | |
   ```

2. Run all automated tests (C++ and Python) and ensure they pass:
   ```bash
   # C++ tests
   ctest --test-dir build --output-on-failure

   # Python tests
   pytest tests/ -v
   ```

3. Verify the complete checklist from the Phase 1 success criteria:
   - [ ] Plugin loads as VST3 in Ableton Live without errors
   - [ ] Plugin passes Pluginval validation
   - [ ] Remote Script discovers tracks containing looper devices via Ableton Live API and reports them over UDP
   - [ ] Plugin receives looper device state from Remote Script and logs it
   - [ ] Looper devices are identified by parameter patterns, not just Ableton Looper by name

4. Create `README.md` at the project root with:
   - Project description (from PROJECT.md)
   - Build instructions (CMake commands)
   - Remote Script installation instructions
   - Development workflow (how to test in Ableton Live)
   - Architecture overview (two-component hybrid, OSC protocol)
</action>

<acceptance_criteria>
- `tests/manual/test_results.md` exists with test result template covering all success criteria
- All C++ tests pass: `ctest --test-dir build --output-on-failure` exits 0
- All Python tests pass: `pytest tests/ -v` exits 0
- `README.md` exists at project root with build instructions and architecture overview
- README contains build commands: `cmake -Bbuild`, `cmake --build build`, install script
- README explains two-component architecture (JUCE plugin + Python Remote Script)
- VST3 plugin binary exists in build output
</acceptance_criteria>

</tasks>

<verification>
1. End-to-end protocol test passes: Python integration test sends messages through OSC and verifies response
2. BridgeClient correctly handles looper_discovered, looper_state_changed, and looper_removed events
3. BridgeServer correctly handles hello and discover commands
4. All automated C++ and Python tests pass
5. Plugin loads in Ableton Live without crash (manual verification)
6. Remote Script loads and discovers loopers (manual verification)
7. Looper state changes propagate from DAW to plugin (manual verification)
8. Pattern-based discovery works for non-AbletonLooper devices (manual or mock test)
9. Pluginval validation passes on VST3 binary
10. README.md documents build and installation process
</verification>

<must_haves>
- Plugin loads in Ableton Live as VST3 without errors (PLUG-01)
- Remote Script discovers loopers across tracks (PLUG-02)
- Bidirectional UDP communication works: plugin sends hello → script responds → script pushes looper state → plugin logs it
- Looper discovery uses parameter patterns, NOT class_name alone (PLUG-03)
- All CONTEXT.md locked decisions validated (D-01 through D-10)
- VST3 binary passes Pluginval
- README documents how to build, install, and test
</must_haves>

<threat_model>
## Threat Model — Plan 4: Integration & Validation

| Threat | STRIDE | Mitigation |
|--------|--------|------------|
| Remote Script fails silently in Ableton Live | Denial of Service | Enable Live debug logging (Options.txt with `-debug`); check Log.txt for Python errors |
| Multiple plugin instances conflict on UDP ports | Denial of Service | Port range fallback (D-05) tries next port; only first instance binds successfully |
| Stale Remote Script process after Live crash | Denial of Service | Handshake verifies liveness; auto-reconnect with backoff (D-07) creates new connection |
| Pluginval flags VST3 compliance issues | Denial of Service | Fix issues before shipping; documented known warnings in test_results.md |
</threat_model>