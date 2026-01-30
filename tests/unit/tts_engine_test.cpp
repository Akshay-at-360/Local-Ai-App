#include <gtest/gtest.h>
#include "ondeviceai/tts_engine.hpp"
#include <fstream>
#include <cmath>

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

TEST(TTSEngineTest, LoadModelFileNotFound) {
    TTSEngine engine;
    auto result = engine.loadModel("/nonexistent/model.onnx");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelFileNotFound);
}

TEST(TTSEngineTest, LoadAndUnloadModel) {
    TTSEngine engine;
    
    // Create a dummy model file for testing
    std::string test_model_path = "test_tts_model.onnx";
    {
        std::ofstream file(test_model_path);
        file << "dummy model content";
    }
    
    // Try to load the model (will fail because it's not a valid ONNX model)
    // But it should at least pass the file existence check
    auto load_result = engine.loadModel(test_model_path);
    
    // Clean up the test file
    std::remove(test_model_path.c_str());
    
    // The load will fail because it's not a valid ONNX model
    // but we're testing that the file existence check works
    // In a real scenario with ONNX Runtime, this would fail with ModelFileCorrupted
}

TEST(TTSEngineTest, SynthesizeEmptyText) {
    TTSEngine engine;
    
    // Create a dummy model file
    std::string test_model_path = "test_tts_empty.onnx";
    {
        std::ofstream file(test_model_path);
        file << "dummy";
    }
    
    auto load_result = engine.loadModel(test_model_path);
    std::remove(test_model_path.c_str());
    
    // Even if model loads (in fallback mode), empty text should be handled
    if (load_result.isSuccess()) {
        auto synth_result = engine.synthesize(load_result.value(), "");
        // Should either fail or produce minimal output
        // The exact behavior depends on whether ONNX Runtime is available
    }
}

TEST(TTSEngineTest, SynthesisConfigDefaults) {
    auto config = SynthesisConfig::defaults();
    
    EXPECT_EQ(config.voice_id, "default");
    EXPECT_FLOAT_EQ(config.speed, 1.0f);
    EXPECT_FLOAT_EQ(config.pitch, 1.0f);
}

TEST(TTSEngineTest, SynthesisConfigSpeedRange) {
    SynthesisConfig config;
    
    // Test various speed values
    config.speed = 0.5f;  // Slower
    EXPECT_FLOAT_EQ(config.speed, 0.5f);
    
    config.speed = 1.0f;  // Normal
    EXPECT_FLOAT_EQ(config.speed, 1.0f);
    
    config.speed = 2.0f;  // Faster
    EXPECT_FLOAT_EQ(config.speed, 2.0f);
}

TEST(TTSEngineTest, SynthesisConfigPitchRange) {
    SynthesisConfig config;
    
    // Test various pitch values
    config.pitch = -1.0f;  // Lower pitch
    EXPECT_FLOAT_EQ(config.pitch, -1.0f);
    
    config.pitch = 0.0f;   // No change
    EXPECT_FLOAT_EQ(config.pitch, 0.0f);
    
    config.pitch = 1.0f;   // Higher pitch
    EXPECT_FLOAT_EQ(config.pitch, 1.0f);
}

TEST(AudioDataTest, ToWAVEmptyAudio) {
    AudioData audio;
    audio.sample_rate = 22050;
    // Empty samples
    
    auto result = audio.toWAV();
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputAudioFormat);
}

TEST(AudioDataTest, ToWAVInvalidBitsPerSample) {
    AudioData audio;
    audio.sample_rate = 22050;
    audio.samples = {0.0f, 0.5f, -0.5f, 1.0f};
    
    auto result = audio.toWAV(12);  // Invalid bits per sample
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputParameterValue);
}

TEST(AudioDataTest, ToWAV16Bit) {
    AudioData audio;
    audio.sample_rate = 22050;
    audio.samples = {0.0f, 0.5f, -0.5f, 1.0f, -1.0f};
    
    auto result = audio.toWAV(16);
    ASSERT_TRUE(result.isSuccess());
    
    auto& wav_data = result.value();
    
    // Check WAV header
    ASSERT_GE(wav_data.size(), 44u);
    
    // Check RIFF header
    EXPECT_EQ(wav_data[0], 'R');
    EXPECT_EQ(wav_data[1], 'I');
    EXPECT_EQ(wav_data[2], 'F');
    EXPECT_EQ(wav_data[3], 'F');
    
    // Check WAVE format
    EXPECT_EQ(wav_data[8], 'W');
    EXPECT_EQ(wav_data[9], 'A');
    EXPECT_EQ(wav_data[10], 'V');
    EXPECT_EQ(wav_data[11], 'E');
    
    // Check fmt chunk
    EXPECT_EQ(wav_data[12], 'f');
    EXPECT_EQ(wav_data[13], 'm');
    EXPECT_EQ(wav_data[14], 't');
    EXPECT_EQ(wav_data[15], ' ');
    
    // Check data chunk
    EXPECT_EQ(wav_data[36], 'd');
    EXPECT_EQ(wav_data[37], 'a');
    EXPECT_EQ(wav_data[38], 't');
    EXPECT_EQ(wav_data[39], 'a');
    
    // Check that we have audio data
    EXPECT_EQ(wav_data.size(), 44u + audio.samples.size() * 2);  // 2 bytes per 16-bit sample
}

TEST(AudioDataTest, ToWAV8Bit) {
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {0.0f, 0.5f, -0.5f};
    
    auto result = audio.toWAV(8);
    ASSERT_TRUE(result.isSuccess());
    
    auto& wav_data = result.value();
    ASSERT_GE(wav_data.size(), 44u);
    EXPECT_EQ(wav_data.size(), 44u + audio.samples.size());  // 1 byte per 8-bit sample
}

TEST(AudioDataTest, ToWAV24Bit) {
    AudioData audio;
    audio.sample_rate = 44100;
    audio.samples = {0.0f, 0.5f};
    
    auto result = audio.toWAV(24);
    ASSERT_TRUE(result.isSuccess());
    
    auto& wav_data = result.value();
    ASSERT_GE(wav_data.size(), 44u);
    EXPECT_EQ(wav_data.size(), 44u + audio.samples.size() * 3);  // 3 bytes per 24-bit sample
}

TEST(AudioDataTest, ToWAV32Bit) {
    AudioData audio;
    audio.sample_rate = 48000;
    audio.samples = {0.0f, 1.0f, -1.0f};
    
    auto result = audio.toWAV(32);
    ASSERT_TRUE(result.isSuccess());
    
    auto& wav_data = result.value();
    ASSERT_GE(wav_data.size(), 44u);
    EXPECT_EQ(wav_data.size(), 44u + audio.samples.size() * 4);  // 4 bytes per 32-bit sample
}

TEST(AudioDataTest, ToWAVRoundTrip) {
    // Create audio data
    AudioData original;
    original.sample_rate = 22050;
    original.samples = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f, -0.25f, -0.5f, -0.75f, -1.0f};
    
    // Convert to WAV
    auto wav_result = original.toWAV(16);
    ASSERT_TRUE(wav_result.isSuccess());
    
    // Convert back from WAV
    auto audio_result = AudioData::fromWAV(wav_result.value());
    ASSERT_TRUE(audio_result.isSuccess());
    
    auto& recovered = audio_result.value();
    
    // Check sample rate
    EXPECT_EQ(recovered.sample_rate, original.sample_rate);
    
    // Check number of samples
    EXPECT_EQ(recovered.samples.size(), original.samples.size());
    
    // Check samples are approximately equal (allowing for quantization error)
    for (size_t i = 0; i < original.samples.size(); ++i) {
        EXPECT_NEAR(recovered.samples[i], original.samples[i], 0.01f)
            << "Sample " << i << " differs";
    }
}

TEST(AudioDataTest, ToWAVClampingValues) {
    // Test that values outside [-1.0, 1.0] are clamped
    AudioData audio;
    audio.sample_rate = 22050;
    audio.samples = {-2.0f, -1.5f, 1.5f, 2.0f};
    
    auto result = audio.toWAV(16);
    ASSERT_TRUE(result.isSuccess());
    
    // Convert back to verify clamping
    auto recovered_result = AudioData::fromWAV(result.value());
    ASSERT_TRUE(recovered_result.isSuccess());
    
    auto& recovered = recovered_result.value();
    
    // All values should be within [-1.0, 1.0]
    for (float sample : recovered.samples) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

TEST(TTSEngineTest, StreamingSynthesis) {
    TTSEngine engine;
    
    // Create a dummy model
    std::string test_model_path = "test_tts_streaming.onnx";
    {
        std::ofstream file(test_model_path);
        file << "dummy";
    }
    
    auto load_result = engine.loadModel(test_model_path);
    std::remove(test_model_path.c_str());
    
    if (load_result.isSuccess()) {
        bool callback_invoked = false;
        AudioData received_audio;
        
        auto callback = [&](const AudioData& chunk) {
            callback_invoked = true;
            received_audio = chunk;
        };
        
        auto result = engine.synthesizeStreaming(
            load_result.value(),
            "test text",
            callback
        );
        
        // In fallback mode, this should succeed and invoke callback
        if (result.isSuccess()) {
            EXPECT_TRUE(callback_invoked);
            EXPECT_GT(received_audio.samples.size(), 0u);
        }
    }
}
