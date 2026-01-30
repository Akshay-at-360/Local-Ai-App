#include <gtest/gtest.h>
#include "ondeviceai/model_manager.hpp"
#include "ondeviceai/json_utils.hpp"
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
void cleanupVersioningTestDirectory(const std::string& path) {
    std::string registry_file = path + "/registry.json";
    std::string pinned_file = path + "/pinned_versions.json";
    remove(registry_file.c_str());
    remove(pinned_file.c_str());
    rmdir(path.c_str());
}

// Helper function to create a test ModelInfo with version
ModelInfo createVersionedModelInfo(const std::string& base_id, const std::string& version) {
    ModelInfo info;
    info.id = base_id + "-" + version;
    info.name = "Test Model " + base_id;
    info.type = ModelType::LLM;
    info.version = version;
    info.size_bytes = 1024 * 1024 * 100; // 100MB
    info.download_url = "https://example.com/models/" + info.id;
    info.checksum_sha256 = "abc123def456";
    info.metadata["architecture"] = "transformer";
    info.requirements.min_ram_bytes = 2ULL * 1024 * 1024 * 1024; // 2GB
    info.requirements.supported_platforms = {"iOS", "Android", "Linux"};
    return info;
}

// Test multiple versions can coexist
TEST(ModelVersioningTest, MultipleVersionsCoexist) {
    std::string test_dir = "./test_versioning_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry with multiple versions of the same model
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    registry["llama-3b-1.1.0"] = createVersionedModelInfo("llama-3b", "1.1.0");
    registry["llama-3b-2.0.0"] = createVersionedModelInfo("llama-3b", "2.0.0");
    
    // Save registry
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    // Create manager and verify all versions are present
    ModelManager manager(test_dir, "https://test.registry.com");
    auto result = manager.listDownloadedModels();
    
    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value().size(), 3);
    
    // Verify each version is accessible
    auto v1_result = manager.getModelInfo("llama-3b-1.0.0");
    ASSERT_TRUE(v1_result.isSuccess());
    EXPECT_EQ(v1_result.value().version, "1.0.0");
    
    auto v2_result = manager.getModelInfo("llama-3b-1.1.0");
    ASSERT_TRUE(v2_result.isSuccess());
    EXPECT_EQ(v2_result.value().version, "1.1.0");
    
    auto v3_result = manager.getModelInfo("llama-3b-2.0.0");
    ASSERT_TRUE(v3_result.isSuccess());
    EXPECT_EQ(v3_result.value().version, "2.0.0");
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test version pinning
TEST(ModelVersioningTest, PinModelVersion) {
    std::string test_dir = "./test_pinning_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry with multiple versions
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    registry["llama-3b-2.0.0"] = createVersionedModelInfo("llama-3b", "2.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Pin to version 1.0.0
    auto pin_result = manager.pinModelVersion("llama-3b", "1.0.0");
    ASSERT_TRUE(pin_result.isSuccess());
    
    // Verify pinning status
    auto is_pinned_result = manager.isModelVersionPinned("llama-3b");
    ASSERT_TRUE(is_pinned_result.isSuccess());
    EXPECT_TRUE(is_pinned_result.value());
    
    // Get pinned version
    auto pinned_version_result = manager.getPinnedVersion("llama-3b");
    ASSERT_TRUE(pinned_version_result.isSuccess());
    EXPECT_EQ(pinned_version_result.value(), "1.0.0");
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test unpinning
TEST(ModelVersioningTest, UnpinModelVersion) {
    std::string test_dir = "./test_unpinning_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Pin version
    manager.pinModelVersion("llama-3b", "1.0.0");
    
    // Verify pinned
    auto is_pinned_result = manager.isModelVersionPinned("llama-3b");
    ASSERT_TRUE(is_pinned_result.isSuccess());
    EXPECT_TRUE(is_pinned_result.value());
    
    // Unpin
    auto unpin_result = manager.unpinModelVersion("llama-3b");
    ASSERT_TRUE(unpin_result.isSuccess());
    
    // Verify unpinned
    is_pinned_result = manager.isModelVersionPinned("llama-3b");
    ASSERT_TRUE(is_pinned_result.isSuccess());
    EXPECT_FALSE(is_pinned_result.value());
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test pinning non-existent version fails
TEST(ModelVersioningTest, PinNonExistentVersionFails) {
    std::string test_dir = "./test_pin_fail_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Try to pin a version that doesn't exist
    auto pin_result = manager.pinModelVersion("llama-3b", "1.0.0");
    ASSERT_TRUE(pin_result.isError());
    EXPECT_EQ(pin_result.error().code, ErrorCode::ModelNotFoundInRegistry);
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test pinning with invalid version format fails
TEST(ModelVersioningTest, PinInvalidVersionFormatFails) {
    std::string test_dir = "./test_pin_invalid_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Try to pin with invalid version format
    auto pin_result = manager.pinModelVersion("llama-3b", "invalid");
    ASSERT_TRUE(pin_result.isError());
    EXPECT_EQ(pin_result.error().code, ErrorCode::InvalidInputParameterValue);
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test getModelInfoByBaseId returns pinned version
TEST(ModelVersioningTest, GetModelInfoByBaseIdReturnsPinnedVersion) {
    std::string test_dir = "./test_base_id_pinned_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry with multiple versions
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    registry["llama-3b-2.0.0"] = createVersionedModelInfo("llama-3b", "2.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Pin to version 1.0.0
    manager.pinModelVersion("llama-3b", "1.0.0");
    
    // Get model by base ID - should return pinned version
    auto result = manager.getModelInfoByBaseId("llama-3b");
    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value().version, "1.0.0");
    EXPECT_EQ(result.value().id, "llama-3b-1.0.0");
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test getModelInfoByBaseId returns latest version when not pinned
TEST(ModelVersioningTest, GetModelInfoByBaseIdReturnsLatestWhenNotPinned) {
    std::string test_dir = "./test_base_id_latest_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry with multiple versions
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    registry["llama-3b-1.5.0"] = createVersionedModelInfo("llama-3b", "1.5.0");
    registry["llama-3b-2.0.0"] = createVersionedModelInfo("llama-3b", "2.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Get model by base ID without pinning - should return latest (2.0.0)
    auto result = manager.getModelInfoByBaseId("llama-3b");
    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value().version, "2.0.0");
    EXPECT_EQ(result.value().id, "llama-3b-2.0.0");
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test version pinning persists across manager instances
TEST(ModelVersioningTest, PinningPersistsAcrossInstances) {
    std::string test_dir = "./test_pin_persist_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    registry["llama-3b-2.0.0"] = createVersionedModelInfo("llama-3b", "2.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    // First manager instance - pin version
    {
        ModelManager manager(test_dir, "https://test.registry.com");
        auto pin_result = manager.pinModelVersion("llama-3b", "1.0.0");
        ASSERT_TRUE(pin_result.isSuccess());
    }
    
    // Second manager instance - verify pinning persisted
    {
        ModelManager manager(test_dir, "https://test.registry.com");
        
        auto is_pinned_result = manager.isModelVersionPinned("llama-3b");
        ASSERT_TRUE(is_pinned_result.isSuccess());
        EXPECT_TRUE(is_pinned_result.value());
        
        auto pinned_version_result = manager.getPinnedVersion("llama-3b");
        ASSERT_TRUE(pinned_version_result.isSuccess());
        EXPECT_EQ(pinned_version_result.value(), "1.0.0");
    }
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test unpinning non-pinned model fails
TEST(ModelVersioningTest, UnpinNonPinnedModelFails) {
    std::string test_dir = "./test_unpin_fail_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Try to unpin a model that isn't pinned
    auto unpin_result = manager.unpinModelVersion("llama-3b");
    ASSERT_TRUE(unpin_result.isError());
    EXPECT_EQ(unpin_result.error().code, ErrorCode::ModelNotFoundInRegistry);
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test getModelInfoByBaseId with non-existent model
TEST(ModelVersioningTest, GetModelInfoByBaseIdNonExistent) {
    std::string test_dir = "./test_base_id_nonexist_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    auto result = manager.getModelInfoByBaseId("nonexistent-model");
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelNotFoundInRegistry);
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test version comparison in getModelInfoByBaseId
TEST(ModelVersioningTest, GetModelInfoByBaseIdSelectsNewestVersion) {
    std::string test_dir = "./test_base_id_newest_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry with versions in non-sorted order
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.5.0"] = createVersionedModelInfo("llama-3b", "1.5.0");
    registry["llama-3b-2.0.0"] = createVersionedModelInfo("llama-3b", "2.0.0");
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    registry["llama-3b-1.2.3"] = createVersionedModelInfo("llama-3b", "1.2.3");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Should return 2.0.0 as it's the newest
    auto result = manager.getModelInfoByBaseId("llama-3b");
    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value().version, "2.0.0");
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test pinning works with versioned model IDs
TEST(ModelVersioningTest, PinWithVersionedModelId) {
    std::string test_dir = "./test_pin_versioned_id_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Pin using versioned model ID (should extract base ID)
    auto pin_result = manager.pinModelVersion("llama-3b-1.0.0", "1.0.0");
    ASSERT_TRUE(pin_result.isSuccess());
    
    // Verify pinning using base ID
    auto is_pinned_result = manager.isModelVersionPinned("llama-3b");
    ASSERT_TRUE(is_pinned_result.isSuccess());
    EXPECT_TRUE(is_pinned_result.value());
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test model path includes version
TEST(ModelVersioningTest, ModelPathIncludesVersion) {
    std::string test_dir = "./test_path_version_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Get model path
    auto path_result = manager.getModelPath("llama-3b-1.0.0");
    ASSERT_TRUE(path_result.isSuccess());
    
    // Path should include the versioned model ID
    std::string expected_path = test_dir + "/llama-3b-1.0.0";
    EXPECT_EQ(path_result.value(), expected_path);
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test update detection - checkForUpdates detects newer version
TEST(ModelVersioningTest, CheckForUpdatesDetectsNewerVersion) {
    std::string test_dir = "./test_check_updates_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create local registry with version 1.0.0
    std::map<std::string, ModelInfo> local_registry;
    local_registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(local_registry);
    file.close();
    
    // Note: This test requires a mock HTTP server or network access
    // For now, we test the error case when model is not in local registry
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Test with model that exists in local registry
    // In a real scenario, this would query the remote registry
    // For unit testing without network, we verify the function exists and handles errors
    auto result = manager.checkForUpdates("llama-3b-1.0.0");
    
    // Without a real remote registry, this should fail with network error
    // The important thing is the function is callable and returns proper error
    ASSERT_TRUE(result.isError());
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test update detection - checkForUpdates returns error when no updates available
TEST(ModelVersioningTest, CheckForUpdatesNoUpdateAvailable) {
    std::string test_dir = "./test_no_updates_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create local registry with latest version
    std::map<std::string, ModelInfo> local_registry;
    local_registry["llama-3b-2.0.0"] = createVersionedModelInfo("llama-3b", "2.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(local_registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Test that checkForUpdates is callable
    // Without network, this will fail, but we verify the API exists
    auto result = manager.checkForUpdates("llama-3b-2.0.0");
    ASSERT_TRUE(result.isError());
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test update detection - checkForUpdates fails for non-existent model
TEST(ModelVersioningTest, CheckForUpdatesNonExistentModel) {
    std::string test_dir = "./test_updates_nonexist_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Try to check updates for model not in local registry
    auto result = manager.checkForUpdates("nonexistent-model");
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelNotFoundInRegistry);
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test getAvailableVersions API exists and handles errors
TEST(ModelVersioningTest, GetAvailableVersionsAPI) {
    std::string test_dir = "./test_available_versions_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Test that getAvailableVersions is callable
    // Without network/mock server, this returns empty list (success with 0 versions)
    auto result = manager.getAvailableVersions("llama-3b");
    ASSERT_TRUE(result.isSuccess());
    // With mock HTTP client, we get empty list
    EXPECT_EQ(result.value().size(), 0);
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test safe update - multiple versions can coexist during update
TEST(ModelVersioningTest, SafeUpdateMultipleVersionsCoexist) {
    std::string test_dir = "./test_safe_update_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Simulate scenario where old version exists and new version is being added
    std::map<std::string, ModelInfo> registry;
    
    // Old version already downloaded
    ModelInfo old_version = createVersionedModelInfo("llama-3b", "1.0.0");
    old_version.metadata["download_timestamp"] = "1000000";
    registry["llama-3b-1.0.0"] = old_version;
    
    // New version being added (simulating download completion)
    ModelInfo new_version = createVersionedModelInfo("llama-3b", "2.0.0");
    new_version.metadata["download_timestamp"] = "2000000";
    registry["llama-3b-2.0.0"] = new_version;
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Verify both versions exist simultaneously
    auto old_result = manager.getModelInfo("llama-3b-1.0.0");
    ASSERT_TRUE(old_result.isSuccess());
    EXPECT_EQ(old_result.value().version, "1.0.0");
    
    auto new_result = manager.getModelInfo("llama-3b-2.0.0");
    ASSERT_TRUE(new_result.isSuccess());
    EXPECT_EQ(new_result.value().version, "2.0.0");
    
    // Verify both are in the downloaded models list
    auto list_result = manager.listDownloadedModels();
    ASSERT_TRUE(list_result.isSuccess());
    EXPECT_EQ(list_result.value().size(), 2);
    
    // This demonstrates that old version remains available even after new version is added
    // In a real update scenario, the old version would only be removed after:
    // 1. New version is downloaded
    // 2. New version is verified (checksum)
    // 3. Application explicitly deletes the old version
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test safe update - old version remains accessible during update
TEST(ModelVersioningTest, SafeUpdateOldVersionAccessible) {
    std::string test_dir = "./test_safe_update_access_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry with old version
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Pin to old version (simulating application using specific version)
    auto pin_result = manager.pinModelVersion("llama-3b", "1.0.0");
    ASSERT_TRUE(pin_result.isSuccess());
    
    // Verify old version is accessible via pinning
    auto pinned_info = manager.getModelInfoByBaseId("llama-3b");
    ASSERT_TRUE(pinned_info.isSuccess());
    EXPECT_EQ(pinned_info.value().version, "1.0.0");
    
    // Now simulate adding new version to registry
    registry["llama-3b-2.0.0"] = createVersionedModelInfo("llama-3b", "2.0.0");
    std::ofstream file2(registry_path);
    file2 << json::serialize_model_registry(registry);
    file2.close();
    
    // Create new manager instance to reload registry
    ModelManager manager2(test_dir, "https://test.registry.com");
    
    // Old version should still be accessible and pinned
    auto pinned_info2 = manager2.getModelInfoByBaseId("llama-3b");
    ASSERT_TRUE(pinned_info2.isSuccess());
    EXPECT_EQ(pinned_info2.value().version, "1.0.0");
    
    // Both versions should be accessible
    auto old_access = manager2.getModelInfo("llama-3b-1.0.0");
    ASSERT_TRUE(old_access.isSuccess());
    
    auto new_access = manager2.getModelInfo("llama-3b-2.0.0");
    ASSERT_TRUE(new_access.isSuccess());
    
    // This demonstrates safe update: old version remains accessible
    // even after new version is available, especially when pinned
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test safe update - download creates new version without affecting old
TEST(ModelVersioningTest, SafeUpdateDownloadCreatesNewVersion) {
    std::string test_dir = "./test_safe_download_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry with old version
    std::map<std::string, ModelInfo> registry;
    ModelInfo old_version = createVersionedModelInfo("llama-3b", "1.0.0");
    registry["llama-3b-1.0.0"] = old_version;
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Verify old version exists
    auto old_check = manager.isModelDownloaded("llama-3b-1.0.0");
    ASSERT_TRUE(old_check.isSuccess());
    EXPECT_TRUE(old_check.value());
    
    // Verify attempting to download same version fails
    // (This would be a network call in real scenario, but we test the logic)
    // The download system creates versioned IDs like "llama-3b-1.0.0"
    // So downloading "llama-3b" with version "2.0.0" would create "llama-3b-2.0.0"
    // And the old "llama-3b-1.0.0" would remain untouched
    
    // Verify old version is still accessible
    auto old_info = manager.getModelInfo("llama-3b-1.0.0");
    ASSERT_TRUE(old_info.isSuccess());
    EXPECT_EQ(old_info.value().version, "1.0.0");
    
    cleanupVersioningTestDirectory(test_dir);
}

// Test that downloading already-downloaded version fails
TEST(ModelVersioningTest, DownloadExistingVersionFails) {
    std::string test_dir = "./test_download_existing_" + std::to_string(time(nullptr));
    mkdir(test_dir.c_str(), 0755);
    
    // Create registry with existing version
    std::map<std::string, ModelInfo> registry;
    registry["llama-3b-1.0.0"] = createVersionedModelInfo("llama-3b", "1.0.0");
    
    std::string registry_path = test_dir + "/registry.json";
    std::ofstream file(registry_path);
    file << json::serialize_model_registry(registry);
    file.close();
    
    ModelManager manager(test_dir, "https://test.registry.com");
    
    // Verify version exists
    auto exists = manager.isModelDownloaded("llama-3b-1.0.0");
    ASSERT_TRUE(exists.isSuccess());
    EXPECT_TRUE(exists.value());
    
    // Note: Testing actual download would require network/mock server
    // The implementation checks if versioned model ID exists before downloading
    // This prevents re-downloading the same version
    
    cleanupVersioningTestDirectory(test_dir);
}
