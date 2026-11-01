cmake_minimum_required(VERSION 3.22)

project(DSBeatMaster VERSION 1.0.0)

# JUCE from local checkout
add_subdirectory(juce)

juce_add_plugin(DSBeatMaster
    COMPANY_NAME "DS BEAT"
    BUNDLE_ID com.dsbeat.dsbeatmaster
    PLUGIN_MANUFACTURER_CODE Dsbt
    PLUGIN_CODE Mstr
    FORMATS VST3 Standalone
    PRODUCT_NAME "DSBeatMaster"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Fx" "Mastering"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)

juce_generate_juce_header(DSBeatMaster)

target_sources(DSBeatMaster PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/MasterEQ.cpp
    Source/DSP/Crossover3.cpp
    Source/DSP/MultibandComp.cpp
    Source/DSP/StereoImager.cpp
    Source/DSP/Saturator.cpp
    Source/DSP/BrickwallLimiter.cpp
    Source/DSP/LufsMeter.cpp
    Source/UI/DSBeatLookAndFeel.cpp
    Source/UI/EQCurveDisplay.cpp
    Source/UI/MeterBridge.cpp
)

target_compile_definitions(DSBeatMaster
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DSBeatMaster
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

#==============================================================================
# DSBeatGainStage — pre-mastering gain staging tool
#==============================================================================
juce_add_plugin(DSBeatGainStage
    COMPANY_NAME "DS BEAT"
    BUNDLE_ID com.dsbeat.dsbeatgainstage
    PLUGIN_MANUFACTURER_CODE Dsbt
    PLUGIN_CODE GnSt
    FORMATS VST3 Standalone
    PRODUCT_NAME "DSBeatGainStage"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Fx" "Tools"
)

juce_generate_juce_header(DSBeatGainStage)

target_sources(DSBeatGainStage PRIVATE
    Source/GainStage/GainStageProcessor.cpp
    Source/GainStage/GainStageEditor.cpp
    Source/DSP/LufsMeter.cpp
    Source/UI/DSBeatLookAndFeel.cpp
)

target_compile_definitions(DSBeatGainStage
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DSBeatGainStage
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

#==============================================================================
# MinGW cross-compile: MSVC auto-links Windows system libs via
# #pragma comment(lib), which MinGW's linker ignores. Link them explicitly.
#==============================================================================
if(MINGW)
    # WindowsGUIDs.cpp defines CLSID_D2D1Opacity and CLSID_SpVoice, which are
    # declared in MinGW headers but missing from its import libs. MSVC's SDK
    # has them, so this file is MinGW-only (would cause duplicate symbols).
    target_sources(DSBeatMaster PRIVATE Source/WindowsGUIDs.cpp)
    target_sources(DSBeatGainStage PRIVATE Source/WindowsGUIDs.cpp)

    set(DSBEAT_WIN32_LIBS
        d2d1 d3d11 dcomp dxgi dwrite dxguid
        opengl32 glu32
        shell32 comctl32 dwmapi imm32 shcore shlwapi vfw32 version
        wininet ws2_32 winmm dbghelp uuid ole32 oleaut32
        ksuser cfgmgr32
    )
    target_link_libraries(DSBeatMaster PRIVATE ${DSBEAT_WIN32_LIBS})
    target_link_libraries(DSBeatGainStage PRIVATE ${DSBEAT_WIN32_LIBS})

    # Windows application manifest: declares dependency on
    # Microsoft.Windows.Common-Controls v6 so the loader binds comctl32 v6
    # (which exports TaskDialogIndirect) instead of v5. Without this, the
    # standalone .exe fails at startup with "TaskDialogIndirect entry point
    # not found". Applied to Standalone targets (the .exe); harmless for VST3.
    target_sources(DSBeatMaster_Standalone PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/windows-manifest.rc"
    )
    target_sources(DSBeatMaster_VST3 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/windows-manifest.rc"
    )
    target_sources(DSBeatGainStage_Standalone PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/windows-manifest.rc"
    )
    target_sources(DSBeatGainStage_VST3 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/windows-manifest.rc"
    )
endif()
