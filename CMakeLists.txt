cmake_minimum_required(VERSION 3.20)
project(xs C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable FetchContent for downloading dependencies
include(FetchContent)

# Platform definitions
# PLATFORM_PC: Desktop platform (shared between Windows and Linux)
# PLATFORM_LINUX: Linux-specific features
add_definitions(-DPLATFORM_PC -DPLATFORM_LINUX -DINSPECTOR -DEDITOR)

# Core source files
set(CORE_SOURCES
    code/configuration.cpp
    code/data.cpp
    code/device.cpp
    code/packager.cpp
    code/fileio.cpp
    code/imgui_impl.cpp
    code/inspector.cpp
    code/log.cpp
    code/main.cpp
    code/profiler.cpp
    code/render.cpp
    code/script.cpp
    code/tools.cpp
    code/version.cpp
    code/xs.cpp
)

# SDL3 sources (shared across PC and Apple platforms)
set(SDL3_SOURCES
    code/sdl3/device_sdl.cpp
    code/sdl3/input_sdl.cpp
)

# Platform-specific files
# Use Linux-specific fileio for XDG paths
set(PLATFORM_SOURCES
    platforms/linux/code/fileio_linux.cpp
    platforms/pc/code/account_pc.cpp
)

# OpenGL rendering backend
set(OPENGL_SOURCES
    code/opengl/opengl.cpp
    code/opengl/render_gl.cpp
)

# Audio implementation (null for Linux until FMOD is available)
set(AUDIO_SOURCES
    platforms/null/code/audio_null.cpp
    platforms/null/code/simple_audio_null.cpp
)

# External: GLAD
set(GLAD_SOURCES
    external/glad/src/glad.c
)

# External: ImGui (with FreeType support)
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_impl_opengl3.cpp
    external/imgui/imgui_impl_sdl3.cpp
    external/imgui/imgui_stdlib.cpp
    external/imgui/misc/freetype/imgui_freetype.cpp
)

# External: ImPlot
set(IMPLOT_SOURCES
    external/implot/implot.cpp
    external/implot/implot_items.cpp
)

# External: Wren
file(GLOB WREN_SOURCES "external/wren/vm/*.c" "external/wren/optional/*.c")

# External: miniz
set(MINIZ_SOURCES
    external/miniz/src/miniz.c
    external/miniz/src/miniz_tdef.c
    external/miniz/src/miniz_tinfl.c
    external/miniz/src/miniz_zip.c
)

# Combine all sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${SDL3_SOURCES}
    ${PLATFORM_SOURCES}
    ${OPENGL_SOURCES}
    ${AUDIO_SOURCES}
    ${GLAD_SOURCES}
    ${IMGUI_SOURCES}
    ${IMPLOT_SOURCES}
    ${WREN_SOURCES}
    ${MINIZ_SOURCES}
)

# Create executable
add_executable(xs ${ALL_SOURCES})

# Include directories
target_include_directories(xs PRIVATE
    ${CMAKE_SOURCE_DIR}/code
    ${CMAKE_SOURCE_DIR}/code/sdl3
    ${CMAKE_SOURCE_DIR}/code/opengl
    ${CMAKE_SOURCE_DIR}/platforms/pc/code
    ${CMAKE_SOURCE_DIR}/platforms/linux/code
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/glad/include
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/implot
    ${CMAKE_SOURCE_DIR}/external/glm
    ${CMAKE_SOURCE_DIR}/external/fmt/include
    ${CMAKE_SOURCE_DIR}/external/json/include
    ${CMAKE_SOURCE_DIR}/external/stb
    ${CMAKE_SOURCE_DIR}/external/wren
    ${CMAKE_SOURCE_DIR}/external/wren/include
    ${CMAKE_SOURCE_DIR}/external/wren/vm
    ${CMAKE_SOURCE_DIR}/external/wren/optional
    ${CMAKE_SOURCE_DIR}/external/miniz
    ${CMAKE_SOURCE_DIR}/external/miniz/include/miniz
    ${CMAKE_SOURCE_DIR}/external/nanosvg/src
    ${CMAKE_SOURCE_DIR}/external/dialogs
    ${CMAKE_SOURCE_DIR}/external/cereal/include
    ${CMAKE_SOURCE_DIR}/external/sdl3/inc
    ${CMAKE_SOURCE_DIR}/external/freetype/inc
)

# Find SDL3 library
find_library(SDL3_LIBRARY
    NAMES SDL3 libSDL3
    PATHS ${CMAKE_SOURCE_DIR}/external/sdl3/lib/linux
    NO_DEFAULT_PATH
    REQUIRED
)

# Find FreeType library
find_library(FREETYPE_LIBRARY
    NAMES freetype libfreetype
    PATHS ${CMAKE_SOURCE_DIR}/external/freetype/lib/linux
    NO_DEFAULT_PATH
    REQUIRED
)

# Link SDL3 and FreeType
target_link_libraries(xs PRIVATE ${SDL3_LIBRARY} ${FREETYPE_LIBRARY})

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(xs PRIVATE OpenGL::GL)

# Threading support
find_package(Threads REQUIRED)
target_link_libraries(xs PRIVATE Threads::Threads)

# Link with dl (for dynamic loading)
target_link_libraries(xs PRIVATE ${CMAKE_DL_LIBS})

# Compiler warnings and options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(xs PRIVATE
        -Wall
        -Wno-unused-variable
        -Wno-unused-but-set-variable
    )
endif()

# miniz preprocessor definitions (for Linux compatibility)
target_compile_definitions(xs PRIVATE
    MINIZ_NO_TIME
)

message(STATUS "XS Game Engine - Linux build configured")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
