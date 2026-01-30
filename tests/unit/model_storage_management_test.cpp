#include <gtest/gtest.h>
#include "ondeviceai/model_manager.hpp"
#include "ondeviceai/logger.hpp"
#include <fstream>
#include <sys/stat.h>

using namespace ondeviceai;

class ModelStorageManagementTest : public ::testing::Test {
protected:
    std::string test_storage_path_;
    
    void SetUp() override {
        // Set log level to INFO to see what's happening
        Logger::getInstance().setLogLevel(LogLevel::Info);
        
        // Create unique test directory
        test_storage_path_ = "./test_storage_mgmt_" + std::to_string(time(nullptr));
        mkdir(test_storage_path_.c_str(), 0755);
    }
    
    void TearDown() override {
        // Clean up test files
        std::remove((test_storage_path_ + "/test_model_1").c_str());
        std::remove((test_storage_path_ + "/test_model_2").c_str());
        std::remove((test_storage_path_ + "/incomplete_1.tmp").c_str());
        std::remove((test_storage_path_ + "/incomplete_2.tmp").c_str());
        std::remove((test_storage_path_ + "/registry.json").c_str());
        std::remove((test_storage_path_ + "/pinned_versions.json").c_str());
        rmdir(test_storage_path_.c_str());
    }
    
    // Helper to create a fake model file
    void createFakeModelFile(const std::string& filename, size_t size_bytes) {
        std::string path = test_storage_path_ + "/" + filename;
        std::ofstream file(path, std::ios::binary);
        std::vector<char> data(size_bytes, 'X');
        file.write(data.data(), data.size());
        file.close();
    }
    
    // Helper to create a registry with models
    void createRegistryWithModels(const std::vector<std::string>& model_ids, 
                                   const std::vector<size_t>& sizes) {
        std::string registry_path = test_storage_path_ + "/registry.json";
        std::ofstream file(registry_path);
        
        file << "{\n";
        file << "  \"version\": \"1.0\",\n";
        file << "  \"models\": [\n";
        for (size_t i = 0; i < model_ids.size(); ++i) {
            if (i > 0) file << ",\n";
            file << "    {\n";
            file << "      \"id\": \"" << model_ids[i] << "\",\n";
            file << "      \"name\": \"Test Model " << (i+1) << "\",\n";
            file << "      \"type\": \"LLM\",\n";
            file << "      \"version\": \"1.0.0\",\n";
            file << "      \"size_bytes\": " << sizes[i] << ",\n";
            file << "      \"download_url\": \"http://example.com/model.gguf\",\n";
            file << "      \"checksum_sha256\": \"abc123\",\n";
            file << "      \"metadata\": {},\n";
            file << "      \"requirements\": {\n";
            file << "        \"min_ram_bytes\": 0,\n";
            file << "        \"min_storage_bytes\": 0,\n";
            file << "        \"supported_platforms\": []\n";
            file << "      }\n";
            file << "    }";
        }
        file << "\n  ]\n";
        file << "}\n";
        file.close();
    }
};

// Test: Delete model successfully
TEST_F(ModelStorageManagementTest, DeleteModel_Success) {
    // Create a fake model file
    createFakeModelFile("test_model_1", 1024 * 1024); // 1MB
    
    // Create registry with the model
    createRegistryWithModels({"test_model_1"}, {1024 * 1024});
    
    // Create ModelManager (will load the registry)
    ModelManager manager(test_storage_path_, "http://example.com/registry.json");
    
    // Verify model exists
    auto info_result = manager.getModelInfo("test_model_1");
    ASSERT_TRUE(info_result.isSuccess());
    
    // Delete the model
    auto delete_result = manager.deleteModel("test_model_1");
    ASSERT_TRUE(delete_result.isSuccess());
    
    // Verify model no longer exists in registry
    auto info_result2 = manager.getModelInfo("test_model_1");
    ASSERT_TRUE(info_result2.isError());
    EXPECT_EQ(info_result2.error().code, ErrorCode::ModelNotFoundInRegistry);
    
    // Verify file was deleted
    struct stat st;
    EXPECT_NE(0, stat((test_storage_path_ + "/test_model_1").c_str(), &st));
}

// Test: Delete non-existent model
TEST_F(ModelStorageManagementTest, DeleteModel_NotFound) {
    ModelManager manager(test_storage_path_, "http://example.com/registry.json");
    
    auto result = manager.deleteModel("nonexistent_model");
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelNotFoundInRegistry);
}

// Test: Delete model updates storage info
TEST_F(ModelStorageManagementTest, DeleteModel_UpdatesStorageInfo) {
    // Create two fake model files
    createFakeModelFile("test_model_1", 1024 * 1024); // 1MB
    createFakeModelFile("test_model_2", 2 * 1024 * 1024); // 2MB
    
    // Create registry with both models
    createRegistryWithModels(
        {"test_model_1", "test_model_2"}, 
        {1024 * 1024, 2 * 1024 * 1024}
    );
    
    ModelManager manager(test_storage_path_, "http://example.com/registry.json");
    
    // Get initial storage info
    auto storage1 = manager.getStorageInfo();
    ASSERT_TRUE(storage1.isSuccess());
    EXPECT_EQ(3 * 1024 * 1024, storage1.value().used_by_models_bytes);
    
    // Delete one model
    auto delete_result = manager.deleteModel("test_model_1");
    ASSERT_TRUE(delete_result.isSuccess());
    
    // Get updated storage info
    auto storage2 = manager.getStorageInfo();
    ASSERT_TRUE(storage2.isSuccess());
    EXPECT_EQ(2 * 1024 * 1024, storage2.value().used_by_models_bytes);
}

// Test: Cleanup incomplete downloads
TEST_F(ModelStorageManagementTest, CleanupIncompleteDownloads_RemovesTempFiles) {
    // Create some temporary download files
    createFakeModelFile("incomplete_1.tmp", 512 * 1024); // 512KB
    createFakeModelFile("incomplete_2.tmp", 256 * 1024); // 256KB
    
    // Create a completed model file (should not be deleted)
    createFakeModelFile("completed_model", 1024 * 1024); // 1MB
    
    // Create ModelManager (will automatically cleanup on init)
    ModelManager manager(test_storage_path_, "http://example.com/registry.json");
    
    // Verify temp files were deleted
    struct stat st;
    EXPECT_NE(0, stat((test_storage_path_ + "/incomplete_1.tmp").c_str(), &st));
    EXPECT_NE(0, stat((test_storage_path_ + "/incomplete_2.tmp").c_str(), &st));
    
    // Verify completed model file still exists
    EXPECT_EQ(0, stat((test_storage_path_ + "/completed_model").c_str(), &st));
}

// Test: Cleanup with no incomplete downloads
TEST_F(ModelStorageManagementTest, CleanupIncompleteDownloads_NoTempFiles) {
    // Create ModelManager with no temp files
    ModelManager manager(test_storage_path_, "http://example.com/registry.json");
    
    // Should succeed without errors
    auto result = manager.cleanupIncompleteDownloads();
    ASSERT_TRUE(result.isSuccess());
}

// Test: Cleanup can be called manually
TEST_F(ModelStorageManagementTest, CleanupIncompleteDownloads_ManualCall) {
    ModelManager manager(test_storage_path_, "http://example.com/registry.json");
    
    // Create temp files after initialization
    createFakeModelFile("new_incomplete.tmp", 128 * 1024);
    
    // Manually call cleanup
    auto result = manager.cleanupIncompleteDownloads();
    ASSERT_TRUE(result.isSuccess());
    
    // Verify temp file was deleted
    struct stat st;
    EXPECT_NE(0, stat((test_storage_path_ + "/new_incomplete.tmp").c_str(), &st));
}

// Test: Storage info reports correct values
TEST_F(ModelStorageManagementTest, GetStorageInfo_ReportsCorrectValues) {
    // Create models with known sizes
    createFakeModelFile("model_1", 1024 * 1024); // 1MB
    createFakeModelFile("model_2", 2 * 1024 * 1024); // 2MB
    
    createRegistryWithModels(
        {"model_1", "model_2"}, 
        {1024 * 1024, 2 * 1024 * 1024}
    );
    
    ModelManager manager(test_storage_path_, "http://example.com/registry.json");
    
    auto storage = manager.getStorageInfo();
    ASSERT_TRUE(storage.isSuccess());
    
    auto info = storage.value();
    
    // Verify used_by_models_bytes is correct
    EXPECT_EQ(3 * 1024 * 1024, info.used_by_models_bytes);
    
    // Verify total and available are reasonable
    EXPECT_GT(info.total_bytes, 0);
    EXPECT_GT(info.available_bytes, 0);
    EXPECT_LE(info.available_bytes, info.total_bytes);
}

// Test: List downloaded models includes all models
TEST_F(ModelStorageManagementTest, ListDownloadedModels_IncludesAllModels) {
    createRegistryWithModels(
        {"model_1", "model_2", "model_3"}, 
        {1024 * 1024, 2 * 1024 * 1024, 3 * 1024 * 1024}
    );
    
    ModelManager manager(test_storage_path_, "http://example.com/registry.json");
    
    auto result = manager.listDownloadedModels();
    ASSERT_TRUE(result.isSuccess());
    
    auto models = result.value();
    EXPECT_EQ(3, models.size());
    
    // Verify all models are present
    std::set<std::string> model_ids;
    for (const auto& model : models) {
        model_ids.insert(model.id);
    }
    
    EXPECT_TRUE(model_ids.count("model_1") > 0);
    EXPECT_TRUE(model_ids.count("model_2") > 0);
    EXPECT_TRUE(model_ids.count("model_3") > 0);
}

// Test: Delete model removes it from list
TEST_F(ModelStorageManagementTest, DeleteModel_RemovesFromList) {
    createFakeModelFile("model_1", 1024 * 1024);
    createFakeModelFile("model_2", 2 * 1024 * 1024);
    
    createRegistryWithModels(
        {"model_1", "model_2"}, 
        {1024 * 1024, 2 * 1024 * 1024}
    );
    
    ModelManager manager(test_storage_path_, "http://example.com/registry.json");
    
    // Initial list should have 2 models
    auto list1 = manager.listDownloadedModels();
    ASSERT_TRUE(list1.isSuccess());
    EXPECT_EQ(2, list1.value().size());
    
    // Delete one model
    auto delete_result = manager.deleteModel("model_1");
    ASSERT_TRUE(delete_result.isSuccess());
    
    // List should now have 1 model
    auto list2 = manager.listDownloadedModels();
    ASSERT_TRUE(list2.isSuccess());
    EXPECT_EQ(1, list2.value().size());
    EXPECT_EQ("model_2", list2.value()[0].id);
}

// Test: Storage warnings are logged appropriately
TEST_F(ModelStorageManagementTest, GetStorageInfo_LogsWarnings) {
    // This test verifies that getStorageInfo succeeds
    // Actual warning logging is tested through log output inspection
    ModelManager manager(test_storage_path_, "http://example.com/registry.json");
    
    auto result = manager.getStorageInfo();
    ASSERT_TRUE(result.isSuccess());
    
    // The method should succeed regardless of storage level
    auto info = result.value();
    EXPECT_GE(info.total_bytes, info.used_by_models_bytes);
}

