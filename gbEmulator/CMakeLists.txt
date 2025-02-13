cmake_minimum_required(VERSION 3.15)
project(gbEmulator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent for third-party dependencies
include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# Fetch GLAD 
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

# Fetch nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Shader handling (copy shaders to build folder)
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/res/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/res")

# Copy shaders to the build directory
file(COPY ${SHADER_SOURCE_DIR} DESTINATION ${SHADER_OUTPUT_DIR})

# Source files
add_executable(gbEmulator 
    src/main.cpp 
    src/gb.cpp
    src/cpu.cpp
    src/mmu.cpp
    src/ppu.cpp
)

# Include directories
target_include_directories(gbEmulator PRIVATE 
    ${glfw_SOURCE_DIR}/include
    ${glad_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(gbEmulator PRIVATE 
    glfw
    glad 
    nlohmann_json::nlohmann_json
)