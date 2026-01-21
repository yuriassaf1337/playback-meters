# CMakeList.txt : Playback-Meters - Audio Metering Application
cmake_minimum_required(VERSION 3.14)

project("playback-meters" VERSION 0.1.0 LANGUAGES C CXX)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

include(FetchContent)

# ==============================================================================
# Dependencies
# ==============================================================================

# Fetch GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Fetch Dear ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG        v1.91.5
)
FetchContent_MakeAvailable(imgui)

# Fetch KissFFT (source only, we compile it ourselves)
FetchContent_Declare(
  kissfft
  GIT_REPOSITORY https://github.com/mborgerding/kissfft
  GIT_TAG        131.1.0
)
FetchContent_GetProperties(kissfft)
if(NOT kissfft_POPULATED)
  FetchContent_Populate(kissfft)
endif()

# ==============================================================================
# ImGui Library
# ==============================================================================

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# ==============================================================================
# KissFFT Library (compiled directly from source)
# ==============================================================================

add_library(kissfft_lib STATIC
    ${kissfft_SOURCE_DIR}/kiss_fft.c
)
target_include_directories(kissfft_lib PUBLIC ${kissfft_SOURCE_DIR})
target_compile_definitions(kissfft_lib PUBLIC kiss_fft_scalar=float)

# ==============================================================================
# Playback-Meters Executable
# ==============================================================================

# Source files
set(PM_SOURCES
    # App
    playback-meters.cpp
    src/app/application.cpp
    
    # Audio
    src/audio/device_enumerator.cpp
    src/audio/audio_capture.cpp
    src/audio/audio_engine.cpp
    
    # DSP
    src/dsp/fft_processor.cpp
    src/dsp/loudness.cpp
    
    # GUI
    src/gui/meter_panel.cpp
    src/gui/layout_manager.cpp
    
    # Meters
    src/meters/oscilloscope.cpp
    src/meters/spectrum.cpp
    src/meters/spectrogram.cpp
    src/meters/loudness_meter.cpp
    src/meters/stereometer.cpp
    src/meters/vu_meter.cpp
    src/meters/waveform.cpp
)

set(PM_HEADERS
    # Common
    src/common/types.h
    
    # App
    src/app/application.h
    
    # Audio
    src/audio/device_enumerator.h
    src/audio/audio_capture.h
    src/audio/audio_engine.h
    
    # DSP
    src/dsp/ring_buffer.h
    src/dsp/fft_processor.h
    src/dsp/loudness.h
    
    # GUI
    src/gui/meter_panel.h
    src/gui/layout_manager.h
    
    # Meters
    src/meters/oscilloscope.h
    src/meters/spectrum.h
    src/meters/spectrogram.h
    src/meters/loudness_meter.h
    src/meters/stereometer.h
    src/meters/vu_meter.h
    src/meters/waveform.h
)

add_executable(playback-meters WIN32 ${PM_SOURCES} ${PM_HEADERS})

target_include_directories(playback-meters PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${kissfft_SOURCE_DIR}
)

# Link libraries
target_link_libraries(playback-meters PRIVATE
    imgui
    glfw
    kissfft_lib
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(playback-meters PRIVATE
        ole32
        winmm
        dwmapi
    )
endif()

# Copy assets to build directory
add_custom_command(TARGET playback-meters POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:playback-meters>/assets"
    COMMENT "Copying assets folder..."
)
