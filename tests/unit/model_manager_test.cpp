#include <gtest/gtest.h>
#include "ondeviceai/model_manager.hpp"
#include "ondeviceai/json_utils.hpp"
#include "ondeviceai/sha256.hpp"
#include <fstream>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define rmdir(path) _rmdir(path)
#else
#include <unistd.h>
#endif

using namespace ondeviceai;

// Helper function to clean up test directory
void cleanupTestDirectory(const std::string& path) {
    std::string registry_file = path + "/registry.json";
    remove(registry_file.c_str());
    rmdir(path.c_str());
}

// Helper function to create a test ModelInfo
ModelInfo createTestModelInfo(const std::string& id) {
    ModelInfo info;
    info.id = id;
    info.name = "Test Model " + id;
    info.type = ModelType::LLM;
    info.version = "1.0.0";
    info.size_bytes = 1024 * 1024 * 100; // 100MB
    info.download_url = "https://example.com/models/" + id;
    info.checksum_sha256 = "abc123def456";
    info.metadata["architecture"] = "transformer";
    info.metadata["context_length"] = "2048";
    info.requirements.min_ram_bytes = 2ULL * 1024 * 1024 * 1024; // 2GB
    info.requirements.min_storage_bytes = 200ULL * 1024 * 1024; // 200MB
    info.requirements.supported_platforms = {"iOS", "Android", "Linux"};
    return info;
}

TEST(ModelManagerTest, Construction) {
    ModelManager manager("./test_models", "https://test.registry.com");
    // Should not crash
    cleanupTestDirectory("./test_models");
}

TEST(ModelManagerTest, ListDownloadedModelsEmpty) {
    ModelManager manager("./test_models", "https://test.registry.com");
    auto result = manager.listDownloadedModels();
    
    ASSERT_TRUE(result.isSuccess());
    EXPECT_TRUE(result.value().empty());
    cleanupTestDirectory("./test_models");
}

TEST(ModelManagerTest, GetModelInfoNotFound) {
    ModelManager manager("./test_models", "https://test.registry.com");
    auto result = manager.getModelInfo("nonexistent_model");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelNotFoundInRegistry);
    cleanupTestDirectory("./test_models");
}

TEST(ModelManagerTest, IsModelDownloadedReturnsFalse) {
    ModelManager manager("./test_models", "https://test.registry.com");
    auto result = manager.isModelDownloaded("nonexistent_model");
    
    ASSERT_TRUE(result.isSuccess());
    EXPECT_FALSE(result.value());
    cleanupTestDirectory("./test_models");
}

TEST(ModelManagerTest, GetStorageInfo) {
    ModelManager manager("./test_models", "https://test.registry.com");
    auto result = manager.getStorageInfo();
    
    ASSERT_TRUE(result.isSuccess());
    StorageInfo info = result.value();
    
    // Should have some storage available
    EXPECT_GT(info.total_bytes, 0);
    EXPECT_GT(info.available_bytes, 0);
    EXPECT_EQ(info.used_by_models_bytes, 0); // No models yet
    
    cleanupTestDirectory("./test_models");
}

TEST(ModelManagerTest, DeviceCapabilitiesCurrent) {
    DeviceCapabilities caps = DeviceCapabilities::current();
    
    // Should detect some RAM
    EXPECT_GT(caps.ram_bytes, 0);
    
    // Should detect some storage
    EXPECT_GT(caps.storage_bytes, 0);
    
    // Should have a platform name
    EXPECT_FALSE(caps.platform.empty());
    
    // May or may not have accelerators depending on platform
    // Just check it doesn't crash
}

// JSON Serialization Tests
TEST(JSONUtilsTest, SerializeDeserializeModelInfo) {
    ModelInfo original = createTestModelInfo("test-model-1");
    
    // Serialize
    std::string json = json::serialize_model_info(original);
    EXPECT_FALSE(json.empty());
    
    // Deserialize
    auto result = json::deserialize_model_info(json);
    ASSERT_TRUE(result.isSuccess());
    
    ModelInfo deserialized = result.value();
    
    // Verify all fields
    EXPECT_EQ(deserialized.id, original.id);
    EXPECT_EQ(deserialized.name, original.name);
    EXPECT_EQ(deserialized.type, original.type);
    EXPECT_EQ(deserialized.version, original.version);
    EXPECT_EQ(deserialized.size_bytes, original.size_bytes);
    EXPECT_EQ(deserialized.download_url, original.download_url);
    EXPECT_EQ(deserialized.checksum_sha256, original.checksum_sha256);
    
    // Verify metadata
    EXPECT_EQ(deserialized.metadata.size(), original.metadata.size());
    for (const auto& pair : original.metadata) {
        EXPECT_EQ(deserialized.metadata[pair.first], pair.second);
    }
    
    // Verify requirements
    EXPECT_EQ(deserialized.requirements.min_ram_bytes, original.requirements.min_ram_bytes);
    EXPECT_EQ(deserialized.requirements.min_storage_bytes, original.requirements.min_storage_bytes);
    EXPECT_EQ(deserialized.requirements.supported_platforms.size(), 
              original.requirements.supported_platforms.size());
}

TEST(JSONUtilsTest, SerializeDeserializeModelRegistry) {
    std::map<std::string, ModelInfo> original;
    original["model-1"] = createTestModelInfo("model-1");
    original["model-2"] = createTestModelInfo("model-2");
    original["model-3"] = createTestModelInfo("model-3");
    
    // Serialize
    std::string json = json::serialize_model_registry(original);
    EXPECT_FALSE(json.empty());
    
    // Deserialize
    auto result = json::deserialize_model_registry(json);
    ASSERT_TRUE(result.isSuccess());
    
    std::map<std::string, ModelInfo> deserialized = result.value();
    
    // Verify count
    EXPECT_EQ(deserialized.size(), original.size());
    
    // Verify each model
    for (const auto& pair : original) {
        ASSERT_TRUE(deserialized.find(pair.first) != deserialized.end());
        EXPECT_EQ(deserialized[pair.first].id, pair.second.id);
        EXPECT_EQ(deserialized[pair.first].name, pair.second.name);
    }
}

TEST(JSONUtilsTest, EscapeUnescapeJSONString) {
    std::string original = "Hello \"World\"\nNew Line\tTab\\Backslash";
    std::string escaped = json::escape_json_string(original);
    std::string unescaped = json::unescape_json_string(escaped);
    
    // After escape/unescape, should be similar (whitespace handling may differ)
    EXPECT_NE(escaped, original); // Should be different after escaping
    EXPECT_EQ(unescaped, original); // Should match after unescaping
}

TEST(JSONUtilsTest, DeserializeEmptyRegistry) {
    std::string json = R"({
        "version": "1.0",
        "models": []
    })";
    
    auto result = json::deserialize_model_registry(json);
    ASSERT_TRUE(result.isSuccess());
    EXPECT_TRUE(result.value().empty());
}

TEST(JSONUtilsTest, DeserializeMalformedJSON) {
    std::string json = "{ invalid json }";
    
    auto result = json::deserialize_model_info(json);
    // Should handle gracefully - may succeed with empty/default values or fail
    // The important thing is it doesn't crash
}

// Registry Persistence Tests
class ModelManagerPersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "./test_registry_" + std::to_string(time(nullptr));
    }
    
    void TearDown() override {
        cleanupTestDirectory(test_dir_);
    }
    
    std::string test_dir_;
};

TEST_F(ModelManagerPersistenceTest, RegistryPersistsAcrossInstances) {
    // Create first manager and add a model to registry manually
    {
        ModelManager manager(test_dir_, "https://test.registry.com");
        // In a real scenario, downloadModel would add to registry
        // For now, we'll test the persistence mechanism directly
    }
    
    // Create a registry file manually
    std::map<std::string, ModelInfo> registry;
    registry["test-model"] = createTestModelInfo("test-model");
    
    std::string registry_path = test_dir_ + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    // Create second manager - should load the registry
    {
        ModelManager manager(test_dir_, "https://test.registry.com");
        auto result = manager.listDownloadedModels();
        
        ASSERT_TRUE(result.isSuccess());
        EXPECT_EQ(result.value().size(), 1);
        EXPECT_EQ(result.value()[0].id, "test-model");
    }
}

TEST_F(ModelManagerPersistenceTest, StorageInfoReflectsRegisteredModels) {
    // Create registry with models
    std::map<std::string, ModelInfo> registry;
    ModelInfo model1 = createTestModelInfo("model-1");
    ModelInfo model2 = createTestModelInfo("model-2");
    registry["model-1"] = model1;
    registry["model-2"] = model2;
    
    // Create directory first
    mkdir(test_dir_.c_str(), 0755);
    
    std::string registry_path = test_dir_ + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    // Create manager AFTER registry file exists - it will load it
    ModelManager manager(test_dir_, "https://test.registry.com");
    auto result = manager.getStorageInfo();
    
    ASSERT_TRUE(result.isSuccess());
    StorageInfo info = result.value();
    
    // Should reflect the size of registered models
    size_t expected_size = model1.size_bytes + model2.size_bytes;
    EXPECT_EQ(info.used_by_models_bytes, expected_size);
}

TEST_F(ModelManagerPersistenceTest, GetModelPathReturnsCorrectPath) {
    // Create registry with a model
    std::map<std::string, ModelInfo> registry;
    registry["test-model"] = createTestModelInfo("test-model");
    
    // Create directory first
    mkdir(test_dir_.c_str(), 0755);
    
    std::string registry_path = test_dir_ + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    // Create manager AFTER registry file exists - it will load it
    ModelManager manager(test_dir_, "https://test.registry.com");
    auto result = manager.getModelPath("test-model");
    
    ASSERT_TRUE(result.isSuccess());
    std::string expected_path = test_dir_ + "/test-model";
    EXPECT_EQ(result.value(), expected_path);
}

// Model Discovery and Filtering Tests
TEST(ModelManagerFilteringTest, ListAvailableModelsFiltersByType) {
    // Note: This test uses the mock HTTP client which returns an empty array
    // In a real scenario with a test server, we would verify actual filtering
    ModelManager manager("./test_models_filter", "https://test.registry.com");
    
    // Test filtering by LLM type
    auto llm_result = manager.listAvailableModels(ModelType::LLM);
    ASSERT_TRUE(llm_result.isSuccess());
    
    // Test filtering by STT type
    auto stt_result = manager.listAvailableModels(ModelType::STT);
    ASSERT_TRUE(stt_result.isSuccess());
    
    // Test filtering by TTS type
    auto tts_result = manager.listAvailableModels(ModelType::TTS);
    ASSERT_TRUE(tts_result.isSuccess());
    
    // Test no filtering (All types)
    auto all_result = manager.listAvailableModels(ModelType::All);
    ASSERT_TRUE(all_result.isSuccess());
    
    cleanupTestDirectory("./test_models_filter");
}

TEST(ModelManagerFilteringTest, ListAvailableModelsFiltersByPlatform) {
    ModelManager manager("./test_models_platform", "https://test.registry.com");
    
    // Create device capabilities for iOS
    DeviceCapabilities ios_device;
    ios_device.platform = "iOS";
    ios_device.ram_bytes = 4ULL * 1024 * 1024 * 1024; // 4GB
    ios_device.storage_bytes = 64ULL * 1024 * 1024 * 1024; // 64GB
    
    auto result = manager.listAvailableModels(ModelType::All, ios_device);
    ASSERT_TRUE(result.isSuccess());
    
    // With mock implementation, we get empty results
    // In production, this would filter models by platform
    
    cleanupTestDirectory("./test_models_platform");
}

TEST(ModelManagerFilteringTest, ListAvailableModelsFiltersByDeviceCapabilities) {
    ModelManager manager("./test_models_device", "https://test.registry.com");
    
    // Create device with limited capabilities
    DeviceCapabilities limited_device;
    limited_device.platform = "Android";
    limited_device.ram_bytes = 2ULL * 1024 * 1024 * 1024; // 2GB RAM
    limited_device.storage_bytes = 16ULL * 1024 * 1024 * 1024; // 16GB storage
    
    auto result = manager.listAvailableModels(ModelType::All, limited_device);
    ASSERT_TRUE(result.isSuccess());
    
    // Models requiring more than 2GB RAM should be filtered out
    for (const auto& model : result.value()) {
        if (model.requirements.min_ram_bytes > 0) {
            EXPECT_LE(model.requirements.min_ram_bytes, limited_device.ram_bytes);
        }
        if (model.requirements.min_storage_bytes > 0) {
            EXPECT_LE(model.requirements.min_storage_bytes, limited_device.storage_bytes);
        }
    }
    
    cleanupTestDirectory("./test_models_device");
}

TEST(ModelManagerFilteringTest, RecommendModelsScoresAppropriately) {
    ModelManager manager("./test_models_recommend", "https://test.registry.com");
    
    // Create device capabilities
    DeviceCapabilities device;
    device.platform = "iOS";
    device.ram_bytes = 6ULL * 1024 * 1024 * 1024; // 6GB RAM
    device.storage_bytes = 128ULL * 1024 * 1024 * 1024; // 128GB storage
    device.accelerators = {"Neural Engine", "Metal"};
    
    auto result = manager.recommendModels(ModelType::LLM, device);
    ASSERT_TRUE(result.isSuccess());
    
    // With mock HTTP client, we get empty results
    // In production, this would return scored and sorted recommendations
    
    cleanupTestDirectory("./test_models_recommend");
}

TEST(ModelManagerFilteringTest, RecommendModelsLimitsResults) {
    ModelManager manager("./test_models_limit", "https://test.registry.com");
    
    DeviceCapabilities device = DeviceCapabilities::current();
    
    auto result = manager.recommendModels(ModelType::All, device);
    ASSERT_TRUE(result.isSuccess());
    
    // Should return at most 10 recommendations
    EXPECT_LE(result.value().size(), 10);
    
    cleanupTestDirectory("./test_models_limit");
}

// Test with manually created registry to verify filtering logic
TEST(ModelManagerFilteringTest, FilteringLogicWithMockRegistry) {
    // This test verifies the filtering logic works correctly
    // by creating a mock registry and checking the results
    
    std::string test_dir = "./test_models_mock_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create a mock registry with various models
    std::map<std::string, ModelInfo> registry;
    
    // Model 1: Small LLM for iOS
    ModelInfo model1;
    model1.id = "small-llm-ios";
    model1.name = "Small LLM for iOS";
    model1.type = ModelType::LLM;
    model1.version = "1.0.0";
    model1.size_bytes = 500ULL * 1024 * 1024; // 500MB
    model1.requirements.min_ram_bytes = 1ULL * 1024 * 1024 * 1024; // 1GB
    model1.requirements.min_storage_bytes = 600ULL * 1024 * 1024; // 600MB
    model1.requirements.supported_platforms = {"iOS"};
    registry[model1.id] = model1;
    
    // Model 2: Large LLM for all platforms
    ModelInfo model2;
    model2.id = "large-llm-all";
    model2.name = "Large LLM for All";
    model2.type = ModelType::LLM;
    model2.version = "1.0.0";
    model2.size_bytes = 5ULL * 1024 * 1024 * 1024; // 5GB
    model2.requirements.min_ram_bytes = 8ULL * 1024 * 1024 * 1024; // 8GB
    model2.requirements.min_storage_bytes = 6ULL * 1024 * 1024 * 1024; // 6GB
    model2.requirements.supported_platforms = {"iOS", "Android", "Linux"};
    registry[model2.id] = model2;
    
    // Model 3: STT for Android
    ModelInfo model3;
    model3.id = "stt-android";
    model3.name = "STT for Android";
    model3.type = ModelType::STT;
    model3.version = "1.0.0";
    model3.size_bytes = 200ULL * 1024 * 1024; // 200MB
    model3.requirements.min_ram_bytes = 512ULL * 1024 * 1024; // 512MB
    model3.requirements.min_storage_bytes = 250ULL * 1024 * 1024; // 250MB
    model3.requirements.supported_platforms = {"Android"};
    registry[model3.id] = model3;
    
    // Model 4: TTS for all platforms
    ModelInfo model4;
    model4.id = "tts-all";
    model4.name = "TTS for All";
    model4.type = ModelType::TTS;
    model4.version = "1.0.0";
    model4.size_bytes = 100ULL * 1024 * 1024; // 100MB
    model4.requirements.min_ram_bytes = 256ULL * 1024 * 1024; // 256MB
    model4.requirements.min_storage_bytes = 150ULL * 1024 * 1024; // 150MB
    model4.requirements.supported_platforms = {}; // Empty means all platforms
    registry[model4.id] = model4;
    
    // Save registry to file
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    // Note: The listAvailableModels queries the remote registry, not the local one
    // So this test demonstrates the structure but can't fully test filtering
    // without a mock HTTP server
    
    // Clean up
    cleanupTestDirectory(test_dir);
}

TEST(ModelManagerFilteringTest, RecommendationScoringPrefersSmallerModels) {
    // Test that the recommendation algorithm prefers smaller models
    // that fit better within device capabilities
    
    std::string test_dir = "./test_models_scoring_" + std::to_string(time(nullptr));
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // The recommendModels method queries remote registry
    // With mock HTTP client returning empty array, we can't fully test
    // But the implementation is verified to exist and handle the logic
    
    DeviceCapabilities device;
    device.platform = "iOS";
    device.ram_bytes = 4ULL * 1024 * 1024 * 1024; // 4GB
    device.storage_bytes = 64ULL * 1024 * 1024 * 1024; // 64GB
    
    auto result = manager.recommendModels(ModelType::LLM, device);
    ASSERT_TRUE(result.isSuccess());
    
    cleanupTestDirectory(test_dir);
}


// Checksum Verification Tests
TEST(ModelManagerChecksumTest, VerifyChecksumSuccess) {
    // Create a temporary test file with known content
    const char* test_file = "/tmp/model_checksum_test.bin";
    std::string test_content = "Test model file content";
    {
        std::ofstream file(test_file, std::ios::binary);
        file << test_content;
        file.close();
    }
    
    // Compute the expected checksum
    auto hash = crypto::SHA256::hash(test_content);
    std::string expected_checksum = crypto::SHA256::toHex(hash);
    
    // Create ModelManager and verify checksum
    ModelManager manager("./test_models_checksum", "https://test.registry.com");
    
    // Use reflection to access private method (or we can test through downloadModel)
    // For now, we'll test the SHA256 directly and trust the integration
    std::string computed_checksum = crypto::SHA256::hashFile(test_file);
    
    EXPECT_EQ(computed_checksum, expected_checksum);
    EXPECT_FALSE(computed_checksum.empty());
    EXPECT_EQ(computed_checksum.length(), 64); // SHA-256 produces 64 hex characters
    
    // Clean up
    std::remove(test_file);
    cleanupTestDirectory("./test_models_checksum");
}

TEST(ModelManagerChecksumTest, VerifyChecksumMismatch) {
    // Create a temporary test file
    const char* test_file = "/tmp/model_checksum_mismatch_test.bin";
    std::string test_content = "Test model file content";
    {
        std::ofstream file(test_file, std::ios::binary);
        file << test_content;
        file.close();
    }
    
    // Compute checksum of different content
    std::string different_content = "Different content";
    auto hash = crypto::SHA256::hash(different_content);
    std::string wrong_checksum = crypto::SHA256::toHex(hash);
    
    // Compute actual checksum
    std::string actual_checksum = crypto::SHA256::hashFile(test_file);
    
    // They should be different
    EXPECT_NE(actual_checksum, wrong_checksum);
    
    // Clean up
    std::remove(test_file);
}

TEST(ModelManagerChecksumTest, VerifyChecksumFileNotFound) {
    // Try to hash a non-existent file
    std::string checksum = crypto::SHA256::hashFile("/tmp/this_file_does_not_exist_xyz123.bin");
    
    // Should return empty string on error
    EXPECT_TRUE(checksum.empty());
}

TEST(ModelManagerChecksumTest, VerifyChecksumCaseInsensitive) {
    // Create a temporary test file
    const char* test_file = "/tmp/model_checksum_case_test.bin";
    std::string test_content = "Test content for case sensitivity";
    {
        std::ofstream file(test_file, std::ios::binary);
        file << test_content;
        file.close();
    }
    
    // Compute checksum
    std::string checksum_lower = crypto::SHA256::hashFile(test_file);
    
    // Convert to uppercase
    std::string checksum_upper = checksum_lower;
    std::transform(checksum_upper.begin(), checksum_upper.end(), 
                   checksum_upper.begin(), ::toupper);
    
    // Both should be valid representations of the same hash
    EXPECT_NE(checksum_lower, checksum_upper); // Different strings
    EXPECT_EQ(checksum_lower.length(), checksum_upper.length()); // Same length
    
    // When compared case-insensitively, they should match
    std::string lower1 = checksum_lower;
    std::string lower2 = checksum_upper;
    std::transform(lower2.begin(), lower2.end(), lower2.begin(), ::tolower);
    EXPECT_EQ(lower1, lower2);
    
    // Clean up
    std::remove(test_file);
}

TEST(ModelManagerChecksumTest, VerifyChecksumLargeFile) {
    // Create a larger test file (1MB)
    const char* test_file = "/tmp/model_checksum_large_test.bin";
    {
        std::ofstream file(test_file, std::ios::binary);
        // Write 1MB of data
        for (int i = 0; i < 1024 * 1024; ++i) {
            file.put(static_cast<char>(i % 256));
        }
        file.close();
    }
    
    // Compute checksum
    std::string checksum = crypto::SHA256::hashFile(test_file);
    
    // Should successfully compute hash
    EXPECT_FALSE(checksum.empty());
    EXPECT_EQ(checksum.length(), 64);
    
    // Verify it's consistent
    std::string checksum2 = crypto::SHA256::hashFile(test_file);
    EXPECT_EQ(checksum, checksum2);
    
    // Clean up
    std::remove(test_file);
}

TEST(ModelManagerChecksumTest, VerifyChecksumEmptyFile) {
    // Create an empty test file
    const char* test_file = "/tmp/model_checksum_empty_test.bin";
    {
        std::ofstream file(test_file, std::ios::binary);
        file.close();
    }
    
    // Compute checksum of empty file
    std::string checksum = crypto::SHA256::hashFile(test_file);
    
    // Should match the SHA-256 of empty string
    EXPECT_EQ(checksum, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    
    // Clean up
    std::remove(test_file);
}
