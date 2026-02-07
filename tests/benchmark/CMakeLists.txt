# Benchmark tests CMakeLists.txt
# Task 22.2 & 22.3: Performance benchmarks and memory/resource tests

# Performance benchmarks
add_executable(ondeviceai_benchmarks
    performance_benchmark_test.cpp
)

target_link_libraries(ondeviceai_benchmarks
    PRIVATE
        ondeviceai_core
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(ondeviceai_benchmarks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

gtest_discover_tests(ondeviceai_benchmarks
    PROPERTIES
        LABELS "benchmark"
        TIMEOUT 300  # 5 minute timeout for benchmarks
)

# Memory and resource stress tests
add_executable(ondeviceai_memory_tests
    memory_resource_test.cpp
)

target_link_libraries(ondeviceai_memory_tests
    PRIVATE
        ondeviceai_core
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(ondeviceai_memory_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

gtest_discover_tests(ondeviceai_memory_tests
    PROPERTIES
        LABELS "stress"
        TIMEOUT 120
)
