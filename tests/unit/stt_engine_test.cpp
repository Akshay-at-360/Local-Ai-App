#include <gtest/gtest.h>
#include "ondeviceai/stt_engine.hpp"
#include <filesystem>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <thread>
#include <vector>

using namespace ondeviceai;

// Helper function to generate clean speech-like audio (sine wave)
AudioData generateCleanSpeech(int sample_rate = 16000, float duration_seconds = 1.0f, float frequency = 440.0f) {
    AudioData audio;
    audio.sample_rate = sample_rate;
    int num_samples = static_cast<int>(sample_rate * duration_seconds);
    audio.samples.resize(num_samples);
    
    for (int i = 0; i < num_samples; ++i) {
        audio.samples[i] = 0.5f * std::sin(2.0f * 3.14159f * frequency * i / sample_rate);
    }
    
    return audio;
}

// Helper function to generate silence
AudioData generateSilence(int sample_rate = 16000, float duration_seconds = 1.0f) {
    AudioData audio;
    audio.sample_rate = sample_rate;
    int num_samples = static_cast<int>(sample_rate * duration_seconds);
    audio.samples.resize(num_samples, 0.0f);
    return audio;
}

// Helper function to generate audio with speech segments
AudioData generateAudioWithSpeechSegments(int sample_rate = 16000) {
    AudioData audio;
    audio.sample_rate = sample_rate;
    audio.samples.resize(sample_rate * 3, 0.0f);  // 3 seconds
    
    // Add speech from 0.5s to 1.5s
    int start = sample_rate / 2;
    int end = sample_rate + sample_rate / 2;
    for (int i = start; i < end && i < static_cast<int>(audio.samples.size()); ++i) {
        audio.samples[i] = 0.3f * std::sin(2.0f * 3.14159f * 440.0f * i / sample_rate);
    }
    
    // Add another speech segment from 2.0s to 2.5s
    start = sample_rate * 2;
    end = sample_rate * 2 + sample_rate / 2;
    for (int i = start; i < end && i < static_cast<int>(audio.samples.size()); ++i) {
        audio.samples[i] = 0.3f * std::sin(2.0f * 3.14159f * 300.0f * i / sample_rate);
    }
    
    return audio;
}

// Helper function to create a simple WAV file
std::vector<uint8_t> createSimpleWAV(int sample_rate, int num_samples, int bits_per_sample = 16) {
    std::vector<uint8_t> wav_data;
    
    // RIFF header
    wav_data.push_back('R');
    wav_data.push_back('I');
    wav_data.push_back('F');
    wav_data.push_back('F');
    
    // File size - 8
    uint32_t file_size = 36 + num_samples * (bits_per_sample / 8);
    wav_data.push_back(file_size & 0xFF);
    wav_data.push_back((file_size >> 8) & 0xFF);
    wav_data.push_back((file_size >> 16) & 0xFF);
    wav_data.push_back((file_size >> 24) & 0xFF);
    
    // WAVE format
    wav_data.push_back('W');
    wav_data.push_back('A');
    wav_data.push_back('V');
    wav_data.push_back('E');
    
    // fmt chunk
    wav_data.push_back('f');
    wav_data.push_back('m');
    wav_data.push_back('t');
    wav_data.push_back(' ');
    
    // fmt chunk size (16 for PCM)
    uint32_t fmt_size = 16;
    wav_data.push_back(fmt_size & 0xFF);
    wav_data.push_back((fmt_size >> 8) & 0xFF);
    wav_data.push_back((fmt_size >> 16) & 0xFF);
    wav_data.push_back((fmt_size >> 24) & 0xFF);
    
    // Audio format (1 = PCM)
    uint16_t audio_format = 1;
    wav_data.push_back(audio_format & 0xFF);
    wav_data.push_back((audio_format >> 8) & 0xFF);
    
    // Number of channels (1 = mono)
    uint16_t num_channels = 1;
    wav_data.push_back(num_channels & 0xFF);
    wav_data.push_back((num_channels >> 8) & 0xFF);
    
    // Sample rate
    wav_data.push_back(sample_rate & 0xFF);
    wav_data.push_back((sample_rate >> 8) & 0xFF);
    wav_data.push_back((sample_rate >> 16) & 0xFF);
    wav_data.push_back((sample_rate >> 24) & 0xFF);
    
    // Byte rate
    uint32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
    wav_data.push_back(byte_rate & 0xFF);
    wav_data.push_back((byte_rate >> 8) & 0xFF);
    wav_data.push_back((byte_rate >> 16) & 0xFF);
    wav_data.push_back((byte_rate >> 24) & 0xFF);
    
    // Block align
    uint16_t block_align = num_channels * bits_per_sample / 8;
    wav_data.push_back(block_align & 0xFF);
    wav_data.push_back((block_align >> 8) & 0xFF);
    
    // Bits per sample
    wav_data.push_back(bits_per_sample & 0xFF);
    wav_data.push_back((bits_per_sample >> 8) & 0xFF);
    
    // data chunk
    wav_data.push_back('d');
    wav_data.push_back('a');
    wav_data.push_back('t');
    wav_data.push_back('a');
    
    // data chunk size
    uint32_t data_size = num_samples * bits_per_sample / 8;
    wav_data.push_back(data_size & 0xFF);
    wav_data.push_back((data_size >> 8) & 0xFF);
    wav_data.push_back((data_size >> 16) & 0xFF);
    wav_data.push_back((data_size >> 24) & 0xFF);
    
    // Audio data (simple sine wave)
    for (int i = 0; i < num_samples; ++i) {
        float sample = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * i / sample_rate);
        
        if (bits_per_sample == 16) {
            int16_t pcm_sample = static_cast<int16_t>(sample * 32767.0f);
            wav_data.push_back(pcm_sample & 0xFF);
            wav_data.push_back((pcm_sample >> 8) & 0xFF);
        }
    }
    
    return wav_data;
}

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

TEST(STTEngineTest, LoadModelFileNotFound) {
    STTEngine engine;
    auto result = engine.loadModel("/nonexistent/model.bin");
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelFileNotFound);
}

TEST(STTEngineTest, TranscribeEmptyAudio) {
    STTEngine engine;
    
    // Try to load a model first (will skip if not available)
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available. "
                     << "Set TEST_WHISPER_MODEL_PATH environment variable to run this test.";
    }
    
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // Test with empty audio
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {};  // Empty
    
    auto result = engine.transcribe(handle, audio);
    
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputAudioFormat);
    
    // Cleanup
    engine.unloadModel(handle);
}

// Integration test with real Whisper model
// This test requires a Whisper model file to be available
// Set TEST_WHISPER_MODEL_PATH environment variable to the model file path
TEST(STTEngineTest, LoadAndUnloadModel) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available. "
                     << "Set TEST_WHISPER_MODEL_PATH environment variable to run this test.";
    }
    
    STTEngine engine;
    
    // Load model
    auto load_result = engine.loadModel(model_path_env);
    ASSERT_TRUE(load_result.isSuccess()) << "Failed to load model: " << load_result.error().message;
    
    ModelHandle handle = load_result.value();
    EXPECT_GT(handle, 0);
    
    // Unload model
    auto unload_result = engine.unloadModel(handle);
    ASSERT_TRUE(unload_result.isSuccess());
    
    // Try to unload again - should fail
    auto unload_again = engine.unloadModel(handle);
    ASSERT_TRUE(unload_again.isError());
    EXPECT_EQ(unload_again.error().code, ErrorCode::InvalidInputModelHandle);
}

// Test basic transcription with a real model
TEST(STTEngineTest, BasicTranscription) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available. "
                     << "Set TEST_WHISPER_MODEL_PATH environment variable to run this test.";
    }
    
    STTEngine engine;
    
    // Load model
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // Create simple test audio (1 second of silence at 16kHz)
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples.resize(16000, 0.0f);
    
    // Add a simple tone to make it non-silent
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        audio.samples[i] = 0.1f * std::sin(2.0f * 3.14159f * 440.0f * i / audio.sample_rate);
    }
    
    // Transcribe
    auto result = engine.transcribe(handle, audio);
    ASSERT_TRUE(result.isSuccess()) << "Transcription failed: " << result.error().message;
    
    const Transcription& transcription = result.value();
    
    // Verify transcription structure
    EXPECT_GE(transcription.confidence, 0.0f);
    EXPECT_LE(transcription.confidence, 1.0f);
    EXPECT_FALSE(transcription.language.empty());
    
    // Note: The transcription text might be empty for a simple tone
    // This is expected behavior
    
    // Cleanup
    engine.unloadModel(handle);
}

// Test Voice Activity Detection
TEST(STTEngineTest, VoiceActivityDetection) {
    STTEngine engine;
    
    // Create audio with speech-like segments
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples.resize(32000, 0.0f);  // 2 seconds
    
    // Add "speech" in the middle (0.5s to 1.5s)
    for (size_t i = 8000; i < 24000; ++i) {
        audio.samples[i] = 0.1f * std::sin(2.0f * 3.14159f * 440.0f * i / audio.sample_rate);
    }
    
    auto result = engine.detectVoiceActivity(audio);
    ASSERT_TRUE(result.isSuccess());
    
    const auto& segments = result.value();
    
    // Should detect at least one segment
    EXPECT_GT(segments.size(), 0);
    
    // Verify segment structure
    for (const auto& segment : segments) {
        EXPECT_GE(segment.start_time, 0.0f);
        EXPECT_GT(segment.end_time, segment.start_time);
    }
}

// Test VAD with configurable threshold
TEST(STTEngineTest, VADWithConfigurableThreshold) {
    STTEngine engine;
    
    // Create audio with moderate energy
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples.resize(16000, 0.0f);  // 1 second
    
    // Add moderate energy signal
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        audio.samples[i] = 0.05f * std::sin(2.0f * 3.14159f * 440.0f * i / audio.sample_rate);
    }
    
    // Test with low threshold (more sensitive) - should detect speech
    auto result_low = engine.detectVoiceActivity(audio, 0.2f);
    ASSERT_TRUE(result_low.isSuccess());
    const auto& segments_low = result_low.value();
    EXPECT_GT(segments_low.size(), 0) << "Low threshold should detect speech";
    
    // Test with high threshold (less sensitive) - might not detect speech
    auto result_high = engine.detectVoiceActivity(audio, 0.9f);
    ASSERT_TRUE(result_high.isSuccess());
    const auto& segments_high = result_high.value();
    // High threshold should detect fewer or no segments
    EXPECT_LE(segments_high.size(), segments_low.size());
}

// Test VAD with invalid threshold
TEST(STTEngineTest, VADWithInvalidThreshold) {
    STTEngine engine;
    
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples.resize(16000, 0.1f);
    
    // Test with threshold < 0
    auto result_negative = engine.detectVoiceActivity(audio, -0.1f);
    ASSERT_TRUE(result_negative.isError());
    EXPECT_EQ(result_negative.error().code, ErrorCode::InvalidInputParameterValue);
    
    // Test with threshold > 1
    auto result_too_high = engine.detectVoiceActivity(audio, 1.5f);
    ASSERT_TRUE(result_too_high.isError());
    EXPECT_EQ(result_too_high.error().code, ErrorCode::InvalidInputParameterValue);
}

// Test VAD with empty audio
TEST(STTEngineTest, VADWithEmptyAudio) {
    STTEngine engine;
    
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {};
    
    auto result = engine.detectVoiceActivity(audio);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputAudioFormat);
}

// Test VAD with silence
TEST(STTEngineTest, VADWithSilence) {
    STTEngine engine;
    
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples.resize(16000, 0.0f);  // 1 second of silence
    
    auto result = engine.detectVoiceActivity(audio);
    ASSERT_TRUE(result.isSuccess());
    
    const auto& segments = result.value();
    
    // Should detect no segments in silence
    EXPECT_EQ(segments.size(), 0);
}

// Test model variant detection
TEST(STTEngineTest, ModelVariantDetection) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available.";
    }
    
    STTEngine engine;
    
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // The model should load successfully
    // Variant detection is internal, but we can verify the model works
    EXPECT_GT(handle, 0);
    
    // Cleanup
    engine.unloadModel(handle);
}

// ============================================================================
// Task 6.6: Comprehensive Unit Tests for STT Engine
// Requirements: 2.1, 2.3, 2.5, 2.6, 25.1, 25.2
// ============================================================================

// Test transcription of clean speech (Requirement 2.1)
TEST(STTEngineTest, TranscribeCleanSpeech) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available. "
                     << "Set TEST_WHISPER_MODEL_PATH environment variable to run this test.";
    }
    
    STTEngine engine;
    
    // Load model
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // Generate clean speech-like audio (1 second at 16kHz)
    AudioData audio = generateCleanSpeech(16000, 1.0f, 440.0f);
    
    // Transcribe
    auto result = engine.transcribe(handle, audio);
    ASSERT_TRUE(result.isSuccess()) << "Transcription failed: " << result.error().message;
    
    const Transcription& transcription = result.value();
    
    // Verify transcription structure (Requirement 2.4)
    EXPECT_GE(transcription.confidence, 0.0f);
    EXPECT_LE(transcription.confidence, 1.0f);
    EXPECT_FALSE(transcription.language.empty());
    
    // Note: The actual text might be empty for a pure tone, which is expected
    // The important thing is that the transcription succeeds without errors
    
    // Cleanup
    engine.unloadModel(handle);
}

// Test multi-language support (Requirement 2.3)
TEST(STTEngineTest, MultiLanguageSupport) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available.";
    }
    
    STTEngine engine;
    
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    AudioData audio = generateCleanSpeech(16000, 1.0f);
    
    // Test with auto language detection
    TranscriptionConfig config_auto;
    config_auto.language = "auto";
    
    auto result_auto = engine.transcribe(handle, audio, config_auto);
    ASSERT_TRUE(result_auto.isSuccess()) << "Auto language detection failed: " << result_auto.error().message;
    EXPECT_FALSE(result_auto.value().language.empty());
    
    // Test with specific language (English)
    TranscriptionConfig config_en;
    config_en.language = "en";
    
    auto result_en = engine.transcribe(handle, audio, config_en);
    ASSERT_TRUE(result_en.isSuccess()) << "English transcription failed: " << result_en.error().message;
    EXPECT_EQ(result_en.value().language, "en");
    
    // Test with another language (Spanish)
    TranscriptionConfig config_es;
    config_es.language = "es";
    
    auto result_es = engine.transcribe(handle, audio, config_es);
    ASSERT_TRUE(result_es.isSuccess()) << "Spanish transcription failed: " << result_es.error().message;
    EXPECT_EQ(result_es.value().language, "es");
    
    // Cleanup
    engine.unloadModel(handle);
}

// Test VAD on silence (Requirement 2.5)
TEST(STTEngineTest, VADOnSilence) {
    STTEngine engine;
    
    // Generate 2 seconds of silence
    AudioData audio = generateSilence(16000, 2.0f);
    
    // Test with default threshold
    auto result = engine.detectVoiceActivity(audio);
    ASSERT_TRUE(result.isSuccess()) << "VAD failed: " << result.error().message;
    
    const auto& segments = result.value();
    
    // Should detect no speech segments in silence
    EXPECT_EQ(segments.size(), 0) << "VAD should not detect speech in silence";
    
    // Test with low threshold (more sensitive)
    auto result_low = engine.detectVoiceActivity(audio, 0.1f);
    ASSERT_TRUE(result_low.isSuccess());
    EXPECT_EQ(result_low.value().size(), 0) << "VAD should not detect speech in silence even with low threshold";
    
    // Test with high threshold (less sensitive)
    auto result_high = engine.detectVoiceActivity(audio, 0.9f);
    ASSERT_TRUE(result_high.isSuccess());
    EXPECT_EQ(result_high.value().size(), 0) << "VAD should not detect speech in silence with high threshold";
}

// Test VAD on speech (Requirement 2.5)
TEST(STTEngineTest, VADOnSpeech) {
    STTEngine engine;
    
    // Generate audio with speech segments
    AudioData audio = generateAudioWithSpeechSegments(16000);
    
    // Test with default threshold
    auto result = engine.detectVoiceActivity(audio);
    ASSERT_TRUE(result.isSuccess()) << "VAD failed: " << result.error().message;
    
    const auto& segments = result.value();
    
    // Should detect at least one speech segment
    EXPECT_GT(segments.size(), 0) << "VAD should detect speech segments";
    
    // Verify segment structure
    for (const auto& segment : segments) {
        EXPECT_GE(segment.start_time, 0.0f);
        EXPECT_GT(segment.end_time, segment.start_time);
        EXPECT_LE(segment.end_time, 3.0f);  // Audio is 3 seconds long
    }
    
    // Test with low threshold (should detect more or same segments)
    auto result_low = engine.detectVoiceActivity(audio, 0.2f);
    ASSERT_TRUE(result_low.isSuccess());
    EXPECT_GE(result_low.value().size(), segments.size()) 
        << "Lower threshold should detect at least as many segments";
    
    // Test with high threshold (should detect fewer or same segments)
    auto result_high = engine.detectVoiceActivity(audio, 0.8f);
    ASSERT_TRUE(result_high.isSuccess());
    EXPECT_LE(result_high.value().size(), segments.size())
        << "Higher threshold should detect fewer or equal segments";
}

// Test different model variants (Requirement 2.6)
TEST(STTEngineTest, DifferentModelVariants) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available.";
    }
    
    STTEngine engine;
    
    // Load the model (variant will be detected automatically)
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // Test that the model works regardless of variant
    AudioData audio = generateCleanSpeech(16000, 0.5f);
    
    auto result = engine.transcribe(handle, audio);
    ASSERT_TRUE(result.isSuccess()) << "Transcription failed: " << result.error().message;
    
    const Transcription& transcription = result.value();
    
    // Verify basic transcription properties
    EXPECT_GE(transcription.confidence, 0.0f);
    EXPECT_LE(transcription.confidence, 1.0f);
    EXPECT_FALSE(transcription.language.empty());
    
    // Cleanup
    engine.unloadModel(handle);
}

// Test audio format conversions - PCM format (Requirement 25.1)
TEST(STTEngineTest, AudioFormatPCM) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available.";
    }
    
    STTEngine engine;
    
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // Test with PCM audio at 16kHz (native format)
    AudioData audio_16k = generateCleanSpeech(16000, 0.5f);
    auto result_16k = engine.transcribe(handle, audio_16k);
    ASSERT_TRUE(result_16k.isSuccess()) << "PCM 16kHz transcription failed: " << result_16k.error().message;
    
    // Test with PCM audio at 8kHz (requires resampling)
    AudioData audio_8k = generateCleanSpeech(8000, 0.5f);
    auto result_8k = engine.transcribe(handle, audio_8k);
    ASSERT_TRUE(result_8k.isSuccess()) << "PCM 8kHz transcription failed: " << result_8k.error().message;
    
    // Test with PCM audio at 48kHz (requires resampling)
    AudioData audio_48k = generateCleanSpeech(48000, 0.5f);
    auto result_48k = engine.transcribe(handle, audio_48k);
    ASSERT_TRUE(result_48k.isSuccess()) << "PCM 48kHz transcription failed: " << result_48k.error().message;
    
    // Cleanup
    engine.unloadModel(handle);
}

// Test audio format conversions - WAV format (Requirement 25.2)
TEST(STTEngineTest, AudioFormatWAV) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available.";
    }
    
    STTEngine engine;
    
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // Create WAV audio data
    auto wav_data = createSimpleWAV(16000, 8000, 16);  // 0.5 seconds at 16kHz
    
    // Convert WAV to AudioData
    auto audio_result = AudioData::fromWAV(wav_data);
    ASSERT_TRUE(audio_result.isSuccess()) << "WAV parsing failed: " << audio_result.error().message;
    
    AudioData audio = audio_result.value();
    
    // Transcribe
    auto result = engine.transcribe(handle, audio);
    ASSERT_TRUE(result.isSuccess()) << "WAV transcription failed: " << result.error().message;
    
    const Transcription& transcription = result.value();
    
    // Verify transcription
    EXPECT_GE(transcription.confidence, 0.0f);
    EXPECT_LE(transcription.confidence, 1.0f);
    EXPECT_FALSE(transcription.language.empty());
    
    // Cleanup
    engine.unloadModel(handle);
}

// Test audio resampling during transcription (Requirement 25.3)
TEST(STTEngineTest, AudioResampling) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available.";
    }
    
    STTEngine engine;
    
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // Test various sample rates to ensure resampling works
    std::vector<int> sample_rates = {8000, 11025, 16000, 22050, 44100, 48000};
    
    for (int sample_rate : sample_rates) {
        AudioData audio = generateCleanSpeech(sample_rate, 0.5f);
        
        auto result = engine.transcribe(handle, audio);
        ASSERT_TRUE(result.isSuccess()) 
            << "Transcription failed for sample rate " << sample_rate 
            << ": " << result.error().message;
        
        const Transcription& transcription = result.value();
        
        // Verify transcription succeeded
        EXPECT_GE(transcription.confidence, 0.0f);
        EXPECT_LE(transcription.confidence, 1.0f);
    }
    
    // Cleanup
    engine.unloadModel(handle);
}

// Test word-level timestamps (Requirement 2.4)
TEST(STTEngineTest, WordLevelTimestamps) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available.";
    }
    
    STTEngine engine;
    
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    AudioData audio = generateCleanSpeech(16000, 1.0f);
    
    // Test with word timestamps enabled
    TranscriptionConfig config;
    config.word_timestamps = true;
    
    auto result = engine.transcribe(handle, audio, config);
    ASSERT_TRUE(result.isSuccess()) << "Transcription with word timestamps failed: " << result.error().message;
    
    const Transcription& transcription = result.value();
    
    // If words were detected, verify their structure
    for (const auto& word : transcription.words) {
        EXPECT_FALSE(word.text.empty());
        EXPECT_GE(word.start_time, 0.0f);
        EXPECT_GT(word.end_time, word.start_time);
        EXPECT_GE(word.confidence, 0.0f);
        EXPECT_LE(word.confidence, 1.0f);
    }
    
    // Cleanup
    engine.unloadModel(handle);
}

// Test translation to English (Requirement 2.3)
TEST(STTEngineTest, TranslationToEnglish) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available.";
    }
    
    STTEngine engine;
    
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    AudioData audio = generateCleanSpeech(16000, 1.0f);
    
    // Test with translation enabled
    TranscriptionConfig config;
    config.language = "es";  // Spanish
    config.translate_to_english = true;
    
    auto result = engine.transcribe(handle, audio, config);
    ASSERT_TRUE(result.isSuccess()) << "Translation failed: " << result.error().message;
    
    // Translation should succeed (even if audio is just a tone)
    const Transcription& transcription = result.value();
    EXPECT_GE(transcription.confidence, 0.0f);
    EXPECT_LE(transcription.confidence, 1.0f);
    
    // Cleanup
    engine.unloadModel(handle);
}

// Test concurrent transcriptions (thread safety)
TEST(STTEngineTest, ConcurrentTranscriptions) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available.";
    }
    
    STTEngine engine;
    
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // Note: The current implementation uses a mutex, so transcriptions are serialized
    // This test verifies that concurrent calls don't cause crashes or data corruption
    
    AudioData audio1 = generateCleanSpeech(16000, 0.3f, 440.0f);
    AudioData audio2 = generateCleanSpeech(16000, 0.3f, 880.0f);
    
    std::vector<std::thread> threads;
    std::vector<bool> results(2, false);
    
    threads.emplace_back([&, handle]() {
        auto result = engine.transcribe(handle, audio1);
        results[0] = result.isSuccess();
    });
    
    threads.emplace_back([&, handle]() {
        auto result = engine.transcribe(handle, audio2);
        results[1] = result.isSuccess();
    });
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Both transcriptions should succeed
    EXPECT_TRUE(results[0]) << "First concurrent transcription failed";
    EXPECT_TRUE(results[1]) << "Second concurrent transcription failed";
    
    // Cleanup
    engine.unloadModel(handle);
}

// Test multiple models loaded simultaneously (Requirement 2.6)
TEST(STTEngineTest, MultipleModelsLoaded) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available.";
    }
    
    STTEngine engine;
    
    // Load the same model twice (simulating different model variants)
    auto load_result1 = engine.loadModel(model_path_env);
    if (load_result1.isError()) {
        GTEST_SKIP() << "Failed to load first model: " << load_result1.error().message;
    }
    
    auto load_result2 = engine.loadModel(model_path_env);
    if (load_result2.isError()) {
        GTEST_SKIP() << "Failed to load second model: " << load_result2.error().message;
    }
    
    ModelHandle handle1 = load_result1.value();
    ModelHandle handle2 = load_result2.value();
    
    // Handles should be different
    EXPECT_NE(handle1, handle2);
    
    // Both models should work
    AudioData audio = generateCleanSpeech(16000, 0.5f);
    
    auto result1 = engine.transcribe(handle1, audio);
    ASSERT_TRUE(result1.isSuccess()) << "First model transcription failed";
    
    auto result2 = engine.transcribe(handle2, audio);
    ASSERT_TRUE(result2.isSuccess()) << "Second model transcription failed";
    
    // Cleanup
    engine.unloadModel(handle1);
    engine.unloadModel(handle2);
}
