cmake_minimum_required(VERSION 3.9)

project(xs)

# TODO: this is not setting xs as startup
set_property(GLOBAL PROPERTY VS_STARTUP_PROJECT xs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


##### OUTPUT DIRECTORIES #####
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

add_compile_definitions(PLATFORM_PC)

##### XS #####
# We might want to move this to CMakeLists in the code subfolder
set(XS_CONSOLE_EXCLUDE "swtich.|switch.|ps5.")

file(GLOB SOURCES "code/source/*.cpp")
file(GLOB HEADERS "code/include/*.h")
file(GLOB IMGUI_SOURCES "external/imgui/*.cpp")
file(GLOB IMGUI_HEADERS "external/imgui/*.h")

list(FILTER SOURCES EXCLUDE REGEX ${XS_CONSOLE_EXCLUDE} )
list(FILTER HEADERS EXCLUDE REGEX ${XS_CONSOLE_EXCLUDE} )
list(FILTER IMGUI_SOURCES EXCLUDE REGEX ${XS_CONSOLE_EXCLUDE} )
list(FILTER IMGUI_HEADERS EXCLUDE REGEX ${XS_CONSOLE_EXCLUDE} )

source_group("imgui" FILES ${IMGUI_SOURCES} ${IMGUI_HEADERS} )

find_library(GLFW_LIB glfw3 "external/GLFW")
find_package(OpenGL REQUIRED)

set(APP_RESOURCE_WIN "${CMAKE_CURRENT_SOURCE_DIR}/xs.rc")

add_executable(xs ${SOURCES} ${HEADERS} ${IMGUI_SOURCES}  ${IMGUI_HEADERS} ${APP_RESOURCE_WIN})

set_target_properties(xs PROPERTIES 
	LINKER_LANGUAGE CXX 
	CXX_ISO_COMPLIANT ON 
	CXX_STANDARD 17 
	CXX_STANDARD_REQUIRED ON 
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_link_libraries(xs ${GLFW_LIB} glad wren)

target_include_directories(xs PUBLIC
	code/include
    external
    external/GLFW
    external/glad/include/
    external/glm/glm
    external/imgui
)