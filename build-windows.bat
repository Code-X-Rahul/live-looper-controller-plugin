@echo off
REM Build Live Looper Controller for Windows
REM Run this from Developer Command Prompt for VS 2022

echo Building Live Looper Controller...
echo.

REM Configure CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build complete!
echo.
echo VST3 plugin location:
echo   build\LiveLooperController_artefacts\Release\VST3\Live Looper Controller.vst3
echo.
echo Copy this folder to:
echo   C:\Program Files\Common Files\VST3\
echo.
pause