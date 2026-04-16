# Phase 1 Manual Test Results

**Date:** _________________
**Ableton Live Version:** _________________
**OS:** _________________ (macOS 12+ / Windows 10+)
**Tester:** _________________
**Plugin Build:** _________________ (commit hash or build #)

---

## Build Environment Note

**Automated Build Status:** Deferred to macOS/Windows build environment
- Linux CI environment lacks JUCE framework and X11 development libraries
- JUCE submodule version: 6.0.8 (Plan specifies 8.0.12 - needs update)
- Full VST3 build requires: macOS with Xcode or Windows with Visual Studio
- Pluginval validation requires the built VST3 binary

**Build Commands (for macOS/Windows):**
```bash
# macOS
cmake -B build -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build --config Release

# Windows
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release

# Validate with pluginval
pluginval --validate-in-builds-dir build/VST3/
```

---

## PLUG-01: Plugin loads as VST3 in Ableton Live

| Test | Result | Notes |
|------|--------|-------|
| Plugin loads without errors | PASS/FAIL | |
| Plugin passes audio through unchanged | PASS/FAIL | |
| Plugin passes Pluginval validation | PASS/FAIL | |
| Editor window opens with connection status | PASS/FAIL | |

**Details:**
_____________________________________________

---

## PLUG-02: Remote Script discovers looper devices

| Test | Result | Notes |
|------|--------|-------|
| Script loads in Live preferences | PASS/FAIL | |
| Script discovers Ableton Looper devices | PASS/FAIL | |
| Script reports discovery over UDP | PASS/FAIL | |
| State changes update in plugin | PASS/FAIL | |
| Round-trip latency < 100ms | PASS/FAIL | |

**Details:**
_____________________________________________

---

## PLUG-03: Pattern-based looper discovery

| Test | Result | Notes |
|------|--------|-------|
| Ableton Looper detected by State+Feedback params | PASS/FAIL | |
| Third-party looper detected by Record+Play+Stop params | PASS/FAIL | |
| Class name "Looper" still detected as fallback | PASS/FAIL | |
| Non-looper device (e.g., Reverb) NOT detected | PASS/FAIL | |

**Details:**
_____________________________________________

---

## Architecture Validation

| Test | Result | Notes |
|------|--------|-------|
| Plugin connects to Remote Script (UDP) | PASS/FAIL | |
| hello handshake completes successfully | PASS/FAIL | |
| looper_discovered events received | PASS/FAIL | |
| looper_state_changed events received | PASS/FAIL | |
| Version mismatch handled gracefully | PASS/FAIL | |

**Details:**
_____________________________________________

---

## Issues Encountered

| Issue | Severity | Resolution |
|-------|----------|------------|
| | | |
| | | |
| | | |

---

## Overall Assessment

**Phase 1 Status:** _____ (READY / NEEDS FIXES / BLOCKED)

**Summary:**
_____________________________________________

_____________________________________________

---

## Sign-off

Tester: _________________ Date: _________________

