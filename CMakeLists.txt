cmake_minimum_required(VERSION 3.16)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
endif()

if(UNIX AND NOT APPLE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

project(OBSIDIAN-Neural VERSION 2.1.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LINK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LINK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /Ot /GL /GS-")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
    
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/RTCu" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    
    add_compile_options(
        /fp:fast     
        /Qpar          
        /arch:AVX2  
        /WX-
    )
    
    message(STATUS "MSVC optimizations enabled for Release builds")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops")

    if(APPLE)
        if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mcpu=apple-m1")
            message(STATUS "Optimizing for Apple Silicon")
        elseif(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=nehalem -mtune=haswell")
            message(STATUS "Optimizing for Intel Mac")
        else()
            message(STATUS "Universal or unknown architecture - using generic optimizations")
        endif()
        
        add_compile_options(
            -fvectorize
            -fslp-vectorize
        )
    endif()
    
    add_compile_options(
        -ffast-math
        -fno-finite-math-only  
    )
    
    message(STATUS "Clang optimizations enabled for Release builds")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops")

    add_compile_options(
        -ffast-math
        -ftree-vectorize
    )
    
    if(NOT DEFINED OBSIDIAN_USE_NATIVE_ARCH)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=x86-64-v2")
        message(STATUS "Using x86-64-v2 for broad compatibility (GitHub Actions safe)")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
        message(STATUS "Using -march=native (local build)")
    endif()
    
    message(STATUS "GCC optimizations enabled for Release builds")
endif()

find_package(JUCE QUIET)
if(NOT JUCE_FOUND)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.8 
        GIT_SHALLOW TRUE     
        GIT_PROGRESS TRUE  
    )
    FetchContent_MakeAvailable(JUCE)
endif()

find_package(SoundTouch QUIET)
if(NOT SoundTouch_FOUND)
    include(FetchContent)
    FetchContent_Declare(soundtouch
        GIT_REPOSITORY https://github.com/innermost47/soundtouch.git
        GIT_TAG 2.4.0
        GIT_SHALLOW TRUE     
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(soundtouch)
endif()

find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
        GIT_SHALLOW TRUE     
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(Ableton QUIET)
include(FetchContent)
FetchContent_Declare(
    AbletonLink
    GIT_REPOSITORY https://github.com/Ableton/link.git
    GIT_TAG Link-4.0.0b4
    GIT_SHALLOW TRUE     
    GIT_PROGRESS TRUE
)

FetchContent_GetProperties(AbletonLink)
if(NOT abletonlink_POPULATED)
    FetchContent_Populate(AbletonLink)
    include(${abletonlink_SOURCE_DIR}/AbletonLinkConfig.cmake)
endif()

if(DEFINED GITHUB_BUILD_NUMBER)
    set(BUILD_NUMBER "${GITHUB_BUILD_NUMBER}")
else()
    string(TIMESTAMP BUILD_NUMBER "%Y%m%d_%H%M")
endif()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config/version.h"
    @ONLY
)

juce_add_plugin(ObsidianNeuralVST
    VERSION "2.1.1"
    PLUGIN_MANUFACTURER_CODE "OBSI"
    PLUGIN_CODE "Obsi"
    COMPANY_NAME "InnerMost47"
    BUNDLE_ID "com.innermost47.obsidian-neural-vst" 
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "OBSIDIAN-Neural"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/logo.png" 
    IS_SYNTH TRUE
    WANTS_MIDI_INPUT TRUE
    PRODUCES_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_CURL TRUE
)

juce_add_binary_data(ObsidianBinaryData
    SOURCES
        src/assets/fonts/michroma.ttf
        src/assets/fonts/noto-bold.ttf
        src/assets/fonts/noto-italic.ttf
        src/assets/fonts/noto-regular.ttf
        src/assets/icons/activity.svg
        src/assets/icons/anchor.svg
        src/assets/icons/branch.svg
        src/assets/icons/broadcast.svg
        src/assets/icons/brush.svg
        src/assets/icons/bucket.svg
        src/assets/icons/caret-down.svg
        src/assets/icons/caret-up.svg
        src/assets/icons/chat-text.svg
        src/assets/icons/collapse.svg
        src/assets/icons/cpu.svg
        src/assets/icons/cpuregular.svg
        src/assets/icons/dj.svg
        src/assets/icons/download.svg
        src/assets/icons/eq.svg
        src/assets/icons/eraser.svg
        src/assets/icons/expand.svg
        src/assets/icons/file-audio.svg
        src/assets/icons/folder.svg
        src/assets/icons/gear.svg
        src/assets/icons/grid.svg
        src/assets/icons/headphones.svg
        src/assets/icons/info.svg
        src/assets/icons/left.svg
        src/assets/icons/lin.svg
        src/assets/icons/lock-fill.svg
        src/assets/icons/mute.svg
        src/assets/icons/pause.svg
        src/assets/icons/pencil.svg
        src/assets/icons/piano.svg
        src/assets/icons/play.svg
        src/assets/icons/plus.svg
        src/assets/icons/redo.svg
        src/assets/icons/refresh.svg
        src/assets/icons/repeat.svg
        src/assets/icons/right.svg
        src/assets/icons/robotfill.svg
        src/assets/icons/robotregular.svg
        src/assets/icons/save.svg
        src/assets/icons/search.svg
        src/assets/icons/shuffle.svg
        src/assets/icons/sliders.svg
        src/assets/icons/square.svg
        src/assets/icons/trash.svg
        src/assets/icons/undo.svg
        src/assets/icons/upload.svg
        src/assets/icons/volume.svg
        src/assets/icons/wind.svg
        src/assets/icons/x.svg
        src/assets/icons/zap.svg
        src/assets/logo.png   
)

target_sources(ObsidianNeuralVST PRIVATE
    src/components/center/mixer/CrossfaderComponent.cpp
    src/components/center/mixer/MixerChannel.cpp
    src/components/center/mixer/MixerPanel.cpp
    src/components/center/tracks/DrawingCanvas.cpp
    src/components/center/tracks/SequencerComponent.cpp
    src/components/center/tracks/TrackComponent.cpp
    src/components/center/tracks/WaveformDisplay.cpp
    src/components/left/CategoryPanel.cpp
    src/components/left/LeftPanelWrapper.cpp
    src/components/left/promptPanel/PromptBank.cpp
    src/components/left/promptPanel/PromptBankItem.cpp
    src/components/left/promptPanel/PromptBankPanel.cpp
    src/components/left/promptPanel/PromptCategoryAccordion.cpp
    src/components/left/promptPanel/PromptModelDefinitions.cpp
    src/components/left/samplePanel/DetailPanel.cpp
    src/components/left/samplePanel/SampleBank.cpp
    src/components/left/samplePanel/SampleBankItem.cpp
    src/components/left/samplePanel/SampleBankPanel.cpp
    src/components/right/ConfigComponent.cpp
    src/components/right/LCDScreen.cpp
    src/components/right/MasterChannel.cpp
    src/components/right/MasterWaveformDisplay.cpp
    src/components/right/RightPanelWrapper.cpp
    src/components/right/ScaleAndDuration.cpp
    src/components/right/SendsPanel.cpp
    src/components/right/StandaloneTransportComponent.cpp
    src/components/right/TrackEffectsPanel.cpp
    src/components/right/TrackRecapPanel.cpp
    src/components/shared/ColourPicker.cpp
    src/components/shared/IconButton.cpp
    src/components/shared/LedRadioButton.cpp
    src/components/shared/ObsidianAccordion.cpp
    src/components/shared/ObsidianBase.cpp
    src/components/shared/ObsidianBaseMidiComponent.cpp
    src/components/shared/ObsidianListItem.cpp
    src/components/shared/ObsidianModal.cpp
    src/components/shared/VuMeter.cpp
    src/dsp/airwindows/Console6Bus.cpp
    src/dsp/airwindows/Console6Channel.cpp
    src/dsp/AudioAnalyzer.cpp
    src/dsp/DelaySend.cpp
    src/dsp/ReverbSend.cpp
    src/dsp/SimpleEQ.cpp
    src/engines/DjIaClient.cpp
    src/engines/ObsidianEngine.cpp
    src/engines/StableAudioEngine.cpp
    src/managers/core/AudioManager.cpp
    src/managers/core/GenerationManager.cpp
    src/managers/core/ParameterManager.cpp
    src/managers/core/SequencerManager.cpp
    src/managers/core/StateManager.cpp
    src/managers/core/TrackManager.cpp
    src/managers/ui/ObsidianAlertManager.cpp
    src/managers/ui/UIGenerationManager.cpp
    src/managers/ui/UILayoutManager.cpp
    src/managers/ui/UIMidiManager.cpp
    src/managers/ui/UIModalManager.cpp
    src/managers/ui/UIPresetManager.cpp
    src/managers/ui/UIStatusManager.cpp
    src/managers/ui/UITrackManager.cpp
    src/midi/MidiLearnManager.cpp
    src/midi/MidiManager.cpp
    src/midi/MidiMappingEditorWindow.cpp
    src/PluginEditor.cpp
    src/PluginEntry.cpp
    src/PluginProcessor.cpp
    src/standalone/core/StandaloneTransport.cpp
    src/standalone/ObsidianStandaloneApp.cpp
    src/style/ColourPalette.cpp
    src/style/CustomLookAndFeel.cpp
)

target_include_directories(ObsidianNeuralVST PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${soundtouch_SOURCE_DIR}/include
    src
    src/components
    src/components/center/mixer
    src/components/center/tracks
    src/components/left
    src/components/left/promptPanel
    src/components/left/samplePanel
    src/components/right
    src/components/shared
    src/config
    src/data
    src/dsp
    src/dsp/airwindows
    src/engines
    src/managers/core
    src/managers/ui
    src/midi
    src/standalone
    src/standalone/core
    src/standalone/transport/ui
    src/style
)

target_compile_definitions(ObsidianNeuralVST 
    PRIVATE
        JucePlugin_IsSynth=1
        JucePlugin_WantsMidiInput=1
        JucePlugin_ProducesMidiOutput=1
        JucePlugin_IsMidiEffect=0
        JucePlugin_VSTNumMidiInputs=16
        OBSIDIAN_HAS_STABLE_AUDIO=1

        JUCE_ENABLE_DPI_AWARENESS=1     
        JUCE_DIRECTWRITE=1             
        JUCE_SUPPORT_RETINA=1           
        JUCE_USE_DIRECTWRITE=1
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        JUCE_WASAPI=1
        JUCE_WASAPI_EXCLUSIVE=1
        
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ObsidianNeuralVST 
    PRIVATE
        ObsidianBinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
        SoundTouch
        nlohmann_json::nlohmann_json
        Ableton::Link
    
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(WIN32)
    target_link_libraries(ObsidianNeuralVST PRIVATE ws2_32 winmm)
    target_compile_definitions(ObsidianNeuralVST PRIVATE
        JUCE_DPI_AWARENESS_CONTEXT=DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    )
elseif(APPLE)
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(COREMIDI_FRAMEWORK CoreMIDI)
    
    if(ACCELERATE_FRAMEWORK)
        target_link_libraries(ObsidianNeuralVST PRIVATE ${ACCELERATE_FRAMEWORK})
    endif()
    if(COREAUDIO_FRAMEWORK)
        target_link_libraries(ObsidianNeuralVST PRIVATE ${COREAUDIO_FRAMEWORK})
    endif()
    if(COREMIDI_FRAMEWORK)
        target_link_libraries(ObsidianNeuralVST PRIVATE ${COREMIDI_FRAMEWORK})
    endif()
    set_target_properties(ObsidianNeuralVST PROPERTIES
        OSX_ARCHITECTURES "x86_64;arm64"
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.15"
    )
elseif(UNIX AND NOT APPLE)
    target_compile_options(ObsidianNeuralVST PRIVATE -fPIC)
    target_compile_options(ObsidianNeuralVST PRIVATE 
        -fPIC
        -w 
    )
    target_link_libraries(ObsidianNeuralVST PRIVATE 
        pthread 
        dl 
        asound
        rt
        atomic 
    )
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    target_link_libraries(ObsidianNeuralVST PRIVATE ${GTK3_LIBRARIES})
    target_include_directories(ObsidianNeuralVST PRIVATE ${GTK3_INCLUDE_DIRS})
endif()

message(STATUS "OBSIDIAN Neural Build Configuration:")
message(STATUS "    Build Number: ${BUILD_NUMBER}")