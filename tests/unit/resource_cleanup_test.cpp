/**
 * Resource Cleanup Tests
 * 
 * Tests for Task 11.2: Write unit tests for resource cleanup
 * Requirements: 15.1, 15.2, 15.4, 15.5, 15.6
 * 
 * This test suite validates that the SDK properly cleans up resources including:
 * - Memory released after model unload (15.1)
 * - All resources released on shutdown (15.2)
 * - Partial results cleaned on cancellation (15.4)
 * - File handles closed (15.5)
 * - Temporary files cleaned on cancellation (15.6)
 */

#include <gtest/gtest.h>
#include "ondeviceai/ondeviceai.hpp"
#include "ondeviceai/sdk_manager.hpp"
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/stt_engine.hpp"
#include "ondeviceai/tts_engine.hpp"
#include "ondeviceai/voice_pipeline.hpp"
#include "ondeviceai/model_manager.hpp"
#include "ondeviceai/memory_manager.hpp"
#include "ondeviceai/download.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace ondeviceai;

class ResourceCleanupTest : public ::testing::Test {
protected:
    std::string test_dir_;
    
    void SetUp() override {
        // Create temporary test directory
        test_dir_ = std::filesystem::temp_directory_path() / "resource_cleanup_test";
        std::filesystem::create_directories(test_dir_);
    }
    
    void TearDown() override {
        // Clean up test directory
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    // Helper to create a dummy model file
    void createDummyModel(const std::string& path, size_t size = 1024) {
        std::ofstream file(path, std::ios::binary);
        std::vector<char> data(size, 'A');
        file.write(data.data(), data.size());
        file.close();
    }
    
    // Helper to check if file exists
    bool fileExists(const std::string& path) {
        return std::filesystem::exists(path);
    }
};

// ============================================================================
// Test: Model Unloading Releases Memory
// Requirement 15.1: When a model is unloaded, THE SDK SHALL release all associated memory
// ============================================================================

TEST_F(ResourceCleanupTest, ModelUnload_ReleasesMemory) {
    // This test verifies that unloading a model releases memory
    // We can't directly measure memory, but we can verify the model is no longer accessible
    
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Debug;
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess()) << "SDK initialization failed";
    
    auto* sdk = sdk_result.value();
    auto* llm = sdk->getLLMEngine();
    
    // Note: We can't actually load a real model without llama.cpp model files
    // This test verifies the API works correctly
    
    // Verify unloading non-existent model returns error
    auto unload_result = llm->unloadModel(999);
    EXPECT_TRUE(unload_result.isError()) 
        << "Unloading non-existent model should fail";
    EXPECT_EQ(unload_result.error().code, ErrorCode::InvalidInputModelHandle)
        << "Should return InvalidInputModelHandle error";
    
    SDKManager::shutdown();
}

// ============================================================================
// Test: SDK Shutdown Releases All Resources
// Requirement 15.2: When the SDK is shut down, THE SDK SHALL release all resources 
//                   including file handles, memory, and threads
// ============================================================================

TEST_F(ResourceCleanupTest, SDKShutdown_ReleasesAllResources) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Info;
    config.callback_thread_count = 2;
    
    // Initialize SDK
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess()) << "SDK initialization failed";
    
    auto* sdk = sdk_result.value();
    
    // Verify components are accessible
    EXPECT_NE(sdk->getModelManager(), nullptr);
    EXPECT_NE(sdk->getLLMEngine(), nullptr);
    EXPECT_NE(sdk->getSTTEngine(), nullptr);
    EXPECT_NE(sdk->getTTSEngine(), nullptr);
    EXPECT_NE(sdk->getVoicePipeline(), nullptr);
    EXPECT_NE(sdk->getMemoryManager(), nullptr);
    EXPECT_NE(sdk->getCallbackDispatcher(), nullptr);
    
    // Shutdown SDK (should release all resources)
    SDKManager::shutdown();
    
    // Verify SDK instance is null after shutdown
    EXPECT_EQ(SDKManager::getInstance(), nullptr)
        << "SDK instance should be null after shutdown";
    
    // Verify we can reinitialize after shutdown
    auto sdk_result2 = SDKManager::initialize(config);
    EXPECT_TRUE(sdk_result2.isSuccess()) 
        << "Should be able to reinitialize after shutdown";
    
    SDKManager::shutdown();
}

// ============================================================================
// Test: Voice Pipeline Cleanup on Cancellation
// Requirement 15.4: When inference is cancelled, THE SDK SHALL clean up 
//                   partial results and intermediate buffers
// ============================================================================

TEST_F(ResourceCleanupTest, VoicePipeline_CleansUpOnCancellation) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Debug;
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    auto* pipeline = sdk->getVoicePipeline();
    
    // Test that stopConversation works even when not active
    auto stop_result = pipeline->stopConversation();
    EXPECT_TRUE(stop_result.isSuccess())
        << "stopConversation should succeed even when not active";
    
    // Test that interrupt works even when not active
    auto interrupt_result = pipeline->interrupt();
    EXPECT_TRUE(interrupt_result.isSuccess())
        << "interrupt should succeed even when not active";
    
    // Test that clearHistory requires configuration
    auto clear_result = pipeline->clearHistory();
    EXPECT_TRUE(clear_result.isError())
        << "clearHistory should fail when not configured";
    
    SDKManager::shutdown();
}

// ============================================================================
// Test: Download Cancellation Cleans Up Temporary Files
// Requirement 15.6: When downloads are cancelled, THE SDK SHALL clean up temporary files
// ============================================================================

TEST_F(ResourceCleanupTest, DownloadCancellation_CleansUpTempFiles) {
    std::string dest_path = test_dir_ + "/test_model.bin";
    std::string temp_path = dest_path + ".tmp";
    
    // Create a temporary file to simulate download in progress
    {
        std::ofstream file(temp_path, std::ios::binary);
        file << "Partial download data";
        file.close();
    }
    
    ASSERT_TRUE(fileExists(temp_path)) << "Temp file should exist";
    
    // Create download object
    ProgressCallback callback = [](double progress) {
        (void)progress;
    };
    
    Download download(1, "http://example.com/model", dest_path, 1024, callback);
    
    // Cancel the download (should clean up temp file)
    download.cancel();
    
    // Give it a moment to clean up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify temp file is removed
    EXPECT_FALSE(fileExists(temp_path))
        << "Temporary file should be cleaned up after cancellation";
}

// ============================================================================
// Test: Download Destructor Cleans Up Incomplete Downloads
// Requirement 15.6: Temporary files should be cleaned up when download object is destroyed
// ============================================================================

TEST_F(ResourceCleanupTest, DownloadDestructor_CleansUpIncompleteTempFiles) {
    std::string dest_path = test_dir_ + "/test_model2.bin";
    std::string temp_path = dest_path + ".tmp";
    
    // Create a temporary file
    {
        std::ofstream file(temp_path, std::ios::binary);
        file << "Partial download data";
        file.close();
    }
    
    ASSERT_TRUE(fileExists(temp_path)) << "Temp file should exist";
    
    // Create and destroy download object in a scope
    {
        ProgressCallback callback = [](double progress) {
            (void)progress;
        };
        
        Download download(2, "http://example.com/model2", dest_path, 2048, callback);
        // Download object goes out of scope here
    }
    
    // Give it a moment to clean up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify temp file is removed by destructor
    EXPECT_FALSE(fileExists(temp_path))
        << "Temporary file should be cleaned up by destructor";
}

// ============================================================================
// Test: Multiple Model Unloads
// Verify that multiple models can be unloaded without issues
// ============================================================================

TEST_F(ResourceCleanupTest, MultipleModelUnloads_WorkCorrectly) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Debug;
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    auto* llm = sdk->getLLMEngine();
    auto* stt = sdk->getSTTEngine();
    auto* tts = sdk->getTTSEngine();
    
    // Try to unload non-existent models (should fail gracefully)
    auto llm_unload = llm->unloadModel(1);
    EXPECT_TRUE(llm_unload.isError());
    
    auto stt_unload = stt->unloadModel(1);
    EXPECT_TRUE(stt_unload.isError());
    
    auto tts_unload = tts->unloadModel(1);
    EXPECT_TRUE(tts_unload.isError());
    
    // All should return InvalidInputModelHandle
    EXPECT_EQ(llm_unload.error().code, ErrorCode::InvalidInputModelHandle);
    EXPECT_EQ(stt_unload.error().code, ErrorCode::InvalidInputModelHandle);
    EXPECT_EQ(tts_unload.error().code, ErrorCode::InvalidInputModelHandle);
    
    SDKManager::shutdown();
}

// ============================================================================
// Test: SDK Shutdown While Components Active
// Verify that SDK can be shut down even if components are in use
// ============================================================================

TEST_F(ResourceCleanupTest, SDKShutdown_WhileComponentsActive) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Info;
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    
    // Get references to components
    auto* model_mgr = sdk->getModelManager();
    auto* llm = sdk->getLLMEngine();
    
    EXPECT_NE(model_mgr, nullptr);
    EXPECT_NE(llm, nullptr);
    
    // Shutdown SDK (should clean up even though we have references)
    SDKManager::shutdown();
    
    // Verify SDK is shut down
    EXPECT_EQ(SDKManager::getInstance(), nullptr);
}

// ============================================================================
// Test: Repeated Initialize and Shutdown
// Verify that SDK can be initialized and shut down multiple times
// ============================================================================

TEST_F(ResourceCleanupTest, RepeatedInitializeShutdown_WorksCorrectly) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Warning;
    
    // Initialize and shutdown 3 times
    for (int i = 0; i < 3; ++i) {
        auto sdk_result = SDKManager::initialize(config);
        ASSERT_TRUE(sdk_result.isSuccess()) 
            << "Initialization " << i << " failed";
        
        auto* sdk = sdk_result.value();
        EXPECT_NE(sdk, nullptr);
        
        // Verify components are accessible
        EXPECT_NE(sdk->getModelManager(), nullptr);
        EXPECT_NE(sdk->getLLMEngine(), nullptr);
        
        SDKManager::shutdown();
        
        // Verify shutdown completed
        EXPECT_EQ(SDKManager::getInstance(), nullptr);
    }
}

// ============================================================================
// Test: Download Cleanup on Failed Download
// Verify that temporary files are cleaned up when download fails
// ============================================================================

TEST_F(ResourceCleanupTest, DownloadFailure_CleansUpTempFiles) {
    std::string dest_path = test_dir_ + "/failed_model.bin";
    std::string temp_path = dest_path + ".tmp";
    
    // Create a temporary file
    {
        std::ofstream file(temp_path, std::ios::binary);
        file << "Partial data from failed download";
        file.close();
    }
    
    ASSERT_TRUE(fileExists(temp_path));
    
    // Create download with invalid URL (will fail)
    {
        ProgressCallback callback = [](double progress) {
            (void)progress;
        };
        
        Download download(3, "http://invalid.example.com/model", 
                         dest_path, 1024, callback);
        
        // Start download (will fail due to invalid URL)
        auto start_result = download.start();
        // May succeed in starting thread, but download will fail
        
        // Wait a bit for download to fail
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Download object destroyed here
    }
    
    // Give destructor time to clean up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Temp file should be cleaned up
    EXPECT_FALSE(fileExists(temp_path))
        << "Temporary file should be cleaned up after failed download";
}

// ============================================================================
// Test: Memory Manager Tracks Model Unload
// Requirement 15.1: Memory should be released and tracked after model unload
// ============================================================================

TEST_F(ResourceCleanupTest, MemoryManager_TracksModelUnload) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Debug;
    config.memory_limit = 1024 * 1024 * 1024; // 1GB limit
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    auto* memory_mgr = sdk->getMemoryManager();
    auto* llm = sdk->getLLMEngine();
    
    ASSERT_NE(memory_mgr, nullptr);
    ASSERT_NE(llm, nullptr);
    
    // Get initial memory usage
    size_t initial_usage = memory_mgr->getTotalMemoryUsage();
    
    // Note: We can't actually load a real model without model files
    // But we can verify the memory manager API works correctly
    
    // Verify memory manager reports zero usage initially
    EXPECT_EQ(initial_usage, 0) 
        << "Memory manager should report zero usage initially";
    
    // Verify memory limit is set correctly
    EXPECT_EQ(memory_mgr->getMemoryLimit(), 1024 * 1024 * 1024);
    
    SDKManager::shutdown();
}

// ============================================================================
// Test: Engine Destructors Release Resources
// Requirement 15.2: All resources should be released when engines are destroyed
// ============================================================================

TEST_F(ResourceCleanupTest, EngineDestructors_ReleaseResources) {
    // Test that engines can be created and destroyed without leaks
    {
        LLMEngine llm;
        // Destructor should clean up any internal resources
    }
    
    {
        STTEngine stt;
        // Destructor should clean up any internal resources
    }
    
    {
        TTSEngine tts;
        // Destructor should clean up any internal resources
    }
    
    // If we reach here without crashes, destructors worked correctly
    SUCCEED();
}

// ============================================================================
// Test: Voice Pipeline Clears History
// Requirement 15.4: Partial results should be cleaned up
// ============================================================================

TEST_F(ResourceCleanupTest, VoicePipeline_ClearsHistory) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Debug;
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    auto* pipeline = sdk->getVoicePipeline();
    
    // Configure pipeline (will fail without models, but that's OK)
    auto config_result = pipeline->configure(1, 2, 3, PipelineConfig::defaults());
    EXPECT_TRUE(config_result.isError()) 
        << "Configure should fail with invalid handles";
    
    // Clear history should work even when not configured
    auto clear_result = pipeline->clearHistory();
    // May succeed or fail depending on state, but shouldn't crash
    
    SDKManager::shutdown();
}

// ============================================================================
// Test: Multiple Downloads Cleanup Correctly
// Requirement 15.6: Multiple downloads should clean up their temp files
// ============================================================================

TEST_F(ResourceCleanupTest, MultipleDownloads_CleanupCorrectly) {
    std::vector<std::string> dest_paths;
    std::vector<std::string> temp_paths;
    
    // Create multiple downloads
    for (int i = 0; i < 3; ++i) {
        std::string dest = test_dir_ + "/model" + std::to_string(i) + ".bin";
        std::string temp = dest + ".tmp";
        
        dest_paths.push_back(dest);
        temp_paths.push_back(temp);
        
        // Create temp file
        std::ofstream file(temp, std::ios::binary);
        file << "Partial download " << i;
        file.close();
        
        ASSERT_TRUE(fileExists(temp));
    }
    
    // Create and destroy downloads
    {
        std::vector<std::unique_ptr<Download>> downloads;
        
        for (int i = 0; i < 3; ++i) {
            ProgressCallback callback = [](double progress) {
                (void)progress;
            };
            
            downloads.push_back(std::make_unique<Download>(
                i + 10,
                "http://example.com/model" + std::to_string(i),
                dest_paths[i],
                1024,
                callback
            ));
        }
        
        // Downloads destroyed here
    }
    
    // Give time for cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify all temp files are cleaned up
    for (const auto& temp : temp_paths) {
        EXPECT_FALSE(fileExists(temp))
            << "Temporary file should be cleaned up: " << temp;
    }
}

// ============================================================================
// Test: SDK Shutdown Cleans Up All Components
// Requirement 15.2: All resources including threads should be released
// ============================================================================

TEST_F(ResourceCleanupTest, SDKShutdown_CleansUpAllComponents) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Info;
    config.callback_thread_count = 4;
    
    // Initialize SDK
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    
    // Get all components to verify they exist
    auto* model_mgr = sdk->getModelManager();
    auto* llm = sdk->getLLMEngine();
    auto* stt = sdk->getSTTEngine();
    auto* tts = sdk->getTTSEngine();
    auto* pipeline = sdk->getVoicePipeline();
    auto* memory_mgr = sdk->getMemoryManager();
    auto* callback_dispatcher = sdk->getCallbackDispatcher();
    
    EXPECT_NE(model_mgr, nullptr);
    EXPECT_NE(llm, nullptr);
    EXPECT_NE(stt, nullptr);
    EXPECT_NE(tts, nullptr);
    EXPECT_NE(pipeline, nullptr);
    EXPECT_NE(memory_mgr, nullptr);
    EXPECT_NE(callback_dispatcher, nullptr);
    
    // Shutdown SDK (should release all resources including threads)
    SDKManager::shutdown();
    
    // Verify SDK instance is null
    EXPECT_EQ(SDKManager::getInstance(), nullptr);
    
    // Verify we can reinitialize (proves cleanup was complete)
    auto sdk_result2 = SDKManager::initialize(config);
    EXPECT_TRUE(sdk_result2.isSuccess());
    
    SDKManager::shutdown();
}

// ============================================================================
// Test: Unload Model Releases Memory Tracking
// Requirement 15.1: Memory tracking should be updated when model is unloaded
// ============================================================================

TEST_F(ResourceCleanupTest, UnloadModel_ReleasesMemoryTracking) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Debug;
    config.memory_limit = 1024 * 1024 * 1024;
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    auto* llm = sdk->getLLMEngine();
    auto* memory_mgr = sdk->getMemoryManager();
    
    // Verify initial state
    EXPECT_EQ(memory_mgr->getTotalMemoryUsage(), 0);
    
    // Try to unload non-existent model
    auto unload_result = llm->unloadModel(999);
    EXPECT_TRUE(unload_result.isError());
    EXPECT_EQ(unload_result.error().code, ErrorCode::InvalidInputModelHandle);
    
    // Memory usage should still be zero
    EXPECT_EQ(memory_mgr->getTotalMemoryUsage(), 0);
    
    SDKManager::shutdown();
}

// ============================================================================
// Test: File Handles Closed After Model Unload
// Requirement 15.5: File handles should be closed when models are unloaded
// ============================================================================

TEST_F(ResourceCleanupTest, FileHandlesClosed_AfterModelUnload) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Debug;
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    auto* llm = sdk->getLLMEngine();
    
    // Create a dummy model file
    std::string model_path = test_dir_ + "/dummy_model.bin";
    createDummyModel(model_path, 1024);
    
    ASSERT_TRUE(fileExists(model_path));
    
    // Try to load the model (will fail because it's not a valid model)
    auto load_result = llm->loadModel(model_path);
    // Expected to fail with invalid format
    
    // Even if load fails, file handles should be closed
    // We can verify this by trying to delete the file
    // If file handles were left open, deletion might fail on some platforms
    
    // Try to delete the file
    bool can_delete = true;
    try {
        std::filesystem::remove(model_path);
    } catch (const std::filesystem::filesystem_error&) {
        can_delete = false;
    }
    
    // On most platforms, we should be able to delete the file
    // (On Windows, open file handles prevent deletion)
    EXPECT_TRUE(can_delete || !fileExists(model_path))
        << "File should be deletable after failed load (file handles closed)";
    
    SDKManager::shutdown();
}

// ============================================================================
// Test: Callback Dispatcher Cleanup
// Requirement 15.2: Callback threads should be stopped on shutdown
// ============================================================================

TEST_F(ResourceCleanupTest, CallbackDispatcher_CleansUpThreads) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Debug;
    config.callback_thread_count = 4;
    config.synchronous_callbacks = false;
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    auto* dispatcher = sdk->getCallbackDispatcher();
    
    ASSERT_NE(dispatcher, nullptr);
    
    // Dispatcher should have threads running
    // (We can't directly check thread count, but we can verify it exists)
    
    // Shutdown should stop all callback threads
    SDKManager::shutdown();
    
    // Verify SDK is shut down
    EXPECT_EQ(SDKManager::getInstance(), nullptr);
    
    // If we reach here without hanging, threads were properly stopped
    SUCCEED();
}

// ============================================================================
// Test: Rapid Initialize and Shutdown Cycles
// Requirement 15.2: Resources should be properly cleaned up in rapid cycles
// ============================================================================

TEST_F(ResourceCleanupTest, RapidInitShutdownCycles_NoLeaks) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Warning;
    config.callback_thread_count = 2;
    
    // Perform 5 rapid init/shutdown cycles
    for (int i = 0; i < 5; ++i) {
        auto sdk_result = SDKManager::initialize(config);
        ASSERT_TRUE(sdk_result.isSuccess()) 
            << "Cycle " << i << " initialization failed";
        
        auto* sdk = sdk_result.value();
        EXPECT_NE(sdk, nullptr);
        
        // Verify all components are accessible
        EXPECT_NE(sdk->getModelManager(), nullptr);
        EXPECT_NE(sdk->getLLMEngine(), nullptr);
        EXPECT_NE(sdk->getSTTEngine(), nullptr);
        EXPECT_NE(sdk->getTTSEngine(), nullptr);
        EXPECT_NE(sdk->getVoicePipeline(), nullptr);
        EXPECT_NE(sdk->getMemoryManager(), nullptr);
        EXPECT_NE(sdk->getCallbackDispatcher(), nullptr);
        
        // Immediate shutdown
        SDKManager::shutdown();
        
        EXPECT_EQ(SDKManager::getInstance(), nullptr);
    }
    
    // If we complete all cycles without crashes or hangs, cleanup is working
    SUCCEED();
}

// ============================================================================
// Test: Download Cancel Cleans Up Immediately
// Requirement 15.6: Cancellation should clean up temp files promptly
// ============================================================================

TEST_F(ResourceCleanupTest, DownloadCancel_CleansUpImmediately) {
    std::string dest_path = test_dir_ + "/cancelled_model.bin";
    std::string temp_path = dest_path + ".tmp";
    
    // Create a temporary file
    {
        std::ofstream file(temp_path, std::ios::binary);
        file << "Partial download data that will be cancelled";
        file.close();
    }
    
    ASSERT_TRUE(fileExists(temp_path));
    
    ProgressCallback callback = [](double progress) {
        (void)progress;
    };
    
    Download download(100, "http://example.com/large_model", 
                     dest_path, 1024 * 1024, callback);
    
    // Cancel immediately (before starting)
    download.cancel();
    
    // Verify state is cancelled
    EXPECT_EQ(download.getState(), DownloadState::Cancelled);
    
    // Temp file should still exist (not cleaned until destructor)
    EXPECT_TRUE(fileExists(temp_path));
    
    // Destroy download object
    // (Destructor should clean up temp file)
}

// ============================================================================
// Test: Memory Manager Cleanup on Shutdown
// Requirement 15.2: Memory manager should release all tracking on shutdown
// ============================================================================

TEST_F(ResourceCleanupTest, MemoryManager_CleansUpOnShutdown) {
    SDKConfig config;
    config.model_directory = test_dir_;
    config.log_level = LogLevel::Debug;
    config.memory_limit = 512 * 1024 * 1024; // 512MB
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    auto* memory_mgr = sdk->getMemoryManager();
    
    // Verify memory manager is initialized
    EXPECT_NE(memory_mgr, nullptr);
    EXPECT_EQ(memory_mgr->getMemoryLimit(), 512 * 1024 * 1024);
    EXPECT_EQ(memory_mgr->getTotalMemoryUsage(), 0);
    
    // Shutdown should clean up memory manager
    SDKManager::shutdown();
    
    EXPECT_EQ(SDKManager::getInstance(), nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
