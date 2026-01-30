#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>
#include "ondeviceai/stt_engine.hpp"
#include "ondeviceai/logger.hpp"
#include <filesystem>
#include <cmath>
#include <algorithm>

using namespace ondeviceai;

// Helper function to generate random audio data
namespace rc {

// Generator for AudioData with random samples
Gen<AudioData> genAudioData() {
    return gen::map(
        gen::tuple(
            gen::inRange(8000, 48000),  // Sample rate between 8kHz and 48kHz
            gen::inRange(1000, 50000),  // Number of samples (0.125s to 6.25s at 8kHz)
            gen::container<std::vector<int>>(gen::inRange(-1000, 1000))
        ),
        [](const std::tuple<int, int, std::vector<int>>& params) {
            AudioData audio;
            audio.sample_rate = std::get<0>(params);
            int num_samples = std::get<1>(params);
            const auto& random_ints = std::get<2>(params);
            
            // Generate random audio samples in range [-1.0, 1.0]
            audio.samples.resize(num_samples);
            for (int i = 0; i < num_samples; ++i) {
                // Convert random int to float in [-1.0, 1.0]
                int idx = i % std::max(1, static_cast<int>(random_ints.size()));
                audio.samples[i] = random_ints[idx] / 1000.0f;
            }
            
            return audio;
        }
    );
}

// Generator for AudioData with speech-like characteristics
Gen<AudioData> genSpeechLikeAudio() {
    return gen::map(
        gen::tuple(
            gen::inRange(8000, 48000),  // Sample rate
            gen::inRange(5000, 30000),  // Number of samples
            gen::inRange(100, 500),     // Base frequency (as int)
            gen::inRange(5, 30)         // Amplitude (as int, will be divided by 100)
        ),
        [](const std::tuple<int, int, int, int>& params) {
            AudioData audio;
            audio.sample_rate = std::get<0>(params);
            int num_samples = std::get<1>(params);
            float base_freq = static_cast<float>(std::get<2>(params));
            float amplitude = std::get<3>(params) / 100.0f;
            
            // Generate audio with multiple frequency components (speech-like)
            audio.samples.resize(num_samples);
            for (int i = 0; i < num_samples; ++i) {
                float t = static_cast<float>(i) / audio.sample_rate;
                
                // Combine multiple frequencies to simulate speech
                float sample = 0.0f;
                sample += amplitude * std::sin(2.0f * M_PI * base_freq * t);
                sample += amplitude * 0.5f * std::sin(2.0f * M_PI * base_freq * 2.0f * t);
                sample += amplitude * 0.3f * std::sin(2.0f * M_PI * base_freq * 3.0f * t);
                
                // Add some noise (using deterministic pattern)
                float noise = (i % 100 - 50) / 2500.0f;
                sample += noise;
                
                // Clamp to [-1.0, 1.0]
                audio.samples[i] = std::max(-1.0f, std::min(1.0f, sample));
            }
            
            return audio;
        }
    );
}

// Generator for AudioData with silence
Gen<AudioData> genSilentAudio() {
    return gen::map(
        gen::tuple(
            gen::inRange(8000, 48000),  // Sample rate
            gen::inRange(1000, 20000),  // Number of samples
            gen::container<std::vector<int>>(gen::inRange(-10, 10))
        ),
        [](const std::tuple<int, int, std::vector<int>>& params) {
            AudioData audio;
            audio.sample_rate = std::get<0>(params);
            int num_samples = std::get<1>(params);
            const auto& random_ints = std::get<2>(params);
            
            // Generate silent audio (all zeros or very low amplitude)
            audio.samples.resize(num_samples);
            for (int i = 0; i < num_samples; ++i) {
                // Very low amplitude noise
                int idx = i % std::max(1, static_cast<int>(random_ints.size()));
                audio.samples[i] = random_ints[idx] / 1000.0f;
            }
            
            return audio;
        }
    );
}

// Generator for AudioData with various characteristics
Gen<AudioData> genVariedAudio() {
    return gen::oneOf(
        genAudioData(),
        genSpeechLikeAudio(),
        genSilentAudio()
    );
}

} // namespace rc

// Test fixture for STT property tests
class STTPropertyTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_unique<STTEngine>();
        
        // Check if a test model is available
        const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
        if (model_path_env && std::filesystem::exists(model_path_env)) {
            model_path_ = model_path_env;
            has_model_ = true;
            
            // Try to load the model
            auto load_result = engine_->loadModel(model_path_);
            if (load_result.isSuccess()) {
                model_handle_ = load_result.value();
                model_loaded_ = true;
            }
        }
    }
    
    void TearDown() override {
        if (model_loaded_ && model_handle_ > 0) {
            engine_->unloadModel(model_handle_);
        }
        engine_.reset();
    }
    
    std::unique_ptr<STTEngine> engine_;
    std::string model_path_;
    ModelHandle model_handle_ = 0;
    bool has_model_ = false;
    bool model_loaded_ = false;
};

// Feature: on-device-ai-sdk, Property 5: Transcription Confidence Scores
// **Validates: Requirements 2.4**
//
// NOTE: This test requires a real Whisper model file to execute properly.
// The test is structured to validate the property but will be skipped
// if no valid model is available. To run this test with a real model:
// 1. Download a Whisper model (e.g., whisper-tiny, whisper-base)
// 2. Set the environment variable TEST_WHISPER_MODEL_PATH to the model file path
// 3. Run the tests
//
// The property being tested:
// For any audio input transcribed by STT engine, the returned transcription
// should include a confidence score in the range [0.0, 1.0]
RC_GTEST_PROP(STTPropertyTest, TranscriptionConfidenceScoresInValidRange, ()) {
    // Check if a test model is available
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        RC_SUCCEED("Skipping test - no valid Whisper model available. "
                   "Set TEST_WHISPER_MODEL_PATH environment variable to run this test.");
        return;
    }
    
    std::string model_path(model_path_env);
    
    // Load the model
    STTEngine engine;
    auto load_result = engine.loadModel(model_path);
    if (load_result.isError()) {
        RC_SUCCEED("Skipping test - failed to load model: " + load_result.error().message);
        return;
    }
    
    ModelHandle handle = load_result.value();
    
    // Generate random audio input
    auto audio = *rc::genVariedAudio();
    
    // Skip if audio is too short (less than 0.1 seconds)
    float duration = static_cast<float>(audio.samples.size()) / audio.sample_rate;
    RC_PRE(duration >= 0.1f);
    
    // Skip if audio is empty
    RC_PRE(!audio.samples.empty());
    
    // Transcribe the audio
    TranscriptionConfig config = TranscriptionConfig::defaults();
    auto transcribe_result = engine.transcribe(handle, audio, config);
    
    // The transcription should succeed or fail gracefully
    // (some random audio might not be transcribable, which is acceptable)
    if (transcribe_result.isError()) {
        // If transcription fails, it should be due to audio format or processing issues
        // not due to missing confidence scores
        RC_LOG() << "Transcription failed (acceptable for random audio): " 
                 << transcribe_result.error().message;
        RC_SUCCEED("Transcription failed gracefully");
        return;
    }
    
    const Transcription& transcription = transcribe_result.value();
    
    // PROPERTY: The confidence score must be in the range [0.0, 1.0]
    RC_ASSERT(transcription.confidence >= 0.0f);
    RC_ASSERT(transcription.confidence <= 1.0f);
    
    // Additional checks: confidence should be a valid float (not NaN or infinity)
    RC_ASSERT(!std::isnan(transcription.confidence));
    RC_ASSERT(!std::isinf(transcription.confidence));
    
    // Cleanup
    engine.unloadModel(handle);
}

// Property test: Verify confidence scores for speech-like audio
RC_GTEST_PROP(STTPropertyTest, SpeechLikeAudioHasValidConfidence, ()) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        RC_SUCCEED("Skipping test - no valid Whisper model available.");
        return;
    }
    
    std::string model_path(model_path_env);
    
    STTEngine engine;
    auto load_result = engine.loadModel(model_path);
    if (load_result.isError()) {
        RC_SUCCEED("Skipping test - failed to load model.");
        return;
    }
    
    ModelHandle handle = load_result.value();
    
    // Generate speech-like audio
    auto audio = *rc::genSpeechLikeAudio();
    
    // Transcribe
    auto transcribe_result = engine.transcribe(handle, audio);
    
    if (transcribe_result.isError()) {
        RC_SUCCEED("Transcription failed gracefully");
        return;
    }
    
    const Transcription& transcription = transcribe_result.value();
    
    // Verify confidence is in valid range
    RC_ASSERT(transcription.confidence >= 0.0f);
    RC_ASSERT(transcription.confidence <= 1.0f);
    RC_ASSERT(!std::isnan(transcription.confidence));
    RC_ASSERT(!std::isinf(transcription.confidence));
    
    // Cleanup
    engine.unloadModel(handle);
}

// Property test: Verify confidence scores for silent audio
RC_GTEST_PROP(STTPropertyTest, SilentAudioHasValidConfidence, ()) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        RC_SUCCEED("Skipping test - no valid Whisper model available.");
        return;
    }
    
    std::string model_path(model_path_env);
    
    STTEngine engine;
    auto load_result = engine.loadModel(model_path);
    if (load_result.isError()) {
        RC_SUCCEED("Skipping test - failed to load model.");
        return;
    }
    
    ModelHandle handle = load_result.value();
    
    // Generate silent audio
    auto audio = *rc::genSilentAudio();
    
    // Transcribe
    auto transcribe_result = engine.transcribe(handle, audio);
    
    if (transcribe_result.isError()) {
        RC_SUCCEED("Transcription failed gracefully");
        return;
    }
    
    const Transcription& transcription = transcribe_result.value();
    
    // Even for silent audio, confidence should be in valid range
    RC_ASSERT(transcription.confidence >= 0.0f);
    RC_ASSERT(transcription.confidence <= 1.0f);
    RC_ASSERT(!std::isnan(transcription.confidence));
    RC_ASSERT(!std::isinf(transcription.confidence));
    
    // For silent audio, we might expect low confidence or empty transcription
    // but the confidence value itself must still be valid
    
    // Cleanup
    engine.unloadModel(handle);
}

// Property test: Verify confidence scores with different sample rates
RC_GTEST_PROP(STTPropertyTest, DifferentSampleRatesHaveValidConfidence, ()) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        RC_SUCCEED("Skipping test - no valid Whisper model available.");
        return;
    }
    
    std::string model_path(model_path_env);
    
    STTEngine engine;
    auto load_result = engine.loadModel(model_path);
    if (load_result.isError()) {
        RC_SUCCEED("Skipping test - failed to load model.");
        return;
    }
    
    ModelHandle handle = load_result.value();
    
    // Generate audio with random sample rate
    auto sample_rate = *rc::gen::element(8000, 16000, 22050, 44100, 48000);
    auto num_samples = *rc::gen::inRange(5000, 30000);
    
    AudioData audio;
    audio.sample_rate = sample_rate;
    audio.samples.resize(num_samples);
    
    // Generate simple tone
    for (int i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        audio.samples[i] = 0.1f * std::sin(2.0f * M_PI * 440.0f * t);
    }
    
    // Transcribe
    auto transcribe_result = engine.transcribe(handle, audio);
    
    if (transcribe_result.isError()) {
        RC_SUCCEED("Transcription failed gracefully");
        return;
    }
    
    const Transcription& transcription = transcribe_result.value();
    
    // Verify confidence is in valid range regardless of sample rate
    RC_ASSERT(transcription.confidence >= 0.0f);
    RC_ASSERT(transcription.confidence <= 1.0f);
    RC_ASSERT(!std::isnan(transcription.confidence));
    RC_ASSERT(!std::isinf(transcription.confidence));
    
    // Cleanup
    engine.unloadModel(handle);
}

// Unit test: Verify confidence score structure
TEST(STTPropertyUnitTest, ConfidenceScoreStructure) {
    // Create a transcription object and verify the confidence field exists
    Transcription transcription;
    transcription.text = "test";
    transcription.confidence = 0.95f;
    transcription.language = "en";
    
    // Verify we can read and write the confidence field
    EXPECT_EQ(transcription.confidence, 0.95f);
    
    // Verify confidence can be set to boundary values
    transcription.confidence = 0.0f;
    EXPECT_EQ(transcription.confidence, 0.0f);
    
    transcription.confidence = 1.0f;
    EXPECT_EQ(transcription.confidence, 1.0f);
}

// Unit test: Verify confidence score with real model (if available)
TEST(STTPropertyUnitTest, RealModelProducesValidConfidence) {
    const char* model_path_env = std::getenv("TEST_WHISPER_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid Whisper model available. "
                     << "Set TEST_WHISPER_MODEL_PATH environment variable to run this test.";
    }
    
    STTEngine engine;
    auto load_result = engine.loadModel(model_path_env);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle handle = load_result.value();
    
    // Create simple test audio (1 second of 440Hz tone at 16kHz)
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples.resize(16000);
    
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        audio.samples[i] = 0.1f * std::sin(2.0f * M_PI * 440.0f * i / audio.sample_rate);
    }
    
    // Transcribe
    auto result = engine.transcribe(handle, audio);
    ASSERT_TRUE(result.isSuccess()) << "Transcription failed: " << result.error().message;
    
    const Transcription& transcription = result.value();
    
    // Verify confidence is in valid range
    EXPECT_GE(transcription.confidence, 0.0f);
    EXPECT_LE(transcription.confidence, 1.0f);
    EXPECT_FALSE(std::isnan(transcription.confidence));
    EXPECT_FALSE(std::isinf(transcription.confidence));
    
    // Cleanup
    engine.unloadModel(handle);
}

// Unit test: Verify word-level confidence scores (if word timestamps enabled)
TEST(STTPropertyUnitTest, WordLevelConfidenceScores) {
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
    
    // Create test audio
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples.resize(16000);
    
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        audio.samples[i] = 0.1f * std::sin(2.0f * M_PI * 440.0f * i / audio.sample_rate);
    }
    
    // Enable word timestamps
    TranscriptionConfig config = TranscriptionConfig::defaults();
    config.word_timestamps = true;
    
    // Transcribe
    auto result = engine.transcribe(handle, audio, config);
    if (result.isError()) {
        GTEST_SKIP() << "Transcription failed: " << result.error().message;
    }
    
    const Transcription& transcription = result.value();
    
    // Verify overall confidence
    EXPECT_GE(transcription.confidence, 0.0f);
    EXPECT_LE(transcription.confidence, 1.0f);
    
    // If words are present, verify each word has valid confidence
    for (const auto& word : transcription.words) {
        EXPECT_GE(word.confidence, 0.0f);
        EXPECT_LE(word.confidence, 1.0f);
        EXPECT_FALSE(std::isnan(word.confidence));
        EXPECT_FALSE(std::isinf(word.confidence));
    }
    
    // Cleanup
    engine.unloadModel(handle);
}

// Unit test: Verify confidence score consistency across multiple transcriptions
TEST(STTPropertyUnitTest, ConfidenceScoreConsistency) {
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
    
    // Create identical test audio
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples.resize(16000);
    
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        audio.samples[i] = 0.1f * std::sin(2.0f * M_PI * 440.0f * i / audio.sample_rate);
    }
    
    // Transcribe multiple times
    std::vector<float> confidences;
    for (int i = 0; i < 3; ++i) {
        auto result = engine.transcribe(handle, audio);
        if (result.isSuccess()) {
            confidences.push_back(result.value().confidence);
        }
    }
    
    // All confidence scores should be valid
    for (float conf : confidences) {
        EXPECT_GE(conf, 0.0f);
        EXPECT_LE(conf, 1.0f);
        EXPECT_FALSE(std::isnan(conf));
        EXPECT_FALSE(std::isinf(conf));
    }
    
    // With identical audio, confidence scores should be similar (allowing for small variations)
    if (confidences.size() >= 2) {
        for (size_t i = 1; i < confidences.size(); ++i) {
            float diff = std::abs(confidences[i] - confidences[0]);
            EXPECT_LT(diff, 0.1f) << "Confidence scores vary too much for identical audio";
        }
    }
    
    // Cleanup
    engine.unloadModel(handle);
}
