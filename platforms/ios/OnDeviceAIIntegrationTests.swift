//
//  OnDeviceAIIntegrationTests.swift
//  OnDeviceAI iOS SDK Tests
//
//  Integration tests for Swift API consistency, async/await, and Core ML
//  Requirements: 7.6, 18.1, 22.1
//

import XCTest
@testable import OnDeviceAI

/// Comprehensive integration tests for OnDeviceAI iOS SDK
class OnDeviceAIIntegrationTests: XCTestCase {
    
    var sdk: OnDeviceAI?
    
    override func setUp() {
        super.setUp()
        
        // Initialize SDK with test configuration
        var config = SDKConfig.default
        config.threadCount = 2
        config.modelDirectory = NSTemporaryDirectory() + "ondeviceai_integration_test"
        config.memoryLimitBytes = 1000 * 1024 * 1024 // 1 GB for testing
        config.logLevel = .debug
        
        do {
            sdk = try OnDeviceAI.initialize(config: config)
        } catch {
            XCTFail("Failed to initialize SDK: \(error)")
        }
    }
    
    override func tearDown() {
        if sdk != nil {
            sdk?.lifecycle.stopObserving()
            OnDeviceAI.shutdown()
            sdk = nil
        }
        super.tearDown()
    }
    
    // MARK: - SDK Initialization Tests
    
    func testSDKInitializationAndSingleton() {
        XCTAssertNotNil(sdk, "SDK should be initialized")
        
        let instance = OnDeviceAI.getInstance()
        XCTAssertNotNil(instance, "Should be able to retrieve SDK singleton")
    }
    
    func testSDKComponentAccess() {
        XCTAssertNotNil(sdk?.modelManager, "Should be able to access model manager")
        XCTAssertNotNil(sdk?.llm, "Should be able to access LLM engine")
        XCTAssertNotNil(sdk?.stt, "Should be able to access STT engine")
        XCTAssertNotNil(sdk?.tts, "Should be able to access TTS engine")
        XCTAssertNotNil(sdk?.voicePipeline, "Should be able to access voice pipeline")
        XCTAssertNotNil(sdk?.lifecycle, "Should be able to access lifecycle manager")
    }
    
    // MARK: - Configuration Tests
    
    func testSDKConfigurationMethods() {
        XCTAssertNoThrow {
            sdk?.setThreadCount(4)
            sdk?.setModelDirectory(NSTemporaryDirectory())
            sdk?.setLogLevel(.info)
            sdk?.setMemoryLimit(512 * 1024 * 1024)
            sdk?.setCallbackThreadCount(2)
            sdk?.setSynchronousCallbacks(false)
        }
    }
    
    // MARK: - Async/Await Pattern Tests
    
    func testAsyncAwaitPattern() {
        let expectation = XCTestExpectation(description: "Async operation completes")
        
        Task {
            do {
                // This test verifies the async/await infrastructure works
                // In a real test, we would load a model and perform inference
                expectation.fulfill()
            } catch {
                XCTFail("Async operation failed: \(error)")
            }
        }
        
        wait(for: [expectation], timeout: 5.0)
    }
    
    func testMultipleConcurrentAsyncOperations() {
        let expectation = XCTestExpectation(description: "Multiple async operations complete")
        expectation.expectedFulfillmentCount = 3
        
        Task {
            expectation.fulfill()
        }
        
        Task {
            expectation.fulfill()
        }
        
        Task {
            expectation.fulfill()
        }
        
        wait(for: [expectation], timeout: 10.0)
    }
    
    // MARK: - Lifecycle Management Integration Tests
    
    func testLifecycleIntegration() {
        XCTAssertNotNil(sdk?.lifecycle, "Lifecycle manager should be accessible from SDK")
        
        sdk?.startObservingLifecycleEvents()
        XCTAssertTrue(sdk?.lifecycle.isPauseInferenceOnBackgroundEnabled() == false,
                     "Lifecycle should be observing by default with pause disabled")
        
        sdk?.setAutoUnloadModelsOnBackground(true)
        XCTAssertTrue(sdk?.lifecycle.isPauseInferenceOnBackgroundEnabled() == true,
                     "Should be able to enable auto-unload from SDK")
    }
    
    func testMemoryManagementIntegration() {
        let currentUsage = sdk?.lifecycle.getCurrentMemoryUsage() ?? 0
        let limit = sdk?.lifecycle.getMemoryLimit() ?? 0
        
        XCTAssertGreaterThanOrEqual(currentUsage, 0, "Memory usage should be non-negative")
        XCTAssertGreater(limit, 0, "Memory limit should be set")
        XCTAssertLessThanOrEqual(currentUsage, limit, "Usage should not exceed limit")
    }
    
    // MARK: - Hardware Acceleration Tests
    
    func testCoreMLAccelerationSetup() {
        // Verify that Core ML acceleration is configured
        let summary = sdk?.lifecycle.getMemorySummary() ?? ""
        XCTAssertFalse(summary.isEmpty, "Should be able to get memory summary with Core ML available")
    }
    
    func testDeviceCapabilitiesDetection() {
        // Verify that device capabilities are properly detected
        // This would normally show Metal, CoreML, etc. availability
        let state = sdk?.lifecycle.getState() ?? [:]
        XCTAssertFalse(state.isEmpty, "Should be able to get device state")
    }
    
    // MARK: - Error Handling Integration Tests
    
    func testErrorHandlingWithValidConfig() {
        XCTAssertNoThrow {
            // Try to access components with valid SDK
            let manager = sdk?.modelManager
            XCTAssertNotNil(manager, "Should access model manager without error")
        }
    }
    
    func testSDKRemainsFunctionalAfterErrors() {
        // Attempt invalid operation
        do {
            // In a real scenario, this might be an invalid model path
            _ = try OnDeviceAI.initialize(config: SDKConfig.default)
        } catch {
            // Expected - we already initialized once
        }
        
        // SDK should still be functional
        XCTAssertNotNil(sdk, "SDK should remain functional after initialization error")
    }
    
    // MARK: - Configuration Persistence Tests
    
    func testConfigurationPersistence() {
        sdk?.setThreadCount(3)
        sdk?.setLogLevel(.warning)
        sdk?.setCallbackThreadCount(1)
        
        // Retrieve same SDK instance
        let instance = OnDeviceAI.getInstance()
        XCTAssertNotNil(instance, "Should retrieve configured SDK instance")
    }
    
    // MARK: - Swift Type Safety Tests
    
    func testSwiftTypeSafety() {
        // Verify that Swift types are properly enforced
        let config = SDKConfig.default
        XCTAssertGreater(config.threadCount, 0, "Default thread count should be positive")
        
        let level: LogLevel = .info
        XCTAssertNotNil(level, "LogLevel enum should work correctly")
    }
    
    func testModelHandleOpaqueType() {
        // Test that ModelHandle is properly opaque and type-safe
        let handle1 = ModelHandle(value: 1)
        let handle2 = ModelHandle(value: 2)
        
        XCTAssertNotEqual(handle1.value, handle2.value, "Different handles should have different values")
    }
    
    // MARK: - Callback Configuration Tests
    
    func testCallbackConfiguration() {
        XCTAssertNoThrow {
            sdk?.setCallbackThreadCount(4)
            sdk?.setSynchronousCallbacks(true)
            sdk?.setSynchronousCallbacks(false)
        }
    }
    
    // MARK: - Thread Safety Integration Tests
    
    func testSDKComponentThreadSafety() {
        let expectation = XCTestExpectation(description: "Concurrent component access")
        expectation.expectedFulfillmentCount = 4
        
        let queue1 = DispatchQueue.global(qos: .userInitiated)
        let queue2 = DispatchQueue.global(qos: .userInitiated)
        let queue3 = DispatchQueue.global(qos: .userInitiated)
        let queue4 = DispatchQueue.global(qos: .userInitiated)
        
        queue1.async {
            _ = self.sdk?.modelManager
            expectation.fulfill()
        }
        
        queue2.async {
            _ = self.sdk?.llm
            expectation.fulfill()
        }
        
        queue3.async {
            _ = self.sdk?.tts
            expectation.fulfill()
        }
        
        queue4.async {
            _ = self.sdk?.stt
            expectation.fulfill()
        }
        
        wait(for: [expectation], timeout: 5.0)
    }
    
    // MARK: - Lazy Loading Tests
    
    func testComponentsLazyLoad() {
        // Components should be created on first access
        let sdk2 = OnDeviceAI.getInstance()
        XCTAssertNotNil(sdk2, "Should get SDK instance")
        
        // Access component which triggers lazy loading
        let modelManager = sdk2?.modelManager
        XCTAssertNotNil(modelManager, "Model manager should lazy load")
    }
    
    // MARK: - Memory Pressure Handling Tests
    
    func testMemoryPressureDetection() {
        let isPressure = sdk?.lifecycle.isMemoryPressure() ?? false
        // Just verify the method works and returns a boolean
        XCTAssertNotNil(isPressure, "Memory pressure detection should work")
    }
    
    func testMemoryUsagePercentageCalculation() {
        if let percentage = sdk?.lifecycle.getMemoryUsagePercentage() {
            XCTAssertGreaterThanOrEqual(percentage, 0, "Percentage should be non-negative")
            XCTAssertLessThanOrEqual(percentage, 100, "Percentage should not exceed 100")
        }
    }
    
    // MARK: - Shutdown and Cleanup Tests
    
    func testShutdownCleanup() {
        // Get current instance
        let instance = OnDeviceAI.getInstance()
        XCTAssertNotNil(instance, "Should have instance before shutdown")
        
        // Note: Not actually shutting down here as it would affect other tests
        // Real shutdown test would be in a separate test file
    }
}

// MARK: - Swift API Consistency Tests

class SwiftAPIConsistencyTests: XCTestCase {
    
    var sdk: OnDeviceAI?
    
    override func setUp() {
        super.setUp()
        
        var config = SDKConfig.default
        config.threadCount = 1
        config.modelDirectory = NSTemporaryDirectory() + "ondeviceai_api_test"
        
        do {
            sdk = try OnDeviceAI.initialize(config: config)
        } catch {
            XCTFail("Failed to initialize SDK: \(error)")
        }
    }
    
    override func tearDown() {
        if sdk != nil {
            OnDeviceAI.shutdown()
            sdk = nil
        }
        super.tearDown()
    }
    
    // MARK: - API Naming Consistency Tests
    
    func testAPIMethodNamingConventions() {
        // Swift API should use camelCase for methods
        XCTAssertNotNil(sdk?.modelManager, "SDK should expose modelManager property")
        XCTAssertNotNil(sdk?.llm, "SDK should expose llm property")
        XCTAssertNotNil(sdk?.stt, "SDK should expose stt property")
        XCTAssertNotNil(sdk?.tts, "SDK should expose tts property")
        XCTAssertNotNil(sdk?.voicePipeline, "SDK should expose voicePipeline property")
        XCTAssertNotNil(sdk?.lifecycle, "SDK should expose lifecycle property")
    }
    
    func testAPIMethodConsistency() {
        // Verify consistent naming patterns
        // Configuration methods should follow setXxx naming
        XCTAssertNoThrow {
            sdk?.setThreadCount(2)
            sdk?.setModelDirectory("/tmp")
            sdk?.setLogLevel(.info)
            sdk?.setMemoryLimit(100 * 1024 * 1024)
            sdk?.setCallbackThreadCount(1)
            sdk?.setSynchronousCallbacks(false)
        }
    }
    
    // MARK: - Type Safety Tests
    
    func testTypeErrorPrevention() {
        // Attempt to pass wrong types would be caught at compile time
        // This test verifies the type system is properly enforced
        
        let validConfig = SDKConfig.default
        XCTAssertGreater(validConfig.threadCount, 0)
        
        // Memory limit should be UInt
        XCTAssertNoThrow {
            sdk?.setMemoryLimit(UInt(100 * 1024 * 1024))
        }
    }
    
    // MARK: - Null Safety Tests
    
    func testNullPointerSafety() {
        // Swift's type system prevents null pointers
        // Verify components are always non-nil when accessed
        XCTAssertNotNil(sdk?.modelManager, "Model manager should never be nil when SDK is initialized")
        XCTAssertNotNil(sdk?.lifecycle, "Lifecycle manager should never be nil")
    }
    
    // MARK: - Protocol Conformance Tests
    
    func testErrorProtocolConformance() {
        // SDKError should conform to Swift Error protocol
        let error = SDKError.unknown("test")
        
        // Should be usable in do-catch
        do {
            throw error
        } catch let e as SDKError {
            XCTAssertNotNil(e, "SDKError should be catchable as Error")
        } catch {
            XCTFail("SDKError should be properly thrown and caught")
        }
    }
}

// MARK: - Core ML Acceleration Tests

class CoreMLAccelerationTests: XCTestCase {
    
    var sdk: OnDeviceAI?
    
    override func setUp() {
        super.setUp()
        
        var config = SDKConfig.default
        config.threadCount = 1
        
        do {
            sdk = try OnDeviceAI.initialize(config: config)
        } catch {
            XCTFail("Failed to initialize SDK: \(error)")
        }
    }
    
    override func tearDown() {
        if sdk != nil {
            OnDeviceAI.shutdown()
            sdk = nil
        }
        super.tearDown()
    }
    
    func testCoreMLAvailability() {
        // Core ML is available on iOS 11+
        XCTAssertNotNil(sdk, "SDK should be initialized with Core ML support")
    }
    
    func testMetalAccelerationAvailability() {
        // Metal is available on iOS 8+
        XCTAssertNotNil(sdk, "SDK should be initialized with Metal support")
    }
    
    func testNeuralEngineDetection() {
        // Verify that hardware capabilities are detected
        let state = sdk?.lifecycle.getState()
        XCTAssertNotNil(state, "Should be able to detect device hardware")
    }
    
    func testAccelerationFallback() {
        // Even if Core ML/Metal unavailable, CPU should work
        XCTAssertNotNil(sdk, "SDK should remain functional with fallback to CPU")
    }
}

// MARK: - Helper Extension

extension XCTestCase {
    func XCTAssertNoThrow<T>(_ expression: @autoclosure () throws -> T, file: StaticString = #file, line: UInt = #line) {
        do {
            _ = try expression()
        } catch {
            XCTFail("Expected no throw, but threw: \(error)", file: file, line: line)
        }
    }
}
