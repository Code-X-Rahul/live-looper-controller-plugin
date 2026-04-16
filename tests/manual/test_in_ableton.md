# Manual Test: Phase 1 Validation in Ableton Live

**Date:** _________________
**Ableton Live Version:** _________________
**OS:** _________________ (macOS / Windows)
**Tester:** _________________

## Prerequisites

Before testing, ensure you have:

- Ableton Live 11+ (Standard or Suite recommended)
- Live Looper Controller VST3 plugin built and installed
- Remote Script installed via `scripts/install_remote_script.sh` (macOS) or `scripts/install_remote_script.bat` (Windows)
- pluginval installed (for Test 5)

---

## Test 1: Plugin Loads in Ableton Live (PLUG-01)

**Objective:** Verify the VST3 plugin loads correctly in Ableton Live without crashes or errors.

### Steps

1. Open Ableton Live
2. Create a new project with at least 2 audio tracks
3. Add a Looper device to Track 1: Click the "+" button on Track 1 > Audio Effects > Looper
4. Add a Looper device to Track 2: Click the "+" button on Track 2 > Audio Effects > Looper
5. On Track 1, add the "Live Looper Controller" VST3 plugin as an audio effect:
   - Click the "+" button on Track 1 > Audio Effects
   - Find "Live Looper Controller" in the list
6. Verify: Plugin loads without error messages in Live
7. Verify: Plugin editor window opens showing "Connecting..." or "Connected" status
8. Verify: Audio passes through unchanged — arm both tracks and play audio, it should not be affected

### Expected Results

| Checkpoint | PASS/FAIL | Notes |
|------------|-----------|-------|
| Plugin appears in audio effects browser | | |
| Plugin loads without error dialog | | |
| Plugin editor window opens | | |
| Editor shows "Connecting..." or "Connected" | | |
| Audio passes through unchanged | | |

---

## Test 2: Remote Script Discovers Loopers (PLUG-02)

**Objective:** Verify the Python Remote Script loads in Ableton Live and discovers looper devices across tracks.

### Steps

1. Open Live's Preferences > Link/Tempo/MIDI > Control Surfaces
2. Select "LooperControl" from the available Control Surfaces dropdown
3. Click "Refresh" if needed
4. Open Live's Log.txt file to check for Remote Script initialization:
   - **macOS:** `~/Library/Logs/Ableton/Live*.log.txt`
   - **Windows:** `%APPDATA%\Ableton\Live*\Log.txt`
5. Look for these log entries:
   - `LooperControlSurface: Initializing...`
   - `LooperControlSurface: Found N looper(s)`
   - `BridgeServer started on port 7011`
6. Verify: The plugin editor shows the discovered looper count
7. Verify: Each looper's track name appears in the plugin status area

### Expected Results

| Checkpoint | PASS/FAIL | Notes |
|------------|-----------|-------|
| "LooperControl" appears in Control Surfaces list | | |
| Remote Script initializes (Log.txt entry) | | |
| BridgeServer starts on port 7011 | | |
| Log shows "Found N looper(s)" | | |
| Plugin editor shows correct looper count | | |
| Track names visible in plugin editor | | |

---

## Test 3: Bidirectional Communication (PLUG-01/PLUG-02)

**Objective:** Verify looper state changes in Ableton's native UI are reflected in the plugin.

### Steps

1. With everything running from Tests 1 and 2:
2. In Ableton's native Looper UI on Track 1, click the **Record** button
3. Watch the Live Looper Controller plugin editor:
   - Verify Track 1's looper state updates to "Recording"
4. In Ableton's native Looper UI on Track 1, click the **Play** button
5. Watch the plugin editor:
   - Verify Track 1's looper state updates to "Playing"
6. In Ableton's native Looper UI on Track 2, click **Record**, then **Play**
7. Verify Track 2's state updates correctly
8. Estimate round-trip latency:
   - Click record and note how quickly the plugin updates
   - Should feel immediate (< 100ms)

### Expected Results

| Checkpoint | PASS/FAIL | Notes |
|------------|-----------|-------|
| Track 1 "Recording" state reflected in plugin | | |
| Track 1 "Playing" state reflected in plugin | | |
| Track 2 state changes reflected | | |
| State updates feel immediate (< 100ms) | | |

---

## Test 4: Pattern-Based Discovery (PLUG-03)

**Objective:** Verify looper discovery works for non-Ableton looper devices using parameter patterns.

### Steps

1. Create a new audio track (Track 3)
2. Add a third-party looper plugin **OR** create a test device:
   - Add any audio effect device
   - Rename it and add parameters named "Record", "Play", "Stop"
   - (This simulates a third-party looper for testing)
3. Check Live's Log.txt for new looper discovery:
   - Should show "Found N looper(s)" with the new device included
4. Verify the plugin editor shows the third-party looper

### Alternative: Test with Multiple Looper Patterns

If you have multiple types of loopers:

1. Track 1: Standard Ableton Looper (State + Feedback params)
2. Track 2: Generic device with Record + Play + Stop params
3. Track 3: Generic device with Record + Overdub + Play params
4. Verify all three are discovered and shown

### Expected Results

| Checkpoint | PASS/FAIL | Notes |
|------------|-----------|-------|
| Third-party looper discovered | | |
| Discovery logged in Log.txt | | |
| All looper patterns detected | | |
| Non-looper devices NOT discovered | | |

---

## Test 5: Pluginval Validation (PLUG-01)

**Objective:** Verify the VST3 plugin passes Pluginval compliance testing.

### Steps

1. Locate the built VST3 plugin binary:
   - **macOS:** `~/Library/Audio/Plug-Ins/VST3/LiveLooperController.vst3`
   - **Windows:** `C:\Program Files\Common Files\VST3\LiveLooperController.vst3`
   - **Build output:** `build/VST3/LiveLooperController.vst3`
2. Run Pluginval validation:
   ```bash
   pluginval --validate-in-builds-dir build/VST3/
   ```
   Or specify the exact path:
   ```bash
   pluginval --validate "/path/to/LiveLooperController.vst3"
   ```
3. Review the validation report

### Expected Results

| Checkpoint | PASS/FAIL | Notes |
|------------|-----------|-------|
| Pluginval runs without crash | | |
| All strictness levels pass | | |
| No critical errors reported | | |

**Note:** Some warnings may be expected for a Phase 1 plugin. Document any warnings below:

```
Warnings/Issues:
_________________________________
_________________________________
_________________________________
```

---

## Cleanup

After completing all tests:

1. **Remove Control Surface assignment:**
   - Open Live Preferences > Link/Tempo/MIDI > Control Surfaces
   - Change "LooperControl" back to "None" or remove it

2. **Remove plugin from tracks:**
   - Right-click on the Live Looper Controller on each track
   - Select "Remove"

3. **Uninstall Remote Script (optional):**
   ```bash
   # macOS
   rm -rf "$HOME/Music/Ableton/User Library/Remote Scripts/LooperControl"

   # Windows
   rmdir /s /q "%USERPROFILE%\Music\Ableton\User Library\Remote Scripts\LooperControl"
   ```

---

## Test Summary

| Test | Result | Issues |
|------|--------|--------|
| Test 1: Plugin Loading | | |
| Test 2: Remote Script Discovery | | |
| Test 3: Bidirectional Communication | | |
| Test 4: Pattern-Based Discovery | | |
| Test 5: Pluginval Validation | | |

**Overall Phase 1 Status:** _____ (Ready / Needs Fixes / Blocked)

---

## Notes

Additional observations, issues, or recommendations:

_____________________________________________

_____________________________________________

_____________________________________________
