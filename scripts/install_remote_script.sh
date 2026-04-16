#!/bin/bash
# Install the Live Looper Controller Remote Script into Ableton Live's User Library
#
# Usage: ./scripts/install_remote_script.sh
#
# This script copies the Remote Script files to Ableton Live's User Library
# directory where Live can discover and load them as a Control Surface.
#
# After running this script:
# 1. Restart Ableton Live (required for Live to discover the new script)
# 2. Open Live Preferences > Link/Tempo/MIDI > Control Surfaces
# 3. Select "LooperControl" from the available Control Surfaces list

set -e

SCRIPT_NAME="LooperControl"
SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/remote-script"

# macOS path: ~/Music/Ableton/User Library/Remote Scripts/
TARGET_DIR="$HOME/Music/Ableton/User Library/Remote Scripts/$SCRIPT_NAME"

echo "============================================"
echo "Live Looper Controller - Remote Script Installer"
echo "============================================"
echo ""
echo "Source: $SOURCE_DIR"
echo "Target: $TARGET_DIR"
echo ""

# Check if source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo "ERROR: Source directory not found: $SOURCE_DIR"
    exit 1
fi

# Check if Ableton Live User Library directory exists, create if not
if [ ! -d "$HOME/Music/Ableton/User Library" ]; then
    echo "Creating Ableton User Library directory..."
    mkdir -p "$HOME/Music/Ableton/User Library"
fi

# Create Remote Scripts directory if it doesn't exist
if [ ! -d "$HOME/Music/Ableton/User Library/Remote Scripts" ]; then
    echo "Creating Remote Scripts directory..."
    mkdir -p "$HOME/Music/Ableton/User Library/Remote Scripts"
fi

# Remove existing installation if present
if [ -d "$TARGET_DIR" ]; then
    echo "Removing existing installation..."
    rm -rf "$TARGET_DIR"
fi

# Create target directory
echo "Creating target directory..."
mkdir -p "$TARGET_DIR"

# Copy all Python files
echo "Copying Python files..."
cp "$SOURCE_DIR/__init__.py" "$TARGET_DIR/"
cp "$SOURCE_DIR/LooperControlSurface.py" "$TARGET_DIR/"
cp "$SOURCE_DIR/LooperDiscovery.py" "$TARGET_DIR/"
cp "$SOURCE_DIR/BridgeServer.py" "$TARGET_DIR/"
cp "$SOURCE_DIR/LiveAPIWrapper.py" "$TARGET_DIR/"

# Copy python-osc library (bundled source - Ableton's embedded Python doesn't support pip)
echo "Copying python-osc library..."
cp -r "$SOURCE_DIR/python_osc" "$TARGET_DIR/"

echo ""
echo "============================================"
echo "Installation complete!"
echo "============================================"
echo ""
echo "Remote Script installed to:"
echo "  $TARGET_DIR"
echo ""
echo "Next steps:"
echo "  1. Restart Ableton Live (mandatory)"
echo "  2. Open Live > Preferences > Link/Tempo/MIDI > Control Surfaces"
echo "  3. Select '$SCRIPT_NAME' from the dropdown"
echo "  4. Ensure the VST3 plugin is loaded on an audio track"
echo ""
echo "To uninstall, simply delete:"
echo "  $TARGET_DIR"
echo ""
