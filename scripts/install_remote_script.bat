@echo off
REM Live Looper Controller - Remote Script Installer for Windows
REM
REM Usage: Double-click this file or run from Command Prompt:
REM   install_remote_script.bat
REM
REM This script copies the Remote Script files to Ableton Live's User Library
REM directory where Live can discover and load them as a Control Surface.
REM
REM After running this script:
REM 1. Restart Ableton Live (required for Live to discover the new script)
REM 2. Open Live Options > Preferences > Link/Tempo/MIDI > Control Surfaces
REM 3. Select "LooperControl" from the available Control Surfaces list

setlocal enabledelayedexpansion

set "SCRIPT_NAME=LooperControl"

REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
set "SOURCE_DIR=%SCRIPT_DIR%..\remote-script"

REM Windows path: %USERPROFILE%\Music\Ableton\User Library\Remote Scripts\
set "TARGET_DIR=%USERPROFILE%\Music\Ableton\User Library\Remote Scripts\%SCRIPT_NAME%"

echo ============================================
echo Live Looper Controller - Remote Script Installer
echo ============================================
echo.
echo Source: %SOURCE_DIR%
echo Target: %TARGET_DIR%
echo.

REM Check if source directory exists
if not exist "%SOURCE_DIR%" (
    echo ERROR: Source directory not found: %SOURCE_DIR%
    exit /b 1
)

REM Create parent directories if they don't exist
if not exist "%USERPROFILE%\Music\Ableton\User Library\Remote Scripts" (
    echo Creating Remote Scripts directory...
    mkdir "%USERPROFILE%\Music\Ableton\User Library\Remote Scripts" 2>nul
)

REM Remove existing installation if present
if exist "%TARGET_DIR%" (
    echo Removing existing installation...
    rmdir /s /q "%TARGET_DIR%"
)

REM Create target directory
echo Creating target directory...
mkdir "%TARGET_DIR%"

REM Copy all Python files
echo Copying Python files...
copy "%SOURCE_DIR%\__init__.py" "%TARGET_DIR%\" >nul
copy "%SOURCE_DIR%\LooperControlSurface.py" "%TARGET_DIR%\" >nul
copy "%SOURCE_DIR%\LooperDiscovery.py" "%TARGET_DIR%\" >nul
copy "%SOURCE_DIR%\BridgeServer.py" "%TARGET_DIR%\" >nul
copy "%SOURCE_DIR%\LiveAPIWrapper.py" "%TARGET_DIR%\" >nul

REM Copy python-osc library (bundled source - Ableton's embedded Python doesn't support pip)
echo Copying python-osc library...
xcopy /e /i /y "%SOURCE_DIR%\python_osc" "%TARGET_DIR%\python_osc\" >nul

echo.
echo ============================================
echo Installation complete!
echo ============================================
echo.
echo Remote Script installed to:
echo   %TARGET_DIR%
echo.
echo Next steps:
echo   1. Restart Ableton Live (mandatory)
echo   2. Open Live ^> Options ^> Preferences ^> Link/Tempo/MIDI ^> Control Surfaces
echo   3. Select '%SCRIPT_NAME%' from the dropdown
echo   4. Ensure the VST3 plugin is loaded on an audio track
echo.
echo To uninstall, simply delete:
echo   %TARGET_DIR%
echo.

endlocal
