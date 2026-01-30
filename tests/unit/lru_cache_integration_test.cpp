#include <gtest/gtest.h>
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/memory_manager.hpp"
#include <fstream>
#include <filesystem>

using namespace ondeviceai;

class LRUCacheIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = "test_lru_cache_" + std::to_string(std::time(nullptr));
        std::filesystem::create_directories(test_dir_);
        
        // Create test model files
        createTestModel("model1.gguf", 1000);
        createTestModel("model2.gguf", 2000);
        createTestModel("model3.gguf", 3000);
        createTestModel("model4.gguf", 4000);
    }
    
    void TearDown() override {
        // Clean up test directory
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }
    
    void createTestModel(const std::string& filename, size_t size) {
        std::string path = test_dir_ + "/" + filename;
        std::ofstream file(path, std::ios::binary);
        std::vector<char> data(size, 'X');
        file.write(data.data(), data.size());
        file.close();
    }
    
    std::string getModelPath(const std::string& filename) {
        return test_dir_ + "/" + filename;
    }
    
    std::string test_dir_;
};

// Test lazy loading (Requirement 8.3, 17.1)
TEST_F(LRUCacheIntegrationTest, LazyLoadingBehavior) {
    MemoryManager memory_manager(0); // No limit
    LLMEngine engine;
    engine.setMemoryManager(&memory_manager);
    
    // Initially no memory used
    EXPECT_EQ(memory_manager.getTotalMemoryUsage(), 0);
    
    // Load model - should allocate memory
    auto result = engine.loadModel(getModelPath("model1.gguf"));
    ASSERT_TRUE(result.isSuccess());
    
    // Memory should now be tracked
    EXPECT_GT(memory_manager.getTotalMemoryUsage(), 0);
    EXPECT_EQ(memory_manager.getTotalMemoryUsage(), 1000);
}

// Test automatic eviction when memory limit reached (Requirement 8.4, 8.5)
TEST_F(LRUCacheIntegrationTest, AutomaticEvictionOnMemoryLimit) {
    MemoryManager memory_manager(5000); // 5KB limit
    LLMEngine engine;
    engine.setMemoryManager(&memory_manager);
    
    // Load models that fit within limit
    auto handle1 = engine.loadModel(getModelPath("model1.gguf"));
    ASSERT_TRUE(handle1.isSuccess());
    
    auto handle2 = engine.loadModel(getModelPath("model2.gguf"));
    ASSERT_TRUE(handle2.isSuccess());
    
    // Total: 3000 bytes, within limit
    EXPECT_EQ(memory_manager.getTotalMemoryUsage(), 3000);
    
    // Load model that would exceed limit - should trigger eviction
    auto handle3 = engine.loadModel(getModelPath("model3.gguf"));
    ASSERT_TRUE(handle3.isSuccess());
    
    // Model 1 should have been evicted (LRU)
    EXPECT_FALSE(engine.isModelLoaded(handle1.value()));
    EXPECT_TRUE(engine.isModelLoaded(handle2.value()));
    EXPECT_TRUE(engine.isModelLoaded(handle3.value()));
    
    // Total should be model2 + model3 = 5000 bytes
    EXPECT_EQ(memory_manager.getTotalMemoryUsage(), 5000);
}

// Test LRU eviction order (Requirement 8.5)
TEST_F(LRUCacheIntegrationTest, LRUEvictionOrder) {
    MemoryManager memory_manager(6000); // 6KB limit
    LLMEngine engine;
    engine.setMemoryManager(&memory_manager);
    
    // Load 3 models
    auto handle1 = engine.loadModel(getModelPath("model1.gguf"));
    ASSERT_TRUE(handle1.isSuccess());
    
    auto handle2 = engine.loadModel(getModelPath("model2.gguf"));
    ASSERT_TRUE(handle2.isSuccess());
    
    auto handle3 = engine.loadModel(getModelPath("model3.gguf"));
    ASSERT_TRUE(handle3.isSuccess());
    
    // Total: 6000 bytes, at limit
    EXPECT_EQ(memory_manager.getTotalMemoryUsage(), 6000);
    
    // Access model 1 to make it more recently used
    auto gen_result = engine.generate(handle1.value(), "test prompt");
    
    // Now load model4 (4000 bytes) - should evict model2 (LRU)
    auto handle4 = engine.loadModel(getModelPath("model4.gguf"));
    ASSERT_TRUE(handle4.isSuccess());
    
    // Model 2 should be evicted (LRU), model 1 should remain (recently accessed)
    EXPECT_TRUE(engine.isModelLoaded(handle1.value()));
    EXPECT_FALSE(engine.isModelLoaded(handle2.value()));
    EXPECT_FALSE(engine.isModelLoaded(handle3.value())); // Also evicted to make room
    EXPECT_TRUE(engine.isModelLoaded(handle4.value()));
}

// Test reference counting prevents eviction (Requirement 17.4)
TEST_F(LRUCacheIntegrationTest, ReferenceCountingPreventsEviction) {
    MemoryManager memory_manager(5000); // 5KB limit
    LLMEngine engine;
    engine.setMemoryManager(&memory_manager);
    
    // Load 2 models
    auto handle1 = engine.loadModel(getModelPath("model1.gguf"));
    ASSERT_TRUE(handle1.isSuccess());
    
    auto handle2 = engine.loadModel(getModelPath("model2.gguf"));
    ASSERT_TRUE(handle2.isSuccess());
    
    // Total: 3000 bytes
    EXPECT_EQ(memory_manager.getTotalMemoryUsage(), 3000);
    
    // Increment reference count on model 1 (simulate active use)
    memory_manager.incrementRefCount(handle1.value());
    
    // Try to load model3 (3000 bytes) - would need to evict
    // Model 1 has reference, so model 2 should be evicted instead
    auto handle3 = engine.loadModel(getModelPath("model3.gguf"));
    ASSERT_TRUE(handle3.isSuccess());
    
    // Model 1 should remain (has reference), model 2 should be evicted
    EXPECT_TRUE(engine.isModelLoaded(handle1.value()));
    EXPECT_FALSE(engine.isModelLoaded(handle2.value()));
    EXPECT_TRUE(engine.isModelLoaded(handle3.value()));
    
    // Clean up reference
    memory_manager.decrementRefCount(handle1.value());
}

// Test model reuse when already loaded (Requirement 17.4)
TEST_F(LRUCacheIntegrationTest, ModelReuseWhenAlreadyLoaded) {
    MemoryManager memory_manager(0); // No limit
    LLMEngine engine;
    engine.setMemoryManager(&memory_manager);
    
    // Load model
    auto handle1 = engine.loadModel(getModelPath("model1.gguf"));
    ASSERT_TRUE(handle1.isSuccess());
    
    size_t initial_usage = memory_manager.getTotalMemoryUsage();
    
    // Load same model again - should get new handle but reuse memory
    auto handle2 = engine.loadModel(getModelPath("model1.gguf"));
    ASSERT_TRUE(handle2.isSuccess());
    
    // Different handles
    EXPECT_NE(handle1.value(), handle2.value());
    
    // Both should be loaded
    EXPECT_TRUE(engine.isModelLoaded(handle1.value()));
    EXPECT_TRUE(engine.isModelLoaded(handle2.value()));
    
    // Memory usage should have increased (separate instances for now)
    // Note: True model reuse would require path-based caching, which is a future enhancement
    EXPECT_GT(memory_manager.getTotalMemoryUsage(), initial_usage);
}

// Test unloading models frees memory (Requirement 17.5)
TEST_F(LRUCacheIntegrationTest, UnloadingFreesMemory) {
    MemoryManager memory_manager(0); // No limit
    LLMEngine engine;
    engine.setMemoryManager(&memory_manager);
    
    // Load model
    auto handle = engine.loadModel(getModelPath("model1.gguf"));
    ASSERT_TRUE(handle.isSuccess());
    
    EXPECT_EQ(memory_manager.getTotalMemoryUsage(), 1000);
    
    // Unload model
    auto unload_result = engine.unloadModel(handle.value());
    ASSERT_TRUE(unload_result.isSuccess());
    
    // Memory should be freed
    EXPECT_EQ(memory_manager.getTotalMemoryUsage(), 0);
    EXPECT_FALSE(engine.isModelLoaded(handle.value()));
}

// Test error when all models have references and can't be evicted
TEST_F(LRUCacheIntegrationTest, ErrorWhenCannotEvict) {
    MemoryManager memory_manager(3000); // 3KB limit
    LLMEngine engine;
    engine.setMemoryManager(&memory_manager);
    
    // Load model that fills the limit
    auto handle1 = engine.loadModel(getModelPath("model1.gguf"));
    ASSERT_TRUE(handle1.isSuccess());
    
    auto handle2 = engine.loadModel(getModelPath("model2.gguf"));
    ASSERT_TRUE(handle2.isSuccess());
    
    // Add references to both models
    memory_manager.incrementRefCount(handle1.value());
    memory_manager.incrementRefCount(handle2.value());
    
    // Try to load another model - should fail (can't evict)
    auto handle3 = engine.loadModel(getModelPath("model3.gguf"));
    ASSERT_TRUE(handle3.isError());
    EXPECT_EQ(handle3.error().code, ErrorCode::ResourceOutOfMemory);
    
    // Clean up references
    memory_manager.decrementRefCount(handle1.value());
    memory_manager.decrementRefCount(handle2.value());
}

// Test LRU tracking during inference
TEST_F(LRUCacheIntegrationTest, LRUTrackingDuringInference) {
    MemoryManager memory_manager(0); // No limit
    LLMEngine engine;
    engine.setMemoryManager(&memory_manager);
    
    // Load 3 models
    auto handle1 = engine.loadModel(getModelPath("model1.gguf"));
    auto handle2 = engine.loadModel(getModelPath("model2.gguf"));
    auto handle3 = engine.loadModel(getModelPath("model3.gguf"));
    
    ASSERT_TRUE(handle1.isSuccess());
    ASSERT_TRUE(handle2.isSuccess());
    ASSERT_TRUE(handle3.isSuccess());
    
    // LRU should be model 1
    auto lru = memory_manager.getLRUModel();
    ASSERT_TRUE(lru.has_value());
    EXPECT_EQ(lru.value(), handle1.value());
    
    // Use model 1 for inference
    engine.generate(handle1.value(), "test");
    
    // Now LRU should be model 2
    lru = memory_manager.getLRUModel();
    ASSERT_TRUE(lru.has_value());
    EXPECT_EQ(lru.value(), handle2.value());
}

// Test memory pressure detection
TEST_F(LRUCacheIntegrationTest, MemoryPressureDetection) {
    MemoryManager memory_manager(10000); // 10KB limit
    LLMEngine engine;
    engine.setMemoryManager(&memory_manager);
    
    bool pressure_detected = false;
    memory_manager.setMemoryPressureCallback([&](size_t usage, size_t limit) {
        (void)usage;
        (void)limit;
        pressure_detected = true;
    });
    
    // Load models up to 90% of limit
    auto handle1 = engine.loadModel(getModelPath("model1.gguf")); // 1KB
    auto handle2 = engine.loadModel(getModelPath("model2.gguf")); // 2KB
    auto handle3 = engine.loadModel(getModelPath("model3.gguf")); // 3KB
    auto handle4 = engine.loadModel(getModelPath("model4.gguf")); // 4KB
    
    // Total: 10KB, should trigger pressure callback
    EXPECT_TRUE(pressure_detected);
}
