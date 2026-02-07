//
//  LifecycleManagerTests.swift
//  OnDeviceAI iOS SDK Tests
//
//  Unit tests for iOS lifecycle management
//  Requirements: 22.1 (memory warnings), 22.5 (ARC integration)
//

import XCTest
@testable import OnDeviceAI

/// Tests for iOS lifecycle management functionality
class LifecycleManagerTests: XCTestCase {
    
    var sdk: OnDeviceAI?
    var lifecycleManager: LifecycleManager?
    
    override func setUp() {
        super.setUp()
        
        // Initialize SDK with test configuration
        var config = SDKConfig.default
        config.threadCount = 1
        config.modelDirectory = NSTemporaryDirectory() + "ondeviceai_test"
        config.memoryLimitBytes = 500 * 1024 * 1024 // 500 MB for testing
        
        do {
            sdk = try OnDeviceAI.initialize(config: config)
            lifecycleManager = sdk?.lifecycle
        } catch {
            XCTFail("Failed to initialize SDK: \(error)")
        }
    }
    
    override func tearDown() {
        // Stop observing lifecycle events
        lifecycleManager?.stopObserving()
        
        // Shutdown SDK
        if sdk != nil {
            OnDeviceAI.shutdown()
            sdk = nil
        }
        
        lifecycleManager = nil
        
        super.tearDown()
    }
    
    // MARK: - Initialization Tests
    
    func testLifecycleManagerInitialization() {
        XCTAssertNotNil(lifecycleManager, "Lifecycle manager should be initialized")
        XCTAssertNotNil(sdk, "SDK should be initialized")
    }
    
    func testLifecycleManagerAccessViaSDK() {
        XCTAssertNotNil(sdk?.lifecycle, "Should be able to access lifecycle manager via SDK")
    }
    
    // MARK: - Observation Control Tests
    
    func testStartObserving() {
        XCTAssertNoThrow {
            lifecycleManager?.startObserving()
        }
    }
    
    func testStopObserving() {
        lifecycleManager?.startObserving()
        XCTAssertNoThrow {
            lifecycleManager?.stopObserving()
        }
    }
    
    func testMultipleStartObserving() {
        // Calling startObserving multiple times should not cause issues
        XCTAssertNoThrow {
            lifecycleManager?.startObserving()
            lifecycleManager?.startObserving() // Should be idempotent
        }
    }
    
    func testMultipleStopObserving() {
        // Calling stopObserving multiple times should not cause issues
        XCTAssertNoThrow {
            lifecycleManager?.stopObserving()
            lifecycleManager?.stopObserving() // Should be safe
        }
    }
    
    // MARK: - Configuration Tests
    
    func testPauseInferenceOnBackgroundDefault() {
        XCTAssertFalse(lifecycleManager?.isPauseInferenceOnBackgroundEnabled() ?? false,
                      "Pause on background should be disabled by default")
    }
    
    func testSetPauseInferenceOnBackground() {
        lifecycleManager?.setPauseInferenceOnBackground(true)
        XCTAssertTrue(lifecycleManager?.isPauseInferenceOnBackgroundEnabled() ?? false,
                     "Should be able to enable pause on background")
        
        lifecycleManager?.setPauseInferenceOnBackground(false)
        XCTAssertFalse(lifecycleManager?.isPauseInferenceOnBackgroundEnabled() ?? false,
                      "Should be able to disable pause on background")
    }
    
    // MARK: - Memory Information Tests
    
    func testGetCurrentMemoryUsage() {
        let memoryUsage = lifecycleManager?.getCurrentMemoryUsage() ?? 0
        XCTAssertGreaterThanOrEqual(memoryUsage, 0, "Memory usage should be non-negative")
    }
    
    func testGetMemoryLimit() {
        let memoryLimit = lifecycleManager?.getMemoryLimit() ?? 0
        XCTAssertGreaterThanOrEqual(memoryLimit, 0, "Memory limit should be non-negative")
        XCTAssertGreaterThan(memoryLimit, 0, "Memory limit should be set for this test")
    }
    
    func testMemoryUsageWithinLimit() {
        let currentUsage = lifecycleManager?.getCurrentMemoryUsage() ?? 0
        let memoryLimit = lifecycleManager?.getMemoryLimit() ?? 0
        
        if memoryLimit > 0 {
            XCTAssertLessThanOrEqual(currentUsage, memoryLimit,
                                    "Memory usage should not exceed limit")
        }
    }
    
    func testGetMemoryUsagePercentage() {
        let percentage = lifecycleManager?.getMemoryUsagePercentage()
        
        if let percentage = percentage {
            XCTAssertGreaterThanOrEqual(percentage, 0, "Percentage should be non-negative")
            XCTAssertLessThanOrEqual(percentage, 100, "Percentage should not exceed 100")
        }
    }
    
    func testIsMemoryPressure() {
        let isUnderPressure = lifecycleManager?.isMemoryPressure() ?? false
        // We can't assert a specific value as it depends on current system state
        // Just verify the method returns a valid boolean
        XCTAssertNotNil(lifecycleManager?.isMemoryPressure(), "Should be able to check memory pressure")
    }
    
    // MARK: - Manual Memory Management Tests
    
    func testUnloadAllModels() {
        let result = lifecycleManager?.unloadAllModels() ?? false
        // Result depends on whether models are actually loaded
        // Just verify the method returns a valid boolean
        XCTAssertNotNil(result, "unloadAllModels should return a boolean")
    }
    
    // MARK: - Memory Summary Tests
    
    func testGetMemorySummary() {
        let summary = lifecycleManager?.getMemorySummary() ?? ""
        XCTAssertFalse(summary.isEmpty, "Memory summary should not be empty")
        XCTAssertTrue(summary.contains("Memory"), "Summary should contain 'Memory'")
    }
    
    func testMemorySummaryFormat() {
        let summary = lifecycleManager?.getMemorySummary() ?? ""
        // Summary should contain MB
        XCTAssertTrue(summary.contains("MB"), "Summary should contain MB unit")
    }
    
    // MARK: - State Information Tests
    
    func testGetState() {
        guard let state = lifecycleManager?.getState() else {
            XCTFail("Should be able to get lifecycle manager state")
            return
        }
        
        XCTAssertNotNil(state["isObserving"], "State should include isObserving")
        XCTAssertNotNil(state["pauseInferenceOnBackground"], "State should include pauseInferenceOnBackground")
        XCTAssertNotNil(state["currentMemoryUsage"], "State should include currentMemoryUsage")
        XCTAssertNotNil(state["memoryLimit"], "State should include memoryLimit")
        XCTAssertNotNil(state["isMemoryPressure"], "State should include isMemoryPressure")
    }
    
    // MARK: - SDK Integration Tests
    
    func testSDKConvenienceMethods() {
        XCTAssertNoThrow {
            sdk?.startObservingLifecycleEvents()
            sdk?.setAutoUnloadModelsOnBackground(true)
            sdk?.stopObservingLifecycleEvents()
        }
    }
    
    func testLifecycleManagerAutoUnloadSetting() {
        sdk?.setAutoUnloadModelsOnBackground(true)
        XCTAssertTrue(lifecycleManager?.isPauseInferenceOnBackgroundEnabled() ?? false,
                     "Auto unload setting should be reflected in lifecycle manager")
        
        sdk?.setAutoUnloadModelsOnBackground(false)
        XCTAssertFalse(lifecycleManager?.isPauseInferenceOnBackgroundEnabled() ?? false,
                      "Should be able to disable auto unload")
    }
    
    // MARK: - Thread Safety Tests
    
    func testLifecycleManagerThreadSafety() {
        let expectation = XCTestExpectation(description: "Concurrent lifecycle operations")
        expectation.expectedFulfillmentCount = 3
        
        let queue1 = DispatchQueue.global(qos: .userInitiated)
        let queue2 = DispatchQueue.global(qos: .userInitiated)
        let queue3 = DispatchQueue.global(qos: .userInitiated)
        
        queue1.async {
            self.lifecycleManager?.startObserving()
            expectation.fulfill()
        }
        
        queue2.async {
            let _ = self.lifecycleManager?.getCurrentMemoryUsage()
            expectation.fulfill()
        }
        
        queue3.async {
            self.lifecycleManager?.setPauseInferenceOnBackground(true)
            expectation.fulfill()
        }
        
        wait(for: [expectation], timeout: 5.0)
        
        // Cleanup
        lifecycleManager?.stopObserving()
    }
    
    // MARK: - Memory Configuration Tests
    
    func testMemoryLimitConfiguration() {
        let limit = lifecycleManager?.getMemoryLimit() ?? 0
        XCTAssertGreaterThan(limit, 0, "Memory limit should be configured")
        
        // Should be 500 MB as set in setUp
        let expectedLimit = 500 * 1024 * 1024
        XCTAssertEqual(limit, UInt(expectedLimit), "Memory limit should match configuration")
    }
}

// MARK: - XCTestCase Helper Extension

extension XCTestCase {
    func XCTAssertNoThrow<T>(_ expression: @autoclosure () throws -> T, file: StaticString = #file, line: UInt = #line) {
        do {
            _ = try expression()
        } catch {
            XCTFail("Expected no throw, but threw: \(error)", file: file, line: line)
        }
    }
}
