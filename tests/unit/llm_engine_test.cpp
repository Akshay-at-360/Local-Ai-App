#include <gtest/gtest.h>
#include "ondeviceai/llm_engine.hpp"

using namespace ondeviceai;

TEST(LLMEngineTest, Construction) {
    LLMEngine engine;
    // Should not crash
}

TEST(LLMEngineTest, UnloadInvalidHandle) {
    LLMEngine engine;
    auto result = engine.unloadModel(999);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST(LLMEngineTest, GenerateWithInvalidHandle) {
    LLMEngine engine;
    auto result = engine.generate(999, "test prompt");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST(LLMEngineTest, ClearContextInvalidHandle) {
    LLMEngine engine;
    auto result = engine.clearContext(999);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

// Tokenization tests
TEST(LLMEngineTest, TokenizeWithInvalidHandle) {
    LLMEngine engine;
    auto result = engine.tokenize(999, "test text");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
    EXPECT_FALSE(result.error().message.empty());
    EXPECT_FALSE(result.error().details.empty());
    EXPECT_TRUE(result.error().recovery_suggestion.has_value());
}

TEST(LLMEngineTest, DetokenizeWithInvalidHandle) {
    LLMEngine engine;
    std::vector<int> tokens = {1, 2, 3};
    auto result = engine.detokenize(999, tokens);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
    EXPECT_FALSE(result.error().message.empty());
    EXPECT_FALSE(result.error().details.empty());
    EXPECT_TRUE(result.error().recovery_suggestion.has_value());
}

TEST(LLMEngineTest, TokenizeEmptyStringWithInvalidHandle) {
    LLMEngine engine;
    auto result = engine.tokenize(999, "");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST(LLMEngineTest, DetokenizeEmptyVectorWithInvalidHandle) {
    LLMEngine engine;
    std::vector<int> empty_tokens;
    auto result = engine.detokenize(999, empty_tokens);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST(LLMEngineTest, TokenizeErrorMessageQuality) {
    LLMEngine engine;
    auto result = engine.tokenize(999, "test");
    
    ASSERT_TRUE(result.isError());
    // Error message should be descriptive
    EXPECT_GT(result.error().message.length(), 10);
    // Should include the handle in details
    EXPECT_NE(result.error().details.find("999"), std::string::npos);
    // Should have recovery suggestion
    EXPECT_TRUE(result.error().recovery_suggestion.has_value());
    EXPECT_GT(result.error().recovery_suggestion->length(), 10);
}

TEST(LLMEngineTest, DetokenizeErrorMessageQuality) {
    LLMEngine engine;
    std::vector<int> tokens = {1, 2, 3};
    auto result = engine.detokenize(999, tokens);
    
    ASSERT_TRUE(result.isError());
    // Error message should be descriptive
    EXPECT_GT(result.error().message.length(), 10);
    // Should include the handle in details
    EXPECT_NE(result.error().details.find("999"), std::string::npos);
    // Should have recovery suggestion
    EXPECT_TRUE(result.error().recovery_suggestion.has_value());
    EXPECT_GT(result.error().recovery_suggestion->length(), 10);
}

// Generation configuration tests
TEST(LLMEngineTest, GenerationConfigDefaults) {
    auto config = GenerationConfig::defaults();
    
    // Verify default values match the design specification
    EXPECT_EQ(config.max_tokens, 512);
    EXPECT_FLOAT_EQ(config.temperature, 0.7f);
    EXPECT_FLOAT_EQ(config.top_p, 0.9f);
    EXPECT_EQ(config.top_k, 40);
    EXPECT_FLOAT_EQ(config.repetition_penalty, 1.1f);
    EXPECT_TRUE(config.stop_sequences.empty());
}

TEST(LLMEngineTest, GenerationConfigCustomValues) {
    GenerationConfig config;
    config.max_tokens = 100;
    config.temperature = 0.5f;
    config.top_p = 0.95f;
    config.top_k = 50;
    config.repetition_penalty = 1.2f;
    config.stop_sequences = {"STOP", "END"};
    
    // Verify custom values are set correctly
    EXPECT_EQ(config.max_tokens, 100);
    EXPECT_FLOAT_EQ(config.temperature, 0.5f);
    EXPECT_FLOAT_EQ(config.top_p, 0.95f);
    EXPECT_EQ(config.top_k, 50);
    EXPECT_FLOAT_EQ(config.repetition_penalty, 1.2f);
    EXPECT_EQ(config.stop_sequences.size(), 2);
    EXPECT_EQ(config.stop_sequences[0], "STOP");
    EXPECT_EQ(config.stop_sequences[1], "END");
}

TEST(LLMEngineTest, GenerationConfigWithStopSequences) {
    GenerationConfig config = GenerationConfig::defaults();
    config.stop_sequences = {"\n\n", "###", "END"};
    
    EXPECT_EQ(config.stop_sequences.size(), 3);
    EXPECT_EQ(config.stop_sequences[0], "\n\n");
    EXPECT_EQ(config.stop_sequences[1], "###");
    EXPECT_EQ(config.stop_sequences[2], "END");
}

// Context management tests
TEST(LLMEngineTest, GetContextUsageInvalidHandle) {
    LLMEngine engine;
    auto result = engine.getContextUsage(999);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST(LLMEngineTest, GetContextCapacityInvalidHandle) {
    LLMEngine engine;
    auto result = engine.getContextCapacity(999);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST(LLMEngineTest, GetConversationHistoryInvalidHandle) {
    LLMEngine engine;
    auto result = engine.getConversationHistory(999);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}
