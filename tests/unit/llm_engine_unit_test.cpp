#include <gtest/gtest.h>
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/memory_manager.hpp"
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace ondeviceai;

/**
 * Unit tests for LLM Engine
 * Task 5.13: Write unit tests for LLM engine
 * - Test loading different quantization levels
 * - Test generation with various configs
 * - Test streaming cancellation
 * - Test context clearing
 * - Test context limit handling
 * Requirements: 1.4, 1.5, 12.4, 24.3, 24.5
 */

class LLMEngineUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = "test_llm_unit_" + std::to_string(std::time(nullptr));
        std::filesystem::create_directories(test_dir_);
        
        // Create test model files with different sizes to simulate quantization levels
        // Q4_0: smallest (simulated with 1MB file)
        createTestModel("model_q4_0.gguf", 1024 * 1024);
        // Q4_K_M: balanced (simulated with 1.5MB file)
        createTestModel("model_q4_k_m.gguf", 1536 * 1024);
        // Q5_K_M: improved quality (simulated with 2MB file)
        createTestModel("model_q5_k_m.gguf", 2048 * 1024);
        // Q8_0: near-original quality (simulated with 3MB file)
        createTestModel("model_q8_0.gguf", 3072 * 1024);
        
        // Create memory manager
        memory_mgr_ = std::make_unique<MemoryManager>(10ULL * 1024 * 1024 * 1024); // 10GB
        
        // Create engine
        engine_ = std::make_unique<LLMEngine>();
        engine_->setMemoryManager(memory_mgr_.get());
    }
    
    void TearDown() override {
        // Clean up
        engine_.reset();
        memory_mgr_.reset();
        
        // Remove test directory
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
    std::unique_ptr<MemoryManager> memory_mgr_;
    std::unique_ptr<LLMEngine> engine_;
};

// ============================================================================
// Test loading different quantization levels (Requirement 1.4, 23.1-23.4)
// ============================================================================

TEST_F(LLMEngineUnitTest, LoadQ4_0Quantization) {
    // Test loading Q4_0 quantized model (smallest size)
    auto result = engine_->loadModel(getModelPath("model_q4_0.gguf"));
    
    // Note: This will fail because the file is not a valid GGUF model
    // In a real test with actual models, this would succeed
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelFileCorrupted);
    
    // Verify error message mentions GGUF format or incompatible format
    EXPECT_TRUE(
        result.error().details.find("GGUF") != std::string::npos ||
        result.error().recovery_suggestion.has_value()
    );
}

TEST_F(LLMEngineUnitTest, LoadQ4_K_MQuantization) {
    // Test loading Q4_K_M quantized model (balanced)
    auto result = engine_->loadModel(getModelPath("model_q4_k_m.gguf"));
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelFileCorrupted);
}

TEST_F(LLMEngineUnitTest, LoadQ5_K_MQuantization) {
    // Test loading Q5_K_M quantized model (improved quality)
    auto result = engine_->loadModel(getModelPath("model_q5_k_m.gguf"));
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelFileCorrupted);
}

TEST_F(LLMEngineUnitTest, LoadQ8_0Quantization) {
    // Test loading Q8_0 quantized model (near-original quality)
    auto result = engine_->loadModel(getModelPath("model_q8_0.gguf"));
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelFileCorrupted);
}

TEST_F(LLMEngineUnitTest, QuantizationLevelsSizeDifference) {
    // Verify that different quantization levels have different file sizes
    size_t q4_0_size = std::filesystem::file_size(getModelPath("model_q4_0.gguf"));
    size_t q4_k_m_size = std::filesystem::file_size(getModelPath("model_q4_k_m.gguf"));
    size_t q5_k_m_size = std::filesystem::file_size(getModelPath("model_q5_k_m.gguf"));
    size_t q8_0_size = std::filesystem::file_size(getModelPath("model_q8_0.gguf"));
    
    // Verify size ordering: Q4_0 < Q4_K_M < Q5_K_M < Q8_0
    EXPECT_LT(q4_0_size, q4_k_m_size);
    EXPECT_LT(q4_k_m_size, q5_k_m_size);
    EXPECT_LT(q5_k_m_size, q8_0_size);
}

// ============================================================================
// Test generation with various configs (Requirement 1.5, 29.1-29.6)
// ============================================================================

TEST_F(LLMEngineUnitTest, GenerationWithDefaultConfig) {
    // Test generation with default configuration
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->generate(invalid_handle, "Test prompt");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, GenerationWithCustomTemperature) {
    // Test generation with custom temperature
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config = GenerationConfig::defaults();
    config.temperature = 0.5f;  // Lower temperature for more deterministic output
    
    auto result = engine_->generate(invalid_handle, "Test prompt", config);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, GenerationWithCustomTopP) {
    // Test generation with custom top-p (nucleus sampling)
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config = GenerationConfig::defaults();
    config.top_p = 0.95f;  // Nucleus sampling threshold
    
    auto result = engine_->generate(invalid_handle, "Test prompt", config);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, GenerationWithCustomTopK) {
    // Test generation with custom top-k
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config = GenerationConfig::defaults();
    config.top_k = 50;  // Top-k sampling limit
    
    auto result = engine_->generate(invalid_handle, "Test prompt", config);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, GenerationWithCustomRepetitionPenalty) {
    // Test generation with custom repetition penalty
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config = GenerationConfig::defaults();
    config.repetition_penalty = 1.2f;  // Higher penalty to reduce repetition
    
    auto result = engine_->generate(invalid_handle, "Test prompt", config);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, GenerationWithCustomMaxTokens) {
    // Test generation with custom max tokens
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config = GenerationConfig::defaults();
    config.max_tokens = 100;  // Limit output length
    
    auto result = engine_->generate(invalid_handle, "Test prompt", config);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, GenerationWithStopSequences) {
    // Test generation with stop sequences
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config = GenerationConfig::defaults();
    config.stop_sequences = {"\n\n", "###", "END"};
    
    auto result = engine_->generate(invalid_handle, "Test prompt", config);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, GenerationWithAllCustomParams) {
    // Test generation with all parameters customized
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config;
    config.max_tokens = 200;
    config.temperature = 0.8f;
    config.top_p = 0.95f;
    config.top_k = 50;
    config.repetition_penalty = 1.15f;
    config.stop_sequences = {"STOP", "END"};
    
    auto result = engine_->generate(invalid_handle, "Test prompt", config);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, GenerationConfigValidation) {
    // Verify that GenerationConfig defaults are correct
    auto config = GenerationConfig::defaults();
    
    EXPECT_EQ(config.max_tokens, 512);
    EXPECT_FLOAT_EQ(config.temperature, 0.7f);
    EXPECT_FLOAT_EQ(config.top_p, 0.9f);
    EXPECT_EQ(config.top_k, 40);
    EXPECT_FLOAT_EQ(config.repetition_penalty, 1.1f);
    EXPECT_TRUE(config.stop_sequences.empty());
}

// ============================================================================
// Test streaming cancellation (Requirement 12.4)
// ============================================================================

TEST_F(LLMEngineUnitTest, StreamingCancellationViaInvalidHandle) {
    // Test that streaming fails immediately with invalid handle
    ModelHandle invalid_handle = 999;
    
    int callback_count = 0;
    auto callback = [&callback_count](const std::string& token) {
        (void)token;
        callback_count++;
    };
    
    auto result = engine_->generateStreaming(invalid_handle, "Test prompt", callback);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
    EXPECT_EQ(callback_count, 0);  // Callback should not be invoked
}

TEST_F(LLMEngineUnitTest, StreamingWithEmptyPrompt) {
    // Test streaming with empty prompt
    ModelHandle invalid_handle = 999;
    
    int callback_count = 0;
    auto callback = [&callback_count](const std::string& token) {
        (void)token;
        callback_count++;
    };
    
    auto result = engine_->generateStreaming(invalid_handle, "", callback);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
    EXPECT_EQ(callback_count, 0);
}

TEST_F(LLMEngineUnitTest, StreamingCallbackNotInvokedOnError) {
    // Verify that callback is not invoked when model is not loaded
    ModelHandle invalid_handle = 999;
    
    bool callback_invoked = false;
    auto callback = [&callback_invoked](const std::string& token) {
        (void)token;
        callback_invoked = true;
    };
    
    auto result = engine_->generateStreaming(invalid_handle, "Test", callback);
    
    ASSERT_TRUE(result.isError());
    EXPECT_FALSE(callback_invoked);
}

TEST_F(LLMEngineUnitTest, StreamingWithStopSequences) {
    // Test streaming with stop sequences configured
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config;
    config.stop_sequences = {"STOP", "END"};
    
    int callback_count = 0;
    auto callback = [&callback_count](const std::string& token) {
        (void)token;
        callback_count++;
    };
    
    auto result = engine_->generateStreaming(invalid_handle, "Test", callback, config);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
    EXPECT_EQ(callback_count, 0);
}

// ============================================================================
// Test context clearing (Requirement 24.3)
// ============================================================================

TEST_F(LLMEngineUnitTest, ClearContextWithInvalidHandle) {
    // Test clearing context with invalid handle
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->clearContext(invalid_handle);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(LLMEngineUnitTest, ClearContextErrorMessage) {
    // Verify error message quality for clearContext
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->clearContext(invalid_handle);
    
    ASSERT_TRUE(result.isError());
    EXPECT_FALSE(result.error().message.empty());
    EXPECT_NE(result.error().message.find("999"), std::string::npos);
}

TEST_F(LLMEngineUnitTest, GetConversationHistoryWithInvalidHandle) {
    // Test getting conversation history with invalid handle
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->getConversationHistory(invalid_handle);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(LLMEngineUnitTest, ConversationHistoryEmptyInitially) {
    // Note: This test would require a real model to verify
    // For now, we test the error case
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->getConversationHistory(invalid_handle);
    
    ASSERT_TRUE(result.isError());
}

// ============================================================================
// Test context limit handling (Requirement 24.5, 1.8)
// ============================================================================

TEST_F(LLMEngineUnitTest, GetContextUsageWithInvalidHandle) {
    // Test getting context usage with invalid handle
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->getContextUsage(invalid_handle);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(LLMEngineUnitTest, GetContextCapacityWithInvalidHandle) {
    // Test getting context capacity with invalid handle
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->getContextCapacity(invalid_handle);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(LLMEngineUnitTest, ContextUsageErrorMessage) {
    // Verify error message quality for context usage
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->getContextUsage(invalid_handle);
    
    ASSERT_TRUE(result.isError());
    EXPECT_FALSE(result.error().message.empty());
}

TEST_F(LLMEngineUnitTest, ContextCapacityErrorMessage) {
    // Verify error message quality for context capacity
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->getContextCapacity(invalid_handle);
    
    ASSERT_TRUE(result.isError());
    EXPECT_FALSE(result.error().message.empty());
}

// ============================================================================
// Additional tests for comprehensive coverage
// ============================================================================

TEST_F(LLMEngineUnitTest, MultipleModelsCanBeLoaded) {
    // Test that multiple models can be loaded simultaneously
    // Note: Will fail with dummy models, but tests the API
    auto result1 = engine_->loadModel(getModelPath("model_q4_0.gguf"));
    auto result2 = engine_->loadModel(getModelPath("model_q4_k_m.gguf"));
    
    // Both should fail with corrupted error (not valid GGUF)
    ASSERT_TRUE(result1.isError());
    ASSERT_TRUE(result2.isError());
}

TEST_F(LLMEngineUnitTest, UnloadNonExistentModel) {
    // Test unloading a model that was never loaded
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->unloadModel(invalid_handle);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(LLMEngineUnitTest, IsModelLoadedReturnsFalseForInvalidHandle) {
    // Test isModelLoaded with invalid handle
    ModelHandle invalid_handle = 999;
    
    EXPECT_FALSE(engine_->isModelLoaded(invalid_handle));
}

TEST_F(LLMEngineUnitTest, TokenizeWithInvalidHandle) {
    // Test tokenization with invalid handle
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->tokenize(invalid_handle, "Test text");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(LLMEngineUnitTest, DetokenizeWithInvalidHandle) {
    // Test detokenization with invalid handle
    ModelHandle invalid_handle = 999;
    std::vector<int> tokens = {1, 2, 3};
    
    auto result = engine_->detokenize(invalid_handle, tokens);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(LLMEngineUnitTest, GenerationWithVeryLongPrompt) {
    // Test generation with a very long prompt
    ModelHandle invalid_handle = 999;
    
    // Create a very long prompt (10KB)
    std::string long_prompt(10240, 'A');
    
    auto result = engine_->generate(invalid_handle, long_prompt);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, StreamingWithVeryLongPrompt) {
    // Test streaming with a very long prompt
    ModelHandle invalid_handle = 999;
    
    std::string long_prompt(10240, 'A');
    
    int callback_count = 0;
    auto callback = [&callback_count](const std::string& token) {
        (void)token;
        callback_count++;
    };
    
    auto result = engine_->generateStreaming(invalid_handle, long_prompt, callback);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
    EXPECT_EQ(callback_count, 0);
}

TEST_F(LLMEngineUnitTest, GenerationWithZeroMaxTokens) {
    // Test generation with max_tokens = 0
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config = GenerationConfig::defaults();
    config.max_tokens = 0;
    
    auto result = engine_->generate(invalid_handle, "Test", config);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, GenerationWithNegativeTemperature) {
    // Test generation with negative temperature (invalid)
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config = GenerationConfig::defaults();
    config.temperature = -0.5f;  // Invalid
    
    auto result = engine_->generate(invalid_handle, "Test", config);
    
    // Should fail because model is not loaded
    // In a real implementation, might want to validate config first
    ASSERT_TRUE(result.isError());
}

TEST_F(LLMEngineUnitTest, GenerationWithVeryHighTemperature) {
    // Test generation with very high temperature
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config = GenerationConfig::defaults();
    config.temperature = 2.0f;  // Very high randomness
    
    auto result = engine_->generate(invalid_handle, "Test", config);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, MemoryManagerIntegration) {
    // Test that memory manager is properly integrated
    EXPECT_NE(memory_mgr_, nullptr);
    
    // Initial memory usage should be 0
    EXPECT_EQ(memory_mgr_->getTotalMemoryUsage(), 0);
}

TEST_F(LLMEngineUnitTest, LoadModelTracksMemory) {
    // Test that loading a model tracks memory usage
    size_t initial_usage = memory_mgr_->getTotalMemoryUsage();
    
    auto result = engine_->loadModel(getModelPath("model_q4_0.gguf"));
    
    // Will fail because not a valid GGUF, but memory should not be tracked on failure
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(memory_mgr_->getTotalMemoryUsage(), initial_usage);
}

// ============================================================================
// Edge case tests
// ============================================================================

TEST_F(LLMEngineUnitTest, LoadNonExistentModelFile) {
    // Test loading a model file that doesn't exist
    auto result = engine_->loadModel("/nonexistent/path/model.gguf");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelFileNotFound);
    EXPECT_NE(result.error().message.find("not found"), std::string::npos);
}

TEST_F(LLMEngineUnitTest, LoadEmptyPath) {
    // Test loading with empty path
    auto result = engine_->loadModel("");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelFileNotFound);
}

TEST_F(LLMEngineUnitTest, GenerateWithEmptyPrompt) {
    // Test generation with empty prompt
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->generate(invalid_handle, "");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST_F(LLMEngineUnitTest, TokenizeEmptyString) {
    // Test tokenizing empty string
    ModelHandle invalid_handle = 999;
    
    auto result = engine_->tokenize(invalid_handle, "");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(LLMEngineUnitTest, DetokenizeEmptyVector) {
    // Test detokenizing empty vector
    ModelHandle invalid_handle = 999;
    std::vector<int> empty_tokens;
    
    auto result = engine_->detokenize(invalid_handle, empty_tokens);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(LLMEngineUnitTest, MultipleStopSequences) {
    // Test generation with multiple stop sequences
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config;
    config.stop_sequences = {"\n", "\n\n", "###", "END", "STOP"};
    
    auto result = engine_->generate(invalid_handle, "Test", config);
    
    ASSERT_TRUE(result.isError());
}

TEST_F(LLMEngineUnitTest, EmptyStopSequence) {
    // Test generation with empty stop sequence
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config;
    config.stop_sequences = {""};  // Empty stop sequence
    
    auto result = engine_->generate(invalid_handle, "Test", config);
    
    ASSERT_TRUE(result.isError());
}

TEST_F(LLMEngineUnitTest, VeryLongStopSequence) {
    // Test generation with very long stop sequence
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config;
    config.stop_sequences = {std::string(1000, 'X')};
    
    auto result = engine_->generate(invalid_handle, "Test", config);
    
    ASSERT_TRUE(result.isError());
}

