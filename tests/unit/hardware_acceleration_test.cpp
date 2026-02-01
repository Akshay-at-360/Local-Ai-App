#include <gtest/gtest.h>
#include "ondeviceai/hardware_acceleration.hpp"
#include "ondeviceai/logger.hpp"

using namespace ondeviceai;

class HardwareAccelerationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for tests
        Logger::getInstance().setLogLevel(LogLevel::Debug);
    }
};

// Test: Accelerator detection
TEST_F(HardwareAccelerationTest, DetectAvailableAccelerators) {
    auto accelerators = HardwareAcceleration::detectAvailableAccelerators();
    
    // CPU should always be available
    ASSERT_FALSE(accelerators.empty());
    
    bool cpu_found = false;
    for (const auto& acc : accelerators) {
        if (acc.type == AcceleratorType::CPU) {
            cpu_found = true;
            EXPECT_TRUE(acc.available);
            EXPECT_EQ(acc.name, "CPU");
        }
    }
    
    EXPECT_TRUE(cpu_found) << "CPU accelerator should always be available";
    
    // Log all detected accelerators
    std::cout << "Detected " << accelerators.size() << " accelerator(s):" << std::endl;
    for (const auto& acc : accelerators) {
        std::cout << "  - " << acc.name << ": " 
                  << (acc.available ? "available" : "unavailable") << std::endl;
        if (!acc.details.empty()) {
            std::cout << "    Details: " << acc.details << std::endl;
        }
    }
}

// Test: CPU is always available
TEST_F(HardwareAccelerationTest, CPUAlwaysAvailable) {
    EXPECT_TRUE(HardwareAcceleration::isAcceleratorAvailable(AcceleratorType::CPU));
}

// Test: Get accelerator names
TEST_F(HardwareAccelerationTest, GetAcceleratorNames) {
    EXPECT_EQ(HardwareAcceleration::getAcceleratorName(AcceleratorType::CPU), "CPU");
    EXPECT_EQ(HardwareAcceleration::getAcceleratorName(AcceleratorType::Metal), "Metal");
    EXPECT_EQ(HardwareAcceleration::getAcceleratorName(AcceleratorType::CoreML), 
              "Core ML / Neural Engine");
    EXPECT_EQ(HardwareAcceleration::getAcceleratorName(AcceleratorType::NNAPI), "NNAPI");
    EXPECT_EQ(HardwareAcceleration::getAcceleratorName(AcceleratorType::Vulkan), "Vulkan");
    EXPECT_EQ(HardwareAcceleration::getAcceleratorName(AcceleratorType::OpenCL), "OpenCL");
    EXPECT_EQ(HardwareAcceleration::getAcceleratorName(AcceleratorType::WebGPU), "WebGPU");
}

// Test: Get best accelerator
TEST_F(HardwareAccelerationTest, GetBestAccelerator) {
    auto best = HardwareAcceleration::getBestAccelerator();
    
    // Should return a valid accelerator type
    EXPECT_TRUE(best == AcceleratorType::CPU ||
                best == AcceleratorType::Metal ||
                best == AcceleratorType::CoreML ||
                best == AcceleratorType::NNAPI ||
                best == AcceleratorType::Vulkan ||
                best == AcceleratorType::OpenCL ||
                best == AcceleratorType::WebGPU);
    
    // The best accelerator should be available
    EXPECT_TRUE(HardwareAcceleration::isAcceleratorAvailable(best));
    
    std::cout << "Best accelerator: " << HardwareAcceleration::getAcceleratorName(best) 
              << std::endl;
}

// Test: Default configuration
TEST_F(HardwareAccelerationTest, DefaultConfiguration) {
    auto config = HardwareAccelerationConfig::defaults();
    
    // Should have at least CPU in preferred accelerators
    ASSERT_FALSE(config.preferred_accelerators.empty());
    
    // CPU should be in the list (as fallback)
    bool cpu_in_list = false;
    for (auto type : config.preferred_accelerators) {
        if (type == AcceleratorType::CPU) {
            cpu_in_list = true;
            break;
        }
    }
    EXPECT_TRUE(cpu_in_list);
    
    // Fallback to CPU should be enabled by default
    EXPECT_TRUE(config.fallback_to_cpu);
    
    std::cout << "Default configuration has " << config.preferred_accelerators.size() 
              << " preferred accelerator(s)" << std::endl;
}

// Test: Performance configuration
TEST_F(HardwareAccelerationTest, PerformanceConfiguration) {
    auto config = HardwareAccelerationConfig::performance();
    
    ASSERT_FALSE(config.preferred_accelerators.empty());
    EXPECT_TRUE(config.fallback_to_cpu);
    
#ifdef PLATFORM_ANDROID
    // Performance mode should prefer fast execution
    EXPECT_EQ(config.platform_options.nnapi_execution_preference, 0);
#endif
}

// Test: Power efficient configuration
TEST_F(HardwareAccelerationTest, PowerEfficientConfiguration) {
    auto config = HardwareAccelerationConfig::power_efficient();
    
    ASSERT_FALSE(config.preferred_accelerators.empty());
    EXPECT_TRUE(config.fallback_to_cpu);
    
#ifdef PLATFORM_ANDROID
    // Power efficient mode should prefer low power
    EXPECT_EQ(config.platform_options.nnapi_execution_preference, 2);
#endif
}

// Test: Configure llama.cpp
TEST_F(HardwareAccelerationTest, ConfigureLlamaCpp) {
    auto config = HardwareAccelerationConfig::defaults();
    auto result = HardwareAcceleration::configureLlamaCpp(config);
    
    // Should succeed (at least with CPU fallback)
    ASSERT_TRUE(result.isSuccess());
    
    auto accel_type = result.value();
    std::cout << "llama.cpp configured with: " 
              << HardwareAcceleration::getAcceleratorName(accel_type) << std::endl;
    
    // The configured accelerator should be available
    EXPECT_TRUE(HardwareAcceleration::isAcceleratorAvailable(accel_type));
}

// Test: Configure whisper.cpp
TEST_F(HardwareAccelerationTest, ConfigureWhisperCpp) {
    auto config = HardwareAccelerationConfig::defaults();
    auto result = HardwareAcceleration::configureWhisperCpp(config);
    
    // Should succeed (at least with CPU fallback)
    ASSERT_TRUE(result.isSuccess());
    
    auto accel_type = result.value();
    std::cout << "whisper.cpp configured with: " 
              << HardwareAcceleration::getAcceleratorName(accel_type) << std::endl;
    
    // The configured accelerator should be available
    EXPECT_TRUE(HardwareAcceleration::isAcceleratorAvailable(accel_type));
}

// Test: Configure ONNX Runtime
TEST_F(HardwareAccelerationTest, ConfigureONNXRuntime) {
    auto config = HardwareAccelerationConfig::defaults();
    auto result = HardwareAcceleration::configureONNXRuntime(config);
    
    // Should succeed (at least with CPU fallback)
    ASSERT_TRUE(result.isSuccess());
    
    auto accel_type = result.value();
    std::cout << "ONNX Runtime configured with: " 
              << HardwareAcceleration::getAcceleratorName(accel_type) << std::endl;
    
    // The configured accelerator should be available
    EXPECT_TRUE(HardwareAcceleration::isAcceleratorAvailable(accel_type));
}

// Test: Fallback to CPU when no acceleration available
TEST_F(HardwareAccelerationTest, FallbackToCPU) {
    HardwareAccelerationConfig config;
    
    // Configure with only unavailable accelerators
    config.preferred_accelerators = {AcceleratorType::CUDA}; // Not supported
    config.fallback_to_cpu = true;
    
    auto result = HardwareAcceleration::configureLlamaCpp(config);
    
    // Should succeed with CPU fallback
    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value(), AcceleratorType::CPU);
}

// Test: No fallback when disabled
TEST_F(HardwareAccelerationTest, NoFallbackWhenDisabled) {
    HardwareAccelerationConfig config;
    
    // Configure with only unavailable accelerators and no fallback
    config.preferred_accelerators = {AcceleratorType::CUDA}; // Not supported
    config.fallback_to_cpu = false;
    
    auto result = HardwareAcceleration::configureLlamaCpp(config);
    
    // Should fail without fallback
    EXPECT_TRUE(result.isError());
    if (result.isError()) {
        EXPECT_EQ(result.error().code, ErrorCode::InferenceHardwareAccelerationFailure);
    }
}

// Test: Platform-specific accelerators
TEST_F(HardwareAccelerationTest, PlatformSpecificAccelerators) {
#ifdef PLATFORM_APPLE
    // On Apple platforms, Metal and/or CoreML should be available
    bool apple_accel_available = 
        HardwareAcceleration::isAcceleratorAvailable(AcceleratorType::Metal) ||
        HardwareAcceleration::isAcceleratorAvailable(AcceleratorType::CoreML);
    
    std::cout << "Apple platform - Metal available: " 
              << HardwareAcceleration::isAcceleratorAvailable(AcceleratorType::Metal) << std::endl;
    std::cout << "Apple platform - CoreML available: " 
              << HardwareAcceleration::isAcceleratorAvailable(AcceleratorType::CoreML) << std::endl;
    
    // Note: This might fail in CI environments without GPU access
    // So we just log the result rather than asserting
    if (!apple_accel_available) {
        std::cout << "Warning: No Apple hardware acceleration detected (may be expected in CI)" 
                  << std::endl;
    }
#endif

#ifdef PLATFORM_ANDROID
    // On Android, NNAPI should be available
    bool nnapi_available = HardwareAcceleration::isAcceleratorAvailable(AcceleratorType::NNAPI);
    std::cout << "Android platform - NNAPI available: " << nnapi_available << std::endl;
#endif
}

// Test: Configuration priority order
TEST_F(HardwareAccelerationTest, ConfigurationPriorityOrder) {
    auto config = HardwareAccelerationConfig::defaults();
    
    // The first accelerator in the list should be tried first
    ASSERT_FALSE(config.preferred_accelerators.empty());
    
    // Try to configure with the default priority
    auto result = HardwareAcceleration::configureLlamaCpp(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto selected = result.value();
    
    // The selected accelerator should be in the preferred list
    bool found = false;
    for (auto type : config.preferred_accelerators) {
        if (type == selected) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Selected accelerator should be in preferred list";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
