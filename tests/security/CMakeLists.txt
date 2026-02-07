# Security tests CMakeLists.txt
# Task 22.4: Security and privacy audit tests

add_executable(ondeviceai_security_tests
    security_privacy_test.cpp
)

target_link_libraries(ondeviceai_security_tests
    PRIVATE
        ondeviceai_core
        GTest::gtest_main
        GTest::gmock
)

target_include_directories(ondeviceai_security_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

gtest_discover_tests(ondeviceai_security_tests
    PROPERTIES
        LABELS "security"
        TIMEOUT 60
)
