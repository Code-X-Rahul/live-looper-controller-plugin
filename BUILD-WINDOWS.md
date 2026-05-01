# Building Live Looper Controller on Windows

## Prerequisites

1. **Visual Studio 2022 Community** (free download from https://visualstudio.microsoft.com/downloads/)
   - During install, select "Desktop development with C++"

2. **Git** (optional, for cloning)
   - Download from https://git-scm.com/download/win

## Build Steps

### Option 1: Using Visual Studio GUI

1. Open Visual Studio 2022
2. Click "Continue without code"
3. File → Open → CMake → Select `CMakeLists.txt` in this folder
4. Wait for CMake configuration to complete (~1-2 minutes)
5. Build → Build All (or press Ctrl+B)

The VST3 plugin will be generated at:
```
build\LiveLooperController_artefacts\Release\VST3\Live Looper Controller.vst3
```

### Option 2: Using Command Line

1. Open "Developer Command Prompt for VS 2022" (from Start menu)
2. Navigate to this project folder
3. Run:
```batch
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Installing the Plugin

1. Copy the entire `Live Looper Controller.vst3` folder to:
   ```
   C:\Program Files\Common Files\VST3\
   ```

2. Open Ableton Live
3. Go to Tracks → Add Track → MIDI Effect (or Audio Effect)
4. Find "Live Looper Controller" in the browser under VST3
5. Drag it onto a track

## Testing with Ableton Live

1. Add a Looper device to a track in Ableton (Live's built-in device)
2. Add "Live Looper Controller" MIDI effect to the same track (or a different track)
3. The plugin should auto-connect to Ableton and discover the Looper
4. Look for the connection status in the plugin header

## Troubleshooting

**CMake fails to configure:**
- Make sure Visual Studio 2022 is up to date
- Try opening "x64 Native Tools Command Prompt for VS 2022" and running cmake from there

**Plugin doesn't appear in Ableton:**
- Confirm the .vst3 folder is in `C:\Program Files\Common Files\VST3\`
- Try restarting Ableton
- Check Ableton's plugin manager (Options → Plugins) for the plugin

**Plugin loads but shows "Disconnected":**
- Make sure the Remote Script is running in Ableton (Live must be running with the project open)
- Check Windows Firewall isn't blocking localhost communication
- Try a simple project with just one Looper device first