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

# Fetch whisper.cpp
FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG v1.5.4  # Use a stable tag
    GIT_SHALLOW TRUE
)

# Configure whisper.cpp build options
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Enable hardware acceleration for whisper.cpp based on platform
if(PLATFORM_MACOS OR PLATFORM_IOS)
    set(WHISPER_COREML ON CACHE BOOL "" FORCE)
    set(WHISPER_METAL ON CACHE BOOL "" FORCE)
elseif(PLATFORM_ANDROID)
    # Android will use CPU for now, NNAPI support can be added later
    set(WHISPER_COREML OFF CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(whisper)

# Disable warnings as errors for whisper.cpp targets
if(TARGET whisper)
    target_compile_options(whisper PRIVATE -Wno-error)
endif()

# Fetch ONNX Runtime
# Platform-specific ONNX Runtime URLs
if(PLATFORM_LINUX)
    set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz")
elseif(PLATFORM_MACOS)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-osx-arm64-1.16.3.tgz")
    else()
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-osx-x86_64-1.16.3.tgz")
    endif()
elseif(PLATFORM_WINDOWS)
    set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-win-x64-1.16.3.zip")
endif()

if(ONNXRUNTIME_URL)
    message(STATUS "Downloading ONNX Runtime from: ${ONNXRUNTIME_URL}")
    FetchContent_Declare(
        onnxruntime
        URL ${ONNXRUNTIME_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(onnxruntime)
    
    # Set ONNX Runtime paths
    set(ONNXRUNTIME_ROOT_DIR ${onnxruntime_SOURCE_DIR})
    set(ONNXRUNTIME_INCLUDE_DIR ${ONNXRUNTIME_ROOT_DIR}/include)
    set(ONNXRUNTIME_LIB_DIR ${ONNXRUNTIME_ROOT_DIR}/lib)
    
    # Find the ONNX Runtime library
    find_library(ONNXRUNTIME_LIBRARY
        NAMES onnxruntime
        PATHS ${ONNXRUNTIME_LIB_DIR}
        NO_DEFAULT_PATH
    )
    
    if(ONNXRUNTIME_LIBRARY)
        message(STATUS "Found ONNX Runtime: ${ONNXRUNTIME_LIBRARY}")
        add_library(onnxruntime_lib SHARED IMPORTED)
        set_target_properties(onnxruntime_lib PROPERTIES
            IMPORTED_LOCATION ${ONNXRUNTIME_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${ONNXRUNTIME_INCLUDE_DIR}
        )
    else()
        message(WARNING "ONNX Runtime library not found, TTS functionality will be limited")
    endif()
else()
    message(WARNING "ONNX Runtime URL not set for this platform, TTS functionality will be limited")
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
