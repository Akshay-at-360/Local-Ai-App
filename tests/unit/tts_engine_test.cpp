#include <gtest/gtest.h>
#include "ondeviceai/tts_engine.hpp"

using namespace ondeviceai;

TEST(TTSEngineTest, Construction) {
    TTSEngine engine;
    // Should not crash
}

TEST(TTSEngineTest, UnloadInvalidHandle) {
    TTSEngine engine;
    auto result = engine.unloadModel(999);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST(TTSEngineTest, SynthesizeWithInvalidHandle) {
    TTSEngine engine;
    auto result = engine.synthesize(999, "test text");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InferenceModelNotLoaded);
}

TEST(TTSEngineTest, GetVoicesInvalidHandle) {
    TTSEngine engine;
    auto result = engine.getAvailableVoices(999);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputModelHandle);
}
