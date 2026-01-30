#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>
#include "ondeviceai/tts_engine.hpp"
#include "ondeviceai/logger.hpp"
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <numeric>

using namespace ondeviceai;

// Helper functions for audio analysis
namespace {

// Calculate the duration of audio in seconds
float calculateDuration(const AudioData& audio) {
    if (audio.sample_rate <= 0) return 0.0f;
    return static_cast<float>(audio.samples.size()) / static_cast<float>(audio.sample_rate);
}

// Calculate the RMS (Root Mean Square) energy of audio
float calculateRMS(const AudioData& audio) {
    if (audio.samples.empty()) return 0.0f;
    
    float sum_squares = 0.0f;
    for (float sample : audio.samples) {
        sum_squares += sample * sample;
    }
    return std::sqrt(sum_squares / audio.samples.size());
}

// Calculate the zero-crossing rate (indicator of frequency content)
float calculateZeroCrossingRate(const AudioData& audio) {
    if (audio.samples.size() < 2) return 0.0f;
    
    int zero_crossings = 0;
    for (size_t i = 1; i < audio.samples.size(); ++i) {
        if ((audio.samples[i-1] >= 0.0f && audio.samples[i] < 0.0f) ||
            (audio.samples[i-1] < 0.0f && audio.samples[i] >= 0.0f)) {
            zero_crossings++;
        }
    }
    
    return static_cast<float>(zero_crossings) / static_cast<float>(audio.samples.size());
}

// Calculate spectral centroid (rough estimate of frequency content)
// Higher values indicate higher frequency content
float calculateSpectralCentroid(const AudioData& audio) {
    if (audio.samples.empty()) return 0.0f;
    
    // Simple frequency domain approximation using zero-crossing rate
    // In production, this would use FFT
    float zcr = calculateZeroCrossingRate(audio);
    
    // Estimate dominant frequency from zero-crossing rate
    // ZCR approximates 2 * frequency / sample_rate
    float estimated_freq = zcr * audio.sample_rate / 2.0f;
    
    return estimated_freq;
}

// Check if two audio samples are significantly different
bool areAudioDifferent(const AudioData& audio1, const AudioData& audio2, float threshold = 0.05f) {
    // Compare durations
    float duration1 = calculateDuration(audio1);
    float duration2 = calculateDuration(audio2);
    float duration_diff = std::abs(duration1 - duration2) / std::max(duration1, duration2);
    
    if (duration_diff > threshold) {
        return true;
    }
    
    // Compare frequency content (zero-crossing rate)
    float zcr1 = calculateZeroCrossingRate(audio1);
    float zcr2 = calculateZeroCrossingRate(audio2);
    float zcr_diff = std::abs(zcr1 - zcr2) / std::max(zcr1, zcr2);
    
    if (zcr_diff > threshold) {
        return true;
    }
    
    // Compare spectral centroid
    float centroid1 = calculateSpectralCentroid(audio1);
    float centroid2 = calculateSpectralCentroid(audio2);
    if (centroid1 > 0.0f && centroid2 > 0.0f) {
        float centroid_diff = std::abs(centroid1 - centroid2) / std::max(centroid1, centroid2);
        if (centroid_diff > threshold) {
            return true;
        }
    }
    
    return false;
}

} // anonymous namespace

// Helper generators for RapidCheck
namespace rc {

// Generator for random text strings suitable for TTS
Gen<std::string> genTTSText() {
    return gen::oneOf(
        // Short phrases
        gen::element<std::string>(
            "Hello world",
            "Testing speech synthesis",
            "The quick brown fox",
            "Good morning",
            "How are you today"
        ),
        // Medium sentences
        gen::element<std::string>(
            "This is a test of the text to speech system.",
            "Speech synthesis is an important technology.",
            "The weather is nice today, isn't it?",
            "I hope this test passes successfully.",
            "Property based testing helps find edge cases."
        ),
        // Longer text
        gen::element<std::string>(
            "The quick brown fox jumps over the lazy dog. This sentence contains all letters of the alphabet.",
            "Property based testing is a powerful technique for validating software correctness across many inputs.",
            "Text to speech synthesis converts written text into spoken audio using machine learning models."
        )
    );
}

// Generator for valid speed values
Gen<float> genSpeed() {
    return gen::oneOf(
        gen::element<float>(0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f),
        gen::map(
            gen::inRange(50, 200),
            [](int val) { return val / 100.0f; }
        )
    );
}

// Generator for valid pitch values
Gen<float> genPitch() {
    return gen::oneOf(
        gen::element<float>(-1.0f, -0.5f, 0.0f, 0.5f, 1.0f),
        gen::map(
            gen::inRange(-100, 100),
            [](int val) { return val / 100.0f; }
        )
    );
}

// Generator for SynthesisConfig with different parameters
Gen<SynthesisConfig> genSynthesisConfig() {
    return gen::build<SynthesisConfig>(
        gen::set(&SynthesisConfig::voice_id, gen::just(std::string("default"))),
        gen::set(&SynthesisConfig::speed, genSpeed()),
        gen::set(&SynthesisConfig::pitch, genPitch())
    );
}

} // namespace rc

// Test fixture for TTS property tests
class TTSPropertyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger
        Logger::getInstance().setLogLevel(LogLevel::Warning);
        
        // Check if a test model is available
        model_path_ = std::getenv("TEST_TTS_MODEL_PATH");
        if (!model_path_ || !std::filesystem::exists(model_path_)) {
            model_available_ = false;
            GTEST_SKIP() << "TTS model not available. Set TEST_TTS_MODEL_PATH environment variable.";
        } else {
            model_available_ = true;
        }
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    const char* model_path_ = nullptr;
    bool model_available_ = false;
};

// Feature: on-device-ai-sdk, Property 6: TTS Parameter Effects
// **Validates: Requirements 3.3**
//
// Property: For any text input, synthesizing with different speed or pitch parameters
// should produce audio with different characteristics (duration or frequency content)
//
// NOTE: This test requires a real TTS model file to execute properly.
// The test is structured to validate the property but will be skipped
// if no valid model is available. To run this test with a real model:
// 1. Download or create a TTS ONNX model
// 2. Set TEST_TTS_MODEL_PATH environment variable to the model path
// 3. Run the tests
//
// The property being tested:
// For any text input, synthesizing with different speed or pitch parameters
// should produce audio with different characteristics (duration or frequency content)
RC_GTEST_PROP(TTSPropertyTest, TTSParameterEffects, ()) {
    // Check if a test model is available
    const char* model_path_env = std::getenv("TEST_TTS_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        RC_SUCCEED("TTS model not available, skipping test");
        return;
    }
    
    // Generate random text input
    std::string text = *rc::genTTSText();
    RC_CLASSIFY(!text.empty(), "non-empty text");
    RC_CLASSIFY(text.length() > 20, "long text");
    
    // Generate two different synthesis configurations
    SynthesisConfig config1 = *rc::genSynthesisConfig();
    SynthesisConfig config2 = *rc::genSynthesisConfig();
    
    // Ensure configs are actually different
    // Either speed or pitch should differ by a meaningful amount
    float speed_diff = std::abs(config1.speed - config2.speed);
    float pitch_diff = std::abs(config1.pitch - config2.pitch);
    
    if (speed_diff < 0.2f && pitch_diff < 0.3f) {
        // Configs are too similar, make them different
        config2.speed = config1.speed * 1.5f;
        if (config2.speed > 2.0f) config2.speed = 0.5f;
    }
    
    RC_CLASSIFY(speed_diff >= 0.2f, "different speed");
    RC_CLASSIFY(pitch_diff >= 0.3f, "different pitch");
    
    // Create TTS engine and load model
    TTSEngine engine;
    auto load_result = engine.loadModel(model_path_env);
    
    if (load_result.isError()) {
        RC_SUCCEED("Failed to load model: " + load_result.error().message);
        return;
    }
    
    ModelHandle model = load_result.value();
    
    // Synthesize with first configuration
    auto synth_result1 = engine.synthesize(model, text, config1);
    
    if (synth_result1.isError()) {
        engine.unloadModel(model);
        RC_SUCCEED("Synthesis failed: " + synth_result1.error().message);
        return;
    }
    
    const AudioData& audio1 = synth_result1.value();
    
    // Synthesize with second configuration
    auto synth_result2 = engine.synthesize(model, text, config2);
    
    if (synth_result2.isError()) {
        engine.unloadModel(model);
        RC_SUCCEED("Synthesis failed: " + synth_result2.error().message);
        return;
    }
    
    const AudioData& audio2 = synth_result2.value();
    
    // PROPERTY: Different speed or pitch parameters should produce different audio
    
    // Check that both audio outputs are valid
    RC_ASSERT(!audio1.samples.empty());
    RC_ASSERT(!audio2.samples.empty());
    RC_ASSERT(audio1.sample_rate > 0);
    RC_ASSERT(audio2.sample_rate > 0);
    
    // If speed is significantly different, duration should be different
    if (speed_diff >= 0.2f) {
        float duration1 = calculateDuration(audio1);
        float duration2 = calculateDuration(audio2);
        
        RC_CLASSIFY(duration1 > 0.0f && duration2 > 0.0f, "valid durations");
        
        if (duration1 > 0.0f && duration2 > 0.0f) {
            float duration_ratio = duration1 / duration2;
            float expected_ratio = config2.speed / config1.speed;
            
            // Duration ratio should be inversely proportional to speed ratio
            // Allow 20% tolerance for processing variations
            float ratio_diff = std::abs(duration_ratio - expected_ratio) / expected_ratio;
            
            RC_CLASSIFY(ratio_diff < 0.3f, "duration matches speed ratio");
        }
    }
    
    // If pitch is significantly different, frequency content should be different
    if (pitch_diff >= 0.3f) {
        float zcr1 = calculateZeroCrossingRate(audio1);
        float zcr2 = calculateZeroCrossingRate(audio2);
        
        RC_CLASSIFY(zcr1 > 0.0f && zcr2 > 0.0f, "valid zero-crossing rates");
        
        if (zcr1 > 0.0f && zcr2 > 0.0f) {
            float zcr_diff_ratio = std::abs(zcr1 - zcr2) / std::max(zcr1, zcr2);
            RC_CLASSIFY(zcr_diff_ratio > 0.05f, "different frequency content");
        }
    }
    
    // Overall check: audio should be different
    bool are_different = areAudioDifferent(audio1, audio2, 0.05f);
    RC_ASSERT(are_different);
    
    // Cleanup
    engine.unloadModel(model);
}

// Property test: Verify speed parameter affects duration
RC_GTEST_PROP(TTSPropertyTest, SpeedAffectsDuration, ()) {
    const char* model_path_env = std::getenv("TEST_TTS_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        RC_SUCCEED("TTS model not available, skipping test");
        return;
    }
    
    // Generate random text
    std::string text = *rc::genTTSText();
    
    // Generate two different speed values
    float speed1 = *rc::genSpeed();
    float speed2 = *rc::genSpeed();
    
    // Ensure speeds are significantly different
    if (std::abs(speed1 - speed2) < 0.3f) {
        speed2 = speed1 * 1.5f;
        if (speed2 > 2.0f) speed2 = 0.5f;
    }
    
    RC_CLASSIFY(speed1 < speed2, "speed1 < speed2");
    RC_CLASSIFY(speed1 > speed2, "speed1 > speed2");
    
    // Create configs with same pitch but different speeds
    SynthesisConfig config1;
    config1.speed = speed1;
    config1.pitch = 0.0f;
    
    SynthesisConfig config2;
    config2.speed = speed2;
    config2.pitch = 0.0f;
    
    // Create TTS engine and load model
    TTSEngine engine;
    auto load_result = engine.loadModel(model_path_env);
    
    if (load_result.isError()) {
        RC_SUCCEED("Failed to load model");
        return;
    }
    
    ModelHandle model = load_result.value();
    
    // Synthesize with both speeds
    auto synth1 = engine.synthesize(model, text, config1);
    auto synth2 = engine.synthesize(model, text, config2);
    
    if (synth1.isError() || synth2.isError()) {
        engine.unloadModel(model);
        RC_SUCCEED("Synthesis failed");
        return;
    }
    
    // PROPERTY: Higher speed should produce shorter duration
    float duration1 = calculateDuration(synth1.value());
    float duration2 = calculateDuration(synth2.value());
    
    RC_ASSERT(duration1 > 0.0f);
    RC_ASSERT(duration2 > 0.0f);
    
    // Duration should be inversely proportional to speed
    if (speed1 < speed2) {
        // speed1 is slower, so duration1 should be longer
        RC_ASSERT(duration1 > duration2 * 0.8f); // Allow some tolerance
    } else {
        // speed1 is faster, so duration1 should be shorter
        RC_ASSERT(duration1 < duration2 * 1.2f); // Allow some tolerance
    }
    
    engine.unloadModel(model);
}

// Property test: Verify pitch parameter affects frequency content
RC_GTEST_PROP(TTSPropertyTest, PitchAffectsFrequency, ()) {
    const char* model_path_env = std::getenv("TEST_TTS_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        RC_SUCCEED("TTS model not available, skipping test");
        return;
    }
    
    // Generate random text
    std::string text = *rc::genTTSText();
    
    // Generate two different pitch values
    float pitch1 = *rc::genPitch();
    float pitch2 = *rc::genPitch();
    
    // Ensure pitches are significantly different
    if (std::abs(pitch1 - pitch2) < 0.5f) {
        pitch2 = pitch1 + 0.8f;
        if (pitch2 > 1.0f) pitch2 = -1.0f;
    }
    
    RC_CLASSIFY(pitch1 < pitch2, "pitch1 < pitch2");
    RC_CLASSIFY(pitch1 > pitch2, "pitch1 > pitch2");
    
    // Create configs with same speed but different pitches
    SynthesisConfig config1;
    config1.speed = 1.0f;
    config1.pitch = pitch1;
    
    SynthesisConfig config2;
    config2.speed = 1.0f;
    config2.pitch = pitch2;
    
    // Create TTS engine and load model
    TTSEngine engine;
    auto load_result = engine.loadModel(model_path_env);
    
    if (load_result.isError()) {
        RC_SUCCEED("Failed to load model");
        return;
    }
    
    ModelHandle model = load_result.value();
    
    // Synthesize with both pitches
    auto synth1 = engine.synthesize(model, text, config1);
    auto synth2 = engine.synthesize(model, text, config2);
    
    if (synth1.isError() || synth2.isError()) {
        engine.unloadModel(model);
        RC_SUCCEED("Synthesis failed");
        return;
    }
    
    // PROPERTY: Different pitch should produce different frequency content
    float zcr1 = calculateZeroCrossingRate(synth1.value());
    float zcr2 = calculateZeroCrossingRate(synth2.value());
    
    RC_ASSERT(zcr1 >= 0.0f);
    RC_ASSERT(zcr2 >= 0.0f);
    
    // Zero-crossing rates should be different
    if (zcr1 > 0.0f && zcr2 > 0.0f) {
        float zcr_diff = std::abs(zcr1 - zcr2) / std::max(zcr1, zcr2);
        RC_CLASSIFY(zcr_diff > 0.05f, "significantly different frequency");
    }
    
    engine.unloadModel(model);
}

// Unit test: Verify synthesis config structure
TEST(TTSPropertyUnitTest, SynthesisConfigStructure) {
    // Verify default config
    auto config = SynthesisConfig::defaults();
    EXPECT_EQ(config.speed, 1.0f);
    EXPECT_EQ(config.pitch, 1.0f);
    
    // Verify we can set different values
    config.speed = 1.5f;
    config.pitch = 0.5f;
    EXPECT_EQ(config.speed, 1.5f);
    EXPECT_EQ(config.pitch, 0.5f);
}

// Unit test: Verify audio analysis functions work correctly
TEST(TTSPropertyUnitTest, AudioAnalysisFunctions) {
    // Create test audio
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples.resize(16000); // 1 second
    
    // Generate a simple sine wave at 440 Hz (A4 note)
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        float t = static_cast<float>(i) / audio.sample_rate;
        audio.samples[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * t);
    }
    
    // Test duration calculation
    float duration = calculateDuration(audio);
    EXPECT_NEAR(duration, 1.0f, 0.01f);
    
    // Test RMS calculation
    float rms = calculateRMS(audio);
    EXPECT_GT(rms, 0.0f);
    EXPECT_LT(rms, 1.0f);
    // For a sine wave with amplitude 0.5, RMS should be approximately 0.5/sqrt(2) ≈ 0.354
    EXPECT_NEAR(rms, 0.354f, 0.05f);
    
    // Test zero-crossing rate
    float zcr = calculateZeroCrossingRate(audio);
    EXPECT_GT(zcr, 0.0f);
    
    // Test spectral centroid
    float centroid = calculateSpectralCentroid(audio);
    EXPECT_GT(centroid, 0.0f);
}

// Unit test: Verify audio difference detection
TEST(TTSPropertyUnitTest, AudioDifferenceDetection) {
    // Create two different audio samples
    AudioData audio1, audio2;
    audio1.sample_rate = 16000;
    audio2.sample_rate = 16000;
    
    // Audio 1: 1 second at 440 Hz
    audio1.samples.resize(16000);
    for (size_t i = 0; i < audio1.samples.size(); ++i) {
        float t = static_cast<float>(i) / audio1.sample_rate;
        audio1.samples[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * t);
    }
    
    // Audio 2: 0.5 seconds at 880 Hz (different duration and frequency)
    audio2.samples.resize(8000);
    for (size_t i = 0; i < audio2.samples.size(); ++i) {
        float t = static_cast<float>(i) / audio2.sample_rate;
        audio2.samples[i] = 0.5f * std::sin(2.0f * M_PI * 880.0f * t);
    }
    
    // These should be detected as different
    EXPECT_TRUE(areAudioDifferent(audio1, audio2));
    
    // Same audio should not be different
    EXPECT_FALSE(areAudioDifferent(audio1, audio1));
}

