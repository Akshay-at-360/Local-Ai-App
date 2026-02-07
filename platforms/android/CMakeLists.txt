# Android platform – JNI bridge to C++ core
cmake_minimum_required(VERSION 3.18)
project(ondeviceai-android LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core SDK (built as a static library or linked via its own CMakeLists)
set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../core")
add_subdirectory("${CORE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/core")

# JNI shared library
add_library(ondeviceai SHARED
    jni/OnDeviceAI_jni.cpp
)

target_include_directories(ondeviceai PRIVATE
    "${CORE_DIR}/include"
)

target_link_libraries(ondeviceai PRIVATE
    ondeviceai_core   # from core CMakeLists
    android
    log
)

# NNAPI support (Android 8.1+)
find_library(NEURAL_NETWORKS neuralnetworks)
if(NEURAL_NETWORKS)
    target_link_libraries(ondeviceai PRIVATE ${NEURAL_NETWORKS})
    target_compile_definitions(ondeviceai PRIVATE HAVE_NNAPI=1)
    message(STATUS "NNAPI support enabled")
else()
    message(STATUS "NNAPI not available – CPU-only fallback")
endif()
