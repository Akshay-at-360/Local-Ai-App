#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include "ondeviceai/memory_manager.hpp"
#include <algorithm>
#include <set>

using namespace ondeviceai;

// RapidCheck generators for memory manager testing

namespace rc {

// Generator for ModelHandle (simple integer handles)
Gen<ModelHandle> genModelHandle() {
    return gen::inRange<ModelHandle>(1, 1000);
}

// Generator for memory sizes (in bytes, reasonable range for testing)
Gen<size_t> genMemorySize() {
    return gen::inRange<size_t>(1000ULL, 10000ULL); // 1KB to 10KB
}

// Generator for a sequence of model accesses
// Returns a vector of ModelHandle representing access order
Gen<std::vector<ModelHandle>> genAccessSequence(size_t num_models, size_t sequence_length) {
    return gen::container<std::vector<ModelHandle>>(
        sequence_length,
        gen::inRange<ModelHandle>(1, static_cast<int>(num_models) + 1)
    );
}

} // namespace rc

// Helper function to simulate model accesses and track expected LRU order
// Returns the expected LRU order (back = least recently used, front = most recently used)
std::vector<ModelHandle> simulateLRUOrder(const std::vector<ModelHandle>& access_sequence) {
    std::vector<ModelHandle> lru_order;
    
    for (ModelHandle handle : access_sequence) {
        // Remove handle if it exists
        auto it = std::find(lru_order.begin(), lru_order.end(), handle);
        if (it != lru_order.end()) {
            lru_order.erase(it);
        }
        
        // Add to front (most recently used)
        lru_order.insert(lru_order.begin(), handle);
    }
    
    // Reverse to get LRU order (back = LRU, front = MRU)
    std::reverse(lru_order.begin(), lru_order.end());
    
    return lru_order;
}

// Feature: on-device-ai-sdk, Property 14: LRU Cache Eviction Order
// **Validates: Requirements 8.5**
RC_GTEST_PROP(MemoryManagerPropertyTest, LRUCacheEvictionOrder,
              ()) {
    // Generate test parameters
    auto num_models = *rc::gen::inRange<size_t>(3, 10); // 3-10 models
    auto sequence_length = *rc::gen::inRange<size_t>(10, 50); // 10-50 accesses
    auto model_size = *rc::gen::inRange<size_t>(1000ULL, 2000ULL); // 1-2KB per model
    
    // Generate access sequence
    auto access_sequence = *rc::gen::container<std::vector<ModelHandle>>(
        sequence_length,
        rc::gen::inRange<ModelHandle>(1, static_cast<int>(num_models) + 1)
    );
    
    // Skip if access sequence is empty
    if (access_sequence.empty()) {
        RC_SUCCEED("Empty access sequence");
    }
    
    // Calculate cache limit: enough for half the models
    size_t cache_limit = (num_models / 2 + 1) * model_size;
    
    // Create MemoryManager with cache limit
    MemoryManager memory_manager(cache_limit);
    
    // Track which models are allocated
    std::set<ModelHandle> allocated_models;
    
    // Simulate model accesses
    for (ModelHandle handle : access_sequence) {
        // If model not allocated, allocate it
        if (allocated_models.find(handle) == allocated_models.end()) {
            memory_manager.trackAllocation(handle, model_size);
            allocated_models.insert(handle);
        }
        
        // Record access
        memory_manager.recordAccess(handle);
    }
    
    // Calculate expected LRU order based on access sequence
    std::vector<ModelHandle> expected_lru_order = simulateLRUOrder(access_sequence);
    
    // Verify LRU order by checking getLRUModel repeatedly
    std::vector<ModelHandle> actual_lru_order;
    
    // Get all models in LRU order
    for (size_t i = 0; i < allocated_models.size(); ++i) {
        auto lru_model = memory_manager.getLRUModel();
        
        if (!lru_model.has_value()) {
            break;
        }
        
        actual_lru_order.push_back(lru_model.value());
        
        // Remove this model from tracking to get the next LRU
        // (We need to temporarily remove it to test the next one)
        // Note: This is a limitation of the current API - we can only get one LRU at a time
        // So we'll test differently: verify that the LRU model is indeed the least recently used
    }
    
    // Alternative approach: Test that when eviction is needed, the correct model is chosen
    // Get the least recently used model
    auto lru_model = memory_manager.getLRUModel();
    RC_ASSERT(lru_model.has_value());
    
    // The LRU model should be the one that was accessed earliest (or never accessed)
    // Find the last access time for each model
    std::map<ModelHandle, size_t> last_access_time;
    for (size_t i = 0; i < access_sequence.size(); ++i) {
        last_access_time[access_sequence[i]] = i;
    }
    
    // The LRU model should have the smallest last_access_time among allocated models
    size_t min_access_time = SIZE_MAX;
    ModelHandle expected_lru = 0;
    
    for (ModelHandle handle : allocated_models) {
        auto it = last_access_time.find(handle);
        if (it != last_access_time.end()) {
            if (it->second < min_access_time) {
                min_access_time = it->second;
                expected_lru = handle;
            }
        }
    }
    
    // Verify the LRU model matches our expectation
    if (expected_lru != 0) {
        RC_ASSERT(lru_model.value() == expected_lru);
    }
}

// Additional property test: Verify eviction candidates are in LRU order
RC_GTEST_PROP(MemoryManagerPropertyTest, EvictionCandidatesInLRUOrder,
              ()) {
    // Generate test parameters
    auto num_models = *rc::gen::inRange<size_t>(5, 10);
    auto model_size = *rc::gen::inRange<size_t>(1000ULL, 2000ULL);
    
    // Create MemoryManager with a limit
    size_t cache_limit = num_models * model_size;
    MemoryManager memory_manager(cache_limit);
    
    // Allocate all models and record accesses in a specific order
    std::vector<ModelHandle> access_order;
    for (size_t i = 1; i <= num_models; ++i) {
        ModelHandle handle = static_cast<ModelHandle>(i);
        memory_manager.trackAllocation(handle, model_size);
        memory_manager.recordAccess(handle);
        access_order.push_back(handle);
    }
    
    // Now request eviction candidates
    size_t required_bytes = model_size * 2; // Need to evict 2 models
    auto candidates = memory_manager.getEvictionCandidates(required_bytes);
    
    // Verify we got candidates
    RC_ASSERT(!candidates.empty());
    
    // Verify candidates are in LRU order (first candidate should be LRU)
    // The first model accessed should be the first candidate
    if (candidates.size() >= 1) {
        RC_ASSERT(candidates[0] == access_order[0]);
    }
    
    if (candidates.size() >= 2) {
        RC_ASSERT(candidates[1] == access_order[1]);
    }
}

// Property test: Verify that accessing a model moves it to MRU position
RC_GTEST_PROP(MemoryManagerPropertyTest, AccessMovesToMRU,
              ()) {
    // Generate test parameters
    auto num_models = *rc::gen::inRange<size_t>(3, 8);
    auto model_size = *rc::gen::inRange<size_t>(1000ULL, 2000ULL);
    
    MemoryManager memory_manager(num_models * model_size);
    
    // Allocate models in order
    std::vector<ModelHandle> models;
    for (size_t i = 1; i <= num_models; ++i) {
        ModelHandle handle = static_cast<ModelHandle>(i);
        memory_manager.trackAllocation(handle, model_size);
        memory_manager.recordAccess(handle);
        models.push_back(handle);
    }
    
    // The LRU should be the first model
    auto lru_before = memory_manager.getLRUModel();
    RC_ASSERT(lru_before.has_value());
    RC_ASSERT(lru_before.value() == models[0]);
    
    // Access the first model again
    memory_manager.recordAccess(models[0]);
    
    // Now the LRU should be the second model
    auto lru_after = memory_manager.getLRUModel();
    RC_ASSERT(lru_after.has_value());
    RC_ASSERT(lru_after.value() == models[1]);
}

// Property test: Verify eviction respects reference counts
RC_GTEST_PROP(MemoryManagerPropertyTest, EvictionRespectsRefCounts,
              ()) {
    // Generate test parameters
    auto num_models = *rc::gen::inRange<size_t>(3, 6);
    auto model_size = *rc::gen::inRange<size_t>(1000ULL, 2000ULL);
    
    MemoryManager memory_manager(num_models * model_size);
    
    // Allocate models
    std::vector<ModelHandle> models;
    for (size_t i = 1; i <= num_models; ++i) {
        ModelHandle handle = static_cast<ModelHandle>(i);
        memory_manager.trackAllocation(handle, model_size);
        memory_manager.recordAccess(handle);
        models.push_back(handle);
    }
    
    // Increment reference count for the LRU model
    auto lru_model = memory_manager.getLRUModel();
    RC_ASSERT(lru_model.has_value());
    memory_manager.incrementRefCount(lru_model.value());
    
    // Get eviction candidates
    size_t required_bytes = model_size;
    auto candidates = memory_manager.getEvictionCandidates(required_bytes);
    
    // The LRU model should NOT be in candidates (it has a reference)
    bool lru_in_candidates = false;
    for (ModelHandle candidate : candidates) {
        if (candidate == lru_model.value()) {
            lru_in_candidates = true;
            break;
        }
    }
    
    RC_ASSERT(!lru_in_candidates);
}

// Property test: Verify multiple accesses maintain correct LRU order
RC_GTEST_PROP(MemoryManagerPropertyTest, MultipleAccessesMaintainLRUOrder,
              ()) {
    // Generate a specific access pattern
    auto num_models = *rc::gen::inRange<size_t>(4, 8);
    auto model_size = *rc::gen::inRange<size_t>(1000ULL, 2000ULL);
    
    MemoryManager memory_manager(num_models * model_size);
    
    // Allocate models
    std::vector<ModelHandle> models;
    for (size_t i = 1; i <= num_models; ++i) {
        ModelHandle handle = static_cast<ModelHandle>(i);
        memory_manager.trackAllocation(handle, model_size);
        models.push_back(handle);
    }
    
    // Access models in a specific pattern: 1, 2, 3, 1, 2, 1
    // After this, LRU order should be: 3 (LRU), 2, 1 (MRU)
    if (models.size() >= 3) {
        memory_manager.recordAccess(models[0]); // 1
        memory_manager.recordAccess(models[1]); // 2
        memory_manager.recordAccess(models[2]); // 3
        memory_manager.recordAccess(models[0]); // 1
        memory_manager.recordAccess(models[1]); // 2
        memory_manager.recordAccess(models[0]); // 1
        
        // LRU should be model 3
        auto lru = memory_manager.getLRUModel();
        RC_ASSERT(lru.has_value());
        RC_ASSERT(lru.value() == models[2]);
    }
}

// Property test: Verify eviction frees enough memory
RC_GTEST_PROP(MemoryManagerPropertyTest, EvictionFreesEnoughMemory,
              ()) {
    // Generate test parameters
    auto num_models = *rc::gen::inRange<size_t>(5, 10);
    auto model_size = *rc::gen::inRange<size_t>(1000ULL, 2000ULL);
    
    // Set cache limit to hold all models
    size_t cache_limit = num_models * model_size;
    MemoryManager memory_manager(cache_limit);
    
    // Allocate all models
    for (size_t i = 1; i <= num_models; ++i) {
        ModelHandle handle = static_cast<ModelHandle>(i);
        memory_manager.trackAllocation(handle, model_size);
        memory_manager.recordAccess(handle);
    }
    
    // Request eviction candidates to make room for a new model
    size_t required_bytes = model_size * 2;
    auto candidates = memory_manager.getEvictionCandidates(required_bytes);
    
    // Calculate total memory that would be freed
    size_t freed_memory = 0;
    for (ModelHandle candidate : candidates) {
        freed_memory += memory_manager.getModelMemoryUsage(candidate);
    }
    
    // Verify that evicting these candidates would free enough memory
    size_t current_usage = memory_manager.getTotalMemoryUsage();
    RC_ASSERT(current_usage - freed_memory + required_bytes <= cache_limit);
}

// Property test: Verify LRU order is consistent across multiple queries
RC_GTEST_PROP(MemoryManagerPropertyTest, LRUOrderConsistentAcrossQueries,
              ()) {
    // Generate test parameters
    auto num_models = *rc::gen::inRange<size_t>(3, 6);
    auto model_size = *rc::gen::inRange<size_t>(1000ULL, 2000ULL);
    
    MemoryManager memory_manager(num_models * model_size);
    
    // Allocate models in order
    for (size_t i = 1; i <= num_models; ++i) {
        ModelHandle handle = static_cast<ModelHandle>(i);
        memory_manager.trackAllocation(handle, model_size);
        memory_manager.recordAccess(handle);
    }
    
    // Query LRU multiple times without any accesses
    auto lru1 = memory_manager.getLRUModel();
    auto lru2 = memory_manager.getLRUModel();
    auto lru3 = memory_manager.getLRUModel();
    
    // All queries should return the same result
    RC_ASSERT(lru1.has_value());
    RC_ASSERT(lru2.has_value());
    RC_ASSERT(lru3.has_value());
    RC_ASSERT(lru1.value() == lru2.value());
    RC_ASSERT(lru2.value() == lru3.value());
}

// Property test: Verify empty cache returns no LRU model
RC_GTEST_PROP(MemoryManagerPropertyTest, EmptyCacheReturnsNoLRU,
              ()) {
    auto cache_limit = *rc::gen::inRange<size_t>(1000ULL, 10000ULL);
    MemoryManager memory_manager(cache_limit);
    
    // Query LRU on empty cache
    auto lru = memory_manager.getLRUModel();
    
    // Should return nullopt
    RC_ASSERT(!lru.has_value());
}

// Property test: Verify single model is always LRU
RC_GTEST_PROP(MemoryManagerPropertyTest, SingleModelAlwaysLRU,
              ()) {
    auto model_size = *rc::gen::inRange<size_t>(1000ULL, 5000ULL);
    MemoryManager memory_manager(model_size * 2);
    
    ModelHandle handle = 1;
    memory_manager.trackAllocation(handle, model_size);
    memory_manager.recordAccess(handle);
    
    // Query LRU
    auto lru = memory_manager.getLRUModel();
    
    // Should return the only model
    RC_ASSERT(lru.has_value());
    RC_ASSERT(lru.value() == handle);
    
    // Access it again
    memory_manager.recordAccess(handle);
    
    // Should still be LRU (only model)
    auto lru2 = memory_manager.getLRUModel();
    RC_ASSERT(lru2.has_value());
    RC_ASSERT(lru2.value() == handle);
}

// Property test: Verify deallocation removes model from LRU tracking
RC_GTEST_PROP(MemoryManagerPropertyTest, DeallocationRemovesFromLRU,
              ()) {
    auto num_models = *rc::gen::inRange<size_t>(3, 6);
    auto model_size = *rc::gen::inRange<size_t>(1000ULL, 2000ULL);
    
    MemoryManager memory_manager(num_models * model_size);
    
    // Allocate models
    std::vector<ModelHandle> models;
    for (size_t i = 1; i <= num_models; ++i) {
        ModelHandle handle = static_cast<ModelHandle>(i);
        memory_manager.trackAllocation(handle, model_size);
        memory_manager.recordAccess(handle);
        models.push_back(handle);
    }
    
    // Get LRU model
    auto lru = memory_manager.getLRUModel();
    RC_ASSERT(lru.has_value());
    ModelHandle lru_handle = lru.value();
    
    // Deallocate the LRU model
    memory_manager.trackDeallocation(lru_handle);
    
    // Get new LRU
    auto new_lru = memory_manager.getLRUModel();
    RC_ASSERT(new_lru.has_value());
    
    // New LRU should be different from the deallocated one
    RC_ASSERT(new_lru.value() != lru_handle);
}
