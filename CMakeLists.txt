cmake_minimum_required(VERSION 3.22)

# ==============================================================================
# iOS Architecture & Code Signing Defaults
# ==============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0" CACHE STRING "Minimum iOS deployment version" FORCE)
    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "MX8BPUQ84Q" CACHE STRING "Apple Development Team ID" FORCE)
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym" CACHE STRING "Debug info format" FORCE)
    set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES" CACHE STRING "Generate debug symbols" FORCE)
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS" FORCE)
endif()

project(JuceAppTemplate VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# JUCE Submodule
# ==============================================================================
add_subdirectory(JUCE)

# ==============================================================================
# Plugin Target Definition (Standalone + AUv3)
# ==============================================================================
juce_add_plugin(JuceAppTemplate
    COMPANY_NAME "Ca5"
    BUNDLE_ID "me.ca5.template"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    MICROPHONE_PERMISSION_ENABLED FALSE
    BACKGROUND_AUDIO_ENABLED TRUE
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon.png"
    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    PLUGIN_CODE "Jatp"
    MANUFACTURER_CODE "Ca5x"
    FORMATS AUv3 Standalone
    PRODUCT_NAME "JuceAppTemplate"
    PLIST_TO_MERGE "<plist><dict><key>ITSAppUsesNonExemptEncryption</key><false/></dict></plist>"
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(JuceAppTemplate)

# ==============================================================================
# C++ Source Files
# ==============================================================================
target_sources(JuceAppTemplate PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(JuceAppTemplate PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(JuceAppTemplate PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ==============================================================================
# React Web UI Build Integration
# ==============================================================================
find_program(NPM_EXECUTABLE npm)
if(NPM_EXECUTABLE)
    add_custom_target(BuildReactUI ALL
        COMMAND ${NPM_EXECUTABLE} run build
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/ui
        COMMENT "Building React Single-File Web UI..."
    )
    add_dependencies(JuceAppTemplate_Standalone BuildReactUI)
    if(TARGET JuceAppTemplate_AUv3)
        add_dependencies(JuceAppTemplate_AUv3 BuildReactUI)
    endif()
endif()

# ==============================================================================
# Asset Copy Rules (Standalone & AUv3 Plugin Bundle)
# ==============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(JuceAppTemplate JuceAppTemplate_Standalone PROPERTIES
        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
        XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone and iPad
    )

    add_custom_command(TARGET JuceAppTemplate_Standalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/ui/dist"
        "$<TARGET_FILE_DIR:JuceAppTemplate_Standalone>/ui/dist"
    )

    if(TARGET JuceAppTemplate_AUv3)
        set_target_properties(JuceAppTemplate_AUv3 PROPERTIES
            XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
            XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        )
        add_custom_command(TARGET JuceAppTemplate_AUv3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/ui/dist"
            "$<TARGET_FILE_DIR:JuceAppTemplate_AUv3>/ui/dist"
        )
    endif()
else()
    # macOS Desktop bundle structure
    add_custom_command(TARGET JuceAppTemplate_Standalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/ui/dist"
        "$<TARGET_FILE_DIR:JuceAppTemplate_Standalone>/../Resources/ui/dist"
    )
    if(TARGET JuceAppTemplate_AUv3)
        add_custom_command(TARGET JuceAppTemplate_AUv3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/ui/dist"
            "$<TARGET_FILE_DIR:JuceAppTemplate_AUv3>/../Resources/ui/dist"
        )
    endif()
endif()
