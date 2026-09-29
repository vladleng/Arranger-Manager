cmake_minimum_required(VERSION 3.22)
project(ArrangerManager VERSION 0.1.8 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 9.0.2 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
option(ARRANGER_BUILD_ARA_INSPECTOR "Build the legacy ARA feasibility plug-in" OFF)
if(ARRANGER_BUILD_ARA_INSPECTOR)
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
endif()

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
    src/ArrangementTag.h src/StudioProColour.h src/SongProgress.h src/SongSnapshot.h src/SongSnapshot.cpp
    src/HubProcessor.h src/HubProcessor.cpp
    src/HubEditor.h src/HubEditor.cpp)
target_compile_definitions(ArrangerManagerHub PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
if(MSVC)
    target_compile_options(ArrangerManagerHub PRIVATE /utf-8)
endif()
target_link_libraries(ArrangerManagerHub PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

juce_add_gui_app(ArrangerManagerDesktop
    COMPANY_NAME "Moon River Studio"
    BUNDLE_ID "com.moonriverstudio.arrangermanager"
    PRODUCT_NAME "Arranger Manager")
juce_generate_juce_header(ArrangerManagerDesktop)
target_sources(ArrangerManagerDesktop PRIVATE
    src/ArrangementTag.h src/StudioProColour.h src/SongProgress.h src/SongCatalog.h src/SongNoteKeys.h src/SongSnapshot.h src/SongSnapshot.cpp
    src/HubEditor.h src/HubEditor.cpp src/HubApplication.cpp)
target_compile_definitions(ArrangerManagerDesktop PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
if(MSVC)
    target_compile_options(ArrangerManagerDesktop PRIVATE /utf-8)
endif()
target_link_libraries(ArrangerManagerDesktop PRIVATE juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

include(CTest)
add_executable(ArrangementTagTests tests/ArrangementTagTests.cpp)
target_include_directories(ArrangementTagTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(ArrangementTagTests PRIVATE cxx_std_20)
add_test(NAME ArrangementTagTests COMMAND ArrangementTagTests)

add_executable(SongSnapshotTests tests/SongSnapshotTests.cpp src/SongSnapshot.cpp)
target_include_directories(SongSnapshotTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(SongSnapshotTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
if(MSVC)
    target_compile_options(SongSnapshotTests PRIVATE /utf-8)
endif()
target_link_libraries(SongSnapshotTests PRIVATE juce::juce_core
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_test(NAME SongSnapshotTests COMMAND SongSnapshotTests)

add_executable(SongCatalogTests tests/SongCatalogTests.cpp)
target_include_directories(SongCatalogTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(SongCatalogTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
if(MSVC)
    target_compile_options(SongCatalogTests PRIVATE /utf-8)
endif()
target_link_libraries(SongCatalogTests PRIVATE juce::juce_core
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_test(NAME SongCatalogTests COMMAND SongCatalogTests)
