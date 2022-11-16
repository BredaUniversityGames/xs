cmake_minimum_required(VERSION 3.9)

project(xs)



set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

##### PREPREOCESSOR DEFINITIONS
# TODO(@Quartenia): there are more macros used in xs
add_compile_definitions(PLATFORM_PC)

##### OUTPUT DIRECTORIES #####
# TODO(@Quartenia): we should match the setup of `xs.sln`
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/executable)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/executable)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/executable)

##### WREN #####
file(GLOB_RECURSE WREN_SOURCES "external/wren/*.c")
file(GLOB_RECURSE WREN_HEADERS "external/wren/*.h")

add_library(wren STATIC ${WREN_SOURCES} ${WREN_HEADERS})
target_include_directories(wren PUBLIC 
	external/wren/include/
	external/wren/optional/
	external/wren/vm/
)

##### GLAD #####
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC 
	external/glad/include/
)

# Additional library directories
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/external")

##### XS #####
set(XS_CONSOLE_EXCLUDE "switch.|ps5.")

# TODO(@Quartenia) We might want to move this to CMakeLists in the code subfolder
file(GLOB SOURCES "code/source/*.cpp")
file(GLOB HEADERS "code/include/*.h")

# platform specific code
file(GLOB PC_SOURCES "code/source/pc/*.cpp")
source_group("Source Files/PC" FILES ${PC_SOURCES} )

file(GLOB IMGUI_SOURCES "external/imgui/*.cpp" "external/imgui/cpp/*.cpp")
file(GLOB IMGUI_HEADERS "external/imgui/*.h" "external/imgui/cpp/*.h")

list(FILTER IMGUI_SOURCES EXCLUDE REGEX ${XS_CONSOLE_EXCLUDE} )
list(FILTER IMGUI_HEADERS EXCLUDE REGEX ${XS_CONSOLE_EXCLUDE} )

source_group("Imgui/Header Files" FILES ${IMGUI_HEADERS} )
source_group("Imgui/Source Files" FILES ${IMGUI_SOURCES} )

find_library(GLFW_LIB glfw3 "external/GLFW")
find_package(OpenGL REQUIRED)

set(APP_RESOURCE_WIN "${CMAKE_CURRENT_SOURCE_DIR}/xs.rc")

add_executable(xs ${SOURCES} ${PC_SOURCES} ${HEADERS} ${IMGUI_SOURCES} ${IMGUI_HEADERS} ${APP_RESOURCE_WIN})

set_target_properties(xs PROPERTIES 
	LINKER_LANGUAGE CXX 
	CXX_ISO_COMPLIANT ON 
	CXX_STANDARD 17 
	CXX_STANDARD_REQUIRED ON 
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

set(FMOD_LIB "${CMAKE_CURRENT_SOURCE_DIR}/external/fmod/lib/fmodL_vc.lib")
set(FMODSTUDIO_LIB "${CMAKE_CURRENT_SOURCE_DIR}/external/fmod/lib/fmodstudioL_vc.lib")

target_link_libraries(xs 
	${GLFW_LIB} glad wren ${FMOD_LIB} ${FMODSTUDIO_LIB} 
)

# post build copy command for FMod dlls
add_custom_command(TARGET xs POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/external/fmod/lib/fmodL.dll ${PROJECT_BINARY_DIR}/executable/Debug/fmodL.dll
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/external/fmod/lib/fmodstudioL.dll ${PROJECT_BINARY_DIR}/executable/Debug/fmodstudioL.dll
)

target_include_directories(xs PUBLIC
	code/include
    external
    external/glad/include/
    external/imgui
	external/fmod
)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT xs)