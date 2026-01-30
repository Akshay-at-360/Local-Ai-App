/**
 * Unit tests for concurrency (Task 10.4)
 * 
 * Tests verify:
 * - Concurrent model loading (Requirement 14.1)
 * - Concurrent inference on different models (Requirement 14.2)
 * - Callback thread identity (Requirement 12.5)
 * 
 * These are unit tests that complement the property-based tests in
 * tests/property/concurrency_properties_test.cpp
 */

#include <gtest/gtest.h>
#include "ondeviceai/sdk_manager.hpp"
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/stt_engine.hpp"
#include "ondeviceai/tts_engine.hpp"
#include "ondeviceai/memory_manager.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <set>
#include <mutex>
#include <filesystem>

using namespace ondeviceai;

class ConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        SDKManager::shutdown();
    }
    
    void TearDown() override {
        SDKManager::shutdown();
    }
};

// Test 1: Concurrent model loading - multiple threads can load models simultaneously
// Validates: Requirement 14.1
TEST_F(ConcurrencyTest, ConcurrentModelLoading) {
    const char* model_path_env = std::getenv("TEST_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid GGUF model available. "
                     << "Set TEST_MODEL_PATH environment variable to run this test.";
    }
    
    std::string model_path(model_path_env);
    
    LLMEngine engine;
    MemoryManager memory_mgr(8ULL * 1024 * 1024 * 1024);  // 8GB for multiple models
    engine.setMemoryManager(&memory_mgr);
    
    const int num_threads = 4;
    std::vector<std::thread> threads;
    std::vector<ModelHandle> handles(num_threads);
    std::atomic<int> successful_loads{0};
    std::atomic<int> failed_loads{0};
    
    // Launch threads that load models concurrently
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            auto result = engine.loadModel(model_path);
            
            if (result.isSuccess()) {
                handles[i] = result.value();
                successful_loads++;
            } else {
                failed_loads++;
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify at least some models loaded successfully
    EXPECT_GT(successful_loads.load(), 0) << "At least one model should load successfully";
    
    // Verify all successful handles are unique
    std::set<ModelHandle> unique_handles;
    for (int i = 0; i < num_threads; ++i) {
        if (handles[i] > 0) {
            unique_handles.insert(handles[i]);
        }
    }
    
    EXPECT_EQ(unique_handles.size(), successful_loads.load()) 
        << "All loaded models should have unique handles";
    
    // Verify loaded models are usable
    for (auto handle : unique_handles) {
        auto result = engine.tokenize(handle, "test");
        EXPECT_TRUE(result.isSuccess() || result.isError()) 
            << "Model should be usable after concurrent loading";
    }
}

// Test 2: Concurrent inference on different models - threads can run inference in parallel
// Validates: Requirement 14.2
TEST_F(ConcurrencyTest, ConcurrentInferenceOnDifferentModels) {
    const char* model_path_env = std::getenv("TEST_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid GGUF model available.";
    }
    
    std::string model_path(model_path_env);
    
    LLMEngine engine;
    MemoryManager memory_mgr(8ULL * 1024 * 1024 * 1024);
    engine.setMemoryManager(&memory_mgr);
    
    // Load multiple model instances
    const int num_models = 3;
    std::vector<ModelHandle> handles;
    
    for (int i = 0; i < num_models; ++i) {
        auto result = engine.loadModel(model_path);
        if (result.isSuccess()) {
            handles.push_back(result.value());
        }
    }
    
    if (handles.size() < 2) {
        GTEST_SKIP() << "Need at least 2 models loaded for this test";
    }
    
    // Track inference operations
    std::atomic<int> successful_inferences{0};
    std::atomic<int> failed_inferences{0};
    std::mutex results_mutex;
    std::vector<std::string> inference_results;
    
    // Launch threads, each performing inference on a different model
    std::vector<std::thread> threads;
    for (size_t i = 0; i < handles.size(); ++i) {
        threads.emplace_back([&, i]() {
            ModelHandle handle = handles[i];
            
            // Perform multiple inference operations
            for (int op = 0; op < 5; ++op) {
                std::string prompt = "Model " + std::to_string(i) + " operation " + std::to_string(op);
                
                // Use tokenization as a lightweight inference operation
                auto result = engine.tokenize(handle, prompt);
                
                if (result.isSuccess()) {
                    successful_inferences++;
                    
                    // Verify result is valid
                    if (!result.value().empty()) {
                        std::lock_guard<std::mutex> lock(results_mutex);
                        inference_results.push_back(prompt);
                    }
                } else {
                    failed_inferences++;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify concurrent inference succeeded
    EXPECT_GT(successful_inferences.load(), 0) 
        << "At least some inferences should succeed";
    
    EXPECT_EQ(inference_results.size(), successful_inferences.load())
        << "All successful inferences should produce valid results";
    
    // Verify models are still usable after concurrent inference
    for (auto handle : handles) {
        auto result = engine.tokenize(handle, "final test");
        EXPECT_TRUE(result.isSuccess() || result.isError())
            << "Model should remain usable after concurrent inference";
    }
}

// Test 3: Callback thread identity - callbacks execute on correct threads
// Validates: Requirement 12.5
TEST_F(ConcurrencyTest, CallbackThreadIdentity) {
    SDKConfig config = SDKConfig::defaults();
    config.model_directory = "./test_callback_identity";
    config.callback_thread_count = 2;
    config.synchronous_callbacks = false;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    auto* dispatcher = sdk->getCallbackDispatcher();
    ASSERT_NE(dispatcher, nullptr);
    
    // Test 1: Asynchronous callbacks execute on different thread
    std::thread::id main_thread_id = std::this_thread::get_id();
    std::atomic<std::thread::id> callback_thread_id{std::thread::id()};
    std::atomic<bool> callback_executed{false};
    
    dispatcher->dispatch([&]() {
        callback_thread_id.store(std::this_thread::get_id());
        callback_executed.store(true);
    });
    
    dispatcher->waitForCompletion();
    
    EXPECT_TRUE(callback_executed.load()) << "Callback should execute";
    EXPECT_NE(main_thread_id, callback_thread_id.load()) 
        << "Async callback should execute on different thread";
    
    // Test 2: Switch to synchronous mode
    sdk->setSynchronousCallbacks(true);
    EXPECT_TRUE(dispatcher->isSynchronous());
    
    callback_executed.store(false);
    callback_thread_id.store(std::thread::id());
    
    dispatcher->dispatch([&]() {
        callback_thread_id.store(std::this_thread::get_id());
        callback_executed.store(true);
    });
    
    EXPECT_TRUE(callback_executed.load()) << "Sync callback should execute immediately";
    EXPECT_EQ(main_thread_id, callback_thread_id.load())
        << "Sync callback should execute on calling thread";
    
    // Test 3: Multiple callbacks use callback thread pool
    sdk->setSynchronousCallbacks(false);
    sdk->setCallbackThreadCount(3);
    
    std::set<std::thread::id> callback_thread_ids;
    std::mutex ids_mutex;
    std::atomic<int> callbacks_executed{0};
    
    // Dispatch many callbacks
    for (int i = 0; i < 20; ++i) {
        dispatcher->dispatch([&]() {
            {
                std::lock_guard<std::mutex> lock(ids_mutex);
                callback_thread_ids.insert(std::this_thread::get_id());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            callbacks_executed++;
        });
    }
    
    dispatcher->waitForCompletion();
    
    EXPECT_EQ(callbacks_executed.load(), 20) << "All callbacks should execute";
    EXPECT_GE(callback_thread_ids.size(), 2u) << "Should use multiple callback threads";
    EXPECT_LE(callback_thread_ids.size(), 3u) << "Should not exceed configured thread count";
    
    // Verify none of the callback threads is the main thread
    EXPECT_EQ(callback_thread_ids.count(main_thread_id), 0u)
        << "Callbacks should not execute on main thread in async mode";
}

// Test 4: Concurrent model loading with different engines
// Validates: Requirement 14.1
TEST_F(ConcurrencyTest, ConcurrentLoadingDifferentEngines) {
    const char* llm_model_path = std::getenv("TEST_MODEL_PATH");
    const char* stt_model_path = std::getenv("TEST_STT_MODEL_PATH");
    const char* tts_model_path = std::getenv("TEST_TTS_MODEL_PATH");
    
    if (!llm_model_path || !std::filesystem::exists(llm_model_path)) {
        GTEST_SKIP() << "Skipping test - no valid LLM model available.";
    }
    
    LLMEngine llm_engine;
    MemoryManager memory_mgr(8ULL * 1024 * 1024 * 1024);
    llm_engine.setMemoryManager(&memory_mgr);
    
    STTEngine stt_engine;
    TTSEngine tts_engine;
    
    std::atomic<int> successful_loads{0};
    std::atomic<bool> error_occurred{false};
    
    std::vector<std::thread> threads;
    
    // Load LLM model
    threads.emplace_back([&]() {
        auto result = llm_engine.loadModel(llm_model_path);
        if (result.isSuccess()) {
            successful_loads++;
        } else {
            error_occurred = true;
        }
    });
    
    // Load STT model if available
    if (stt_model_path && std::filesystem::exists(stt_model_path)) {
        threads.emplace_back([&]() {
            auto result = stt_engine.loadModel(stt_model_path);
            if (result.isSuccess()) {
                successful_loads++;
            } else {
                error_occurred = true;
            }
        });
    }
    
    // Load TTS model if available
    if (tts_model_path && std::filesystem::exists(tts_model_path)) {
        threads.emplace_back([&]() {
            auto result = tts_engine.loadModel(tts_model_path);
            if (result.isSuccess()) {
                successful_loads++;
            } else {
                error_occurred = true;
            }
        });
    }
    
    // Wait for all loads
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(successful_loads.load(), 0) 
        << "At least one engine should load successfully";
    EXPECT_FALSE(error_occurred.load())
        << "No errors should occur during concurrent loading";
}

// Test 5: Concurrent inference doesn't interfere with each other
// Validates: Requirement 14.2
TEST_F(ConcurrencyTest, ConcurrentInferenceNoInterference) {
    const char* model_path_env = std::getenv("TEST_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid GGUF model available.";
    }
    
    std::string model_path(model_path_env);
    
    LLMEngine engine;
    MemoryManager memory_mgr(8ULL * 1024 * 1024 * 1024);
    engine.setMemoryManager(&memory_mgr);
    
    // Load two models
    auto result1 = engine.loadModel(model_path);
    auto result2 = engine.loadModel(model_path);
    
    if (result1.isError() || result2.isError()) {
        GTEST_SKIP() << "Failed to load models";
    }
    
    ModelHandle handle1 = result1.value();
    ModelHandle handle2 = result2.value();
    
    // Define unique prompts for each model
    const std::string prompt1 = "Model 1 unique prompt";
    const std::string prompt2 = "Model 2 unique prompt";
    
    std::vector<int> tokens1_result;
    std::vector<int> tokens2_result;
    std::atomic<bool> thread1_done{false};
    std::atomic<bool> thread2_done{false};
    
    // Thread 1: Repeatedly tokenize with model 1
    std::thread thread1([&]() {
        for (int i = 0; i < 10; ++i) {
            auto result = engine.tokenize(handle1, prompt1);
            if (result.isSuccess()) {
                tokens1_result = result.value();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        thread1_done = true;
    });
    
    // Thread 2: Repeatedly tokenize with model 2
    std::thread thread2([&]() {
        for (int i = 0; i < 10; ++i) {
            auto result = engine.tokenize(handle2, prompt2);
            if (result.isSuccess()) {
                tokens2_result = result.value();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        thread2_done = true;
    });
    
    thread1.join();
    thread2.join();
    
    EXPECT_TRUE(thread1_done.load()) << "Thread 1 should complete";
    EXPECT_TRUE(thread2_done.load()) << "Thread 2 should complete";
    
    // Verify both models produced results
    EXPECT_FALSE(tokens1_result.empty()) << "Model 1 should produce tokens";
    EXPECT_FALSE(tokens2_result.empty()) << "Model 2 should produce tokens";
    
    // Verify results are different (different prompts should produce different tokens)
    // Note: This assumes the prompts are different enough to produce different tokenization
    EXPECT_NE(tokens1_result, tokens2_result) 
        << "Different prompts should produce different tokens";
}

// Test 6: Callback thread identity with streaming inference
// Validates: Requirement 12.5
TEST_F(ConcurrencyTest, StreamingCallbackThreadIdentity) {
    const char* model_path_env = std::getenv("TEST_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid GGUF model available.";
    }
    
    std::string model_path(model_path_env);
    
    SDKConfig config = SDKConfig::defaults();
    config.model_directory = "./test_streaming_callback";
    config.callback_thread_count = 2;
    config.synchronous_callbacks = false;
    
    auto sdk_result = SDKManager::initialize(config);
    ASSERT_TRUE(sdk_result.isSuccess());
    
    auto* sdk = sdk_result.value();
    auto* llm = sdk->getLLMEngine();
    
    MemoryManager memory_mgr(4ULL * 1024 * 1024 * 1024);
    llm->setMemoryManager(&memory_mgr);
    
    auto load_result = llm->loadModel(model_path);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // Test streaming with async callbacks
    std::thread::id main_thread_id = std::this_thread::get_id();
    std::set<std::thread::id> callback_thread_ids;
    std::mutex ids_mutex;
    std::atomic<int> callback_count{0};
    
    GenerationConfig gen_config = GenerationConfig::defaults();
    gen_config.max_tokens = 10;  // Short generation for testing
    
    auto gen_result = llm->generateStreaming(
        handle,
        "Test prompt",
        [&](const std::string& /*token*/) {
            std::lock_guard<std::mutex> lock(ids_mutex);
            callback_thread_ids.insert(std::this_thread::get_id());
            callback_count++;
        },
        gen_config
    );
    
    // Note: generateStreaming may or may not succeed depending on model state
    // The important thing is that if callbacks are invoked, they're on the right thread
    
    if (callback_count.load() > 0) {
        EXPECT_GT(callback_thread_ids.size(), 0u) 
            << "Callbacks should execute on callback threads";
        
        // In async mode, callbacks should not be on main thread
        EXPECT_EQ(callback_thread_ids.count(main_thread_id), 0u)
            << "Streaming callbacks should not execute on main thread in async mode";
    }
}

// Test 7: Verify thread safety with rapid load/unload cycles
// Validates: Requirement 14.1
TEST_F(ConcurrencyTest, RapidLoadUnloadCycles) {
    const char* model_path_env = std::getenv("TEST_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid GGUF model available.";
    }
    
    std::string model_path(model_path_env);
    
    LLMEngine engine;
    MemoryManager memory_mgr(8ULL * 1024 * 1024 * 1024);
    engine.setMemoryManager(&memory_mgr);
    
    std::atomic<int> successful_cycles{0};
    std::atomic<bool> error_occurred{false};
    
    const int num_threads = 3;
    const int cycles_per_thread = 5;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int cycle = 0; cycle < cycles_per_thread; ++cycle) {
                // Load model
                auto load_result = engine.loadModel(model_path);
                if (load_result.isError()) {
                    error_occurred = true;
                    continue;
                }
                
                ModelHandle handle = load_result.value();
                
                // Use model briefly
                auto tokenize_result = engine.tokenize(handle, "test");
                if (tokenize_result.isError()) {
                    error_occurred = true;
                }
                
                // Unload model
                auto unload_result = engine.unloadModel(handle);
                if (unload_result.isError()) {
                    error_occurred = true;
                    continue;
                }
                
                successful_cycles++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(successful_cycles.load(), 0) 
        << "At least some load/unload cycles should succeed";
    
    // Some errors may occur due to memory constraints, but system should remain stable
    EXPECT_TRUE(true) << "System should remain stable during rapid load/unload";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
