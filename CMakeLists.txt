cmake_minimum_required(VERSION 3.22)
project(LiveLooperController VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party/JUCE)

juce_add_plugin(LiveLooperController
    VERSION 0.1.0
    COMPANY_NAME "LiveLooperController"
    PLUGIN_MANUFACTURER_CODE Llct
    PLUGIN_CODE Llcr
    FORMATS VST3
    PRODUCT_NAME "Live Looper Controller"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES "Fx" "Instrument"
)

target_sources(LiveLooperController PRIVATE
    src/Plugin/PluginProcessor.cpp
    src/Plugin/PluginEditor.cpp
    src/Model/LooperState.h
    src/Model/LooperState.cpp
    src/Model/LooperTracker.cpp
    src/Bridge/BridgeClient.cpp
    src/Bridge/MessageProtocol.cpp
    src/Shared/ProtocolDefs.cpp
    # UI components (per plan 02-02)
    src/UI/TransportButton.cpp
    src/UI/StatusHeaderBar.cpp
    src/UI/ExpandableDetailSection.cpp
    src/UI/LooperTrackComponent.cpp
    src/UI/LooperListView.cpp
)

target_include_directories(LiveLooperController PRIVATE
    src
)

target_link_libraries(LiveLooperController PRIVATE
    juce::juce_audio_processors
    juce::juce_osc
    juce::juce_data_structures
    juce::juce_gui_basics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(LiveLooperController PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Do NOT link juce_dsp — no audio processing needed

add_subdirectory(tests)
