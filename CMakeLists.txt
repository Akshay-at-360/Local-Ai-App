cmake_minimum_required(VERSION 3.20)
project(OnDeviceAI VERSION 0.1.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_IOS "Build iOS framework" OFF)
option(BUILD_ANDROID "Build Android library" OFF)
option(BUILD_WASM "Build WebAssembly" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# Platform detection
if(APPLE)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(PLATFORM_IOS TRUE)
    else()
        set(PLATFORM_MACOS TRUE)
    endif()
elseif(ANDROID)
    set(PLATFORM_ANDROID TRUE)
elseif(EMSCRIPTEN)
    set(PLATFORM_WEB TRUE)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
elseif(WIN32)
    set(PLATFORM_WINDOWS TRUE)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Sanitizers
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
endif()

# Dependencies
find_package(Threads REQUIRED)

# Include vcpkg or conan integration
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

# Fetch llama.cpp
include(FetchContent)
FetchContent_Declare(
    llama
    GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
    GIT_TAG b3909  # Use a stable tag/commit
    GIT_SHALLOW TRUE
)

# Configure llama.cpp build options
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(LLAMA_STATIC ON CACHE BOOL "" FORCE)

# Enable hardware acceleration based on platform
if(PLATFORM_MACOS OR PLATFORM_IOS)
    set(GGML_METAL ON CACHE BOOL "" FORCE)
    set(GGML_ACCELERATE ON CACHE BOOL "" FORCE)
elseif(PLATFORM_ANDROID OR PLATFORM_LINUX)
    set(GGML_CUDA OFF CACHE BOOL "" FORCE)
    set(GGML_VULKAN ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(llama)

# Disable warnings as errors for llama.cpp targets
if(TARGET llama)
    target_compile_options(llama PRIVATE -Wno-error)
endif()
if(TARGET ggml)
    target_compile_options(ggml PRIVATE -Wno-error)
endif()

# Core library
add_subdirectory(core)

# Platform wrappers
if(BUILD_IOS OR PLATFORM_IOS OR PLATFORM_MACOS)
    add_subdirectory(platforms/ios)
endif()

if(BUILD_ANDROID OR PLATFORM_ANDROID)
    add_subdirectory(platforms/android)
endif()

if(BUILD_WASM OR PLATFORM_WEB)
    add_subdirectory(platforms/web)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
add_subdirectory(examples EXCLUDE_FROM_ALL)

# Installation
install(TARGETS ondeviceai_core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY core/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
