cmake_minimum_required(VERSION 3.22)
project(Revora VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime: users do not need to install the VC++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Revora
    COMPANY_NAME "REVORA"
    PLUGIN_MANUFACTURER_CODE Yrnm
    PLUGIN_CODE Rvra
    FORMATS VST3 Standalone
    PRODUCT_NAME "Revora"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/logo.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Assets/logo.png"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Revora PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PresetManager.cpp
    Source/PresetManager.h
    Source/PresetBar.h)

target_compile_definitions(Revora PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=1)

target_link_libraries(Revora
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags)
