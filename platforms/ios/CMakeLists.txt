# iOS Platform Bridge Layer
# Requirements: 7.1, 7.8, 22.1, 22.5

# Objective-C++ bridge sources
set(IOS_BRIDGE_SOURCES
    ODAITypeConversions.mm
    ODAIBridgeTypes.mm
    ODAISDKManager.mm
    ODAIModelManager.mm
    ODAILifecycleManager.mm
    # Additional engine implementations will be added in task 15.2
)

# Objective-C++ bridge headers
set(IOS_BRIDGE_HEADERS
    OnDeviceAI-Bridging.h
    ODAITypeConversions.h
    ODAISDKManager.h
    ODAIModelManager.h
    ODAILLMEngine.h
    ODAISTTEngine.h
    ODAITTSEngine.h
    ODAIVoicePipeline.h
    ODAILifecycleManager.h
)

# Swift API layer sources
set(IOS_SWIFT_SOURCES
    OnDeviceAI.swift
    ModelManager.swift
    LLMEngine.swift
    STTEngine.swift
    TTSEngine.swift
    VoicePipeline.swift
    LifecycleManager.swift
    Types.swift
    SDKError.swift
)

# Swift test sources (for iOS test bundle)
set(IOS_SWIFT_TEST_SOURCES
    LifecycleManagerTests.swift
)

# Create iOS bridge library
add_library(ondeviceai_ios_bridge STATIC
    ${IOS_BRIDGE_SOURCES}
)

# Set Objective-C++ compilation
set_source_files_properties(
    ${IOS_BRIDGE_SOURCES}
    PROPERTIES
    COMPILE_FLAGS "-x objective-c++ -fobjc-arc"
)

# Link with core library
target_link_libraries(ondeviceai_ios_bridge
    PRIVATE
        ondeviceai_core
)

# Include directories
target_include_directories(ondeviceai_ios_bridge
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/core/include
)

# iOS-specific frameworks
if(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_libraries(ondeviceai_ios_bridge
        PUBLIC
            "-framework Foundation"
            "-framework CoreFoundation"
    )
endif()

# Install headers
install(FILES ${IOS_BRIDGE_HEADERS}
    DESTINATION include/ondeviceai/ios
)

# Install Swift sources (for Swift Package Manager or manual integration)
install(FILES ${IOS_SWIFT_SOURCES}
    DESTINATION include/ondeviceai/ios/swift
)

# Install library
install(TARGETS ondeviceai_ios_bridge
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
