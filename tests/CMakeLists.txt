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

# Test executable
add_executable(ondeviceai_tests
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
    unit/llm_engine_test.cpp
    unit/llm_engine_integration_test.cpp
    unit/stt_engine_test.cpp
    unit/tts_engine_test.cpp
    unit/voice_pipeline_test.cpp
    unit/memory_manager_test.cpp
    unit/logger_test.cpp
    unit/lru_cache_integration_test.cpp
)

target_link_libraries(ondeviceai_tests
    PRIVATE
        ondeviceai_core
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(ondeviceai_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Discover tests
gtest_discover_tests(ondeviceai_tests)

# Fetch RapidCheck for property-based testing
FetchContent_Declare(
    rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG master
)
FetchContent_MakeAvailable(rapidcheck)

# Property-based tests
add_subdirectory(property)
