<!-- GSD:project-start source:PROJECT.md -->
## Project

**Live Looper Controller**

A minimal AU/VST3 control surface plugin that gives musicians a single panel to see the state of and control all looper devices across multiple tracks in their DAW. Designed DAW-agnostic but shipping Ableton Live first, it eliminates the need to navigate scattered looper devices during a live performance — one interface, full control.

**Core Value:** Perform a complete live looping set without touching the native DAW UI — see every looper's state and control record, overdub, play, and stop in real time from a single surface.

### Constraints

- **Tech format**: Must be AU/VST3 plugin (C++ with JUCE framework is the standard approach)
- **DAW compatibility**: v1 targets Ableton Live only; architecture should not prevent future DAW support
- **Platform**: macOS and Windows (Ableton Live runs on both)
- **No audio DSP**: Plugin acts as a controller/MIDI effect, not an audio processor
- **Ableton Live 1.0+**: Must work with Ableton Live Intro (not just Suite)
<!-- GSD:project-end -->

<!-- GSD:stack-start source:research/STACK.md -->
## Technology Stack

## Recommended Stack
### Core Technologies
| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| JUCE | 8.0.12 | AU/VST3 plugin framework | De facto industry standard for audio plugin development. Provides cross-platform AU/VST3/AAX support, built-in parameter management (AudioProcessorValueTreeState), GUI toolkit, MIDI handling, OSC, and IPC — everything needed for a control surface plugin. No alternative comes close in maturity and community support. |
| C++ | 17 | Primary language | JUCE 8 requires C++17 minimum. Modern C++ provides structured bindings, optional, variant, filesystem — all useful for this project. C++20 not required and would unnecessarily limit compiler support. |
| CMake | 3.22+ | Build system | JUCE's official and recommended build system. `juce_add_plugin()` provides first-class support for all plugin formats, signing, and deployment. Projucer (the alternative) is deprecated in favor of CMake. CMake 3.22 is the minimum JUCE 8 requires. |
| Python 3 | 3.9+ | Ableton Live Remote Script | Ableton Live ships with its own embedded Python interpreter (3.9 in Live 12). The Remote Script must be written in Python to run inside Ableton's scripting environment. This is not a choice — it's the only way to access Live's internal API for track/device discovery and parameter control. |
### Supporting Libraries
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| JUCE `juce_osc` module | (included in JUCE 8) | OSC send/receive between plugin and Remote Script | Always — this is the primary communication channel between the JUCE plugin and the Ableton Remote Script |
| JUCE `juce_audio_processors` module | (included in JUCE 8) | AudioProcessorValueTreeState for parameter management | Always — standard JUCE parameter management for plugin state |
| JUCE `juce_gui_basics` + `juce_gui_extra` modules | (included in JUCE 8) | Control surface UI (buttons, sliders, state indicators) | Always — the plugin needs a rich UI for showing looper states |
| JUCE `juce_data_structures` module | (included in JUCE 8) | ValueTree + UndoManager for plugin state and serialization | Always — for persisting plugin state (looper configurations, mappings) |
| `python-osc` | 1.9+ | OSC communication in the Remote Script | Always — PythonOSC is the only viable option for OSC in Ableton's embedded Python. Pure Python, no C extensions needed. |
| Catch2 | 3.x | C++ unit testing framework | During development — modern BDD/TDD testing for plugin logic |
| fmt (or spdlog) | 11.x+ / 1.15+ | C++ string formatting and logging | During development — better logging than JUCE's DBG macro. spdlog adds async logging; fmt alone is lighter. |
### Development Tools
| Tool | Purpose | Notes |
|------|---------|-------|
| Xcode | macOS build, debug, AU validation | Required for AU development on macOS. Use latest stable. |
| Visual Studio 2022 | Windows build, VST3 debug | Required for Windows builds. Community edition works. |
| Ableton Live Suite 12 | Testing, Remote Script development | Suite required for max track count and all devices. Intro lacks Looper device in some configurations. |
| Pluginval | Plugin validation and stress testing | Free tool specifically designed for AU/VST3 validation. Catches format compliance issues early. |
| JUCE AudioPluginHost | Quick plugin testing | JUCE ships with a lightweight host for rapid iteration. Faster than loading into Ableton for basic testing. |
## Installation
# Core: Clone JUCE as submodule
# Dev dependencies
# CMake 3.22+ (system package manager or cmake.org)
# Catch2 and fmt via CMake FetchContent or vcpkg
# Remote Script dependencies
# python-osc: installed inside the Remote Script directory
# (Ableton's embedded Python doesn't support pip directly —
#  bundle python-osc source or copy it into the script directory)
# Build (macOS example)
# Build (Windows example)
## Alternatives Considered
| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| JUCE | iPlug2 | iPlug2 is lighter-weight and faster compile times, but has far less documentation, fewer format wrappers, smaller community, and no built-in OSC. Not worth the risk for a first plugin. |
| JUCE | Steinberg VST3 SDK directly | Only if you need absolute maximum control over VST3 internals and are willing to write AU wrappers yourself (or skip AU). 5-10x more work for no tangible benefit in this project. |
| JUCE | Max for Live (Max/MSP) | Max for Live can access Live's API directly — BUT it locks you into Ableton exclusively, cannot ship as AU/VST3, and doesn't support the DAW-agnostic architecture required. Use only if you're willing to abandon AU/VST3. |
| CMake | Projucer | Projucer is JUCE's legacy GUI-based project generator. JUCE's official recommendation is CMake. Projucer lacks CI/CD integration and is no longer actively developed. Use only for quick prototyping, never for production. |
| OSC over localhost | Named pipes (IPC) | Named pipes are faster but platform-specific (macOS vs Windows). OSC works identically on both platforms and is debuggable with generic OSC tools. Use IPC only if latency becomes problematic. |
| OSC over localhost | Virtual MIDI ports (JUCE MidiOutput) | MIDI CC feedback requires creating virtual MIDI ports which is fragile (port name collisions, OS permissions). OSC is more reliable, carries richer data (arbitrary addresses, multiple types), and doesn't require MIDI port setup from users. Use virtual MIDI only as a fallback for DAWs that don't support Remote Scripts. |
## What NOT to Use
| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Projucer for production builds | Deprecated in favor of CMake. No CI/CD support. Manual .jucer file management is error-prone and doesn't scale. | CMake with `juce_add_plugin()` |
| VST2 (VST2.4) format | Steinberg terminated the VST2 license in 2018. No new products may use it. Ableton Live 10+ supports VST3 natively. | VST3 + AU formats only |
| AAX format (for v1) | Pro Tools is out of scope for v1. AAX requires PACE iLok SDK integration, signing, and a separate developer agreement with Avid. Massive overhead for no v1 benefit. | VST3 + AU, add AAX later if needed |
| LV2 format (for v1) | LV2 is primarily for Linux DAWs. Ableton Live doesn't support LV2. Adding format targets is easy with JUCE but adds testing burden. | VST3 + AU, add LV2 later for Linux support |
| Raw MIDI for state feedback | MIDI CCs only carry 7-bit values (0-127). Looper state is categorical (record/play/overdub/stop), but feeding back via MIDI CCs requires round-trip mapping and virtual ports. Fragile and limited. | OSC for plugin↔Remote Script communication |
| JUCE `juce_dsp` module | This module provides FFT, oscillators, convolution, and other DSP building blocks. This project is a control surface plugin with no audio DSP. Including it adds unnecessary binary size. | Omit `juce_dsp` from linked modules |
| Ableton Live Intro for testing | Live Intro may lack the Looper device or have track limitations that prevent multi-track looper testing. | Use Live Standard or Suite for development |
## Stack Patterns by Variant
- Build AU + VST3 formats for macOS
- Use Xcode as the primary development environment
- Use `auval` (Apple's AU validation tool) for format compliance testing
- Remote Script testing: Ableton Live 12 on macOS
- Build VST3 format for Windows
- Use Visual Studio 2022 with CMake integration
- Remote Script works identically on Windows Ableton — Python is embedded
- CMake handles cross-platform builds natively with JUCE
- Format targets: VST3 (both platforms) + AU (macOS only)
- CI/CD: GitHub Actions with macOS and Windows runners
- Remote Script: Python code is platform-agnostic
- The Remote Script component is DAW-specific — each DAW needs its own bridge
- For Logic Pro: AU plugin + Logic's Control Surface protocol (different from Ableton's)
- For Reaper: VST3 plugin + Reaper's JSFX/ReaScript API
- The JUCE plugin itself is DAW-agnostic — only the bridge script changes
- The OSC communication protocol must be documented and versioned for each DAW bridge
## Architecture Note: Two-Component Design
## Version Compatibility
| Package A | Compatible With | Notes |
|-----------|-----------------|-------|
| JUCE 8.0.x | CMake 3.22+ | Minimum CMake version. 3.25+ recommended for newer features. |
| JUCE 8.0.x | C++17 compilers | GCC 9+, Clang 10+, MSVC 2019 16.8+. |
| JUCE 8.0.x | macOS 12+ (Monterey) | Set `CMAKE_OSX_DEPLOYMENT_TARGET=12.0` for broad compatibility. |
| JUCE 8.0.x | Xcode 14+ | Required for Apple Silicon + Universal Binary builds. |
| python-osc | Python 3.9+ | Ableton Live 12 embeds Python 3.9. Scripts must be compatible. |
| AU format | macOS only | JUCE handles AU builds only on macOS. Conditional in CMake. |
| VST3 format | macOS + Windows | Cross-platform. JUCE's VST3 SDK is embedded. |
## Sources
- JUCE official CMake API documentation — verified `juce_add_plugin()` with `IS_MIDI_EFFECT`, `FORMATS`, `NEEDS_MIDI_INPUT`/`NEEDS_MIDI_OUTPUT` options
- JUCE GitHub releases — JUCE 8.0.12 confirmed as latest stable (Dec 2025)
- JUCE AudioProcessor docs — verified `isMidiEffect()`, `AudioProcessorValueTreeState`, parameter management
- JUCE OSC module docs — verified `OSCSender`/`OSCReceiver` API for localhost UDP communication
- Ableton Live 12 manual — verified Looper device parameters (Record, Overdub, Play, Stop, Feedback, Record Length, Song Sync)
- Ableton Live 12 MIDI Remote Scripts (gluon/AbletonLive12_MidiRemoteScripts) — verified ControlSurface architecture, device parameter control, and user configuration format
- Ableton.js (leolabs/ableton-js) — confirmed Remote Script pattern for Live API access via socket communication
- JUCE license page — confirmed dual-license (AGPLv3 free for open source, paid tiers for closed source)
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

Conventions not yet established. Will populate as patterns emerge during development.
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

Architecture not yet mapped. Follow existing patterns found in the codebase.
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, or `.github/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
