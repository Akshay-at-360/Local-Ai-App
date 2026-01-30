#include <gtest/gtest.h>
#include "ondeviceai/voice_pipeline.hpp"
#include "ondeviceai/stt_engine.hpp"
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/tts_engine.hpp"

using namespace ondeviceai;

class VoicePipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        stt_engine = std::make_unique<STTEngine>();
        llm_engine = std::make_unique<LLMEngine>();
        tts_engine = std::make_unique<TTSEngine>();
        pipeline = std::make_unique<VoicePipeline>(
            stt_engine.get(), llm_engine.get(), tts_engine.get()
        );
    }
    
    std::unique_ptr<STTEngine> stt_engine;
    std::unique_ptr<LLMEngine> llm_engine;
    std::unique_ptr<TTSEngine> tts_engine;
    std::unique_ptr<VoicePipeline> pipeline;
};

TEST_F(VoicePipelineTest, Construction) {
    // Should not crash
    EXPECT_NE(pipeline, nullptr);
}

TEST_F(VoicePipelineTest, StartConversationWithoutConfiguration) {
    auto result = pipeline->startConversation(
        []() { return AudioData(); },
        [](const AudioData&) {},
        [](const std::string&) {},
        [](const std::string&) {}
    );
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputConfiguration);
}

TEST_F(VoicePipelineTest, ClearHistorySucceeds) {
    auto result = pipeline->clearHistory();
    ASSERT_TRUE(result.isSuccess());
}

TEST_F(VoicePipelineTest, GetHistoryEmpty) {
    auto result = pipeline->getHistory();
    ASSERT_TRUE(result.isSuccess());
    EXPECT_TRUE(result.value().empty());
}
