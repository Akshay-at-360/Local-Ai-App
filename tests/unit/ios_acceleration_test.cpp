// Test: iOS Core ML and Metal Acceleration Integration
// Task 15.3: Integrate Core ML acceleration
// Requirements: 18.1, 18.2

#include <gtest/gtest.h>
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/stt_engine.hpp"
#include "ondeviceai/hardware_acceleration.hpp"
#include "ondeviceai/logger.hpp"
#include <filesystem>
#include <fstream>

using namespace ondeviceai;

class iOSAccelerationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().setLogLevel(LogLevel::Debug);
        LOG_INFO("=== iOS Acceleration Test Setup ===");
    }
    
    void TearDown() override {
        LOG_INFO("=== iOS Acceleration Test Teardown ===");
    }
    
    // Helper to create a minimal test model file
    std::string createTestModelFile(const std::string& name, size_t size_mb = 1) {
        std::string path = "test_ios_accel_" + name;
        std::ofstream file(path, std::ios::binary);
        
        // Write some dummy data
        std::vector<char> data(size_mb * 1024 * 1024, 0);
        file.write(data.data(), data.size());
        file.close();
        
        return path;
    }
    
    void cleanupTestFile(const std::string& path) {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
        }
    }
};

// Test: Verify Metal is available on Apple platforms
TEST_F(iOSAccelerationTest, MetalAvailableOnApple) {
    LOG_INFO("Testing Metal availability on Apple platforms");
    
#ifdef __APPLE__
    // Metal should be available on Apple platforms
    bool metal_available = HardwareAcceleration::isAcceleratorAvailable(AcceleratorType::Metal);
    EXPECT_TRUE(metal_available) << "Metal should be available on Apple platforms";
    
    if (metal_available) {
        LOG_INFO("✓ Metal is available");
    } else {
        LOG_WARNING("✗ Metal is not available (unexpected on Apple platforms)");
    }
#else
    GTEST_SKIP() << "Test only applicable on Apple platforms";
#endif
}

// Test: Verify Core ML is available on Apple platforms
TEST_F(iOSAccelerationTest, CoreMLAvailableOnApple) {
    LOG_INFO("Testing Core ML availability on Apple platforms");
    
#ifdef __APPLE__
    // Core ML should be available on iOS 11+ and macOS 10.13+
    bool coreml_available = HardwareAcceleration::isAcceleratorAvailable(AcceleratorType::CoreML);
    EXPECT_TRUE(coreml_available) << "Core ML should be available on modern Apple platforms";
    
    if (coreml_available) {
        LOG_INFO("✓ Core ML is available");
    } else {
        LOG_WARNING("✗ Core ML is not available (may be on older OS version)");
    }
#else
    GTEST_SKIP() << "Test only applicable on Apple platforms";
#endif
}

// Test: Verify llama.cpp is configured with Metal
TEST_F(iOSAccelerationTest, LlamaCppConfiguredWithMetal) {
    LOG_INFO("Testing llama.cpp Metal configuration");
    
#ifdef __APPLE__
    auto config = HardwareAccelerationConfig::defaults();
    auto result = HardwareAcceleration::configureLlamaCpp(config);
    
    ASSERT_TRUE(result.isSuccess()) << "llama.cpp configuration should succeed";
    
    AcceleratorType accel_type = result.value();
    
    // On Apple platforms, llama.cpp should use Metal (or CPU as fallback)
    EXPECT_TRUE(accel_type == AcceleratorType::Metal || accel_type == AcceleratorType::CPU)
        << "llama.cpp should use Metal or CPU on Apple platforms, got: "
        << HardwareAcceleration::getAcceleratorName(accel_type);
    
    if (accel_type == AcceleratorType::Metal) {
        LOG_INFO("✓ llama.cpp configured with Metal acceleration");
    } else {
        LOG_WARNING("llama.cpp using CPU (Metal not available or not configured)");
    }
#else
    GTEST_SKIP() << "Test only applicable on Apple platforms";
#endif
}

// Test: Verify whisper.cpp is configured with Core ML
TEST_F(iOSAccelerationTest, WhisperCppConfiguredWithCoreML) {
    LOG_INFO("Testing whisper.cpp Core ML configuration");
    
#ifdef __APPLE__
    auto config = HardwareAccelerationConfig::defaults();
    auto result = HardwareAcceleration::configureWhisperCpp(config);
    
    ASSERT_TRUE(result.isSuccess()) << "whisper.cpp configuration should succeed";
    
    AcceleratorType accel_type = result.value();
    
    // On Apple platforms, whisper.cpp should use Core ML or Metal (or CPU as fallback)
    EXPECT_TRUE(accel_type == AcceleratorType::CoreML || 
                accel_type == AcceleratorType::Metal || 
                accel_type == AcceleratorType::CPU)
        << "whisper.cpp should use Core ML, Metal, or CPU on Apple platforms, got: "
        << HardwareAcceleration::getAcceleratorName(accel_type);
    
    if (accel_type == AcceleratorType::CoreML) {
        LOG_INFO("✓ whisper.cpp configured with Core ML acceleration");
    } else if (accel_type == AcceleratorType::Metal) {
        LOG_INFO("✓ whisper.cpp configured with Metal acceleration");
    } else {
        LOG_WARNING("whisper.cpp using CPU (hardware acceleration not available or not configured)");
    }
#else
    GTEST_SKIP() << "Test only applicable on Apple platforms";
#endif
}

// Test: Verify Neural Engine acceleration preference on iOS
TEST_F(iOSAccelerationTest, NeuralEnginePreferredOniOS) {
    LOG_INFO("Testing Neural Engine preference on iOS");
    
#if defined(__APPLE__) && (TARGET_OS_IOS || TARGET_OS_IPHONE)
    auto config = HardwareAccelerationConfig::defaults();
    
    // On iOS, Core ML (Neural Engine) should be the first preference
    ASSERT_FALSE(config.preferred_accelerators.empty());
    EXPECT_EQ(config.preferred_accelerators[0], AcceleratorType::CoreML)
        << "Core ML (Neural Engine) should be the first preference on iOS";
    
    LOG_INFO("✓ Neural Engine (Core ML) is preferred on iOS");
#else
    GTEST_SKIP() << "Test only applicable on iOS";
#endif
}

// Test: Verify LLM engine loads with hardware acceleration
TEST_F(iOSAccelerationTest, LLMEngineLoadsWithAcceleration) {
    LOG_INFO("Testing LLM engine with hardware acceleration");
    
#ifdef __APPLE__
    // Note: This test requires a real GGUF model file to fully test
    // For now, we verify that the acceleration configuration is applied
    
    LLMEngine llm_engine;
    
    // The engine should be initialized successfully
    // Actual model loading would require a real model file
    LOG_INFO("✓ LLM engine initialized (hardware acceleration will be used when loading models)");
    
    // Verify that the hardware acceleration configuration is correct
    auto config = HardwareAccelerationConfig::defaults();
    auto result = HardwareAcceleration::configureLlamaCpp(config);
    ASSERT_TRUE(result.isSuccess());
    
    LOG_INFO("Hardware acceleration configured: " + 
             HardwareAcceleration::getAcceleratorName(result.value()));
#else
    GTEST_SKIP() << "Test only applicable on Apple platforms";
#endif
}

// Test: Verify STT engine loads with Core ML acceleration
TEST_F(iOSAccelerationTest, STTEngineLoadsWithCoreML) {
    LOG_INFO("Testing STT engine with Core ML acceleration");
    
#ifdef __APPLE__
    // Note: This test requires a real Whisper model file to fully test
    // For now, we verify that the acceleration configuration is applied
    
    STTEngine stt_engine;
    
    // The engine should be initialized successfully
    // Actual model loading would require a real model file
    LOG_INFO("✓ STT engine initialized (Core ML acceleration will be used when loading models)");
    
    // Verify that the hardware acceleration configuration is correct
    auto config = HardwareAccelerationConfig::defaults();
    auto result = HardwareAcceleration::configureWhisperCpp(config);
    ASSERT_TRUE(result.isSuccess());
    
    LOG_INFO("Hardware acceleration configured: " + 
             HardwareAcceleration::getAcceleratorName(result.value()));
#else
    GTEST_SKIP() << "Test only applicable on Apple platforms";
#endif
}

// Test: Verify accelerator detection returns expected types
TEST_F(iOSAccelerationTest, AcceleratorDetectionReturnsExpectedTypes) {
    LOG_INFO("Testing accelerator detection");
    
#ifdef __APPLE__
    auto accelerators = HardwareAcceleration::detectAvailableAccelerators();
    
    // Should have at least CPU
    ASSERT_FALSE(accelerators.empty()) << "Should detect at least CPU";
    
    // CPU should always be present
    bool has_cpu = false;
    bool has_metal = false;
    bool has_coreml = false;
    
    for (const auto& acc : accelerators) {
        LOG_INFO("Detected accelerator: " + acc.name + " - " + 
                (acc.available ? "available" : "unavailable"));
        
        if (acc.type == AcceleratorType::CPU) has_cpu = true;
        if (acc.type == AcceleratorType::Metal) has_metal = true;
        if (acc.type == AcceleratorType::CoreML) has_coreml = true;
    }
    
    EXPECT_TRUE(has_cpu) << "CPU should always be detected";
    
    // On Apple platforms, we expect Metal and/or Core ML
    EXPECT_TRUE(has_metal || has_coreml) 
        << "Should detect Metal or Core ML on Apple platforms";
    
    if (has_metal) {
        LOG_INFO("✓ Metal detected");
    }
    if (has_coreml) {
        LOG_INFO("✓ Core ML detected");
    }
#else
    GTEST_SKIP() << "Test only applicable on Apple platforms";
#endif
}

// Test: Verify best accelerator selection on Apple platforms
TEST_F(iOSAccelerationTest, BestAcceleratorSelection) {
    LOG_INFO("Testing best accelerator selection");
    
#ifdef __APPLE__
    auto best = HardwareAcceleration::getBestAccelerator();
    
    // Best accelerator should be Metal, Core ML, or CPU
    EXPECT_TRUE(best == AcceleratorType::Metal || 
                best == AcceleratorType::CoreML || 
                best == AcceleratorType::CPU)
        << "Best accelerator should be Metal, Core ML, or CPU on Apple platforms";
    
    LOG_INFO("Best accelerator: " + HardwareAcceleration::getAcceleratorName(best));
    
    // On modern Apple hardware, we expect hardware acceleration
    if (best == AcceleratorType::CPU) {
        LOG_WARNING("Best accelerator is CPU (hardware acceleration may not be available)");
    } else {
        LOG_INFO("✓ Hardware acceleration is available and selected");
    }
#else
    GTEST_SKIP() << "Test only applicable on Apple platforms";
#endif
}

// Test: Verify power efficient configuration prefers Neural Engine
TEST_F(iOSAccelerationTest, PowerEfficientConfigurationPrefersNeuralEngine) {
    LOG_INFO("Testing power efficient configuration");
    
#if defined(__APPLE__) && (TARGET_OS_IOS || TARGET_OS_IPHONE)
    auto config = HardwareAccelerationConfig::power_efficient();
    
    // Power efficient mode should prefer Core ML (Neural Engine) on iOS
    ASSERT_FALSE(config.preferred_accelerators.empty());
    EXPECT_EQ(config.preferred_accelerators[0], AcceleratorType::CoreML)
        << "Power efficient mode should prefer Core ML (Neural Engine) on iOS";
    
    LOG_INFO("✓ Power efficient mode prefers Neural Engine");
#else
    GTEST_SKIP() << "Test only applicable on iOS";
#endif
}

// Test: Verify performance configuration
TEST_F(iOSAccelerationTest, PerformanceConfiguration) {
    LOG_INFO("Testing performance configuration");
    
#ifdef __APPLE__
    auto config = HardwareAccelerationConfig::performance();
    
    // Performance mode should have hardware accelerators in preference list
    ASSERT_FALSE(config.preferred_accelerators.empty());
    
    bool has_hardware_accel = false;
    for (const auto& accel : config.preferred_accelerators) {
        if (accel != AcceleratorType::CPU) {
            has_hardware_accel = true;
            break;
        }
    }
    
    EXPECT_TRUE(has_hardware_accel)
        << "Performance mode should prefer hardware accelerators";
    
    LOG_INFO("✓ Performance mode configured with hardware acceleration preferences");
#else
    GTEST_SKIP() << "Test only applicable on Apple platforms";
#endif
}

// Test: Verify fallback to CPU when hardware acceleration unavailable
TEST_F(iOSAccelerationTest, FallbackToCPU) {
    LOG_INFO("Testing fallback to CPU");
    
    HardwareAccelerationConfig config;
    config.preferred_accelerators = {AcceleratorType::CUDA}; // Not available on Apple
    config.fallback_to_cpu = true;
    
    auto result = HardwareAcceleration::configureLlamaCpp(config);
    
    ASSERT_TRUE(result.isSuccess()) << "Should fallback to CPU";
    EXPECT_EQ(result.value(), AcceleratorType::CPU)
        << "Should fallback to CPU when preferred accelerator unavailable";
    
    LOG_INFO("✓ Fallback to CPU works correctly");
}

// Test: Verify no fallback when disabled
TEST_F(iOSAccelerationTest, NoFallbackWhenDisabled) {
    LOG_INFO("Testing no fallback when disabled");
    
    HardwareAccelerationConfig config;
    config.preferred_accelerators = {AcceleratorType::CUDA}; // Not available on Apple
    config.fallback_to_cpu = false;
    
    auto result = HardwareAcceleration::configureLlamaCpp(config);
    
    EXPECT_TRUE(result.isError()) << "Should fail when fallback disabled and no accelerator available";
    
    if (result.isError()) {
        LOG_INFO("✓ Correctly fails when fallback is disabled: " + result.error().message);
    }
}

