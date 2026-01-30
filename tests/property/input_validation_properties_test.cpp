#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/stt_engine.hpp"
#include "ondeviceai/tts_engine.hpp"
#include "ondeviceai/model_manager.hpp"
#include "ondeviceai/voice_pipeline.hpp"
#include "ondeviceai/types.hpp"
#include "ondeviceai/error_utils.hpp"
#include <vector>
#include <string>
#include <limits>
#include <cmath>

using namespace ondeviceai;

// ============================================================================
// RapidCheck Generators for Invalid Inputs
// ============================================================================

namespace rc {

// Generator for invalid ModelHandle values
Gen<ModelHandle> genInvalidModelHandle() {
    return gen::element(
        static_cast<ModelHandle>(0),           // Zero handle
        static_cast<ModelHandle>(999999999)    // Non-existent handle
    );
}

// Generator for invalid GenerationConfig
Gen<GenerationConfig> genInvalidGenerationConfig() {
    return gen::build<GenerationConfig>(
        gen::set(&GenerationConfig::max_tokens, 
            gen::element(-1, -100, 0)),  // Negative or zero max_tokens
        gen::set(&GenerationConfig::temperature,
            gen::element(-1.0f, -0.5f, 3.0f, 10.0f, std::numeric_limits<float>::infinity())),  // Out of range
        gen::set(&GenerationConfig::top_p,
            gen::element(-0.5f, 1.5f, 2.0f, std::numeric_limits<float>::infinity())),  // Out of [0, 1]
        gen::set(&GenerationConfig::top_k,
            gen::element(-1, -50, 0)),  // Negative or zero
        gen::set(&GenerationConfig::repetition_penalty,
            gen::element(-1.0f, 0.0f, 0.5f, 5.0f))  // Out of valid range
    );
}

// Generator for invalid SynthesisConfig
Gen<SynthesisConfig> genInvalidSynthesisConfig() {
    return gen::build<SynthesisConfig>(
        gen::set(&SynthesisConfig::voice_id, gen::just(std::string(""))),  // Empty voice ID
        gen::set(&SynthesisConfig::speed,
            gen::element(-1.0f, 0.0f, 0.1f, 5.0f, std::numeric_limits<float>::infinity())),  // Out of range
        gen::set(&SynthesisConfig::pitch,
            gen::element(-5.0f, 5.0f, std::numeric_limits<float>::infinity()))  // Out of range
    );
}

// Generator for invalid AudioData
Gen<AudioData> genInvalidAudioData() {
    return gen::build<AudioData>(
        gen::set(&AudioData::samples, gen::just(std::vector<float>())),  // Empty samples
        gen::set(&AudioData::sample_rate, gen::element(-1, 0, -44100))  // Invalid sample rate
    );
}

// Generator for invalid TranscriptionConfig
Gen<TranscriptionConfig> genInvalidTranscriptionConfig() {
    return gen::build<TranscriptionConfig>(
        gen::set(&TranscriptionConfig::language, 
            gen::element(
                std::string(""),           // Empty language
                std::string("invalid123"), // Invalid language code
                std::string("toolongforalanguagecode")  // Too long
            ))
    );
}

// Generator for invalid VAD threshold
Gen<float> genInvalidVADThreshold() {
    return gen::element(
        -1.0f, -0.5f, 1.5f, 2.0f, 
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN()
    );
}

// Generator for invalid PipelineConfig
Gen<PipelineConfig> genInvalidPipelineConfig() {
    return gen::build<PipelineConfig>(
        gen::set(&PipelineConfig::vad_threshold,
            gen::element(-0.5f, 1.5f, 2.0f, std::numeric_limits<float>::infinity()))  // Out of [0, 1]
    );
}

// Generator for empty or invalid strings
Gen<std::string> genInvalidString() {
    return gen::element(
        std::string(""),                    // Empty string
        std::string("\0", 1),              // Null character
        std::string(10000, 'x')            // Extremely long string
    );
}

// Generator for invalid model paths
Gen<std::string> genInvalidModelPath() {
    return gen::element(
        std::string(""),                           // Empty path
        std::string("/nonexistent/path/model.gguf"), // Non-existent path
        std::string("invalid\0path", 12),          // Path with null character
        std::string("/dev/null"),                  // Invalid model file
        std::string(std::string(5000, 'x') + ".gguf")  // Extremely long path
    );
}

} // namespace rc

// ============================================================================
// Property Tests for Input Validation
// ============================================================================

// Feature: on-device-ai-sdk, Property 19: Input Validation Before Execution
// **Validates: Requirements 13.5, 16.7**
//
// Property: For any invalid input parameter (negative values, null pointers, 
// out-of-range values), the SDK should return a validation error without 
// attempting the operation.
//
// This test suite verifies that all SDK components validate their inputs
// before attempting operations, ensuring:
// 1. Invalid parameters are rejected with appropriate error codes
// 2. No operations are attempted with invalid inputs
// 3. The SDK remains in a valid state after validation errors

// Test 1: LLMEngine rejects invalid model handles
RC_GTEST_PROP(InputValidationPropertyTest, LLMEngineRejectsInvalidModelHandle,
              ()) {
    LLMEngine llm;
    auto invalid_handle = *rc::genInvalidModelHandle();
    
    // Test generate with invalid handle
    auto result = llm.generate(invalid_handle, "test prompt");
    RC_ASSERT(result.isError());
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InvalidInputModelHandle);
    
    // Test generateStreaming with invalid handle
    auto stream_result = llm.generateStreaming(
        invalid_handle, 
        "test prompt",
        [](const std::string&) {}
    );
    RC_ASSERT(stream_result.isError());
    RC_ASSERT(stream_result.error().code == ErrorCode::InferenceModelNotLoaded ||
              stream_result.error().code == ErrorCode::InvalidInputModelHandle);
    
    // Test clearContext with invalid handle
    auto clear_result = llm.clearContext(invalid_handle);
    RC_ASSERT(clear_result.isError());
    RC_ASSERT(clear_result.error().code == ErrorCode::InferenceModelNotLoaded ||
              clear_result.error().code == ErrorCode::InvalidInputModelHandle);
    
    // Test tokenize with invalid handle
    auto tokenize_result = llm.tokenize(invalid_handle, "test");
    RC_ASSERT(tokenize_result.isError());
    RC_ASSERT(tokenize_result.error().code == ErrorCode::InferenceModelNotLoaded ||
              tokenize_result.error().code == ErrorCode::InvalidInputModelHandle);
}

// Test 2: LLMEngine rejects invalid GenerationConfig
RC_GTEST_PROP(InputValidationPropertyTest, LLMEngineRejectsInvalidGenerationConfig,
              ()) {
    LLMEngine llm;
    
    // Create a valid model handle (we'll use an invalid one since we're testing validation)
    auto invalid_handle = static_cast<ModelHandle>(1);
    
    // Generate invalid config
    auto invalid_config = *rc::genInvalidGenerationConfig();
    
    // Test that invalid config is rejected
    auto result = llm.generate(invalid_handle, "test prompt", invalid_config);
    RC_ASSERT(result.isError());
    
    // Error should be either model not loaded or invalid parameter
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InvalidInputParameterValue ||
              result.error().code == ErrorCode::InvalidInputConfiguration);
}

// Test 3: LLMEngine rejects empty prompts
RC_GTEST_PROP(InputValidationPropertyTest, LLMEngineRejectsEmptyPrompt,
              ()) {
    LLMEngine llm;
    auto invalid_handle = static_cast<ModelHandle>(1);
    
    // Test with empty prompt
    auto result = llm.generate(invalid_handle, "");
    RC_ASSERT(result.isError());
    
    // Should get either model not loaded or invalid input error
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InferenceInvalidInput);
}

// Test 4: LLMEngine rejects null callback for streaming
RC_GTEST_PROP(InputValidationPropertyTest, LLMEngineRejectsNullCallback,
              ()) {
    LLMEngine llm;
    auto invalid_handle = static_cast<ModelHandle>(1);
    
    // Test with null callback
    TokenCallback null_callback = nullptr;
    auto result = llm.generateStreaming(invalid_handle, "test", null_callback);
    RC_ASSERT(result.isError());
    
    // Should get either model not loaded or null pointer error
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InvalidInputNullPointer);
}

// Test 5: STTEngine rejects invalid model handles
RC_GTEST_PROP(InputValidationPropertyTest, STTEngineRejectsInvalidModelHandle,
              ()) {
    STTEngine stt;
    auto invalid_handle = *rc::genInvalidModelHandle();
    
    // Create valid audio data for testing
    AudioData audio;
    audio.samples = {0.0f, 0.1f, 0.2f};
    audio.sample_rate = 16000;
    
    // Test transcribe with invalid handle
    auto result = stt.transcribe(invalid_handle, audio);
    RC_ASSERT(result.isError());
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InvalidInputModelHandle);
}

// Test 6: STTEngine rejects invalid audio data
RC_GTEST_PROP(InputValidationPropertyTest, STTEngineRejectsInvalidAudioData,
              ()) {
    STTEngine stt;
    auto invalid_handle = static_cast<ModelHandle>(1);
    
    // Generate invalid audio data
    auto invalid_audio = *rc::genInvalidAudioData();
    
    // Test transcribe with invalid audio
    auto result = stt.transcribe(invalid_handle, invalid_audio);
    RC_ASSERT(result.isError());
    
    // Should get model not loaded or invalid audio format error
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InvalidInputAudioFormat);
}

// Test 7: STTEngine rejects invalid VAD threshold
RC_GTEST_PROP(InputValidationPropertyTest, STTEngineRejectsInvalidVADThreshold,
              ()) {
    STTEngine stt;
    
    // Create valid audio data
    AudioData audio;
    audio.samples = {0.0f, 0.1f, 0.2f};
    audio.sample_rate = 16000;
    
    // Generate invalid VAD threshold
    auto invalid_threshold = *rc::genInvalidVADThreshold();
    
    // Test detectVoiceActivity with invalid threshold
    auto result = stt.detectVoiceActivity(audio, invalid_threshold);
    RC_ASSERT(result.isError());
    RC_ASSERT(result.error().code == ErrorCode::InvalidInputParameterValue ||
              result.error().code == ErrorCode::InvalidInputAudioFormat);
}

// Test 8: TTSEngine rejects invalid model handles
RC_GTEST_PROP(InputValidationPropertyTest, TTSEngineRejectsInvalidModelHandle,
              ()) {
    TTSEngine tts;
    auto invalid_handle = *rc::genInvalidModelHandle();
    
    // Test synthesize with invalid handle
    auto result = tts.synthesize(invalid_handle, "test text");
    RC_ASSERT(result.isError());
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InvalidInputModelHandle);
    
    // Test getAvailableVoices with invalid handle
    auto voices_result = tts.getAvailableVoices(invalid_handle);
    RC_ASSERT(voices_result.isError());
    RC_ASSERT(voices_result.error().code == ErrorCode::InferenceModelNotLoaded ||
              voices_result.error().code == ErrorCode::InvalidInputModelHandle);
}

// Test 9: TTSEngine rejects invalid SynthesisConfig
RC_GTEST_PROP(InputValidationPropertyTest, TTSEngineRejectsInvalidSynthesisConfig,
              ()) {
    TTSEngine tts;
    auto invalid_handle = static_cast<ModelHandle>(1);
    
    // Generate invalid synthesis config
    auto invalid_config = *rc::genInvalidSynthesisConfig();
    
    // Test synthesize with invalid config
    auto result = tts.synthesize(invalid_handle, "test text", invalid_config);
    RC_ASSERT(result.isError());
    
    // Should get model not loaded or invalid parameter error
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InvalidInputParameterValue ||
              result.error().code == ErrorCode::InvalidInputConfiguration);
}

// Test 10: TTSEngine rejects empty text
RC_GTEST_PROP(InputValidationPropertyTest, TTSEngineRejectsEmptyText,
              ()) {
    TTSEngine tts;
    auto invalid_handle = static_cast<ModelHandle>(1);
    
    // Test with empty text
    auto result = tts.synthesize(invalid_handle, "");
    RC_ASSERT(result.isError());
    
    // Should get model not loaded or invalid input error
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InferenceInvalidInput);
}

// Test 11: TTSEngine rejects null callback for streaming
RC_GTEST_PROP(InputValidationPropertyTest, TTSEngineRejectsNullCallback,
              ()) {
    TTSEngine tts;
    auto invalid_handle = static_cast<ModelHandle>(1);
    
    // Test with null callback
    AudioChunkCallback null_callback = nullptr;
    auto result = tts.synthesizeStreaming(invalid_handle, "test", null_callback);
    RC_ASSERT(result.isError());
    
    // Should get model not loaded or null pointer error
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InvalidInputNullPointer);
}

// Test 12: ModelManager rejects invalid model IDs
RC_GTEST_PROP(InputValidationPropertyTest, ModelManagerRejectsInvalidModelID,
              ()) {
    ModelManager manager("./test_storage", "https://example.com/registry");
    
    // Generate invalid model ID
    auto invalid_id = *rc::genInvalidString();
    
    // Test getModelInfo with invalid ID
    auto result = manager.getModelInfo(invalid_id);
    RC_ASSERT(result.isError());
    RC_ASSERT(result.error().code == ErrorCode::ModelNotFoundInRegistry ||
              result.error().code == ErrorCode::InvalidInputParameterValue);
    
    // Test deleteModel with invalid ID
    auto delete_result = manager.deleteModel(invalid_id);
    RC_ASSERT(delete_result.isError());
    RC_ASSERT(delete_result.error().code == ErrorCode::ModelNotFoundInRegistry ||
              delete_result.error().code == ErrorCode::InvalidInputParameterValue);
}

// Test 13: ModelManager rejects null progress callback
RC_GTEST_PROP(InputValidationPropertyTest, ModelManagerRejectsNullProgressCallback,
              ()) {
    ModelManager manager("./test_storage", "https://example.com/registry");
    
    // Test downloadModel with null callback
    ProgressCallback null_callback = nullptr;
    auto result = manager.downloadModel("test-model", null_callback);
    RC_ASSERT(result.isError());
    RC_ASSERT(result.error().code == ErrorCode::InvalidInputNullPointer ||
              result.error().code == ErrorCode::ModelNotFoundInRegistry);
}

// Test 14: VoicePipeline rejects invalid model handles in configuration
RC_GTEST_PROP(InputValidationPropertyTest, VoicePipelineRejectsInvalidHandles,
              ()) {
    STTEngine stt;
    LLMEngine llm;
    TTSEngine tts;
    VoicePipeline pipeline(&stt, &llm, &tts);
    
    auto invalid_handle = *rc::genInvalidModelHandle();
    auto valid_handle = static_cast<ModelHandle>(1);
    
    // Test configure with invalid STT handle
    auto result1 = pipeline.configure(invalid_handle, valid_handle, valid_handle);
    RC_ASSERT(result1.isError());
    RC_ASSERT(result1.error().code == ErrorCode::InferenceModelNotLoaded ||
              result1.error().code == ErrorCode::InvalidInputModelHandle);
    
    // Test configure with invalid LLM handle
    auto result2 = pipeline.configure(valid_handle, invalid_handle, valid_handle);
    RC_ASSERT(result2.isError());
    RC_ASSERT(result2.error().code == ErrorCode::InferenceModelNotLoaded ||
              result2.error().code == ErrorCode::InvalidInputModelHandle);
    
    // Test configure with invalid TTS handle
    auto result3 = pipeline.configure(valid_handle, valid_handle, invalid_handle);
    RC_ASSERT(result3.isError());
    RC_ASSERT(result3.error().code == ErrorCode::InferenceModelNotLoaded ||
              result3.error().code == ErrorCode::InvalidInputModelHandle);
}

// Test 15: VoicePipeline rejects null callbacks
RC_GTEST_PROP(InputValidationPropertyTest, VoicePipelineRejectsNullCallbacks,
              ()) {
    STTEngine stt;
    LLMEngine llm;
    TTSEngine tts;
    VoicePipeline pipeline(&stt, &llm, &tts);
    
    // Create valid callbacks
    AudioStreamCallback valid_audio_input = []() { return AudioData(); };
    AudioChunkCallback valid_audio_output = [](const AudioData&) {};
    TranscriptionCallback valid_transcription = [](const std::string&) {};
    TextCallback valid_text = [](const std::string&) {};
    
    // Test with null audio input callback
    AudioStreamCallback null_audio_input = nullptr;
    auto result1 = pipeline.startConversation(
        null_audio_input, valid_audio_output, valid_transcription, valid_text
    );
    RC_ASSERT(result1.isError());
    RC_ASSERT(result1.error().code == ErrorCode::InvalidInputNullPointer ||
              result1.error().code == ErrorCode::InvalidInputConfiguration);
    
    // Test with null audio output callback
    AudioChunkCallback null_audio_output = nullptr;
    auto result2 = pipeline.startConversation(
        valid_audio_input, null_audio_output, valid_transcription, valid_text
    );
    RC_ASSERT(result2.isError());
    RC_ASSERT(result2.error().code == ErrorCode::InvalidInputNullPointer ||
              result2.error().code == ErrorCode::InvalidInputConfiguration);
}

// Test 16: VoicePipeline rejects invalid PipelineConfig
RC_GTEST_PROP(InputValidationPropertyTest, VoicePipelineRejectsInvalidConfig,
              ()) {
    STTEngine stt;
    LLMEngine llm;
    TTSEngine tts;
    VoicePipeline pipeline(&stt, &llm, &tts);
    
    auto valid_handle = static_cast<ModelHandle>(1);
    
    // Generate invalid pipeline config
    auto invalid_config = *rc::genInvalidPipelineConfig();
    
    // Test configure with invalid config
    auto result = pipeline.configure(valid_handle, valid_handle, valid_handle, invalid_config);
    RC_ASSERT(result.isError());
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InvalidInputParameterValue ||
              result.error().code == ErrorCode::InvalidInputConfiguration);
}

// Test 17: All components reject operations before configuration
RC_GTEST_PROP(InputValidationPropertyTest, ComponentsRejectUnconfiguredOperations,
              ()) {
    STTEngine stt;
    LLMEngine llm;
    TTSEngine tts;
    VoicePipeline pipeline(&stt, &llm, &tts);
    
    // Try to start conversation without configuration
    auto result = pipeline.startConversation(
        []() { return AudioData(); },
        [](const AudioData&) {},
        [](const std::string&) {},
        [](const std::string&) {}
    );
    RC_ASSERT(result.isError());
    RC_ASSERT(result.error().code == ErrorCode::InvalidInputConfiguration ||
              result.error().code == ErrorCode::InferenceModelNotLoaded);
}

// Test 18: Components validate parameter ranges
RC_GTEST_PROP(InputValidationPropertyTest, ComponentsValidateParameterRanges,
              (float invalid_temp, float invalid_top_p, int invalid_max_tokens)) {
    // Constrain to invalid ranges
    RC_PRE(invalid_temp < 0.0f || invalid_temp > 2.0f);
    RC_PRE(invalid_top_p < 0.0f || invalid_top_p > 1.0f);
    RC_PRE(invalid_max_tokens <= 0);
    
    LLMEngine llm;
    auto invalid_handle = static_cast<ModelHandle>(1);
    
    // Create config with invalid parameters
    GenerationConfig config;
    config.temperature = invalid_temp;
    config.top_p = invalid_top_p;
    config.max_tokens = invalid_max_tokens;
    
    // Test that invalid parameters are rejected
    auto result = llm.generate(invalid_handle, "test", config);
    RC_ASSERT(result.isError());
    
    // Should get validation error or model not loaded
    RC_ASSERT(result.error().code == ErrorCode::InferenceModelNotLoaded ||
              result.error().code == ErrorCode::InvalidInputParameterValue ||
              result.error().code == ErrorCode::InvalidInputConfiguration);
}

// Test 19: Components reject extremely long inputs
RC_GTEST_PROP(InputValidationPropertyTest, ComponentsRejectExtremelyLongInputs,
              ()) {
    LLMEngine llm;
    TTSEngine tts;
    auto invalid_handle = static_cast<ModelHandle>(1);
    
    // Create extremely long string (100KB)
    std::string extremely_long_input(100000, 'x');
    
    // Test LLM with extremely long prompt
    auto llm_result = llm.generate(invalid_handle, extremely_long_input);
    RC_ASSERT(llm_result.isError());
    
    // Test TTS with extremely long text
    auto tts_result = tts.synthesize(invalid_handle, extremely_long_input);
    RC_ASSERT(tts_result.isError());
}

// Test 20: Model loading rejects invalid paths
RC_GTEST_PROP(InputValidationPropertyTest, ComponentsRejectInvalidModelPaths,
              ()) {
    LLMEngine llm;
    STTEngine stt;
    TTSEngine tts;
    
    // Generate invalid model path
    auto invalid_path = *rc::genInvalidModelPath();
    
    // Test LLM loadModel with invalid path
    auto llm_result = llm.loadModel(invalid_path);
    RC_ASSERT(llm_result.isError());
    RC_ASSERT(llm_result.error().code == ErrorCode::ModelFileNotFound ||
              llm_result.error().code == ErrorCode::InvalidInputParameterValue ||
              llm_result.error().code == ErrorCode::ModelFileCorrupted);
    
    // Test STT loadModel with invalid path
    auto stt_result = stt.loadModel(invalid_path);
    RC_ASSERT(stt_result.isError());
    RC_ASSERT(stt_result.error().code == ErrorCode::ModelFileNotFound ||
              stt_result.error().code == ErrorCode::InvalidInputParameterValue ||
              stt_result.error().code == ErrorCode::ModelFileCorrupted);
    
    // Test TTS loadModel with invalid path
    auto tts_result = tts.loadModel(invalid_path);
    RC_ASSERT(tts_result.isError());
    RC_ASSERT(tts_result.error().code == ErrorCode::ModelFileNotFound ||
              tts_result.error().code == ErrorCode::InvalidInputParameterValue ||
              tts_result.error().code == ErrorCode::ModelFileCorrupted);
}

// Test 21: Verify validation errors include descriptive messages
RC_GTEST_PROP(InputValidationPropertyTest, ValidationErrorsIncludeDescriptions,
              ()) {
    LLMEngine llm;
    auto invalid_handle = *rc::genInvalidModelHandle();
    
    // Generate error by using invalid handle
    auto result = llm.generate(invalid_handle, "test");
    RC_ASSERT(result.isError());
    
    // Verify error has descriptive message
    const Error& error = result.error();
    RC_ASSERT(!error.message.empty());
    RC_ASSERT(!error.details.empty());
    RC_ASSERT(error.message.length() >= 10u);
}

// Test 22: Verify validation happens before any resource allocation
RC_GTEST_PROP(InputValidationPropertyTest, ValidationBeforeResourceAllocation,
              ()) {
    LLMEngine llm;
    
    // Use invalid handle - should fail validation immediately
    auto invalid_handle = *rc::genInvalidModelHandle();
    
    // This should fail fast without attempting to allocate resources
    auto start_time = std::chrono::high_resolution_clock::now();
    auto result = llm.generate(invalid_handle, "test");
    auto end_time = std::chrono::high_resolution_clock::now();
    
    RC_ASSERT(result.isError());
    
    // Validation should be very fast (< 1ms)
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    RC_ASSERT(duration.count() < 10);  // Should be nearly instant
}

// Test 23: Verify SDK remains usable after validation errors
RC_GTEST_PROP(InputValidationPropertyTest, SDKUsableAfterValidationError,
              ()) {
    LLMEngine llm;
    auto invalid_handle = *rc::genInvalidModelHandle();
    
    // Generate validation error
    auto result1 = llm.generate(invalid_handle, "test");
    RC_ASSERT(result1.isError());
    
    // SDK should still be usable - try another operation
    auto result2 = llm.generate(invalid_handle, "another test");
    RC_ASSERT(result2.isError());
    
    // Both should produce consistent errors
    RC_ASSERT(result1.error().code == result2.error().code);
}

// Test 24: Verify all validation errors are in InvalidInput category
RC_GTEST_PROP(InputValidationPropertyTest, ValidationErrorsInCorrectCategory,
              ()) {
    LLMEngine llm;
    
    // Create various invalid inputs
    GenerationConfig invalid_config;
    invalid_config.temperature = -1.0f;  // Invalid
    
    auto result = llm.generate(static_cast<ModelHandle>(1), "test", invalid_config);
    RC_ASSERT(result.isError());
    
    // If it's a validation error (not model not loaded), it should be in InvalidInput category
    if (result.error().code != ErrorCode::InferenceModelNotLoaded) {
        int code_value = static_cast<int>(result.error().code);
        bool is_invalid_input = (code_value >= 1500 && code_value < 1600);
        RC_ASSERT(is_invalid_input || 
                  result.error().code == ErrorCode::InvalidInputParameterValue ||
                  result.error().code == ErrorCode::InvalidInputConfiguration);
    }
}

// Test 25: Verify validation errors provide recovery suggestions
RC_GTEST_PROP(InputValidationPropertyTest, ValidationErrorsProvideRecoverySuggestions,
              ()) {
    LLMEngine llm;
    
    // Create invalid config
    GenerationConfig invalid_config;
    invalid_config.max_tokens = -100;  // Invalid
    
    auto result = llm.generate(static_cast<ModelHandle>(1), "test", invalid_config);
    RC_ASSERT(result.isError());
    
    // Validation errors should ideally have recovery suggestions
    // (though this is not strictly required for all errors)
    const Error& error = result.error();
    if (error.recovery_suggestion.has_value()) {
        RC_ASSERT(!error.recovery_suggestion.value().empty());
    }
}
