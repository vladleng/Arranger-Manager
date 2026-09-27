cmake_minimum_required(VERSION 3.22)
project(ArrangerManager VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 9.0.2 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
FetchContent_Declare(ARA_SDK GIT_REPOSITORY https://github.com/Celemony/ARA_SDK.git
    GIT_TAG releases/2.3.0 GIT_SUBMODULES_RECURSE TRUE)
FetchContent_GetProperties(ARA_SDK)
if(NOT ara_sdk_POPULATED)
    FetchContent_Populate(ARA_SDK)
endif()
juce_set_ara_sdk_path("${ara_sdk_SOURCE_DIR}")

juce_add_plugin(ArrangerManagerInspector
    COMPANY_NAME "Moon River Studio"
    BUNDLE_ID "com.moonriverstudio.arrangermanagerinspector"
    PLUGIN_MANUFACTURER_CODE MNRV
    PLUGIN_CODE AmIn
    FORMATS VST3
    PRODUCT_NAME "Arranger Manager Inspector"
    IS_SYNTH FALSE
    VST3_CATEGORIES Fx
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    IS_ARA_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(ArrangerManagerInspector)
target_sources(ArrangerManagerInspector PRIVATE
    src/ArrangementTag.h
    src/InspectorState.h src/SharedArrangementMap.h
    src/InspectorDocumentController.h src/InspectorDocumentController.cpp
    src/PluginProcessor.h src/PluginProcessor.cpp
    src/PluginEditor.h src/PluginEditor.cpp)
target_compile_definitions(ArrangerManagerInspector PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
if(MSVC)
    target_compile_options(ArrangerManagerInspector PRIVATE /utf-8)
endif()
target_link_libraries(ArrangerManagerInspector PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

juce_add_plugin(ArrangerManagerHub
    COMPANY_NAME "Moon River Studio"
    BUNDLE_ID "com.moonriverstudio.arrangermanagerhub"
    PLUGIN_MANUFACTURER_CODE MNRV
    PLUGIN_CODE AmHb
    FORMATS VST3
    PRODUCT_NAME "Arranger Manager Hub"
    IS_SYNTH FALSE
    VST3_CATEGORIES Fx
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    IS_ARA_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(ArrangerManagerHub)
target_sources(ArrangerManagerHub PRIVATE
    src/ArrangementTag.h src/SharedArrangementMap.h
    src/HubProcessor.h src/HubProcessor.cpp
    src/HubEditor.h src/HubEditor.cpp)
target_compile_definitions(ArrangerManagerHub PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
if(MSVC)
    target_compile_options(ArrangerManagerHub PRIVATE /utf-8)
endif()
target_link_libraries(ArrangerManagerHub PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

include(CTest)
add_executable(ArrangementTagTests tests/ArrangementTagTests.cpp)
target_include_directories(ArrangementTagTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(ArrangementTagTests PRIVATE cxx_std_20)
add_test(NAME ArrangementTagTests COMMAND ArrangementTagTests)
