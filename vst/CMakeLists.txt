cmake_minimum_required(VERSION 3.16)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
endif()

if(UNIX AND NOT APPLE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

project(OBSIDIAN-Neural VERSION 1.0.3)

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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(JUCE QUIET)
if(NOT JUCE_FOUND)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.8  
    )
    FetchContent_MakeAvailable(JUCE)
endif()

find_package(SoundTouch QUIET)
if(NOT SoundTouch_FOUND)
    include(FetchContent)
    FetchContent_Declare(soundtouch
        GIT_REPOSITORY https://codeberg.org/soundtouch/soundtouch.git
        GIT_TAG 2.4.0
    )
    FetchContent_MakeAvailable(soundtouch)
endif()

find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

string(TIMESTAMP BUILD_NUMBER "%Y%m%d_%H%M") 
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
    @ONLY
)

juce_add_plugin(ObsidianNeuralVST
    VERSION "1.0.3"
    PLUGIN_MANUFACTURER_CODE "OBSI"
    PLUGIN_CODE "Obsi"
    COMPANY_NAME "InnerMost47"
    BUNDLE_ID "com.innermost47.obsidian-neural-vst" 
    FORMATS VST3
    PRODUCT_NAME "OBSIDIAN-Neural"
    IS_SYNTH TRUE
    WANTS_MIDI_INPUT TRUE
    PRODUCES_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_CURL TRUE
)

target_sources(ObsidianNeuralVST PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/BinaryData.cpp  
    src/PluginEntry.cpp
    src/MidiLearnManager.cpp
    src/MixerChannel.cpp
    src/TrackComponent.cpp
    src/MasterChannel.cpp
    src/WaveformDisplay.cpp
    src/SequencerComponent.cpp
    src/ColourPalette.cpp
    src/MixerPanel.cpp
    src/StableAudioEngine.cpp
    src/SampleBank.cpp
    src/SampleBankPanel.cpp
    src/CategoryWindow.cpp
    src/MidiMappingEditorWindow.cpp
)

target_include_directories(ObsidianNeuralVST PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    src
    ${soundtouch_SOURCE_DIR}/include 
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
        
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ObsidianNeuralVST PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    SoundTouch
    nlohmann_json::nlohmann_json
    
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
        -Wno-macro-redefined
        -Wno-builtin-macro-redefined
    )
    target_link_libraries(ObsidianNeuralVST PRIVATE 
        pthread 
        dl 
        asound  
    )
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    target_link_libraries(ObsidianNeuralVST PRIVATE ${GTK3_LIBRARIES})
    target_include_directories(ObsidianNeuralVST PRIVATE ${GTK3_INCLUDE_DIRS})
endif()

message(STATUS "OBSIDIAN Neural Build Configuration:")
message(STATUS "    Build Number: ${BUILD_NUMBER}")