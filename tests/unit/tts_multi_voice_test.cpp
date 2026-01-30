#include <gtest/gtest.h>
#include "ondeviceai/tts_engine.hpp"
#include <fstream>

using namespace ondeviceai;

class TTSMultiVoiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a dummy model file for testing
        test_model_path = "test_tts_multivoice.onnx";
        std::ofstream file(test_model_path);
        file << "dummy model content";
        file.close();
    }
    
    void TearDown() override {
        // Clean up test file
        std::remove(test_model_path.c_str());
    }
    
    std::string test_model_path;
};

TEST_F(TTSMultiVoiceTest, GetAvailableVoicesReturnsMultipleVoices) {
    TTSEngine engine;
    
    auto load_result = engine.loadModel(test_model_path);
    ASSERT_TRUE(load_result.isSuccess());
    
    auto voices_result = engine.getAvailableVoices(load_result.value());
    ASSERT_TRUE(voices_result.isSuccess());
    
    auto& voices = voices_result.value();
    
    // Should have multiple voices loaded
    EXPECT_GT(voices.size(), 1u);
    
    // Check that voices have required fields
    for (const auto& voice : voices) {
        EXPECT_FALSE(voice.id.empty());
        EXPECT_FALSE(voice.name.empty());
        EXPECT_FALSE(voice.language.empty());
        EXPECT_FALSE(voice.gender.empty());
    }
}

TEST_F(TTSMultiVoiceTest, GetAvailableVoicesIncludesMultipleLanguages) {
    TTSEngine engine;
    
    auto load_result = engine.loadModel(test_model_path);
    ASSERT_TRUE(load_result.isSuccess());
    
    auto voices_result = engine.getAvailableVoices(load_result.value());
    ASSERT_TRUE(voices_result.isSuccess());
    
    auto& voices = voices_result.value();
    
    // Collect unique languages
    std::set<std::string> languages;
    for (const auto& voice : voices) {
        languages.insert(voice.language);
    }
    
    // Should support multiple languages
    EXPECT_GT(languages.size(), 1u);
    
    // Check for some expected languages
    bool has_english = false;
    bool has_other_language = false;
    
    for (const auto& lang : languages) {
        if (lang.find("en") == 0) {
            has_english = true;
        } else {
            has_other_language = true;
        }
    }
    
    EXPECT_TRUE(has_english);
    EXPECT_TRUE(has_other_language);
}

TEST_F(TTSMultiVoiceTest, SynthesizeWithDefaultVoice) {
    TTSEngine engine;
    
    auto load_result = engine.loadModel(test_model_path);
    ASSERT_TRUE(load_result.isSuccess());
    
    // Use default config (empty voice_id should use first available voice)
    SynthesisConfig config = SynthesisConfig::defaults();
    config.voice_id = "";  // Empty to test default selection
    
    auto synth_result = engine.synthesize(load_result.value(), "Hello world", config);
    
    // Should succeed with default voice
    ASSERT_TRUE(synth_result.isSuccess());
    
    auto& audio = synth_result.value();
    EXPECT_GT(audio.samples.size(), 0u);
    EXPECT_GT(audio.sample_rate, 0);
}

TEST_F(TTSMultiVoiceTest, SynthesizeWithSpecificVoice) {
    TTSEngine engine;
    
    auto load_result = engine.loadModel(test_model_path);
    ASSERT_TRUE(load_result.isSuccess());
    
    // Get available voices
    auto voices_result = engine.getAvailableVoices(load_result.value());
    ASSERT_TRUE(voices_result.isSuccess());
    ASSERT_GT(voices_result.value().size(), 0u);
    
    // Use the first available voice explicitly
    SynthesisConfig config = SynthesisConfig::defaults();
    config.voice_id = voices_result.value()[0].id;
    
    auto synth_result = engine.synthesize(load_result.value(), "Hello world", config);
    
    // Should succeed with specified voice
    ASSERT_TRUE(synth_result.isSuccess());
    
    auto& audio = synth_result.value();
    EXPECT_GT(audio.samples.size(), 0u);
}

TEST_F(TTSMultiVoiceTest, SynthesizeWithInvalidVoiceFails) {
    TTSEngine engine;
    
    auto load_result = engine.loadModel(test_model_path);
    ASSERT_TRUE(load_result.isSuccess());
    
    // Use an invalid voice ID
    SynthesisConfig config = SynthesisConfig::defaults();
    config.voice_id = "nonexistent-voice-id";
    
    auto synth_result = engine.synthesize(load_result.value(), "Hello world", config);
    
    // Should fail with invalid voice
    ASSERT_TRUE(synth_result.isError());
    EXPECT_EQ(synth_result.error().code, ErrorCode::InvalidInputParameterValue);
    
    // Error message should mention available voices
    EXPECT_FALSE(synth_result.error().details.empty());
}

TEST_F(TTSMultiVoiceTest, SynthesizeWithDifferentLanguageVoices) {
    TTSEngine engine;
    
    auto load_result = engine.loadModel(test_model_path);
    ASSERT_TRUE(load_result.isSuccess());
    
    // Get available voices
    auto voices_result = engine.getAvailableVoices(load_result.value());
    ASSERT_TRUE(voices_result.isSuccess());
    
    auto& voices = voices_result.value();
    
    // Find voices with different languages
    std::string first_language;
    std::string second_language;
    std::string first_voice_id;
    std::string second_voice_id;
    
    for (const auto& voice : voices) {
        if (first_language.empty()) {
            first_language = voice.language;
            first_voice_id = voice.id;
        } else if (voice.language != first_language) {
            second_language = voice.language;
            second_voice_id = voice.id;
            break;
        }
    }
    
    // If we have multiple languages, test synthesis with each
    if (!second_language.empty()) {
        SynthesisConfig config1 = SynthesisConfig::defaults();
        config1.voice_id = first_voice_id;
        
        auto synth1 = engine.synthesize(load_result.value(), "Test text", config1);
        ASSERT_TRUE(synth1.isSuccess());
        
        SynthesisConfig config2 = SynthesisConfig::defaults();
        config2.voice_id = second_voice_id;
        
        auto synth2 = engine.synthesize(load_result.value(), "Test text", config2);
        ASSERT_TRUE(synth2.isSuccess());
        
        // Both should succeed
        EXPECT_GT(synth1.value().samples.size(), 0u);
        EXPECT_GT(synth2.value().samples.size(), 0u);
    }
}

TEST_F(TTSMultiVoiceTest, VoicesHaveUniqueIds) {
    TTSEngine engine;
    
    auto load_result = engine.loadModel(test_model_path);
    ASSERT_TRUE(load_result.isSuccess());
    
    auto voices_result = engine.getAvailableVoices(load_result.value());
    ASSERT_TRUE(voices_result.isSuccess());
    
    auto& voices = voices_result.value();
    
    // Check that all voice IDs are unique
    std::set<std::string> voice_ids;
    for (const auto& voice : voices) {
        EXPECT_TRUE(voice_ids.insert(voice.id).second) 
            << "Duplicate voice ID found: " << voice.id;
    }
    
    EXPECT_EQ(voice_ids.size(), voices.size());
}

TEST_F(TTSMultiVoiceTest, VoicesHaveValidGenders) {
    TTSEngine engine;
    
    auto load_result = engine.loadModel(test_model_path);
    ASSERT_TRUE(load_result.isSuccess());
    
    auto voices_result = engine.getAvailableVoices(load_result.value());
    ASSERT_TRUE(voices_result.isSuccess());
    
    auto& voices = voices_result.value();
    
    // Check that all voices have valid gender values
    std::set<std::string> valid_genders = {"male", "female", "neutral"};
    
    for (const auto& voice : voices) {
        EXPECT_TRUE(valid_genders.count(voice.gender) > 0)
            << "Invalid gender for voice " << voice.id << ": " << voice.gender;
    }
}

TEST_F(TTSMultiVoiceTest, GetAvailableVoicesWithInvalidHandle) {
    TTSEngine engine;
    
    auto voices_result = engine.getAvailableVoices(999);
    
    ASSERT_TRUE(voices_result.isError());
    EXPECT_EQ(voices_result.error().code, ErrorCode::InvalidInputModelHandle);
}

TEST_F(TTSMultiVoiceTest, MultipleModelsHaveIndependentVoices) {
    TTSEngine engine;
    
    // Load first model
    auto load1 = engine.loadModel(test_model_path);
    ASSERT_TRUE(load1.isSuccess());
    
    // Create second model file
    std::string test_model_path2 = "test_tts_multivoice2.onnx";
    {
        std::ofstream file(test_model_path2);
        file << "dummy model content 2";
    }
    
    // Load second model
    auto load2 = engine.loadModel(test_model_path2);
    ASSERT_TRUE(load2.isSuccess());
    
    // Get voices from both models
    auto voices1 = engine.getAvailableVoices(load1.value());
    auto voices2 = engine.getAvailableVoices(load2.value());
    
    ASSERT_TRUE(voices1.isSuccess());
    ASSERT_TRUE(voices2.isSuccess());
    
    // Both should have voices
    EXPECT_GT(voices1.value().size(), 0u);
    EXPECT_GT(voices2.value().size(), 0u);
    
    // Clean up second model file
    std::remove(test_model_path2.c_str());
}
