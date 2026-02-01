// Standalone test for iOS Core ML and Metal Acceleration
// Task 15.3: Integrate Core ML acceleration
// Requirements: 18.1, 18.2

#include "ondeviceai/hardware_acceleration.hpp"
#include "ondeviceai/logger.hpp"
#include <iostream>

using namespace ondeviceai;

int main() {
    Logger::getInstance().setLogLevel(LogLevel::Info);
    
    std::cout << "\n=== iOS Core ML and Metal Acceleration Test ===\n" << std::endl;
    
    // Test 1: Detect available accelerators
    std::cout << "Test 1: Detecting available accelerators..." << std::endl;
    auto accelerators = HardwareAcceleration::detectAvailableAccelerators();
    
    bool has_cpu = false;
    bool has_metal = false;
    bool has_coreml = false;
    
    for (const auto& acc : accelerators) {
        std::cout << "  - " << acc.name << ": " << (acc.available ? "available" : "unavailable");
        if (!acc.details.empty()) {
            std::cout << " (" << acc.details << ")";
        }
        std::cout << std::endl;
        
        if (acc.type == AcceleratorType::CPU) has_cpu = true;
        if (acc.type == AcceleratorType::Metal) has_metal = true;
        if (acc.type == AcceleratorType::CoreML) has_coreml = true;
    }
    
    if (!has_cpu) {
        std::cerr << "ERROR: CPU not detected (should always be available)" << std::endl;
        return 1;
    }
    std::cout << "✓ CPU detected" << std::endl;
    
#ifdef __APPLE__
    if (!has_metal && !has_coreml) {
        std::cerr << "WARNING: Neither Metal nor Core ML detected on Apple platform" << std::endl;
    } else {
        if (has_metal) std::cout << "✓ Metal detected" << std::endl;
        if (has_coreml) std::cout << "✓ Core ML detected" << std::endl;
    }
#endif
    
    // Test 2: Configure llama.cpp with Metal
    std::cout << "\nTest 2: Configuring llama.cpp..." << std::endl;
    auto config = HardwareAccelerationConfig::defaults();
    auto llama_result = HardwareAcceleration::configureLlamaCpp(config);
    
    if (llama_result.isError()) {
        std::cerr << "ERROR: llama.cpp configuration failed: " 
                  << llama_result.error().message << std::endl;
        return 1;
    }
    
    AcceleratorType llama_accel = llama_result.value();
    std::cout << "  llama.cpp configured with: " 
              << HardwareAcceleration::getAcceleratorName(llama_accel) << std::endl;
    
#ifdef __APPLE__
    if (llama_accel == AcceleratorType::Metal) {
        std::cout << "✓ llama.cpp using Metal acceleration" << std::endl;
    } else if (llama_accel == AcceleratorType::CPU) {
        std::cout << "⚠ llama.cpp using CPU (Metal not available or not configured)" << std::endl;
    }
#endif
    
    // Test 3: Configure whisper.cpp with Core ML
    std::cout << "\nTest 3: Configuring whisper.cpp..." << std::endl;
    auto whisper_result = HardwareAcceleration::configureWhisperCpp(config);
    
    if (whisper_result.isError()) {
        std::cerr << "ERROR: whisper.cpp configuration failed: " 
                  << whisper_result.error().message << std::endl;
        return 1;
    }
    
    AcceleratorType whisper_accel = whisper_result.value();
    std::cout << "  whisper.cpp configured with: " 
              << HardwareAcceleration::getAcceleratorName(whisper_accel) << std::endl;
    
#ifdef __APPLE__
    if (whisper_accel == AcceleratorType::CoreML) {
        std::cout << "✓ whisper.cpp using Core ML acceleration" << std::endl;
    } else if (whisper_accel == AcceleratorType::Metal) {
        std::cout << "✓ whisper.cpp using Metal acceleration" << std::endl;
    } else if (whisper_accel == AcceleratorType::CPU) {
        std::cout << "⚠ whisper.cpp using CPU (hardware acceleration not available or not configured)" << std::endl;
    }
#endif
    
    // Test 4: Check best accelerator
    std::cout << "\nTest 4: Determining best accelerator..." << std::endl;
    auto best = HardwareAcceleration::getBestAccelerator();
    std::cout << "  Best accelerator: " << HardwareAcceleration::getAcceleratorName(best) << std::endl;
    
#ifdef __APPLE__
    if (best != AcceleratorType::CPU) {
        std::cout << "✓ Hardware acceleration is available and selected" << std::endl;
    } else {
        std::cout << "⚠ Best accelerator is CPU (hardware acceleration may not be available)" << std::endl;
    }
#endif
    
    // Test 5: Check iOS-specific configuration
#if defined(__APPLE__) && (TARGET_OS_IOS || TARGET_OS_IPHONE)
    std::cout << "\nTest 5: Checking iOS-specific configuration..." << std::endl;
    auto ios_config = HardwareAccelerationConfig::defaults();
    
    if (!ios_config.preferred_accelerators.empty()) {
        std::cout << "  Preferred accelerators on iOS:" << std::endl;
        for (size_t i = 0; i < ios_config.preferred_accelerators.size(); ++i) {
            std::cout << "    " << (i + 1) << ". " 
                      << HardwareAcceleration::getAcceleratorName(ios_config.preferred_accelerators[i]) 
                      << std::endl;
        }
        
        if (ios_config.preferred_accelerators[0] == AcceleratorType::CoreML) {
            std::cout << "✓ Core ML (Neural Engine) is the first preference on iOS" << std::endl;
        }
    }
    
    // Check power efficient configuration
    auto power_config = HardwareAccelerationConfig::power_efficient();
    if (!power_config.preferred_accelerators.empty() && 
        power_config.preferred_accelerators[0] == AcceleratorType::CoreML) {
        std::cout << "✓ Power efficient mode prefers Neural Engine" << std::endl;
    }
#endif
    
    // Test 6: Verify fallback behavior
    std::cout << "\nTest 6: Testing fallback behavior..." << std::endl;
    HardwareAccelerationConfig fallback_config;
    fallback_config.preferred_accelerators = {AcceleratorType::CUDA}; // Not available on Apple
    fallback_config.fallback_to_cpu = true;
    
    auto fallback_result = HardwareAcceleration::configureLlamaCpp(fallback_config);
    if (fallback_result.isSuccess() && fallback_result.value() == AcceleratorType::CPU) {
        std::cout << "✓ Fallback to CPU works correctly" << std::endl;
    } else {
        std::cerr << "ERROR: Fallback to CPU failed" << std::endl;
        return 1;
    }
    
    // Summary
    std::cout << "\n=== Test Summary ===\n" << std::endl;
    std::cout << "All tests passed successfully!" << std::endl;
    
#ifdef __APPLE__
    std::cout << "\nHardware Acceleration Status:" << std::endl;
    std::cout << "  - Metal: " << (has_metal ? "Available" : "Not available") << std::endl;
    std::cout << "  - Core ML: " << (has_coreml ? "Available" : "Not available") << std::endl;
    std::cout << "  - llama.cpp: Using " << HardwareAcceleration::getAcceleratorName(llama_accel) << std::endl;
    std::cout << "  - whisper.cpp: Using " << HardwareAcceleration::getAcceleratorName(whisper_accel) << std::endl;
    
    if ((llama_accel == AcceleratorType::Metal || llama_accel == AcceleratorType::CoreML) &&
        (whisper_accel == AcceleratorType::CoreML || whisper_accel == AcceleratorType::Metal)) {
        std::cout << "\n✓✓✓ Core ML/Metal acceleration is properly configured! ✓✓✓" << std::endl;
    } else {
        std::cout << "\n⚠ Hardware acceleration may not be fully configured" << std::endl;
    }
#endif
    
    return 0;
}
