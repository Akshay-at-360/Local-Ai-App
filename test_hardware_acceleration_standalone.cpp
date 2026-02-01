#include "core/include/ondeviceai/hardware_acceleration.hpp"
#include "core/include/ondeviceai/logger.hpp"
#include <iostream>

using namespace ondeviceai;

int main() {
    // Initialize logger
    Logger::getInstance().setLogLevel(LogLevel::Info);
    
    std::cout << "=== Hardware Acceleration Detection Test ===" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Detect available accelerators
    std::cout << "Test 1: Detecting available accelerators..." << std::endl;
    auto accelerators = HardwareAcceleration::detectAvailableAccelerators();
    std::cout << "Found " << accelerators.size() << " accelerator(s):" << std::endl;
    for (const auto& acc : accelerators) {
        std::cout << "  - " << acc.name << ": " 
                  << (acc.available ? "available" : "unavailable") << std::endl;
        if (!acc.details.empty()) {
            std::cout << "    Details: " << acc.details << std::endl;
        }
    }
    std::cout << std::endl;
    
    // Test 2: Get best accelerator
    std::cout << "Test 2: Getting best accelerator..." << std::endl;
    auto best = HardwareAcceleration::getBestAccelerator();
    std::cout << "Best accelerator: " << HardwareAcceleration::getAcceleratorName(best) << std::endl;
    std::cout << std::endl;
    
    // Test 3: Default configuration
    std::cout << "Test 3: Default configuration..." << std::endl;
    auto config = HardwareAccelerationConfig::defaults();
    std::cout << "Preferred accelerators (in priority order):" << std::endl;
    for (size_t i = 0; i < config.preferred_accelerators.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " 
                  << HardwareAcceleration::getAcceleratorName(config.preferred_accelerators[i]) 
                  << std::endl;
    }
    std::cout << "Fallback to CPU: " << (config.fallback_to_cpu ? "enabled" : "disabled") << std::endl;
    std::cout << std::endl;
    
    // Test 4: Configure llama.cpp
    std::cout << "Test 4: Configuring llama.cpp..." << std::endl;
    auto llama_result = HardwareAcceleration::configureLlamaCpp(config);
    if (llama_result.isSuccess()) {
        std::cout << "✓ llama.cpp configured with: " 
                  << HardwareAcceleration::getAcceleratorName(llama_result.value()) << std::endl;
    } else {
        std::cout << "✗ Failed to configure llama.cpp: " << llama_result.error().message << std::endl;
    }
    std::cout << std::endl;
    
    // Test 5: Configure whisper.cpp
    std::cout << "Test 5: Configuring whisper.cpp..." << std::endl;
    auto whisper_result = HardwareAcceleration::configureWhisperCpp(config);
    if (whisper_result.isSuccess()) {
        std::cout << "✓ whisper.cpp configured with: " 
                  << HardwareAcceleration::getAcceleratorName(whisper_result.value()) << std::endl;
    } else {
        std::cout << "✗ Failed to configure whisper.cpp: " << whisper_result.error().message << std::endl;
    }
    std::cout << std::endl;
    
    // Test 6: Configure ONNX Runtime
    std::cout << "Test 6: Configuring ONNX Runtime..." << std::endl;
    auto onnx_result = HardwareAcceleration::configureONNXRuntime(config);
    if (onnx_result.isSuccess()) {
        std::cout << "✓ ONNX Runtime configured with: " 
                  << HardwareAcceleration::getAcceleratorName(onnx_result.value()) << std::endl;
    } else {
        std::cout << "✗ Failed to configure ONNX Runtime: " << onnx_result.error().message << std::endl;
    }
    std::cout << std::endl;
    
    // Test 7: Performance configuration
    std::cout << "Test 7: Performance configuration..." << std::endl;
    auto perf_config = HardwareAccelerationConfig::performance();
    std::cout << "Performance mode preferred accelerators:" << std::endl;
    for (size_t i = 0; i < perf_config.preferred_accelerators.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " 
                  << HardwareAcceleration::getAcceleratorName(perf_config.preferred_accelerators[i]) 
                  << std::endl;
    }
    std::cout << std::endl;
    
    // Test 8: Power efficient configuration
    std::cout << "Test 8: Power efficient configuration..." << std::endl;
    auto power_config = HardwareAccelerationConfig::power_efficient();
    std::cout << "Power efficient mode preferred accelerators:" << std::endl;
    for (size_t i = 0; i < power_config.preferred_accelerators.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " 
                  << HardwareAcceleration::getAcceleratorName(power_config.preferred_accelerators[i]) 
                  << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=== All tests completed ===" << std::endl;
    
    return 0;
}
