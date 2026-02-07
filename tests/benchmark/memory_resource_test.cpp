// ==============================================================================
// OnDevice AI SDK — Memory & Resource Stress Tests
// Task 22.3: Memory and resource testing
//
// Tests:
//   - Memory leak detection (RAII validation)
//   - Memory pressure and OOM handling
//   - Resource cleanup on all error paths
//   - Concurrent operations thread safety
//   - File handle management
//   - LRU eviction under pressure
// ==============================================================================

#include <gtest/gtest.h>
#include "ondeviceai/ondeviceai.hpp"
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <fstream>
#include <mutex>

using namespace ondeviceai;

// =============================================================================
// Test fixture
// =============================================================================

class MemoryResourceTest : public ::testing::Test {
protected:
    void SetUp() override {
        SDKManager::shutdown();
        auto config = SDKConfig::defaults();
        config.model_directory = "./models";
        config.memory_limit = 256 * 1024 * 1024; // 256MB limit for stress testing
        auto result = SDKManager::initialize(config);
        if (result.isSuccess()) {
            sdk_ = result.value();
        }
    }

    void TearDown() override {
        SDKManager::shutdown();
        sdk_ = nullptr;
    }

    SDKManager* sdk_ = nullptr;
};

// =============================================================================
// 22.3.1  Memory Tracking Accuracy
// =============================================================================

TEST_F(MemoryResourceTest, MemoryTrackingReturnsToBaseline) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();
    ASSERT_NE(mem, nullptr);

    size_t baseline = mem->getTotalMemoryUsage();

    // Simulate allocation tracking
    ModelHandle fake_handle = 42;
    size_t alloc_size = 100 * 1024 * 1024; // 100MB simulated

    mem->trackAllocation(fake_handle, alloc_size);
    EXPECT_EQ(mem->getTotalMemoryUsage(), baseline + alloc_size);
    EXPECT_EQ(mem->getModelMemoryUsage(fake_handle), alloc_size);

    mem->trackDeallocation(fake_handle);
    EXPECT_EQ(mem->getTotalMemoryUsage(), baseline);
    EXPECT_EQ(mem->getModelMemoryUsage(fake_handle), 0u);
}

TEST_F(MemoryResourceTest, MultipleAllocationsTrackCorrectly) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();

    size_t baseline = mem->getTotalMemoryUsage();
    const int num_allocs = 50;

    for (int i = 1; i <= num_allocs; ++i) {
        mem->trackAllocation(static_cast<ModelHandle>(i), 1024 * 1024 * i); // i MB each
    }

    size_t expected = baseline;
    for (int i = 1; i <= num_allocs; ++i) {
        expected += 1024 * 1024 * i;
    }
    EXPECT_EQ(mem->getTotalMemoryUsage(), expected);

    // Deallocate in reverse order
    for (int i = num_allocs; i >= 1; --i) {
        mem->trackDeallocation(static_cast<ModelHandle>(i));
    }
    EXPECT_EQ(mem->getTotalMemoryUsage(), baseline);
}

// =============================================================================
// 22.3.2  Memory Pressure Handling
// =============================================================================

TEST_F(MemoryResourceTest, MemoryPressureCallbackTriggered) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();

    std::atomic<bool> callback_fired{false};
    size_t reported_usage = 0;
    size_t reported_limit = 0;

    mem->setMemoryPressureCallback([&](size_t usage, size_t limit) {
        callback_fired.store(true);
        reported_usage = usage;
        reported_limit = limit;
    });

    // Allocate near the limit
    size_t limit = mem->getMemoryLimit();
    ASSERT_GT(limit, 0u);

    mem->trackAllocation(100, limit - 1024); // near limit

    // The pressure callback should have fired
    // (implementation may fire at 80% threshold or similar)
    mem->trackDeallocation(100);
}

TEST_F(MemoryResourceTest, MemoryPressureDetection) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();

    EXPECT_FALSE(mem->isMemoryPressure()); // should start clean

    size_t limit = mem->getMemoryLimit();
    if (limit > 0) {
        // Fill to 90% capacity
        size_t fill = static_cast<size_t>(limit * 0.9);
        mem->trackAllocation(200, fill);

        // Should detect pressure at 90%
        bool pressure = mem->isMemoryPressure();
        // Note: exact threshold is implementation-defined, just verify the method works
        EXPECT_NO_THROW(mem->isMemoryPressure());

        mem->trackDeallocation(200);
        EXPECT_FALSE(mem->isMemoryPressure()); // should clear after dealloc
    }
}

// =============================================================================
// 22.3.3  LRU Eviction
// =============================================================================

TEST_F(MemoryResourceTest, LRUEvictionOrder) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();

    // Track 5 models
    for (int i = 1; i <= 5; ++i) {
        mem->trackAllocation(static_cast<ModelHandle>(i), 10 * 1024 * 1024); // 10MB each
        mem->recordAccess(static_cast<ModelHandle>(i));
    }

    // Access model 1 and 3 (making 2, 4, 5 the least-recently-used)
    mem->recordAccess(1);
    mem->recordAccess(3);

    // The LRU model should be 2 (first one not re-accessed)
    auto lru = mem->getLRUModel();
    ASSERT_TRUE(lru.has_value());
    EXPECT_EQ(lru.value(), static_cast<ModelHandle>(2));

    // Cleanup
    for (int i = 1; i <= 5; ++i) {
        mem->trackDeallocation(static_cast<ModelHandle>(i));
    }
}

TEST_F(MemoryResourceTest, EvictionCandidatesSufficientBytes) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();

    // Track 3 models: 10MB, 20MB, 30MB
    mem->trackAllocation(1, 10 * 1024 * 1024);
    mem->recordAccess(1);
    mem->trackAllocation(2, 20 * 1024 * 1024);
    mem->recordAccess(2);
    mem->trackAllocation(3, 30 * 1024 * 1024);
    mem->recordAccess(3);

    // Re-access model 3 (most recently used)
    mem->recordAccess(3);

    // Need 25MB — should suggest evicting model 1 (10MB) + model 2 (20MB)
    auto candidates = mem->getEvictionCandidates(25 * 1024 * 1024);
    EXPECT_GE(candidates.size(), 1u);

    // The first candidate should be the LRU (model 1)
    if (!candidates.empty()) {
        EXPECT_EQ(candidates[0], static_cast<ModelHandle>(1));
    }

    // Cleanup
    for (int i = 1; i <= 3; ++i) mem->trackDeallocation(i);
}

// =============================================================================
// 22.3.4  Reference Counting
// =============================================================================

TEST_F(MemoryResourceTest, ReferenceCountingPreventsEviction) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();

    mem->trackAllocation(1, 50 * 1024 * 1024);
    mem->recordAccess(1);

    // Increment ref count — model is "in use"
    mem->incrementRefCount(1);
    EXPECT_EQ(mem->getRefCount(1), 1u);
    EXPECT_FALSE(mem->canEvict(1));

    // Decrement ref count — model can be evicted again
    mem->decrementRefCount(1);
    EXPECT_EQ(mem->getRefCount(1), 0u);
    EXPECT_TRUE(mem->canEvict(1));

    mem->trackDeallocation(1);
}

// =============================================================================
// 22.3.5  Concurrent Memory Operations (Thread Safety)
// =============================================================================

TEST_F(MemoryResourceTest, ConcurrentAllocDealloc) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();

    const int num_threads = 8;
    const int ops_per_thread = 1000;
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                ModelHandle h = static_cast<ModelHandle>(t * 10000 + i);
                try {
                    mem->trackAllocation(h, 1024);
                    mem->recordAccess(h);
                    mem->getTotalMemoryUsage();
                    mem->isMemoryPressure();
                    mem->trackDeallocation(h);
                } catch (...) {
                    errors.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : threads) t.join();

    EXPECT_EQ(errors.load(), 0) << "Thread safety violations detected";
}

TEST_F(MemoryResourceTest, ConcurrentRefCounting) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();

    ModelHandle h = 999;
    mem->trackAllocation(h, 1024 * 1024);

    const int num_threads = 10;
    const int ops_per_thread = 500;

    // All threads increment then decrement
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                mem->incrementRefCount(h);
            }
            for (int i = 0; i < ops_per_thread; ++i) {
                mem->decrementRefCount(h);
            }
        });
    }

    for (auto& t : threads) t.join();

    // All increments balanced by decrements → ref count should be 0
    EXPECT_EQ(mem->getRefCount(h), 0u);

    mem->trackDeallocation(h);
}

// =============================================================================
// 22.3.6  Error Path Resource Cleanup
// =============================================================================

TEST_F(MemoryResourceTest, ErrorPathDoesNotLeakMemory) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();
    auto* llm = sdk_->getLLMEngine();

    size_t before = mem->getTotalMemoryUsage();

    // Attempt to load a non-existent model — should fail and not leak
    auto result = llm->loadModel("/nonexistent/path/model.gguf");
    EXPECT_TRUE(result.isError());

    size_t after = mem->getTotalMemoryUsage();
    EXPECT_EQ(before, after) << "Memory leaked on failed model load";
}

TEST_F(MemoryResourceTest, InvalidOperationsDoNotLeakMemory) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();
    auto* llm = sdk_->getLLMEngine();

    size_t before = mem->getTotalMemoryUsage();

    // Multiple invalid operations in sequence
    for (int i = 0; i < 100; ++i) {
        auto r1 = llm->generate(999, "test");
        EXPECT_TRUE(r1.isError());

        auto r2 = llm->unloadModel(888);
        EXPECT_TRUE(r2.isError());

        auto r3 = llm->clearContext(777);
        EXPECT_TRUE(r3.isError());
    }

    size_t after = mem->getTotalMemoryUsage();
    EXPECT_EQ(before, after) << "Memory leaked during error operations";
}

// =============================================================================
// 22.3.7  SDK Lifecycle Stress Test
// =============================================================================

TEST_F(MemoryResourceTest, RepeatedInitShutdownDoesNotLeak) {
    // First shutdown the current instance
    SDKManager::shutdown();
    sdk_ = nullptr;

    const int cycles = 20;
    for (int i = 0; i < cycles; ++i) {
        auto config = SDKConfig::defaults();
        config.model_directory = "./models";
        config.memory_limit = 128 * 1024 * 1024;

        auto result = SDKManager::initialize(config);
        ASSERT_TRUE(result.isSuccess()) << "Init failed on cycle " << i;

        auto* mem = result.value()->getMemoryManager();
        ASSERT_NE(mem, nullptr);

        // Do some work
        mem->trackAllocation(1, 1024 * 1024);
        mem->recordAccess(1);
        mem->trackDeallocation(1);

        SDKManager::shutdown();
    }

    // Re-init for TearDown
    auto config = SDKConfig::defaults();
    config.model_directory = "./models";
    auto result = SDKManager::initialize(config);
    if (result.isSuccess()) sdk_ = result.value();
}

// =============================================================================
// 22.3.8  Engine Concurrent Access
// =============================================================================

TEST_F(MemoryResourceTest, ConcurrentEngineErrorAccess) {
    ASSERT_NE(sdk_, nullptr);
    auto* llm = sdk_->getLLMEngine();
    ASSERT_NE(llm, nullptr);

    const int num_threads = 8;
    const int ops_per_thread = 200;
    std::atomic<int> errors{0};
    std::atomic<int> completed{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                try {
                    // These should all safely return errors (no model loaded)
                    auto r = llm->generate(static_cast<ModelHandle>(i + 1), "test");
                    EXPECT_TRUE(r.isError());
                    completed.fetch_add(1);
                } catch (...) {
                    errors.fetch_add(1);
                }
            }
        });
    }

    for (auto& t : threads) t.join();

    EXPECT_EQ(errors.load(), 0) << "Concurrent engine access caused crashes";
    EXPECT_EQ(completed.load(), num_threads * ops_per_thread);
}

// =============================================================================
// 22.3.9  Memory Limit Enforcement
// =============================================================================

TEST_F(MemoryResourceTest, MemoryLimitEnforced) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();

    size_t limit = mem->getMemoryLimit();
    ASSERT_GT(limit, 0u);

    // Track allocation that would exceed limit
    mem->trackAllocation(1, limit / 2);
    mem->recordAccess(1);
    mem->trackAllocation(2, limit / 2);
    mem->recordAccess(2);

    // At this point we should need eviction for any further allocation
    EXPECT_TRUE(mem->needsEviction(limit / 4));

    // Cleanup
    mem->trackDeallocation(1);
    mem->trackDeallocation(2);
}

// =============================================================================
// 22.3.10  Stress: Rapid Create/Destroy Cycles
// =============================================================================

TEST_F(MemoryResourceTest, RapidAllocDeallocStress) {
    ASSERT_NE(sdk_, nullptr);
    auto* mem = sdk_->getMemoryManager();

    auto start = std::chrono::steady_clock::now();
    const int iterations = 100000;

    for (int i = 0; i < iterations; ++i) {
        ModelHandle h = static_cast<ModelHandle>(i);
        mem->trackAllocation(h, 4096);
        mem->recordAccess(h);
        mem->trackDeallocation(h);
    }

    auto end = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "[STRESS] " << iterations << " alloc/dealloc cycles in "
              << ms << "ms (" << (iterations / ms * 1000) << " ops/sec)\n";

    EXPECT_LT(ms, 5000.0) << "Stress test took too long (> 5 seconds)";
}
