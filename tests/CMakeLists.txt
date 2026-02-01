# Testing configuration
include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()
include(GoogleTest)

# Due to duplicate symbol issues with llama.cpp and whisper.cpp both including ggml,
# we create separate test executables for different components

# Core tests (no LLM/STT dependencies)
add_executable(ondeviceai_core_tests
    unit/sdk_manager_test.cpp
    unit/model_manager_test.cpp
    unit/model_discovery_integration_test.cpp
    unit/model_download_integration_test.cpp
    unit/model_download_verification_test.cpp
    unit/model_storage_management_test.cpp
    unit/download_test.cpp
    unit/sha256_test.cpp
    unit/version_utils_test.cpp
    unit/model_versioning_test.cpp
    unit/memory_manager_test.cpp
    unit/logger_test.cpp
    unit/lru_cache_integration_test.cpp
    unit/error_handling_test.cpp
    unit/input_validation_test.cpp
    unit/error_recovery_test.cpp
    unit/callback_threading_test.cpp
    unit/concurrency_test.cpp
    unit/resource_cleanup_test.cpp
    unit/hardware_acceleration_test.cpp
    unit/ios_acceleration_test.cpp
    unit/offline_operation_test.cpp
    unit/secure_downloads_test.cpp
)

target_link_libraries(ondeviceai_core_tests
    PRIVATE
        ondeviceai_core
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(ondeviceai_core_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Discover tests
gtest_discover_tests(ondeviceai_core_tests)

# LLM tests (separate executable to avoid symbol conflicts)
add_executable(ondeviceai_llm_tests
    unit/llm_engine_test.cpp
    unit/llm_engine_integration_test.cpp
    unit/llm_engine_unit_test.cpp
)

target_link_libraries(ondeviceai_llm_tests
    PRIVATE
        ondeviceai_core
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(ondeviceai_llm_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

gtest_discover_tests(ondeviceai_llm_tests)

# STT tests (separate executable)
add_executable(ondeviceai_stt_tests
    unit/stt_engine_test.cpp
    unit/audio_preprocessing_test.cpp
)

target_link_libraries(ondeviceai_stt_tests
    PRIVATE
        ondeviceai_core
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(ondeviceai_stt_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

gtest_discover_tests(ondeviceai_stt_tests)

# TTS tests (separate executable)
add_executable(ondeviceai_tts_tests
    unit/tts_engine_test.cpp
    unit/tts_multi_voice_test.cpp
    unit/tts_comprehensive_test.cpp
)

target_link_libraries(ondeviceai_tts_tests
    PRIVATE
        ondeviceai_core
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(ondeviceai_tts_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

gtest_discover_tests(ondeviceai_tts_tests)

# Voice Pipeline tests (separate executable - uses all components)
add_executable(ondeviceai_pipeline_tests
    unit/voice_pipeline_test.cpp
    unit/voice_pipeline_comprehensive_test.cpp
)

target_link_libraries(ondeviceai_pipeline_tests
    PRIVATE
        ondeviceai_core
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(ondeviceai_pipeline_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

gtest_discover_tests(ondeviceai_pipeline_tests)

# Fetch RapidCheck for property-based testing
FetchContent_Declare(
    rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG master
)
FetchContent_MakeAvailable(rapidcheck)

# Property-based tests
add_subdirectory(property)
