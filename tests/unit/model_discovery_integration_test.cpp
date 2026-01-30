#include <gtest/gtest.h>
#include "ondeviceai/model_manager.hpp"
#include "ondeviceai/json_utils.hpp"
#include <fstream>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#define rmdir(path) _rmdir(path)
#else
#include <unistd.h>
#endif

using namespace ondeviceai;

// Integration test for model discovery and listing functionality
// This test demonstrates the complete workflow of task 3.2

class ModelDiscoveryIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "./test_discovery_" + std::to_string(time(nullptr));
        mkdir(test_dir_.c_str(), 0755);
    }
    
    void TearDown() override {
        std::string registry_file = test_dir_ + "/registry.json";
        remove(registry_file.c_str());
        rmdir(test_dir_.c_str());
    }
    
    std::string test_dir_;
};

TEST_F(ModelDiscoveryIntegrationTest, CompleteModelDiscoveryWorkflow) {
    // This test demonstrates the complete model discovery workflow
    // as specified in task 3.2
    
    // Step 1: Create ModelManager with HTTP client
    ModelManager manager(test_dir_, "https://example.com/model-registry");
    
    // Step 2: Query available models with filtering
    DeviceCapabilities device;
    device.platform = "iOS";
    device.ram_bytes = 4ULL * 1024 * 1024 * 1024; // 4GB
    device.storage_bytes = 64ULL * 1024 * 1024 * 1024; // 64GB
    device.accelerators = {"Neural Engine", "Metal"};
    
    // List all available LLM models for this device
    auto llm_result = manager.listAvailableModels(ModelType::LLM, device);
    ASSERT_TRUE(llm_result.isSuccess());
    
    // With mock HTTP client, we get empty results
    // In production, this would return filtered models from remote registry
    auto llm_models = llm_result.value();
    
    // Step 3: Get model recommendations based on device capabilities
    auto recommendations_result = manager.recommendModels(ModelType::LLM, device);
    ASSERT_TRUE(recommendations_result.isSuccess());
    
    auto recommendations = recommendations_result.value();
    
    // Recommendations should be limited to 10
    EXPECT_LE(recommendations.size(), 10);
    
    // Step 4: List downloaded models from local registry
    auto downloaded_result = manager.listDownloadedModels();
    ASSERT_TRUE(downloaded_result.isSuccess());
    
    // Initially empty
    EXPECT_TRUE(downloaded_result.value().empty());
    
    // Step 5: Verify filtering works correctly
    // Test different model types
    auto stt_result = manager.listAvailableModels(ModelType::STT, device);
    ASSERT_TRUE(stt_result.isSuccess());
    
    auto tts_result = manager.listAvailableModels(ModelType::TTS, device);
    ASSERT_TRUE(tts_result.isSuccess());
    
    auto all_result = manager.listAvailableModels(ModelType::All, device);
    ASSERT_TRUE(all_result.isSuccess());
}

TEST_F(ModelDiscoveryIntegrationTest, FilteringByDeviceCapabilities) {
    ModelManager manager(test_dir_, "https://example.com/model-registry");
    
    // Test with limited device
    DeviceCapabilities limited_device;
    limited_device.platform = "Android";
    limited_device.ram_bytes = 2ULL * 1024 * 1024 * 1024; // 2GB
    limited_device.storage_bytes = 16ULL * 1024 * 1024 * 1024; // 16GB
    
    auto result = manager.listAvailableModels(ModelType::All, limited_device);
    ASSERT_TRUE(result.isSuccess());
    
    // Verify all returned models fit within device capabilities
    for (const auto& model : result.value()) {
        if (model.requirements.min_ram_bytes > 0) {
            EXPECT_LE(model.requirements.min_ram_bytes, limited_device.ram_bytes)
                << "Model " << model.id << " requires more RAM than available";
        }
        if (model.requirements.min_storage_bytes > 0) {
            EXPECT_LE(model.requirements.min_storage_bytes, limited_device.storage_bytes)
                << "Model " << model.id << " requires more storage than available";
        }
    }
}

TEST_F(ModelDiscoveryIntegrationTest, FilteringByPlatform) {
    ModelManager manager(test_dir_, "https://example.com/model-registry");
    
    // Test iOS platform
    DeviceCapabilities ios_device;
    ios_device.platform = "iOS";
    ios_device.ram_bytes = 6ULL * 1024 * 1024 * 1024;
    ios_device.storage_bytes = 128ULL * 1024 * 1024 * 1024;
    
    auto ios_result = manager.listAvailableModels(ModelType::All, ios_device);
    ASSERT_TRUE(ios_result.isSuccess());
    
    // Test Android platform
    DeviceCapabilities android_device;
    android_device.platform = "Android";
    android_device.ram_bytes = 6ULL * 1024 * 1024 * 1024;
    android_device.storage_bytes = 128ULL * 1024 * 1024 * 1024;
    
    auto android_result = manager.listAvailableModels(ModelType::All, android_device);
    ASSERT_TRUE(android_result.isSuccess());
    
    // Both should succeed (with mock, both return empty)
    // In production, results would differ based on platform compatibility
}

TEST_F(ModelDiscoveryIntegrationTest, ModelRecommendationScoring) {
    ModelManager manager(test_dir_, "https://example.com/model-registry");
    
    // Test with high-end device
    DeviceCapabilities high_end_device;
    high_end_device.platform = "iOS";
    high_end_device.ram_bytes = 8ULL * 1024 * 1024 * 1024; // 8GB
    high_end_device.storage_bytes = 256ULL * 1024 * 1024 * 1024; // 256GB
    high_end_device.accelerators = {"Neural Engine", "Metal"};
    
    auto result = manager.recommendModels(ModelType::LLM, high_end_device);
    ASSERT_TRUE(result.isSuccess());
    
    auto recommendations = result.value();
    
    // Should return at most 10 recommendations
    EXPECT_LE(recommendations.size(), 10);
    
    // In production, recommendations would be sorted by score
    // with better-fitting models first
}

TEST_F(ModelDiscoveryIntegrationTest, LocalRegistryPersistence) {
    // Create a local registry with some models
    std::map<std::string, ModelInfo> registry;
    
    ModelInfo model1;
    model1.id = "llama-3b-q4";
    model1.name = "Llama 3B Q4";
    model1.type = ModelType::LLM;
    model1.version = "1.0.0";
    model1.size_bytes = 2ULL * 1024 * 1024 * 1024; // 2GB
    model1.requirements.min_ram_bytes = 3ULL * 1024 * 1024 * 1024; // 3GB
    model1.requirements.supported_platforms = {"iOS", "Android", "Linux"};
    registry[model1.id] = model1;
    
    ModelInfo model2;
    model2.id = "whisper-base";
    model2.name = "Whisper Base";
    model2.type = ModelType::STT;
    model2.version = "1.0.0";
    model2.size_bytes = 150ULL * 1024 * 1024; // 150MB
    model2.requirements.min_ram_bytes = 512ULL * 1024 * 1024; // 512MB
    model2.requirements.supported_platforms = {"iOS", "Android"};
    registry[model2.id] = model2;
    
    // Save to file
    std::string registry_path = test_dir_ + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    // Create manager - should load the registry
    ModelManager manager(test_dir_, "https://example.com/model-registry");
    
    // List downloaded models
    auto result = manager.listDownloadedModels();
    ASSERT_TRUE(result.isSuccess());
    
    auto models = result.value();
    EXPECT_EQ(models.size(), 2);
    
    // Verify models are loaded correctly
    bool found_llm = false;
    bool found_stt = false;
    
    for (const auto& model : models) {
        if (model.id == "llama-3b-q4") {
            found_llm = true;
            EXPECT_EQ(model.type, ModelType::LLM);
            EXPECT_EQ(model.name, "Llama 3B Q4");
        } else if (model.id == "whisper-base") {
            found_stt = true;
            EXPECT_EQ(model.type, ModelType::STT);
            EXPECT_EQ(model.name, "Whisper Base");
        }
    }
    
    EXPECT_TRUE(found_llm);
    EXPECT_TRUE(found_stt);
}

TEST_F(ModelDiscoveryIntegrationTest, DeviceCapabilitiesDetection) {
    // Test that device capabilities can be detected
    DeviceCapabilities caps = DeviceCapabilities::current();
    
    // Should detect RAM
    EXPECT_GT(caps.ram_bytes, 0) << "Failed to detect device RAM";
    
    // Should detect storage
    EXPECT_GT(caps.storage_bytes, 0) << "Failed to detect device storage";
    
    // Should have platform name
    EXPECT_FALSE(caps.platform.empty()) << "Failed to detect platform";
    
    // Log detected capabilities for debugging
    std::cout << "Detected device capabilities:" << std::endl;
    std::cout << "  Platform: " << caps.platform << std::endl;
    std::cout << "  RAM: " << (caps.ram_bytes / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "  Storage: " << (caps.storage_bytes / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "  Accelerators: " << caps.accelerators.size() << std::endl;
}
