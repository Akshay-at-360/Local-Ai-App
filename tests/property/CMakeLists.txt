# Property-based tests using RapidCheck

add_executable(ondeviceai_property_tests
    model_manager_properties_test.cpp
    memory_manager_properties_test.cpp
    llm_properties_test.cpp
)

target_link_libraries(ondeviceai_property_tests
    PRIVATE
        ondeviceai_core
        rapidcheck
        GTest::gtest_main
)

target_include_directories(ondeviceai_property_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${rapidcheck_SOURCE_DIR}/include
        ${rapidcheck_SOURCE_DIR}/extras/gtest/include
)

# Discover tests
gtest_discover_tests(ondeviceai_property_tests)
