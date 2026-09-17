cmake_minimum_required(VERSION 3.16)

if(APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for macOS")
endif()

set(CMAKE_POLICY_VERSION_MINIMUM 3.5) 

if(UNIX AND NOT APPLE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(PROJECT_VENDOR "InnerMost47")

project(OBSIDIAN-Neural VERSION 3.2.0)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ Standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE AND CMAKE_OSX_ARCHITECTURES MATCHES "x86_64" AND CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
    set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch x86_64 -arch arm64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch x86_64 -arch arm64")
endif()

if(WIN32)
    set(ORT_LIB_NAME "onnxruntime.dll")
    set(ORT_IMPORT_LIB "onnxruntime.lib")
elseif(APPLE)
    set(ORT_LIB_NAME "libonnxruntime.dylib")
else()
    set(ORT_LIB_NAME "libonnxruntime.so")
endif()

set(ORT_VERSION "1.26.0")
set(ORT_BASE_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VERSION}")
set(ORT_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/ort_download")
set(ORT_EXTRACT_DIR "${CMAKE_BINARY_DIR}/ort_extracted")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|ARM64")
    set(ORT_ARCH "arm64")
else()
    set(ORT_ARCH "x64")
endif()

if(WIN32)
    set(ORT_ARCHIVE "onnxruntime-win-${ORT_ARCH}-${ORT_VERSION}.zip")
elseif(APPLE)
    if(ORT_ARCH STREQUAL "arm64")
        set(ORT_ARCHIVE "onnxruntime-osx-arm64-${ORT_VERSION}.tgz")
    else()
        set(ORT_ARCHIVE "onnxruntime-osx-x86_64-${ORT_VERSION}.tgz")
    endif()
else()
    set(ORT_ARCHIVE "onnxruntime-linux-${ORT_ARCH}-${ORT_VERSION}.tgz")
endif()

set(ORT_ARCHIVE_PATH "${ORT_DOWNLOAD_DIR}/${ORT_ARCHIVE}")

if(NOT EXISTS "${ORT_ARCHIVE_PATH}")
    message(STATUS "[ORT] Downloading: ${ORT_ARCHIVE}...")
    file(DOWNLOAD
        "${ORT_BASE_URL}/${ORT_ARCHIVE}"
        "${ORT_ARCHIVE_PATH}"
        SHOW_PROGRESS
        STATUS ORT_DOWNLOAD_STATUS
    )
    list(GET ORT_DOWNLOAD_STATUS 0 ORT_DOWNLOAD_CODE)
    if(NOT ORT_DOWNLOAD_CODE EQUAL 0)
        file(REMOVE "${ORT_ARCHIVE_PATH}")
        message(FATAL_ERROR "[ORT] Download failed: ${ORT_DOWNLOAD_STATUS}")
    endif()
endif()

if(NOT EXISTS "${ORT_EXTRACT_DIR}")
    file(ARCHIVE_EXTRACT INPUT "${ORT_ARCHIVE_PATH}" DESTINATION "${ORT_EXTRACT_DIR}")
endif()

file(GLOB ORT_SUBDIRS "${ORT_EXTRACT_DIR}/onnxruntime-*")
list(GET ORT_SUBDIRS 0 ORT_ROOT)
message(STATUS "[ORT] Root: ${ORT_ROOT}")

add_library(onnxruntime SHARED IMPORTED GLOBAL)
if(WIN32)
    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION             "${ORT_ROOT}/lib/${ORT_LIB_NAME}"
        IMPORTED_IMPLIB               "${ORT_ROOT}/lib/${ORT_IMPORT_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${ORT_ROOT}/include"
    )
else()
    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION             "${ORT_ROOT}/lib/${ORT_LIB_NAME}"
        INTERFACE_INCLUDE_DIRECTORIES "${ORT_ROOT}/include"
    )
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LINK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LINK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(MSGPACK_CXX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MSGPACK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MSGPACK_INSTALL OFF CACHE BOOL "" FORCE)

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /Ot /GL /Gy")
    
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF /DEBUG:NONE")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF /DEBUG:NONE")
    
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/RTCu" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    
    add_compile_options(
        /fp:fast     
        /Qpar          
        /arch:AVX2  
        /WX-
        /w14265
    )
    
    message(STATUS "MSVC optimizations and maximum Stripping enabled for Release builds")

    string(REPLACE "/Zi" "/ZI" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    add_compile_options($<$<CONFIG:Debug>:/ZI>)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /EDITANDCONTINUE")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -fvisibility=hidden -fvisibility-inlines-hidden")

    if(APPLE)
        add_compile_options(
            -fvectorize
            -fslp-vectorize
        )
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -Wl,-x -Wl,-dead_strip")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Wl,-x -Wl,-dead_strip")
        message(STATUS "Universal macOS build: Clang optimizations & Symbol Stripping enabled")
    endif()
    
    add_compile_options(
        -ffast-math
        -fno-finite-math-only  
    )
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -fvisibility=hidden -fvisibility-inlines-hidden")

    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -s")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")

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

option(OBSIDIAN_ASAN "Build with AddressSanitizer" OFF)

if(MSVC AND OBSIDIAN_ASAN)
    add_compile_options(/fsanitize=address /Zi)
    string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/EDITANDCONTINUE" "" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
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

find_package(signalsmith QUIET)
if(NOT signalsmith_dsp_FOUND)
    include(FetchContent)
    FetchContent_Declare(signalsmith_dsp
        GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
        GIT_TAG 1.1.0
        GIT_SHALLOW TRUE     
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(signalsmith_dsp)
endif()

FetchContent_Declare(
    tokenizers_cpp
    GIT_REPOSITORY https://github.com/mlc-ai/tokenizers-cpp.git
    GIT_TAG        main
    GIT_SUBMODULES_RECURSE TRUE
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(tokenizers_cpp)

FetchContent_Declare(
    libsamplerate
    GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
    GIT_TAG        0.2.2
    GIT_SHALLOW    TRUE
)
set(LIBSAMPLERATE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIBSAMPLERATE_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libsamplerate)

if(DEFINED GITHUB_BUILD_TIMESTAMP)
    set(BUILD_TIMESTAMP "${GITHUB_BUILD_TIMESTAMP}")
else()
    string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%dT%H-%M-%S")
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
    VERSION ${PROJECT_VERSION}
    PLUGIN_MANUFACTURER_CODE "OBSI"
    PLUGIN_CODE "Obsi"
    COMPANY_NAME ${PROJECT_VENDOR}
    BUNDLE_ID "com.innermost47.obsidian-neural-vst" 
    FORMATS VST3 AU Standalone
    PRODUCT_NAME ${PROJECT_NAME}
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/logo.png" 
    IS_SYNTH TRUE
    WANTS_MIDI_INPUT TRUE
    PRODUCES_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_CURL TRUE
    VST3_AUTO_MANIFEST FALSE
)

foreach(target_format VST3 AU Standalone)
    if(TARGET ObsidianNeuralVST_${target_format})
        if(WIN32)
            add_custom_command(TARGET ObsidianNeuralVST_${target_format} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${ORT_ROOT}/lib/onnxruntime.dll"
                    "$<TARGET_FILE_DIR:ObsidianNeuralVST_${target_format}>"
            )
        elseif(APPLE)
            set(_ort_fwk_dir "$<TARGET_FILE_DIR:ObsidianNeuralVST_${target_format}>/../Frameworks")
            set_target_properties(ObsidianNeuralVST_${target_format} PROPERTIES
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH "@loader_path/../Frameworks"
            )
            add_custom_command(TARGET ObsidianNeuralVST_${target_format} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${_ort_fwk_dir}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${ORT_ROOT}/lib/libonnxruntime.${ORT_VERSION}.dylib"
                    "${_ort_fwk_dir}/libonnxruntime.1.dylib"
                COMMAND install_name_tool -id "@rpath/libonnxruntime.1.dylib"
                    "${_ort_fwk_dir}/libonnxruntime.1.dylib"
                COMMAND install_name_tool -change
                    "@rpath/libonnxruntime.${ORT_VERSION}.dylib"
                    "@rpath/libonnxruntime.1.dylib"
                    "$<TARGET_FILE:ObsidianNeuralVST_${target_format}>" || true
                COMMAND install_name_tool -change
                    "@rpath/libonnxruntime.1.dylib"
                    "@rpath/libonnxruntime.1.dylib"
                    "$<TARGET_FILE:ObsidianNeuralVST_${target_format}>" || true
                VERBATIM
            )
        elseif(UNIX)
            add_custom_command(TARGET ObsidianNeuralVST_${target_format} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${ORT_ROOT}/lib/libonnxruntime.so.${ORT_VERSION}"
                    "$<TARGET_FILE_DIR:ObsidianNeuralVST_${target_format}>"
            )
        endif()
    endif()
endforeach()

juce_add_binary_data(ObsidianBinaryData
    SOURCES
        src/assets/fonts/michroma.ttf
        src/assets/fonts/noto-bold.ttf
        src/assets/fonts/noto-italic.ttf
        src/assets/fonts/noto-regular.ttf
        src/assets/icons/activity.svg
        src/assets/icons/arrowsvertical.svg
        src/assets/icons/anchor.svg
        src/assets/icons/branch.svg
        src/assets/icons/broadcast.svg
        src/assets/icons/brush.svg
        src/assets/icons/bucket.svg
        src/assets/icons/caret-down.svg
        src/assets/icons/caret-left.svg
        src/assets/icons/caret-right.svg
        src/assets/icons/caret-up.svg
        src/assets/icons/cassette-tape.svg
        src/assets/icons/chat-text.svg
        src/assets/icons/collapse.svg
        src/assets/icons/cpu.svg
        src/assets/icons/cpuregular.svg
        src/assets/icons/dice.svg
        src/assets/icons/dj.svg
        src/assets/icons/download.svg
        src/assets/icons/eq.svg
        src/assets/icons/eraser.svg
        src/assets/icons/expand.svg
        src/assets/icons/export.svg
        src/assets/icons/file-audio.svg
        src/assets/icons/folder-closed.svg
        src/assets/icons/folder-open.svg
        src/assets/icons/folder-plus.svg
        src/assets/icons/gear.svg
        src/assets/icons/grid.svg
        src/assets/icons/hand-grabbing.svg
        src/assets/icons/headphones.svg
        src/assets/icons/info.svg
        src/assets/icons/info-fill.svg
        src/assets/icons/left.svg
        src/assets/icons/lin.svg
        src/assets/icons/lock-fill.svg
        src/assets/icons/map.svg
        src/assets/icons/music-notes.svg
        src/assets/icons/mute.svg
        src/assets/icons/pause.svg
        src/assets/icons/pencil.svg
        src/assets/icons/piano.svg
        src/assets/icons/play.svg
        src/assets/icons/plus.svg
        src/assets/icons/power.svg
        src/assets/icons/power-fill.svg
        src/assets/icons/prohibit.svg
        src/assets/icons/radioactive.svg
        src/assets/icons/redo.svg
        src/assets/icons/refresh.svg
        src/assets/icons/repeat.svg
        src/assets/icons/rewind.svg
        src/assets/icons/right.svg
        src/assets/icons/robotfill.svg
        src/assets/icons/robotregular.svg
        src/assets/icons/save.svg
        src/assets/icons/search.svg
        src/assets/icons/shuffle.svg
        src/assets/icons/sliders.svg
        src/assets/icons/square.svg
        src/assets/icons/trash.svg
        src/assets/icons/tree-structure.svg
        src/assets/icons/undo.svg
        src/assets/icons/upload.svg
        src/assets/icons/volume.svg
        src/assets/icons/warning.svg
        src/assets/icons/waveform.svg
        src/assets/icons/wave-sine.svg
        src/assets/icons/wind.svg
        src/assets/icons/x.svg
        src/assets/icons/zap.svg
        src/assets/jucelogo.png
        src/assets/logo.png  
        src/assets/sailogo.png
)

target_sources(ObsidianNeuralVST PRIVATE
    src/components/center/mixer/CrossfaderComponent.cpp
    src/components/center/mixer/MixerChannel.cpp
    src/components/center/mixer/MixerPanel.cpp
    src/components/center/tracks/SequencerComponent.cpp
    src/components/center/tracks/TrackComponent.cpp
    src/components/center/tracks/WaveformDisplay.cpp
    src/components/left/BasePanel.cpp
    src/components/left/LeftPanelWrapper.cpp
    src/components/left/promptPanel/PromptBank.cpp
    src/components/left/promptPanel/PromptBankItem.cpp
    src/components/left/promptPanel/PromptBankPanel.cpp
    src/components/left/promptPanel/PromptCategoryAccordion.cpp
    src/components/left/promptPanel/PromptModelDefinitions.cpp
    src/components/left/samplePanel/CategoryPanel.cpp
    src/components/left/samplePanel/DetailPanel.cpp
    src/components/left/samplePanel/SampleBank.cpp
    src/components/left/samplePanel/SampleBankItem.cpp
    src/components/left/samplePanel/SampleBankPanel.cpp
    src/components/onboarding/ConceptRowComponent.cpp 
    src/components/onboarding/OnboardingStep.cpp
    src/components/onboarding/OnboardingFlow.cpp
    src/components/right/ConfigComponent.cpp
    src/components/right/fx/BitCrusherComponent.cpp
    src/components/right/fx/ChorusComponent.cpp
    src/components/right/fx/CompressorComponent.cpp
    src/components/right/fx/DistortionComponent.cpp
    src/components/right/fx/EqualizerComponent.cpp
    src/components/right/fx/FilterComponent.cpp
    src/components/right/fx/FlangerComponent.cpp
    src/components/right/fx/GateComponent.cpp
    src/components/right/fx/LimiterComponent.cpp
    src/components/right/fx/PhaserComponent.cpp
    src/components/right/fx/SendsPanel.cpp
    src/components/right/fx/TrackEffectsPanel.cpp
    src/components/right/glitch/GlitchLegend.cpp
    src/components/right/glitch/GlitchSequencerPanel.cpp
    src/components/right/glitch/GlitchStepButton.cpp
    src/components/right/glitch/GlitchStepGrid.cpp
    src/components/right/glitch/GlitchSequenceRow.cpp
    src/components/right/glitch/GlitchMetaSequenceRow.cpp
    src/components/right/glitch/GlitchMetaStepButton.cpp
    src/components/right/mod/ModulationPanel.cpp
    src/components/right/mod/ModulatorRow.cpp
    src/components/right/LCDScreen.cpp
    src/components/right/MasterChannel.cpp
    src/components/right/RightPanelWrapper.cpp
    src/components/right/ScaleAndDuration.cpp
    src/components/right/StandaloneTransportComponent.cpp
    src/components/right/TrackRecapPanel.cpp
    src/components/right/TrackSelectorBar.cpp
    src/components/shared/accordion/ObsidianAccordion.cpp
    src/components/shared/accordion/ObsidianListItem.cpp
    src/components/shared/bank/BankCategoryOperations.cpp
    src/components/shared/bank/ObsidianBankHeader.cpp
    src/components/shared/button/IconButton.cpp
    src/components/shared/button/LedRadioButton.cpp
    src/components/shared/ColourPicker.cpp
    src/components/shared/midi/ObsidianBaseMidiComponent.cpp
    src/components/shared/modal/ObsidianModal.cpp
    src/components/shared/ObsidianBase.cpp
    src/components/shared/VuMeter.cpp
    src/data/GlitchPresets.cpp
    src/data/TrackData.cpp
    src/data/ModulationTypes.cpp
    src/dsp/airwindows/Console6Bus.cpp
    src/dsp/airwindows/Console6Channel.cpp
    src/dsp/BitCrusher.cpp
    src/dsp/Chorus.cpp
    src/dsp/Compressor.cpp
    src/dsp/DelaySend.cpp
    src/dsp/Distortion.cpp
    src/dsp/Equalizer.cpp
    src/dsp/Filter.cpp
    src/dsp/Flanger.cpp
    src/dsp/Gate.cpp
    src/dsp/Limiter.cpp
    src/dsp/BeatDetektor.cpp
    src/dsp/MonoEQ.cpp
    src/dsp/Phaser.cpp
    src/dsp/ReverbSend.cpp
    src/dsp/SimpleEQ.cpp
    src/engines/DjIaClient.cpp
    src/managers/core/AssetDownloadManager.cpp
    src/managers/core/AudioManager.cpp
    src/managers/core/BeatDetektor.cpp
    src/managers/core/GenerationManager.cpp
    src/managers/core/GlitchSequencerEngine.cpp
    src/managers/core/LocalGenerationEngine.cpp
    src/managers/core/ModulationEngine.cpp
    src/managers/core/ParameterManager.cpp
    src/managers/core/SequencerManager.cpp
    src/managers/core/StateManager.cpp
    src/managers/core/TrackManager.cpp
    src/managers/ui/AssetDownloadContent.cpp
    src/managers/ui/LicenseAgreementContent.cpp
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
    src/sa3/SA3PipeLine.cpp
    src/sa3/SA3Tokenizer.cpp
    src/standalone/core/StandaloneTransport.cpp
    src/standalone/ObsidianStandaloneApp.cpp
    src/standalone/SplashScreen.cpp
    src/style/ColourPalette.cpp
    src/style/CustomLookAndFeel.cpp
)

target_include_directories(ObsidianNeuralVST PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    src
    src/components
    src/components/center/mixer
    src/components/center/tracks
    src/components/left
    src/components/left/promptPanel
    src/components/left/samplePanel
    src/components/onboarding
    src/components/right
    src/components/right/fx
    src/components/right/glitch
    src/components/right/mod
    src/components/shared
    src/components/shared/accordion
    src/components/shared/bank
    src/components/shared/button
    src/components/shared/midi
    src/components/shared/modal
    src/config
    src/data
    src/dsp
    src/dsp/airwindows
    src/engines
    src/managers/core
    src/managers/ui
    src/midi
    src/sa3
    src/standalone
    src/standalone/core
    src/standalone/transport/ui
    src/style
)

target_include_directories(ObsidianNeuralVST SYSTEM PRIVATE
    ${signalsmith_dsp_SOURCE_DIR}
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
        JUCE_USE_WIN_ACCESSIBILITY_BACKEND=0
        JUCE_DISABLE_JUCE_VERSION_PRINTING=1
        JUCE_COREGRAPHICS_RENDER_WITH_MULTIPLE_PAINT_CALLS=0
        ORT_VERSION_STR="${ORT_VERSION}"
        
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
        juce::juce_cryptography
        nlohmann_json::nlohmann_json
        Ableton::Link
        onnxruntime
        tokenizers_cpp
        SampleRate::samplerate
    
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(WIN32)
    target_link_libraries(ObsidianNeuralVST PRIVATE ws2_32 winmm ntdll bcrypt userenv)
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
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0"
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

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR ${PROJECT_VENDOR})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

if(WIN32)
    set(CPACK_MONOLITHIC_INSTALL ON)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "OBSIDIAN-Neural")

    install(TARGETS ObsidianNeuralVST_Standalone DESTINATION "Standalone")
    install(FILES "${ORT_ROOT}/lib/onnxruntime.dll" DESTINATION "Standalone")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt" DESTINATION ".")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/THIRD-PARTY-LICENSES.txt" DESTINATION ".")

    set(VST3_BUILD_PATH "${CMAKE_BINARY_DIR}/ObsidianNeuralVST_artefacts/Release/VST3/OBSIDIAN-Neural.vst3")
    install(DIRECTORY "${VST3_BUILD_PATH}" DESTINATION "VST3")

    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        RMDir /r '$PROGRAMFILES64\\\\Common Files\\\\VST3\\\\OBSIDIAN-Neural.vst3'
        CreateDirectory '$PROGRAMFILES64\\\\Common Files\\\\VST3'
        CopyFiles '$INSTDIR\\\\VST3\\\\OBSIDIAN-Neural.vst3' '$PROGRAMFILES64\\\\Common Files\\\\VST3'
    ")

    set(CPACK_NSIS_CREATE_ICONS_EXTRA "
        CreateShortCut '$DESKTOP\\\\OBSIDIAN-Neural.lnk' '$INSTDIR\\\\Standalone\\\\OBSIDIAN-Neural.exe'
    ")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA "
        Delete '$DESKTOP\\\\OBSIDIAN-Neural.lnk'
    ")
    
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        RMDir /r '$PROGRAMFILES64\\\\Common Files\\\\VST3\\\\OBSIDIAN-Neural.vst3'
        MessageBox MB_YESNO|MB_ICONQUESTION 'Do you also want to delete your user data, presets, generated audios, local models, and sessions?' IDNO +2
        RMDir /r '$APPDATA\\\\OBSIDIAN-Neural'
    ")

elseif(APPLE)
    set(CPACK_GENERATOR "productbuild")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/")

    install(TARGETS ObsidianNeuralVST_VST3
        DESTINATION "Library/Audio/Plug-Ins/VST3"
        COMPONENT vst3)
    install(TARGETS ObsidianNeuralVST_AU
        DESTINATION "Library/Audio/Plug-Ins/Components"
        COMPONENT au)
    install(TARGETS ObsidianNeuralVST_Standalone
        DESTINATION "Applications"
        COMPONENT standalone)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
        DESTINATION "usr/local/share/OBSIDIAN-Neural" COMPONENT standalone)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/THIRD-PARTY-LICENSES.txt"
        DESTINATION "usr/local/share/OBSIDIAN-Neural" COMPONENT standalone)

    set(CPACK_COMPONENTS_ALL vst3 au standalone)

else()
    set(CPACK_MONOLITHIC_INSTALL ON)
    set(CPACK_GENERATOR "TGZ")
    install(TARGETS ObsidianNeuralVST_VST3
        DESTINATION "vst3")
    install(TARGETS ObsidianNeuralVST_Standalone
        DESTINATION "bin")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt" DESTINATION ".")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/THIRD-PARTY-LICENSES.txt" DESTINATION ".")
endif()

include(CPack)

message(STATUS "OBSIDIAN Neural Build Configuration:")
message(STATUS "    Build Number: ${BUILD_NUMBER}")