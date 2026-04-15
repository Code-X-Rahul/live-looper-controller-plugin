cmake_minimum_required(VERSION 3.22)
project(LiveLooperControllerTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project root is one level up from tests/
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)

# Collect JUCE module sources needed for tests (juce_core only - no X11 dependency)
file(GLOB_RECURSE JUCE_CORE_SOURCES
    ${PROJECT_ROOT}/third_party/JUCE/modules/juce_core/juce_core.cpp
    ${PROJECT_ROOT}/third_party/JUCE/modules/juce_core/juce_core_CompilationTime.cpp
)

add_executable(LiveLooperControllerTests
    catch2_main.cpp
    TestMessageProtocol.cpp
    TestLooperState.cpp
    TestBridgeClient.cpp
    ${PROJECT_ROOT}/src/Bridge/MessageProtocol.cpp
    ${PROJECT_ROOT}/src/Shared/ProtocolDefs.cpp
    ${PROJECT_ROOT}/src/Model/LooperState.cpp
    ${PROJECT_ROOT}/src/Model/LooperTracker.cpp
    ${JUCE_CORE_SOURCES}
)

target_include_directories(LiveLooperControllerTests PRIVATE
    ${PROJECT_ROOT}/src
    ${PROJECT_ROOT}/third_party/JUCE/modules
)

target_compile_definitions(LiveLooperControllerTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_MODULE_AVAILABLE_juce_core=1
    JUCE_STANDALONE_APPLICATION=1
)

target_link_libraries(LiveLooperControllerTests PRIVATE
    Catch2::Catch2WithMain
    pthread
    stdc++fs
)

include(CTest)
include(Catch)
catch_discover_tests(LiveLooperControllerTests)