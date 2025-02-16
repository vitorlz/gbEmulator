cmake_minimum_required(VERSION 3.15)
project(gbEmulator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "" FORCE)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT gbEmulator)
endif()

# Use FetchContent to bring in third-party dependencies
include(FetchContent)

### Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

### Fetch GLAD 
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

### Fetch nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

### Fetch ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.8
)
FetchContent_MakeAvailable(imgui)

### Fetch tinyfiledialogs
FetchContent_Declare(
    tinyfiledialogs
    GIT_REPOSITORY https://github.com/native-toolkit/tinyfiledialogs.git
    GIT_TAG 2.9.3
)
FetchContent_MakeAvailable(tinyfiledialogs)

# Copy shaders to the build directory
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/res/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/res")
file(COPY ${SHADER_SOURCE_DIR} DESTINATION ${SHADER_OUTPUT_DIR})

# Create the executable and add your source files
add_executable(gbEmulator 
    src/main.cpp 
    src/gb.cpp
    src/cpu.cpp
    src/mmu.cpp
    src/ppu.cpp
    src/App.cpp            
    src/renderingManager.cpp  
)
# Add ImGui source files 
target_sources(gbEmulator PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Add tinyfiledialogs source file 
target_sources(gbEmulator PRIVATE
    ${tinyfiledialogs_SOURCE_DIR}/tinyfiledialogs.c
)

# Include directories for all dependencies
target_include_directories(gbEmulator PRIVATE 
    ${glfw_SOURCE_DIR}/include
    ${glad_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${tinyfiledialogs_SOURCE_DIR}
)

# Link libraries
target_link_libraries(gbEmulator PRIVATE 
    glfw
    glad 
    nlohmann_json::nlohmann_json
)

# Linux stuff
if(UNIX AND NOT APPLE)
    target_link_libraries(gbEmulator PRIVATE m dl pthread X11)
endif()