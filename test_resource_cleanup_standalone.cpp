/**
 * Standalone Resource Cleanup Test
 * 
 * Tests for Task 11.1: Implement resource cleanup for all components
 * Requirements: 15.1, 15.2, 15.4, 15.5, 15.6
 */

#include "ondeviceai/ondeviceai.hpp"
#include "ondeviceai/sdk_manager.hpp"
#include "ondeviceai/download.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace ondeviceai;

bool fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

void createDummyFile(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    file << content;
    file.close();
}

int main() {
    std::cout << "=== Resource Cleanup Tests ===" << std::endl;
    std::cout << "Testing Task 11.1 implementation" << std::endl << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    // Create test directory
    std::string test_dir = std::filesystem::temp_directory_path() / "cleanup_test";
    std::filesystem::create_directories(test_dir);
    
    // Test 1: SDK Shutdown Releases All Resources (Requirement 15.2)
    std::cout << "Test 1: SDK Shutdown Releases All Resources" << std::endl;
    {
        SDKConfig config;
        config.model_directory = test_dir;
        config.log_level = LogLevel::Warning;
        
        auto sdk_result = SDKManager::initialize(config);
        if (!sdk_result.isSuccess()) {
            std::cout << "  ✗ FAILED: SDK initialization failed" << std::endl;
            failed++;
        } else {
            auto* sdk = sdk_result.value();
            
            // Verify components are accessible
            bool components_ok = (sdk->getModelManager() != nullptr &&
                                 sdk->getLLMEngine() != nullptr &&
                                 sdk->getSTTEngine() != nullptr &&
                                 sdk->getTTSEngine() != nullptr &&
                                 sdk->getVoicePipeline() != nullptr);
            
            SDKManager::shutdown();
            
            // Verify SDK is null after shutdown
            bool shutdown_ok = (SDKManager::getInstance() == nullptr);
            
            if (components_ok && shutdown_ok) {
                std::cout << "  ✓ PASSED: SDK shutdown released all resources" << std::endl;
                passed++;
            } else {
                std::cout << "  ✗ FAILED: SDK shutdown did not complete properly" << std::endl;
                failed++;
            }
        }
    }
    
    // Test 2: Model Unload Returns Error for Invalid Handle (Requirement 15.1)
    std::cout << "\nTest 2: Model Unload Validates Handle" << std::endl;
    {
        SDKConfig config;
        config.model_directory = test_dir;
        config.log_level = LogLevel::Warning;
        
        auto sdk_result = SDKManager::initialize(config);
        if (!sdk_result.isSuccess()) {
            std::cout << "  ✗ FAILED: SDK initialization failed" << std::endl;
            failed++;
        } else {
            auto* sdk = sdk_result.value();
            auto* llm = sdk->getLLMEngine();
            
            // Try to unload non-existent model
            auto unload_result = llm->unloadModel(999);
            
            if (unload_result.isError() && 
                unload_result.error().code == ErrorCode::InvalidInputModelHandle) {
                std::cout << "  ✓ PASSED: Unload correctly validates model handle" << std::endl;
                passed++;
            } else {
                std::cout << "  ✗ FAILED: Unload did not validate handle correctly" << std::endl;
                failed++;
            }
            
            SDKManager::shutdown();
        }
    }
    
    // Test 3: Download Cancellation Cleans Up Temp Files (Requirement 15.6)
    std::cout << "\nTest 3: Download Cancellation Cleans Up Temp Files" << std::endl;
    {
        std::string dest_path = test_dir + "/test_model.bin";
        std::string temp_path = dest_path + ".tmp";
        
        // Create a temporary file
        createDummyFile(temp_path, "Partial download data");
        
        if (!fileExists(temp_path)) {
            std::cout << "  ✗ FAILED: Could not create temp file" << std::endl;
            failed++;
        } else {
            // Create download and cancel it
            {
                ProgressCallback callback = [](double) {};
                Download download(1, "http://example.com/model", dest_path, 1024, callback);
                download.cancel();
            }
            
            // Give it time to clean up
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (!fileExists(temp_path)) {
                std::cout << "  ✓ PASSED: Temp file cleaned up after cancellation" << std::endl;
                passed++;
            } else {
                std::cout << "  ✗ FAILED: Temp file not cleaned up" << std::endl;
                // Clean up manually
                std::filesystem::remove(temp_path);
                failed++;
            }
        }
    }
    
    // Test 4: Download Destructor Cleans Up Incomplete Downloads (Requirement 15.6)
    std::cout << "\nTest 4: Download Destructor Cleans Up Temp Files" << std::endl;
    {
        std::string dest_path = test_dir + "/test_model2.bin";
        std::string temp_path = dest_path + ".tmp";
        
        // Create a temporary file
        createDummyFile(temp_path, "Partial download data");
        
        if (!fileExists(temp_path)) {
            std::cout << "  ✗ FAILED: Could not create temp file" << std::endl;
            failed++;
        } else {
            // Create and destroy download in scope
            {
                ProgressCallback callback = [](double) {};
                Download download(2, "http://example.com/model2", dest_path, 2048, callback);
                // Destructor called here
            }
            
            // Give it time to clean up
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (!fileExists(temp_path)) {
                std::cout << "  ✓ PASSED: Temp file cleaned up by destructor" << std::endl;
                passed++;
            } else {
                std::cout << "  ✗ FAILED: Temp file not cleaned up by destructor" << std::endl;
                // Clean up manually
                std::filesystem::remove(temp_path);
                failed++;
            }
        }
    }
    
    // Test 5: Voice Pipeline Cleanup (Requirement 15.4)
    std::cout << "\nTest 5: Voice Pipeline Cleanup Methods" << std::endl;
    {
        SDKConfig config;
        config.model_directory = test_dir;
        config.log_level = LogLevel::Warning;
        
        auto sdk_result = SDKManager::initialize(config);
        if (!sdk_result.isSuccess()) {
            std::cout << "  ✗ FAILED: SDK initialization failed" << std::endl;
            failed++;
        } else {
            auto* sdk = sdk_result.value();
            auto* pipeline = sdk->getVoicePipeline();
            
            // Test that cleanup methods work even when not active
            auto stop_result = pipeline->stopConversation();
            auto interrupt_result = pipeline->interrupt();
            
            if (stop_result.isSuccess() && interrupt_result.isSuccess()) {
                std::cout << "  ✓ PASSED: Voice pipeline cleanup methods work correctly" << std::endl;
                passed++;
            } else {
                std::cout << "  ✗ FAILED: Voice pipeline cleanup methods failed" << std::endl;
                failed++;
            }
            
            SDKManager::shutdown();
        }
    }
    
    // Test 6: Repeated Initialize and Shutdown (Requirement 15.2)
    std::cout << "\nTest 6: Repeated Initialize and Shutdown" << std::endl;
    {
        bool all_ok = true;
        
        for (int i = 0; i < 3; ++i) {
            SDKConfig config;
            config.model_directory = test_dir;
            config.log_level = LogLevel::Error;
            
            auto sdk_result = SDKManager::initialize(config);
            if (!sdk_result.isSuccess()) {
                all_ok = false;
                break;
            }
            
            SDKManager::shutdown();
            
            if (SDKManager::getInstance() != nullptr) {
                all_ok = false;
                break;
            }
        }
        
        if (all_ok) {
            std::cout << "  ✓ PASSED: Multiple init/shutdown cycles work correctly" << std::endl;
            passed++;
        } else {
            std::cout << "  ✗ FAILED: Multiple init/shutdown cycles failed" << std::endl;
            failed++;
        }
    }
    
    // Clean up test directory
    std::filesystem::remove_all(test_dir);
    
    // Summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Total:  " << (passed + failed) << std::endl;
    
    if (failed == 0) {
        std::cout << "\n✓ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ Some tests failed" << std::endl;
        return 1;
    }
}
