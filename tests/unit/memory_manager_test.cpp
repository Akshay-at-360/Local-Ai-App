#include <gtest/gtest.h>
#include "ondeviceai/memory_manager.hpp"

using namespace ondeviceai;

TEST(MemoryManagerTest, ConstructionWithNoLimit) {
    MemoryManager manager(0);
    EXPECT_EQ(manager.getTotalMemoryUsage(), 0);
    EXPECT_FALSE(manager.isMemoryPressure());
}

TEST(MemoryManagerTest, ConstructionWithLimit) {
    MemoryManager manager(1024 * 1024 * 1024); // 1GB
    EXPECT_EQ(manager.getTotalMemoryUsage(), 0);
    EXPECT_FALSE(manager.isMemoryPressure());
}

TEST(MemoryManagerTest, TrackAllocation) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    EXPECT_EQ(manager.getTotalMemoryUsage(), 1000);
    EXPECT_EQ(manager.getModelMemoryUsage(1), 1000);
}

TEST(MemoryManagerTest, TrackMultipleAllocations) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    manager.trackAllocation(2, 2000);
    manager.trackAllocation(3, 3000);
    
    EXPECT_EQ(manager.getTotalMemoryUsage(), 6000);
    EXPECT_EQ(manager.getModelMemoryUsage(1), 1000);
    EXPECT_EQ(manager.getModelMemoryUsage(2), 2000);
    EXPECT_EQ(manager.getModelMemoryUsage(3), 3000);
}

TEST(MemoryManagerTest, TrackDeallocation) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    manager.trackAllocation(2, 2000);
    EXPECT_EQ(manager.getTotalMemoryUsage(), 3000);
    
    manager.trackDeallocation(1);
    EXPECT_EQ(manager.getTotalMemoryUsage(), 2000);
    EXPECT_EQ(manager.getModelMemoryUsage(1), 0);
}

TEST(MemoryManagerTest, MemoryPressureDetection) {
    MemoryManager manager(1000);
    
    manager.trackAllocation(1, 800);
    EXPECT_FALSE(manager.isMemoryPressure());
    
    manager.trackAllocation(2, 150);
    EXPECT_TRUE(manager.isMemoryPressure()); // 950 > 900 (90% of 1000)
}

TEST(MemoryManagerTest, LRUTracking) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    manager.trackAllocation(2, 2000);
    manager.trackAllocation(3, 3000);
    
    manager.recordAccess(1);
    manager.recordAccess(2);
    manager.recordAccess(3);
    
    // LRU should be model 1
    auto lru = manager.getLRUModel();
    ASSERT_TRUE(lru.has_value());
    EXPECT_EQ(lru.value(), 1);
}

TEST(MemoryManagerTest, LRUUpdateOnAccess) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    manager.trackAllocation(2, 2000);
    manager.trackAllocation(3, 3000);
    
    manager.recordAccess(1);
    manager.recordAccess(2);
    manager.recordAccess(3);
    
    // Access model 1 again
    manager.recordAccess(1);
    
    // Now LRU should be model 2
    auto lru = manager.getLRUModel();
    ASSERT_TRUE(lru.has_value());
    EXPECT_EQ(lru.value(), 2);
}

TEST(MemoryManagerTest, MemoryPressureCallback) {
    MemoryManager manager(1000);
    
    bool callback_invoked = false;
    size_t callback_usage = 0;
    size_t callback_limit = 0;
    
    manager.setMemoryPressureCallback([&](size_t usage, size_t limit) {
        callback_invoked = true;
        callback_usage = usage;
        callback_limit = limit;
    });
    
    // Allocate below threshold - callback should not be invoked
    manager.trackAllocation(1, 800);
    EXPECT_FALSE(callback_invoked);
    
    // Allocate above threshold - callback should be invoked
    manager.trackAllocation(2, 150);
    EXPECT_TRUE(callback_invoked);
    EXPECT_EQ(callback_usage, 950);
    EXPECT_EQ(callback_limit, 1000);
}

TEST(MemoryManagerTest, MemoryPressureCallbackOnLimitChange) {
    MemoryManager manager(2000);
    
    bool callback_invoked = false;
    size_t callback_usage = 0;
    size_t callback_limit = 0;
    
    manager.setMemoryPressureCallback([&](size_t usage, size_t limit) {
        callback_invoked = true;
        callback_usage = usage;
        callback_limit = limit;
    });
    
    // Allocate memory
    manager.trackAllocation(1, 1500);
    EXPECT_FALSE(callback_invoked); // 1500 < 1800 (90% of 2000)
    
    // Lower the limit to trigger pressure
    manager.setMemoryLimit(1600);
    EXPECT_TRUE(callback_invoked); // 1500 > 1440 (90% of 1600)
    EXPECT_EQ(callback_usage, 1500);
    EXPECT_EQ(callback_limit, 1600);
}

TEST(MemoryManagerTest, GetModelMemoryUsageForNonExistentModel) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    
    // Query non-existent model
    EXPECT_EQ(manager.getModelMemoryUsage(999), 0);
}

TEST(MemoryManagerTest, TrackDeallocationOfNonExistentModel) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    EXPECT_EQ(manager.getTotalMemoryUsage(), 1000);
    
    // Deallocate non-existent model - should not crash or affect total
    manager.trackDeallocation(999);
    EXPECT_EQ(manager.getTotalMemoryUsage(), 1000);
}

TEST(MemoryManagerTest, LRUEmptyList) {
    MemoryManager manager(0);
    
    // Get LRU from empty list
    auto lru = manager.getLRUModel();
    EXPECT_FALSE(lru.has_value());
}

TEST(MemoryManagerTest, MemoryLimitZeroMeansNoLimit) {
    MemoryManager manager(0);
    
    // Allocate large amount with no limit
    manager.trackAllocation(1, 1000000000); // 1GB
    EXPECT_FALSE(manager.isMemoryPressure());
    EXPECT_EQ(manager.getTotalMemoryUsage(), 1000000000);
}

// New tests for reference counting (Requirement 17.4)
TEST(MemoryManagerTest, ReferenceCountingBasic) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    EXPECT_EQ(manager.getRefCount(1), 0);
    
    manager.incrementRefCount(1);
    EXPECT_EQ(manager.getRefCount(1), 1);
    
    manager.incrementRefCount(1);
    EXPECT_EQ(manager.getRefCount(1), 2);
    
    manager.decrementRefCount(1);
    EXPECT_EQ(manager.getRefCount(1), 1);
    
    manager.decrementRefCount(1);
    EXPECT_EQ(manager.getRefCount(1), 0);
}

TEST(MemoryManagerTest, CanEvictWithZeroReferences) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    EXPECT_TRUE(manager.canEvict(1));
    
    manager.incrementRefCount(1);
    EXPECT_FALSE(manager.canEvict(1));
    
    manager.decrementRefCount(1);
    EXPECT_TRUE(manager.canEvict(1));
}

TEST(MemoryManagerTest, GetEvictionCandidatesRespectsReferences) {
    MemoryManager manager(10000);
    
    // Allocate 3 models
    manager.trackAllocation(1, 3000);
    manager.trackAllocation(2, 3000);
    manager.trackAllocation(3, 3000);
    
    // Record access order: 1, 2, 3
    manager.recordAccess(1);
    manager.recordAccess(2);
    manager.recordAccess(3);
    
    // Add reference to model 1 (LRU)
    manager.incrementRefCount(1);
    
    // Try to get eviction candidates for 4000 bytes
    auto candidates = manager.getEvictionCandidates(4000);
    
    // Should not include model 1 (has reference)
    EXPECT_FALSE(std::find(candidates.begin(), candidates.end(), 1) != candidates.end());
    
    // Should include model 2 (LRU without references)
    EXPECT_TRUE(std::find(candidates.begin(), candidates.end(), 2) != candidates.end());
}

TEST(MemoryManagerTest, GetEvictionCandidatesLRUOrder) {
    MemoryManager manager(10000);
    
    // Allocate 3 models
    manager.trackAllocation(1, 2000);
    manager.trackAllocation(2, 2000);
    manager.trackAllocation(3, 2000);
    
    // Record access order: 1, 2, 3
    manager.recordAccess(1);
    manager.recordAccess(2);
    manager.recordAccess(3);
    
    // Try to get eviction candidates for 5000 bytes
    // Current usage: 6000, limit: 10000
    // After adding 5000: 11000 (exceeds limit by 1000)
    // Need to free at least 1000 bytes
    auto candidates = manager.getEvictionCandidates(5000);
    
    // Should return at least model 1 (LRU, 2000 bytes)
    EXPECT_GE(candidates.size(), 1);
    if (candidates.size() >= 1) {
        EXPECT_EQ(candidates[0], 1); // LRU should be first
    }
}

TEST(MemoryManagerTest, NeedsEvictionCheck) {
    MemoryManager manager(10000);
    
    manager.trackAllocation(1, 6000);
    
    // Adding 3000 bytes would be within limit
    EXPECT_FALSE(manager.needsEviction(3000));
    
    // Adding 5000 bytes would exceed limit
    EXPECT_TRUE(manager.needsEviction(5000));
}

TEST(MemoryManagerTest, NeedsEvictionWithNoLimit) {
    MemoryManager manager(0); // No limit
    
    manager.trackAllocation(1, 6000);
    
    // Should never need eviction with no limit
    EXPECT_FALSE(manager.needsEviction(1000000000));
}

TEST(MemoryManagerTest, ReferenceCountDecrementBelowZero) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    EXPECT_EQ(manager.getRefCount(1), 0);
    
    // Decrement when already at 0 - should stay at 0
    manager.decrementRefCount(1);
    EXPECT_EQ(manager.getRefCount(1), 0);
}

TEST(MemoryManagerTest, GetRefCountForNonExistentModel) {
    MemoryManager manager(0);
    
    // Query non-existent model
    EXPECT_EQ(manager.getRefCount(999), 0);
}

TEST(MemoryManagerTest, DeallocationClearsRefCount) {
    MemoryManager manager(0);
    
    manager.trackAllocation(1, 1000);
    manager.incrementRefCount(1);
    manager.incrementRefCount(1);
    EXPECT_EQ(manager.getRefCount(1), 2);
    
    manager.trackDeallocation(1);
    
    // After deallocation, refcount should be cleared
    EXPECT_EQ(manager.getRefCount(1), 0);
}

// Task 4.4: Unit tests for memory management
// Requirement 8.4: Memory pressure triggers unloading
TEST(MemoryManagerTest, MemoryPressureTriggersUnloading) {
    MemoryManager manager(10000);
    
    // Allocate models that fit within limit
    manager.trackAllocation(1, 3000);
    manager.trackAllocation(2, 3000);
    manager.trackAllocation(3, 3000);
    
    // Record access order: 1, 2, 3 (1 is LRU)
    manager.recordAccess(1);
    manager.recordAccess(2);
    manager.recordAccess(3);
    
    EXPECT_EQ(manager.getTotalMemoryUsage(), 9000);
    EXPECT_FALSE(manager.isMemoryPressure());
    
    // Try to allocate more memory that would exceed limit
    // This should trigger the need for eviction
    EXPECT_TRUE(manager.needsEviction(3000)); // 9000 + 3000 = 12000 > 10000
    
    // Get eviction candidates - should return LRU models
    auto candidates = manager.getEvictionCandidates(3000);
    EXPECT_FALSE(candidates.empty());
    
    // Verify that evicting the candidates would free enough memory
    size_t freed = 0;
    for (auto handle : candidates) {
        freed += manager.getModelMemoryUsage(handle);
    }
    EXPECT_GE(freed, 2000); // Need to free at least 2000 bytes
    
    // Simulate unloading the LRU model
    manager.trackDeallocation(candidates[0]);
    
    // Now we should have room for the new allocation
    EXPECT_FALSE(manager.needsEviction(3000));
}

// Requirement 8.3: Lazy loading behavior
TEST(MemoryManagerTest, LazyLoadingBehavior) {
    MemoryManager manager(10000);
    
    // Initially no models loaded
    EXPECT_EQ(manager.getTotalMemoryUsage(), 0);
    
    // Simulate lazy loading - model is only loaded when first accessed
    // Model 1 is not loaded yet
    EXPECT_EQ(manager.getModelMemoryUsage(1), 0);
    
    // First access triggers loading
    manager.trackAllocation(1, 3000);
    manager.recordAccess(1);
    EXPECT_EQ(manager.getModelMemoryUsage(1), 3000);
    EXPECT_EQ(manager.getTotalMemoryUsage(), 3000);
    
    // Model 2 is still not loaded
    EXPECT_EQ(manager.getModelMemoryUsage(2), 0);
    
    // Access model 2 triggers its loading
    manager.trackAllocation(2, 2000);
    manager.recordAccess(2);
    EXPECT_EQ(manager.getModelMemoryUsage(2), 2000);
    EXPECT_EQ(manager.getTotalMemoryUsage(), 5000);
    
    // Verify that models are only loaded when needed, not all at once
    EXPECT_EQ(manager.getModelMemoryUsage(3), 0); // Model 3 never accessed, not loaded
}

// Requirement 8.3: Lazy unloading when not in use
TEST(MemoryManagerTest, LazyUnloadingWhenNotInUse) {
    MemoryManager manager(10000);
    
    // Load multiple models
    manager.trackAllocation(1, 3000);
    manager.trackAllocation(2, 3000);
    manager.trackAllocation(3, 3000);
    
    manager.recordAccess(1);
    manager.recordAccess(2);
    manager.recordAccess(3);
    
    EXPECT_EQ(manager.getTotalMemoryUsage(), 9000);
    
    // Increment reference for model 3 (in use)
    manager.incrementRefCount(3);
    
    // When memory pressure occurs, only unused models should be candidates for unloading
    auto candidates = manager.getEvictionCandidates(3000);
    
    // Model 3 should not be in candidates (it's in use)
    EXPECT_FALSE(std::find(candidates.begin(), candidates.end(), 3) != candidates.end());
    
    // Models 1 and 2 should be candidates (not in use)
    EXPECT_TRUE(std::find(candidates.begin(), candidates.end(), 1) != candidates.end() ||
                std::find(candidates.begin(), candidates.end(), 2) != candidates.end());
    
    // Decrement reference for model 3
    manager.decrementRefCount(3);
    
    // Now model 3 can be evicted
    EXPECT_TRUE(manager.canEvict(3));
}

// Requirement 8.6: Memory callbacks invoked
TEST(MemoryManagerTest, MemoryCallbacksInvoked) {
    MemoryManager manager(10000);
    
    int callback_count = 0;
    size_t last_usage = 0;
    size_t last_limit = 0;
    
    manager.setMemoryPressureCallback([&](size_t usage, size_t limit) {
        callback_count++;
        last_usage = usage;
        last_limit = limit;
    });
    
    // Allocate below pressure threshold (90%)
    manager.trackAllocation(1, 5000);
    EXPECT_EQ(callback_count, 0); // No callback yet
    
    // Allocate to reach pressure threshold
    manager.trackAllocation(2, 4100); // Total: 9100 > 9000 (90% of 10000)
    EXPECT_EQ(callback_count, 1); // Callback invoked once
    EXPECT_EQ(last_usage, 9100);
    EXPECT_EQ(last_limit, 10000);
    
    // Allocate more - callback should be invoked again
    manager.trackAllocation(3, 500); // Total: 9600
    EXPECT_EQ(callback_count, 2); // Callback invoked again
    EXPECT_EQ(last_usage, 9600);
    EXPECT_EQ(last_limit, 10000);
}

// Requirement 8.6: Memory callbacks invoked on limit change
TEST(MemoryManagerTest, MemoryCallbacksInvokedOnLimitChange) {
    MemoryManager manager(20000);
    
    int callback_count = 0;
    size_t last_usage = 0;
    size_t last_limit = 0;
    
    manager.setMemoryPressureCallback([&](size_t usage, size_t limit) {
        callback_count++;
        last_usage = usage;
        last_limit = limit;
    });
    
    // Allocate memory below pressure threshold
    manager.trackAllocation(1, 10000);
    EXPECT_EQ(callback_count, 0); // 10000 < 18000 (90% of 20000)
    
    // Lower the limit to trigger pressure
    manager.setMemoryLimit(11000);
    EXPECT_EQ(callback_count, 1); // 10000 > 9900 (90% of 11000)
    EXPECT_EQ(last_usage, 10000);
    EXPECT_EQ(last_limit, 11000);
    
    // Raise the limit to relieve pressure
    manager.setMemoryLimit(20000);
    EXPECT_EQ(callback_count, 1); // No new callback (no pressure)
    
    // Lower again to trigger pressure
    manager.setMemoryLimit(10000);
    EXPECT_EQ(callback_count, 2); // 10000 > 9000 (90% of 10000)
    EXPECT_EQ(last_usage, 10000);
    EXPECT_EQ(last_limit, 10000);
}

// Requirement 8.7: Graceful degradation on OOM
TEST(MemoryManagerTest, GracefulDegradationOnOOM) {
    MemoryManager manager(10000);
    
    // Allocate models up to the limit
    manager.trackAllocation(1, 3000);
    manager.trackAllocation(2, 3000);
    manager.trackAllocation(3, 3000);
    
    manager.recordAccess(1);
    manager.recordAccess(2);
    manager.recordAccess(3);
    
    EXPECT_EQ(manager.getTotalMemoryUsage(), 9000);
    
    // Try to allocate more than available memory
    EXPECT_TRUE(manager.needsEviction(5000)); // Would exceed limit
    
    // Get eviction candidates to free memory
    auto candidates = manager.getEvictionCandidates(5000);
    EXPECT_FALSE(candidates.empty());
    
    // Verify we can gracefully handle the situation by evicting models
    // Calculate how much memory would be freed
    size_t freed = 0;
    for (auto handle : candidates) {
        freed += manager.getModelMemoryUsage(handle);
    }
    
    // Simulate eviction
    for (auto handle : candidates) {
        manager.trackDeallocation(handle);
    }
    
    // Now we should have room for the new allocation
    size_t remaining = manager.getTotalMemoryUsage();
    EXPECT_LE(remaining + 5000, 10000 + freed);
}

// Requirement 8.7: Graceful degradation with active references
TEST(MemoryManagerTest, GracefulDegradationWithActiveReferences) {
    MemoryManager manager(10000);
    
    // Allocate models
    manager.trackAllocation(1, 3000);
    manager.trackAllocation(2, 3000);
    manager.trackAllocation(3, 3000);
    
    manager.recordAccess(1);
    manager.recordAccess(2);
    manager.recordAccess(3);
    
    // All models are in use
    manager.incrementRefCount(1);
    manager.incrementRefCount(2);
    manager.incrementRefCount(3);
    
    EXPECT_EQ(manager.getTotalMemoryUsage(), 9000);
    
    // Try to allocate more memory
    EXPECT_TRUE(manager.needsEviction(5000));
    
    // Get eviction candidates - should be empty since all models are in use
    auto candidates = manager.getEvictionCandidates(5000);
    EXPECT_TRUE(candidates.empty()); // No models can be evicted
    
    // This represents a true OOM situation where we cannot free memory
    // The system should handle this gracefully by returning an error
    // rather than crashing
    
    // Release one model
    manager.decrementRefCount(1);
    
    // Now we should have at least one eviction candidate
    candidates = manager.getEvictionCandidates(5000);
    EXPECT_FALSE(candidates.empty());
    EXPECT_TRUE(std::find(candidates.begin(), candidates.end(), 1) != candidates.end());
}

// Requirement 8.7: Graceful degradation prevents crashes
TEST(MemoryManagerTest, GracefulDegradationPreventsCrashes) {
    MemoryManager manager(10000);
    
    // Allocate models
    manager.trackAllocation(1, 4000);
    manager.trackAllocation(2, 4000);
    
    manager.recordAccess(1);
    manager.recordAccess(2);
    
    EXPECT_EQ(manager.getTotalMemoryUsage(), 8000);
    
    // Try to allocate way more than available memory
    EXPECT_TRUE(manager.needsEviction(20000)); // Requesting 20000, only have 2000 free
    
    // Get eviction candidates
    auto candidates = manager.getEvictionCandidates(20000);
    
    // Even if we evict all models, we still won't have enough
    size_t freed = 0;
    for (auto handle : candidates) {
        freed += manager.getModelMemoryUsage(handle);
    }
    
    // The system should recognize this and handle gracefully
    // by returning all evictable models
    EXPECT_EQ(candidates.size(), 2); // Both models are candidates
    EXPECT_EQ(freed, 8000); // Would free 8000 bytes
    
    // Even after eviction, we'd have 8000 bytes but need 20000
    // This should be handled by returning an error, not crashing
    EXPECT_LT(freed, 20000); // Not enough memory even after eviction
}
