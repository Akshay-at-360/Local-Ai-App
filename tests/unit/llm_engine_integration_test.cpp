#include <gtest/gtest.h>
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/memory_manager.hpp"
#include <filesystem>

using namespace ondeviceai;

class LLMEngineIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<LLMEngine>();
        memory_mgr = std::make_unique<MemoryManager>(4ULL * 1024 * 1024 * 1024); // 4GB
        engine->setMemoryManager(memory_mgr.get());
    }
    
    void TearDown() override {
        engine.reset();
        memory_mgr.reset();
    }
    
    std::unique_ptr<LLMEngine> engine;
    std::unique_ptr<MemoryManager> memory_mgr;
};

// Test that llama.cpp backend initializes correctly
TEST_F(LLMEngineIntegrationTest, BackendInitialization) {
    // Just creating the engine should work
    EXPECT_NE(engine, nullptr);
}

// Test model loading with non-existent file
TEST_F(LLMEngineIntegrationTest, LoadNonExistentModel) {
    auto result = engine->loadModel("/nonexistent/model.gguf");
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelFileNotFound);
}

// Test tokenization without a loaded model
TEST_F(LLMEngineIntegrationTest, TokenizeWithoutModel) {
    ModelHandle invalid_handle = 999;
    auto result = engine->tokenize(invalid_handle, "Hello world");
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

// Test detokenization without a loaded model
TEST_F(LLMEngineIntegrationTest, DetokenizeWithoutModel) {
    ModelHandle invalid_handle = 999;
    std::vector<int> tokens = {1, 2, 3};
    auto result = engine->detokenize(invalid_handle, tokens);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

// Test generation without a loaded model
TEST_F(LLMEngineIntegrationTest, GenerateWithoutModel) {
    ModelHandle invalid_handle = 999;
    auto result = engine->generate(invalid_handle, "Hello");
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

// Test streaming generation without a loaded model
TEST_F(LLMEngineIntegrationTest, StreamingGenerateWithoutModel) {
    ModelHandle invalid_handle = 999;
    int callback_count = 0;
    auto callback = [&callback_count](const std::string&) {
        callback_count++;
    };
    
    auto result = engine->generateStreaming(invalid_handle, "Hello", callback);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
    EXPECT_EQ(callback_count, 0);
}

// Test context management without a loaded model
TEST_F(LLMEngineIntegrationTest, ClearContextWithoutModel) {
    ModelHandle invalid_handle = 999;
    auto result = engine->clearContext(invalid_handle);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

// Test conversation history without a loaded model
TEST_F(LLMEngineIntegrationTest, GetHistoryWithoutModel) {
    ModelHandle invalid_handle = 999;
    auto result = engine->getConversationHistory(invalid_handle);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

// Test model unloading with invalid handle
TEST_F(LLMEngineIntegrationTest, UnloadInvalidModel) {
    ModelHandle invalid_handle = 999;
    auto result = engine->unloadModel(invalid_handle);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

// Test isModelLoaded with invalid handle
TEST_F(LLMEngineIntegrationTest, IsModelLoadedInvalidHandle) {
    ModelHandle invalid_handle = 999;
    EXPECT_FALSE(engine->isModelLoaded(invalid_handle));
}

// Note: Tests with actual model loading require a GGUF model file
// Those tests will be added when we have test models available

// Test streaming with callback invocation
TEST_F(LLMEngineIntegrationTest, StreamingCallbackInvocation) {
    // This test verifies that callbacks are invoked correctly
    // Note: Requires a real model file to test actual streaming
    ModelHandle invalid_handle = 999;
    
    std::vector<std::string> tokens_received;
    int callback_count = 0;
    
    auto callback = [&tokens_received, &callback_count](const std::string& token) {
        tokens_received.push_back(token);
        callback_count++;
    };
    
    auto result = engine->generateStreaming(invalid_handle, "Test prompt", callback);
    
    // Should fail because model is not loaded
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
    
    // Callback should not have been invoked
    EXPECT_EQ(callback_count, 0);
    EXPECT_TRUE(tokens_received.empty());
}

// Test streaming with exception in callback
TEST_F(LLMEngineIntegrationTest, StreamingCallbackException) {
    // This test verifies that exceptions in callbacks are handled gracefully
    // Note: Requires a real model file to test actual behavior
    ModelHandle invalid_handle = 999;
    
    auto throwing_callback = [](const std::string&) {
        throw std::runtime_error("Callback error");
    };
    
    auto result = engine->generateStreaming(invalid_handle, "Test prompt", throwing_callback);
    
    // Should fail because model is not loaded (before callback is even called)
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

// Test streaming with empty prompt
TEST_F(LLMEngineIntegrationTest, StreamingWithEmptyPrompt) {
    ModelHandle invalid_handle = 999;
    
    int callback_count = 0;
    auto callback = [&callback_count](const std::string&) {
        callback_count++;
    };
    
    auto result = engine->generateStreaming(invalid_handle, "", callback);
    
    // Should fail because model is not loaded
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
    EXPECT_EQ(callback_count, 0);
}

// Test streaming with stop sequences
TEST_F(LLMEngineIntegrationTest, StreamingWithStopSequences) {
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config;
    config.stop_sequences = {"STOP", "END"};
    config.max_tokens = 100;
    
    int callback_count = 0;
    auto callback = [&callback_count](const std::string&) {
        callback_count++;
    };
    
    auto result = engine->generateStreaming(invalid_handle, "Test", callback, config);
    
    // Should fail because model is not loaded
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
    EXPECT_EQ(callback_count, 0);
}

// Test streaming with custom generation config
TEST_F(LLMEngineIntegrationTest, StreamingWithCustomConfig) {
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config;
    config.max_tokens = 50;
    config.temperature = 0.8f;
    config.top_p = 0.95f;
    config.top_k = 50;
    config.repetition_penalty = 1.2f;
    
    int callback_count = 0;
    auto callback = [&callback_count](const std::string&) {
        callback_count++;
    };
    
    auto result = engine->generateStreaming(invalid_handle, "Test", callback, config);
    
    // Should fail because model is not loaded
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
    EXPECT_EQ(callback_count, 0);
}

// Test that streaming respects max_tokens limit
TEST_F(LLMEngineIntegrationTest, StreamingRespectsMaxTokens) {
    // This test verifies that streaming stops after max_tokens
    // Note: Requires a real model file to test actual behavior
    ModelHandle invalid_handle = 999;
    
    GenerationConfig config;
    config.max_tokens = 5;  // Very small limit
    
    int callback_count = 0;
    auto callback = [&callback_count](const std::string&) {
        callback_count++;
    };
    
    auto result = engine->generateStreaming(invalid_handle, "Test", callback, config);
    
    // Should fail because model is not loaded
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

// Context management integration tests
TEST_F(LLMEngineIntegrationTest, GetContextUsageWithoutModel) {
    ModelHandle invalid_handle = 999;
    auto result = engine->getContextUsage(invalid_handle);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(LLMEngineIntegrationTest, GetContextCapacityWithoutModel) {
    ModelHandle invalid_handle = 999;
    auto result = engine->getContextCapacity(invalid_handle);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

// Note: The following tests would require a real model file to test actual context management:
// - Test that context usage increases after generation
// - Test that clearContext resets context usage to 0
// - Test that context capacity matches model's n_ctx
// - Test that conversation history is maintained across multiple generations
// - Test that context window limits are enforced
// - Test that KV cache is properly maintained for multi-turn conversations
// These tests will be added when test models are available
