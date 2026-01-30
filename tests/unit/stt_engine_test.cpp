#include <gtest/gtest.h>
#include "ondeviceai/stt_engine.hpp"

using namespace ondeviceai;

TEST(STTEngineTest, Construction) {
    STTEngine engine;
    // Should not crash
}

TEST(STTEngineTest, UnloadInvalidHandle) {
    STTEngine engine;
    auto result = engine.unloadModel(999);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST(STTEngineTest, TranscribeWithInvalidHandle) {
    STTEngine engine;
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {0.0f, 0.1f, 0.2f};
    
    auto result = engine.transcribe(999, audio);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}
