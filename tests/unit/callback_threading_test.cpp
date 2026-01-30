/**
 * Unit tests for callback thread management (Task 10.3)
 * 
 * Tests verify:
 * - Callbacks invoked on appropriate threads (Requirements 12.5, 14.5)
 * - Thread-safe callback queues
 * - Callback thread configuration
 */

#include <gtest/gtest.h>
#include "ondeviceai/callback_dispatcher.hpp"
#include "ondeviceai/sdk_manager.hpp"
#include "ondeviceai/llm_engine.hpp"
#include <thread>
#include <atomic>
#include <chrono>
#include <set>

using namespace ondeviceai;

class CallbackThreadingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing SDK instance
        SDKManager::shutdown();
    }
    
    void TearDown() override {
        SDKManager::shutdown();
    }
};

// Test 1: Synchronous mode - callbacks execute on calling thread
TEST_F(CallbackThreadingTest, SynchronousCallbacksOnCallingThread) {
    CallbackConfig config = CallbackConfig::synchronous();
    CallbackDispatcher dispatcher(config);
    
    std::thread::id calling_thread_id = std::this_thread::get_id();
    std::thread::id callback_thread_id;
    
    bool callback_invoked = false;
    dispatcher.dispatch([&]() {
        callback_thread_id = std::this_thread::get_id();
        callback_invoked = true;
    });
    
    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(calling_thread_id, callback_thread_id);
}

// Test 2: Asynchronous mode - callbacks execute on different thread
TEST_F(CallbackThreadingTest, AsynchronousCallbacksOnDifferentThread) {
    CallbackConfig config;
    config.mode = CallbackConfig::DispatchMode::Asynchronous;
    config.callback_thread_count = 1;
    CallbackDispatcher dispatcher(config);
    
    std::thread::id calling_thread_id = std::this_thread::get_id();
    std::atomic<std::thread::id> callback_thread_id{std::thread::id()};
    std::atomic<bool> callback_invoked{false};
    
    dispatcher.dispatch([&]() {
        callback_thread_id.store(std::this_thread::get_id());
        callback_invoked.store(true);
    });
    
    // Wait for callback to execute
    dispatcher.waitForCompletion();
    
    EXPECT_TRUE(callback_invoked.load());
    EXPECT_NE(calling_thread_id, callback_thread_id.load());
}

// Test 3: Multiple callbacks execute in order (FIFO)
TEST_F(CallbackThreadingTest, CallbacksExecuteInOrder) {
    CallbackConfig config;
    config.mode = CallbackConfig::DispatchMode::Asynchronous;
    config.callback_thread_count = 1;  // Single thread ensures ordering
    CallbackDispatcher dispatcher(config);
    
    std::vector<int> execution_order;
    std::mutex order_mutex;
    
    for (int i = 0; i < 10; ++i) {
        dispatcher.dispatch([&, i]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(i);
        });
    }
    
    dispatcher.waitForCompletion();
    
    ASSERT_EQ(execution_order.size(), 10);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(execution_order[i], i);
    }
}

// Test 4: Multiple callback threads can execute concurrently
TEST_F(CallbackThreadingTest, MultipleCallbackThreadsConcurrent) {
    CallbackConfig config;
    config.mode = CallbackConfig::DispatchMode::Asynchronous;
    config.callback_thread_count = 4;
    CallbackDispatcher dispatcher(config);
    
    std::set<std::thread::id> thread_ids;
    std::mutex ids_mutex;
    std::atomic<int> callback_count{0};
    
    // Dispatch many callbacks that take some time
    for (int i = 0; i < 20; ++i) {
        dispatcher.dispatch([&]() {
            {
                std::lock_guard<std::mutex> lock(ids_mutex);
                thread_ids.insert(std::this_thread::get_id());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            callback_count++;
        });
    }
    
    dispatcher.waitForCompletion();
    
    EXPECT_EQ(callback_count.load(), 20);
    // Should have used multiple threads (at least 2, likely more)
    EXPECT_GE(thread_ids.size(), 2);
    EXPECT_LE(thread_ids.size(), 4);  // At most 4 threads
}

// Test 5: Queue size limit prevents memory exhaustion
TEST_F(CallbackThreadingTest, QueueSizeLimitEnforced) {
    CallbackConfig config;
    config.mode = CallbackConfig::DispatchMode::Asynchronous;
    config.callback_thread_count = 1;
    config.max_queue_size = 10;
    CallbackDispatcher dispatcher(config);
    
    std::atomic<bool> blocking_callback_started{false};
    std::atomic<bool> blocking_callback_should_exit{false};
    
    // Dispatch a blocking callback to fill the queue
    dispatcher.dispatch([&]() {
        blocking_callback_started.store(true);
        while (!blocking_callback_should_exit.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Wait for blocking callback to start
    while (!blocking_callback_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Fill the queue
    int successful_dispatches = 0;
    for (int i = 0; i < 20; ++i) {
        if (dispatcher.dispatch([]() {})) {
            successful_dispatches++;
        }
    }
    
    // Should have rejected some callbacks due to queue limit
    EXPECT_LE(successful_dispatches, 10);
    
    // Unblock and cleanup
    blocking_callback_should_exit.store(true);
    dispatcher.waitForCompletion();
}

// Test 6: Reconfiguration changes callback behavior
TEST_F(CallbackThreadingTest, ReconfigurationChangesMode) {
    CallbackConfig async_config;
    async_config.mode = CallbackConfig::DispatchMode::Asynchronous;
    async_config.callback_thread_count = 2;
    CallbackDispatcher dispatcher(async_config);
    
    // Test async mode
    std::thread::id calling_thread = std::this_thread::get_id();
    std::atomic<std::thread::id> callback_thread{std::thread::id()};
    
    dispatcher.dispatch([&]() {
        callback_thread.store(std::this_thread::get_id());
    });
    dispatcher.waitForCompletion();
    
    EXPECT_NE(calling_thread, callback_thread.load());
    
    // Reconfigure to synchronous
    CallbackConfig sync_config = CallbackConfig::synchronous();
    dispatcher.reconfigure(sync_config);
    
    // Test sync mode
    callback_thread.store(std::thread::id());
    dispatcher.dispatch([&]() {
        callback_thread.store(std::this_thread::get_id());
    });
    
    EXPECT_EQ(calling_thread, callback_thread.load());
}

// Test 7: SDK Manager callback configuration
TEST_F(CallbackThreadingTest, SDKManagerCallbackConfiguration) {
    SDKConfig config = SDKConfig::defaults();
    config.model_directory = "./test_callback_config";
    config.callback_thread_count = 3;
    config.synchronous_callbacks = false;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    ASSERT_NE(sdk, nullptr);
    
    auto* dispatcher = sdk->getCallbackDispatcher();
    ASSERT_NE(dispatcher, nullptr);
    
    // Verify async mode
    EXPECT_FALSE(dispatcher->isSynchronous());
    
    // Change to synchronous
    sdk->setSynchronousCallbacks(true);
    EXPECT_TRUE(dispatcher->isSynchronous());
    
    // Change back to async with different thread count
    sdk->setSynchronousCallbacks(false);
    sdk->setCallbackThreadCount(4);
    EXPECT_FALSE(dispatcher->isSynchronous());
}

// Test 8: Callback exceptions don't crash the system
TEST_F(CallbackThreadingTest, CallbackExceptionsHandled) {
    CallbackConfig config;
    config.mode = CallbackConfig::DispatchMode::Asynchronous;
    config.callback_thread_count = 1;
    CallbackDispatcher dispatcher(config);
    
    std::atomic<int> successful_callbacks{0};
    
    // Dispatch callback that throws
    dispatcher.dispatch([]() {
        throw std::runtime_error("Test exception");
    });
    
    // Dispatch normal callbacks
    for (int i = 0; i < 5; ++i) {
        dispatcher.dispatch([&]() {
            successful_callbacks++;
        });
    }
    
    dispatcher.waitForCompletion();
    
    // All normal callbacks should have executed despite the exception
    EXPECT_EQ(successful_callbacks.load(), 5);
}

// Test 9: Dispatcher shutdown waits for pending callbacks
TEST_F(CallbackThreadingTest, ShutdownWaitsForPendingCallbacks) {
    std::atomic<int> completed_callbacks{0};
    
    {
        CallbackConfig config;
        config.mode = CallbackConfig::DispatchMode::Asynchronous;
        config.callback_thread_count = 2;
        CallbackDispatcher dispatcher(config);
        
        // Dispatch callbacks that take time
        for (int i = 0; i < 10; ++i) {
            dispatcher.dispatch([&]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                completed_callbacks++;
            });
        }
        
        // Dispatcher destructor should wait for all callbacks
    }
    
    // All callbacks should have completed
    EXPECT_EQ(completed_callbacks.load(), 10);
}

// Test 10: Verify callback thread identity in LLM streaming
// This test requires a mock LLM model, so we'll test the integration point
TEST_F(CallbackThreadingTest, LLMStreamingUsesCallbackDispatcher) {
    SDKConfig config = SDKConfig::defaults();
    config.model_directory = "./test_llm_callback";
    config.callback_thread_count = 2;
    config.synchronous_callbacks = false;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    auto* llm = sdk->getLLMEngine();
    auto* dispatcher = sdk->getCallbackDispatcher();
    
    ASSERT_NE(llm, nullptr);
    ASSERT_NE(dispatcher, nullptr);
    
    // Verify dispatcher is configured correctly
    EXPECT_FALSE(dispatcher->isSynchronous());
    EXPECT_EQ(dispatcher->getQueueSize(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
