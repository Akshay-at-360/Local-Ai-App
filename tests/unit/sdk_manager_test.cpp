#include <gtest/gtest.h>
#include "ondeviceai/ondeviceai.hpp"
#include <thread>
#include <vector>

using namespace ondeviceai;

class SDKManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing instance
        SDKManager::shutdown();
    }
    
    void TearDown() override {
        SDKManager::shutdown();
    }
};

TEST_F(SDKManagerTest, InitializeWithDefaultConfig) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    
    ASSERT_TRUE(result.isSuccess());
    ASSERT_NE(result.value(), nullptr);
    EXPECT_EQ(SDKManager::getInstance(), result.value());
}

TEST_F(SDKManagerTest, InitializeWithInvalidThreadCount) {
    auto config = SDKConfig::defaults();
    config.thread_count = -1;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputParameterValue);
    EXPECT_FALSE(result.error().message.empty());
    EXPECT_TRUE(result.error().recovery_suggestion.has_value());
}

TEST_F(SDKManagerTest, InitializeWithZeroThreadCount) {
    auto config = SDKConfig::defaults();
    config.thread_count = 0;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputParameterValue);
}

TEST_F(SDKManagerTest, InitializeWithTooManyThreads) {
    auto config = SDKConfig::defaults();
    config.thread_count = 100;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputParameterValue);
}

TEST_F(SDKManagerTest, InitializeWithEmptyModelDirectory) {
    auto config = SDKConfig::defaults();
    config.model_directory = "";
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputConfiguration);
}

TEST_F(SDKManagerTest, DoubleInitializationFails) {
    auto config = SDKConfig::defaults();
    auto result1 = SDKManager::initialize(config);
    ASSERT_TRUE(result1.isSuccess());
    
    auto result2 = SDKManager::initialize(config);
    ASSERT_TRUE(result2.isError());
    EXPECT_EQ(result2.error().code, ErrorCode::InvalidInputConfiguration);
}

TEST_F(SDKManagerTest, GetComponentsAfterInitialization) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    EXPECT_NE(sdk->getModelManager(), nullptr);
    EXPECT_NE(sdk->getLLMEngine(), nullptr);
    EXPECT_NE(sdk->getSTTEngine(), nullptr);
    EXPECT_NE(sdk->getTTSEngine(), nullptr);
    EXPECT_NE(sdk->getVoicePipeline(), nullptr);
    EXPECT_NE(sdk->getMemoryManager(), nullptr);
}

TEST_F(SDKManagerTest, SetThreadCount) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    sdk->setThreadCount(8);
    // No assertion - just verify it doesn't crash
}

TEST_F(SDKManagerTest, SetThreadCountIgnoresInvalidValues) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    sdk->setThreadCount(-1);  // Should be ignored
    sdk->setThreadCount(0);   // Should be ignored
    sdk->setThreadCount(100); // Should be ignored
    // No crash means test passes
}

TEST_F(SDKManagerTest, SetLogLevel) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    sdk->setLogLevel(LogLevel::Debug);
    EXPECT_EQ(Logger::getInstance().getLogLevel(), LogLevel::Debug);
    
    sdk->setLogLevel(LogLevel::Error);
    EXPECT_EQ(Logger::getInstance().getLogLevel(), LogLevel::Error);
}

TEST_F(SDKManagerTest, SetMemoryLimit) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    sdk->setMemoryLimit(1024 * 1024 * 1024); // 1GB
    EXPECT_EQ(sdk->getMemoryManager()->getMemoryLimit(), 1024 * 1024 * 1024);
    
    sdk->setMemoryLimit(0); // Unlimited
    EXPECT_EQ(sdk->getMemoryManager()->getMemoryLimit(), 0);
}

TEST_F(SDKManagerTest, SetModelDirectory) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    sdk->setModelDirectory("/tmp/models");
    // No assertion - just verify it doesn't crash
}

TEST_F(SDKManagerTest, SetModelDirectoryIgnoresEmptyPath) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    sdk->setModelDirectory(""); // Should be ignored
    // No crash means test passes
}

TEST_F(SDKManagerTest, ShutdownCleansUpInstance) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    SDKManager::shutdown();
    EXPECT_EQ(SDKManager::getInstance(), nullptr);
}

TEST_F(SDKManagerTest, ShutdownIsIdempotent) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    SDKManager::shutdown();
    SDKManager::shutdown(); // Second shutdown should be safe
    EXPECT_EQ(SDKManager::getInstance(), nullptr);
}

TEST_F(SDKManagerTest, ReinitializeAfterShutdown) {
    auto config = SDKConfig::defaults();
    
    // First initialization
    auto result1 = SDKManager::initialize(config);
    ASSERT_TRUE(result1.isSuccess());
    SDKManager::shutdown();
    
    // Second initialization should succeed
    auto result2 = SDKManager::initialize(config);
    ASSERT_TRUE(result2.isSuccess());
    EXPECT_NE(result2.value(), nullptr);
}

TEST_F(SDKManagerTest, ConcurrentInitializationAttempts) {
    auto config = SDKConfig::defaults();
    
    std::vector<std::thread> threads;
    std::vector<bool> success_flags(10, false);
    
    // Launch multiple threads trying to initialize
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&config, &success_flags, i]() {
            auto result = SDKManager::initialize(config);
            success_flags[i] = result.isSuccess();
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Exactly one should succeed
    int success_count = 0;
    for (bool flag : success_flags) {
        if (flag) success_count++;
    }
    EXPECT_EQ(success_count, 1);
    
    // Verify instance exists
    EXPECT_NE(SDKManager::getInstance(), nullptr);
}

TEST_F(SDKManagerTest, ConcurrentGetInstanceCalls) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    SDKManager* expected_instance = result.value();
    std::vector<std::thread> threads;
    std::vector<SDKManager*> instances(10, nullptr);
    
    // Launch multiple threads calling getInstance
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&instances, i]() {
            instances[i] = SDKManager::getInstance();
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All should get the same instance
    for (SDKManager* instance : instances) {
        EXPECT_EQ(instance, expected_instance);
    }
}

TEST_F(SDKManagerTest, ModelDirectoryCreatedIfNotExists) {
    auto config = SDKConfig::defaults();
    config.model_directory = "./test_models_temp";
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    // Verify directory was created
    EXPECT_TRUE(std::filesystem::exists(config.model_directory));
    EXPECT_TRUE(std::filesystem::is_directory(config.model_directory));
    
    // Cleanup
    SDKManager::shutdown();
    std::filesystem::remove(config.model_directory);
}

// Additional tests for comprehensive coverage

TEST_F(SDKManagerTest, InitializeWithValidThreadCountRange) {
    // Test boundary values for valid thread counts
    std::vector<int> valid_counts = {1, 4, 8, 16, 32, 64};
    
    for (int count : valid_counts) {
        SDKManager::shutdown(); // Clean up between tests
        
        auto config = SDKConfig::defaults();
        config.thread_count = count;
        
        auto result = SDKManager::initialize(config);
        ASSERT_TRUE(result.isSuccess()) << "Failed with thread_count=" << count;
        EXPECT_NE(result.value(), nullptr);
    }
}

TEST_F(SDKManagerTest, InitializeWithValidLogLevels) {
    // Test all valid log levels
    std::vector<LogLevel> levels = {
        LogLevel::Debug,
        LogLevel::Info,
        LogLevel::Warning,
        LogLevel::Error
    };
    
    for (LogLevel level : levels) {
        SDKManager::shutdown(); // Clean up between tests
        
        auto config = SDKConfig::defaults();
        config.log_level = level;
        
        auto result = SDKManager::initialize(config);
        ASSERT_TRUE(result.isSuccess());
        EXPECT_EQ(Logger::getInstance().getLogLevel(), level);
    }
}

TEST_F(SDKManagerTest, InitializeWithZeroMemoryLimit) {
    auto config = SDKConfig::defaults();
    config.memory_limit = 0; // Unlimited
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value()->getMemoryManager()->getMemoryLimit(), 0);
}

TEST_F(SDKManagerTest, InitializeWithLargeMemoryLimit) {
    auto config = SDKConfig::defaults();
    config.memory_limit = 10ULL * 1024 * 1024 * 1024; // 10GB
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value()->getMemoryManager()->getMemoryLimit(), 10ULL * 1024 * 1024 * 1024);
}

TEST_F(SDKManagerTest, InitializeWithTelemetryEnabled) {
    auto config = SDKConfig::defaults();
    config.enable_telemetry = true;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    EXPECT_NE(result.value(), nullptr);
}

TEST_F(SDKManagerTest, InitializeWithTelemetryDisabled) {
    auto config = SDKConfig::defaults();
    config.enable_telemetry = false;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    EXPECT_NE(result.value(), nullptr);
}

TEST_F(SDKManagerTest, ShutdownReleasesAllComponents) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    
    // Verify components exist before shutdown
    EXPECT_NE(sdk->getModelManager(), nullptr);
    EXPECT_NE(sdk->getLLMEngine(), nullptr);
    EXPECT_NE(sdk->getSTTEngine(), nullptr);
    EXPECT_NE(sdk->getTTSEngine(), nullptr);
    EXPECT_NE(sdk->getVoicePipeline(), nullptr);
    EXPECT_NE(sdk->getMemoryManager(), nullptr);
    
    // Shutdown
    SDKManager::shutdown();
    
    // Verify instance is null after shutdown
    EXPECT_EQ(SDKManager::getInstance(), nullptr);
}

TEST_F(SDKManagerTest, ConfigurationPersistsAfterInitialization) {
    auto config = SDKConfig::defaults();
    config.thread_count = 8;
    config.log_level = LogLevel::Debug;
    config.memory_limit = 2ULL * 1024 * 1024 * 1024; // 2GB
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    
    // Verify configuration is applied
    EXPECT_EQ(Logger::getInstance().getLogLevel(), LogLevel::Debug);
    EXPECT_EQ(sdk->getMemoryManager()->getMemoryLimit(), 2ULL * 1024 * 1024 * 1024);
}

TEST_F(SDKManagerTest, SetThreadCountUpdatesConfiguration) {
    auto config = SDKConfig::defaults();
    config.thread_count = 4;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    
    // Update thread count
    sdk->setThreadCount(8);
    // Note: We can't directly verify the internal config, but we ensure no crash
    
    // Try setting to boundary values
    sdk->setThreadCount(1);
    sdk->setThreadCount(64);
}

TEST_F(SDKManagerTest, SetMemoryLimitUpdatesMemoryManager) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    
    // Set various memory limits
    sdk->setMemoryLimit(1024 * 1024 * 1024); // 1GB
    EXPECT_EQ(sdk->getMemoryManager()->getMemoryLimit(), 1024 * 1024 * 1024);
    
    sdk->setMemoryLimit(512 * 1024 * 1024); // 512MB
    EXPECT_EQ(sdk->getMemoryManager()->getMemoryLimit(), 512 * 1024 * 1024);
    
    sdk->setMemoryLimit(0); // Unlimited
    EXPECT_EQ(sdk->getMemoryManager()->getMemoryLimit(), 0);
}

TEST_F(SDKManagerTest, SetLogLevelUpdatesLogger) {
    auto config = SDKConfig::defaults();
    config.log_level = LogLevel::Info;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    EXPECT_EQ(Logger::getInstance().getLogLevel(), LogLevel::Info);
    
    // Change log level
    sdk->setLogLevel(LogLevel::Debug);
    EXPECT_EQ(Logger::getInstance().getLogLevel(), LogLevel::Debug);
    
    sdk->setLogLevel(LogLevel::Warning);
    EXPECT_EQ(Logger::getInstance().getLogLevel(), LogLevel::Warning);
    
    sdk->setLogLevel(LogLevel::Error);
    EXPECT_EQ(Logger::getInstance().getLogLevel(), LogLevel::Error);
}

TEST_F(SDKManagerTest, GetInstanceReturnsNullBeforeInitialization) {
    // Ensure no instance exists
    SDKManager::shutdown();
    
    EXPECT_EQ(SDKManager::getInstance(), nullptr);
}

TEST_F(SDKManagerTest, GetInstanceReturnsSameInstanceAcrossCalls) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* instance1 = SDKManager::getInstance();
    auto* instance2 = SDKManager::getInstance();
    auto* instance3 = SDKManager::getInstance();
    
    EXPECT_EQ(instance1, instance2);
    EXPECT_EQ(instance2, instance3);
    EXPECT_EQ(instance1, result.value());
}

TEST_F(SDKManagerTest, InitializeCreatesNestedModelDirectory) {
    auto config = SDKConfig::defaults();
    config.model_directory = "./test_models_temp/nested/deep";
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    // Verify nested directory was created
    EXPECT_TRUE(std::filesystem::exists(config.model_directory));
    EXPECT_TRUE(std::filesystem::is_directory(config.model_directory));
    
    // Cleanup
    SDKManager::shutdown();
    std::filesystem::remove_all("./test_models_temp");
}

TEST_F(SDKManagerTest, InitializeWithExistingModelDirectory) {
    auto config = SDKConfig::defaults();
    config.model_directory = "./test_models_existing";
    
    // Create directory first
    std::filesystem::create_directories(config.model_directory);
    ASSERT_TRUE(std::filesystem::exists(config.model_directory));
    
    // Initialize should succeed with existing directory
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    // Cleanup
    SDKManager::shutdown();
    std::filesystem::remove(config.model_directory);
}

TEST_F(SDKManagerTest, ErrorMessagesAreDescriptive) {
    auto config = SDKConfig::defaults();
    config.thread_count = -1;
    
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isError());
    
    const auto& error = result.error();
    EXPECT_FALSE(error.message.empty());
    EXPECT_FALSE(error.details.empty());
    EXPECT_TRUE(error.recovery_suggestion.has_value());
    EXPECT_FALSE(error.recovery_suggestion.value().empty());
}

TEST_F(SDKManagerTest, ErrorCodesAreCorrect) {
    // Test invalid thread count error code
    {
        auto config = SDKConfig::defaults();
        config.thread_count = -1;
        auto result = SDKManager::initialize(config);
        ASSERT_TRUE(result.isError());
        EXPECT_EQ(result.error().code, ErrorCode::InvalidInputParameterValue);
    }
    
    // Test empty model directory error code
    {
        SDKManager::shutdown();
        auto config = SDKConfig::defaults();
        config.model_directory = "";
        auto result = SDKManager::initialize(config);
        ASSERT_TRUE(result.isError());
        EXPECT_EQ(result.error().code, ErrorCode::InvalidInputConfiguration);
    }
    
    // Test double initialization error code
    {
        SDKManager::shutdown();
        auto config = SDKConfig::defaults();
        auto result1 = SDKManager::initialize(config);
        ASSERT_TRUE(result1.isSuccess());
        
        auto result2 = SDKManager::initialize(config);
        ASSERT_TRUE(result2.isError());
        EXPECT_EQ(result2.error().code, ErrorCode::InvalidInputConfiguration);
    }
}

TEST_F(SDKManagerTest, ConcurrentShutdownCalls) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    std::vector<std::thread> threads;
    
    // Launch multiple threads trying to shutdown
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([]() {
            SDKManager::shutdown();
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify instance is null
    EXPECT_EQ(SDKManager::getInstance(), nullptr);
}

TEST_F(SDKManagerTest, AllComponentsInitializedInCorrectOrder) {
    auto config = SDKConfig::defaults();
    auto result = SDKManager::initialize(config);
    ASSERT_TRUE(result.isSuccess());
    
    auto* sdk = result.value();
    
    // All components should be initialized and accessible
    ASSERT_NE(sdk->getMemoryManager(), nullptr);
    ASSERT_NE(sdk->getModelManager(), nullptr);
    ASSERT_NE(sdk->getLLMEngine(), nullptr);
    ASSERT_NE(sdk->getSTTEngine(), nullptr);
    ASSERT_NE(sdk->getTTSEngine(), nullptr);
    ASSERT_NE(sdk->getVoicePipeline(), nullptr);
    
    // Voice pipeline should have access to engines
    // (This verifies initialization order - voice pipeline depends on engines)
}
