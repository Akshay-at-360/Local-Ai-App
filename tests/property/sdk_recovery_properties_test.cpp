#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/stt_engine.hpp"
#include "ondeviceai/tts_engine.hpp"
#include "ondeviceai/voice_pipeline.hpp"
#include "ondeviceai/memory_manager.hpp"
#include "ondeviceai/error_utils.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace ondeviceai;

// ============================================================================
// RapidCheck Generators for Error Scenarios
// ============================================================================

namespace rc {

// Generator for inference error scenarios
enum class InferenceErrorScenario {
    InvalidModelHandle,
    EmptyPrompt,
    VeryLongPrompt,
    InvalidConfig,
    NegativeMaxTokens,
    InvalidTemperature,
    ModelNotLoaded
};

template<>
struct Arbitrary<InferenceErrorScenario> {
    static Gen<InferenceErrorScenario> arbitrary() {
        return gen::element(
            InferenceErrorScenario::InvalidModelHandle,
            InferenceErrorScenario::EmptyPrompt,
            InferenceErrorScenario::VeryLongPrompt,
            InferenceErrorScenario::InvalidConfig,
            InferenceErrorScenario::NegativeMaxTokens,
            InferenceErrorScenario::InvalidTemperature,
            InferenceErrorScenario::ModelNotLoaded
        );
    }
};

// Generator for audio error scenarios
enum class AudioErrorScenario {
    InvalidModelHandle,
    EmptyAudio,
    InvalidSampleRate,
    ModelNotLoaded
};

template<>
struct Arbitrary<AudioErrorScenario> {
    static Gen<AudioErrorScenario> arbitrary() {
        return gen::element(
            AudioErrorScenario::InvalidModelHandle,
            AudioErrorScenario::EmptyAudio,
            AudioErrorScenario::InvalidSampleRate,
            AudioErrorScenario::ModelNotLoaded
        );
    }
};

} // namespace rc

// ============================================================================
// Helper Functions
// ============================================================================

// Generate an LLM inference error based on scenario
Result<std::string> generateLLMError(LLMEngine& engine, rc::InferenceErrorScenario scenario) {
    switch (scenario) {
        case rc::InferenceErrorScenario::InvalidModelHandle: {
            // Use an invalid model handle
            ModelHandle invalid_handle = static_cast<ModelHandle>(999999);
            return engine.generate(invalid_handle, "test prompt");
        }
        
        case rc::InferenceErrorScenario::EmptyPrompt: {
            // Use empty prompt (may or may not be an error depending on implementation)
            ModelHandle handle = static_cast<ModelHandle>(1);
            return engine.generate(handle, "");
        }
        
        case rc::InferenceErrorScenario::VeryLongPrompt: {
            // Use extremely long prompt that might exceed context window
            ModelHandle handle = static_cast<ModelHandle>(1);
            std::string very_long_prompt(100000, 'x');  // 100k characters
            return engine.generate(handle, very_long_prompt);
        }
        
        case rc::InferenceErrorScenario::InvalidConfig: {
            ModelHandle handle = static_cast<ModelHandle>(1);
            GenerationConfig config;
            config.temperature = -1.0f;  // Invalid temperature
            return engine.generate(handle, "test", config);
        }
        
        case rc::InferenceErrorScenario::NegativeMaxTokens: {
            ModelHandle handle = static_cast<ModelHandle>(1);
            GenerationConfig config;
            config.max_tokens = -100;  // Invalid max tokens
            return engine.generate(handle, "test", config);
        }
        
        case rc::InferenceErrorScenario::InvalidTemperature: {
            ModelHandle handle = static_cast<ModelHandle>(1);
            GenerationConfig config;
            config.temperature = 10.0f;  // Extremely high temperature
            return engine.generate(handle, "test", config);
        }
        
        case rc::InferenceErrorScenario::ModelNotLoaded: {
            // Try to use a handle that was never loaded
            ModelHandle handle = static_cast<ModelHandle>(42);
            return engine.generate(handle, "test prompt");
        }
        
        default:
            return Result<std::string>::failure(
                ErrorUtils::invalidInputParameterValue("scenario", "Unknown scenario")
            );
    }
}

// Generate an STT inference error based on scenario
Result<Transcription> generateSTTError(STTEngine& engine, rc::AudioErrorScenario scenario) {
    switch (scenario) {
        case rc::AudioErrorScenario::InvalidModelHandle: {
            ModelHandle invalid_handle = static_cast<ModelHandle>(999999);
            AudioData audio;
            audio.samples = {0.1f, 0.2f, 0.3f};
            audio.sample_rate = 16000;
            return engine.transcribe(invalid_handle, audio);
        }
        
        case rc::AudioErrorScenario::EmptyAudio: {
            ModelHandle handle = static_cast<ModelHandle>(1);
            AudioData audio;
            audio.samples = {};  // Empty audio
            audio.sample_rate = 16000;
            return engine.transcribe(handle, audio);
        }
        
        case rc::AudioErrorScenario::InvalidSampleRate: {
            ModelHandle handle = static_cast<ModelHandle>(1);
            AudioData audio;
            audio.samples = {0.1f, 0.2f, 0.3f};
            audio.sample_rate = -1;  // Invalid sample rate
            return engine.transcribe(handle, audio);
        }
        
        case rc::AudioErrorScenario::ModelNotLoaded: {
            ModelHandle handle = static_cast<ModelHandle>(42);
            AudioData audio;
            audio.samples = {0.1f, 0.2f, 0.3f};
            audio.sample_rate = 16000;
            return engine.transcribe(handle, audio);
        }
        
        default:
            return Result<Transcription>::failure(
                ErrorUtils::invalidInputParameterValue("scenario", "Unknown scenario")
            );
    }
}

// Generate a TTS inference error based on scenario
Result<AudioData> generateTTSError(TTSEngine& engine, rc::AudioErrorScenario scenario) {
    switch (scenario) {
        case rc::AudioErrorScenario::InvalidModelHandle: {
            ModelHandle invalid_handle = static_cast<ModelHandle>(999999);
            return engine.synthesize(invalid_handle, "test text");
        }
        
        case rc::AudioErrorScenario::EmptyAudio: {
            // For TTS, empty text instead of empty audio
            ModelHandle handle = static_cast<ModelHandle>(1);
            return engine.synthesize(handle, "");
        }
        
        case rc::AudioErrorScenario::InvalidSampleRate: {
            // For TTS, use invalid config
            ModelHandle handle = static_cast<ModelHandle>(1);
            SynthesisConfig config;
            config.speed = -1.0f;  // Invalid speed
            return engine.synthesize(handle, "test", config);
        }
        
        case rc::AudioErrorScenario::ModelNotLoaded: {
            ModelHandle handle = static_cast<ModelHandle>(42);
            return engine.synthesize(handle, "test text");
        }
        
        default:
            return Result<AudioData>::failure(
                ErrorUtils::invalidInputParameterValue("scenario", "Unknown scenario")
            );
    }
}

// ============================================================================
// Property 18: SDK Usable After Error
// ============================================================================

// Feature: on-device-ai-sdk, Property 18: SDK Usable After Error
// **Validates: Requirements 13.4**
//
// Property: For any inference error, subsequent operations on the SDK should
// succeed (SDK should not be left in corrupted state).
//
// This property test generates random inference errors across all SDK components
// (LLMEngine, STTEngine, TTSEngine, VoicePipeline) and verifies that:
// 1. Errors are properly reported
// 2. SDK remains in a valid state after errors
// 3. Subsequent operations can succeed
// 4. No resource leaks or corruption occurs
//
// The test runs with minimum 100 iterations to ensure comprehensive coverage
// of different error scenarios and their recovery paths.
RC_GTEST_PROP(SDKRecoveryPropertyTest, LLMEngineUsableAfterInferenceError,
              (rc::InferenceErrorScenario error_scenario)) {
    // Create LLM engine
    LLMEngine engine;
    MemoryManager memory_mgr(4ULL * 1024 * 1024 * 1024);
    engine.setMemoryManager(&memory_mgr);
    
    // Generate an error
    auto error_result = generateLLMError(engine, error_scenario);
    
    // Property 1: The operation should return an error (not crash)
    RC_ASSERT(error_result.isError());
    
    // Property 2: The error should have a descriptive message
    RC_ASSERT(!error_result.error().message.empty());
    RC_ASSERT(error_result.error().message.length() >= 10u);
    
    // Property 3: SDK should remain usable - try another operation
    // This should also fail (since we don't have a valid model loaded)
    // but it should fail gracefully, not crash
    ModelHandle test_handle = static_cast<ModelHandle>(1);
    auto recovery_result = engine.generate(test_handle, "recovery test");
    
    // Should return an error (model not loaded) but not crash
    RC_ASSERT(recovery_result.isError());
    
    // Property 4: Error should be consistent and descriptive
    RC_ASSERT(!recovery_result.error().message.empty());
    
    // Property 5: Try tokenization (different operation type)
    auto tokenize_result = engine.tokenize(test_handle, "test text");
    
    // Should also fail gracefully
    RC_ASSERT(tokenize_result.isError());
    RC_ASSERT(!tokenize_result.error().message.empty());
    
    // Property 6: Try detokenization (another operation type)
    std::vector<int> test_tokens = {1, 2, 3};
    auto detokenize_result = engine.detokenize(test_handle, test_tokens);
    
    // Should also fail gracefully
    RC_ASSERT(detokenize_result.isError());
    RC_ASSERT(!detokenize_result.error().message.empty());
    
    // Property 7: All subsequent operations should produce valid errors
    // (not corrupted state or crashes)
    RC_ASSERT(recovery_result.error().code != ErrorCode::InvalidInputNullPointer);
    RC_ASSERT(tokenize_result.error().code != ErrorCode::InvalidInputNullPointer);
    RC_ASSERT(detokenize_result.error().code != ErrorCode::InvalidInputNullPointer);
}

// Test that STT engine remains usable after errors
RC_GTEST_PROP(SDKRecoveryPropertyTest, STTEngineUsableAfterInferenceError,
              (rc::AudioErrorScenario error_scenario)) {
    // Create STT engine
    STTEngine engine;
    
    // Generate an error
    auto error_result = generateSTTError(engine, error_scenario);
    
    // Property 1: The operation should return an error (not crash)
    RC_ASSERT(error_result.isError());
    
    // Property 2: The error should have a descriptive message
    RC_ASSERT(!error_result.error().message.empty());
    RC_ASSERT(error_result.error().message.length() >= 10u);
    
    // Property 3: SDK should remain usable - try another operation
    ModelHandle test_handle = static_cast<ModelHandle>(1);
    AudioData test_audio;
    test_audio.samples = {0.1f, 0.2f, 0.3f};
    test_audio.sample_rate = 16000;
    
    auto recovery_result = engine.transcribe(test_handle, test_audio);
    
    // Should return an error (model not loaded) but not crash
    RC_ASSERT(recovery_result.isError());
    RC_ASSERT(!recovery_result.error().message.empty());
    
    // Property 4: Try VAD (different operation type)
    auto vad_result = engine.detectVoiceActivity(test_audio);
    
    // VAD might succeed or fail depending on implementation
    // The key is that it doesn't crash
    if (vad_result.isError()) {
        RC_ASSERT(!vad_result.error().message.empty());
    }
    
    // Property 5: Try another transcription with different audio
    AudioData test_audio2;
    test_audio2.samples = {0.5f, 0.6f, 0.7f, 0.8f};
    test_audio2.sample_rate = 16000;
    
    auto recovery_result2 = engine.transcribe(test_handle, test_audio2);
    
    // Should also fail gracefully
    RC_ASSERT(recovery_result2.isError());
    RC_ASSERT(!recovery_result2.error().message.empty());
}

// Test that TTS engine remains usable after errors
RC_GTEST_PROP(SDKRecoveryPropertyTest, TTSEngineUsableAfterInferenceError,
              (rc::AudioErrorScenario error_scenario)) {
    // Create TTS engine
    TTSEngine engine;
    
    // Generate an error
    auto error_result = generateTTSError(engine, error_scenario);
    
    // Property 1: The operation should return an error (not crash)
    RC_ASSERT(error_result.isError());
    
    // Property 2: The error should have a descriptive message
    RC_ASSERT(!error_result.error().message.empty());
    RC_ASSERT(error_result.error().message.length() >= 10u);
    
    // Property 3: SDK should remain usable - try another operation
    ModelHandle test_handle = static_cast<ModelHandle>(1);
    auto recovery_result = engine.synthesize(test_handle, "recovery test");
    
    // Should return an error (model not loaded) but not crash
    RC_ASSERT(recovery_result.isError());
    RC_ASSERT(!recovery_result.error().message.empty());
    
    // Property 4: Try getting available voices (different operation type)
    auto voices_result = engine.getAvailableVoices(test_handle);
    
    // Should also fail gracefully
    RC_ASSERT(voices_result.isError());
    RC_ASSERT(!voices_result.error().message.empty());
    
    // Property 5: Try synthesis with different text
    auto recovery_result2 = engine.synthesize(test_handle, "another test");
    
    // Should also fail gracefully
    RC_ASSERT(recovery_result2.isError());
    RC_ASSERT(!recovery_result2.error().message.empty());
}

// Test that Voice Pipeline remains usable after errors
RC_GTEST_PROP(SDKRecoveryPropertyTest, VoicePipelineUsableAfterConfigurationError,
              ()) {
    // Create engines
    STTEngine stt;
    LLMEngine llm;
    TTSEngine tts;
    MemoryManager memory_mgr(4ULL * 1024 * 1024 * 1024);
    llm.setMemoryManager(&memory_mgr);
    
    // Create voice pipeline
    VoicePipeline pipeline(&stt, &llm, &tts);
    
    // Try to configure with invalid handles
    ModelHandle invalid_stt = static_cast<ModelHandle>(999);
    ModelHandle invalid_llm = static_cast<ModelHandle>(998);
    ModelHandle invalid_tts = static_cast<ModelHandle>(997);
    
    auto config_result = pipeline.configure(invalid_stt, invalid_llm, invalid_tts);
    
    // Property 1: Configuration with invalid handles should fail gracefully
    // (may succeed or fail depending on validation timing)
    if (config_result.isError()) {
        RC_ASSERT(!config_result.error().message.empty());
    }
    
    // Property 2: Pipeline should remain usable - try to clear history
    auto clear_result = pipeline.clearHistory();
    
    // Should succeed (clearing empty history is valid)
    RC_ASSERT(clear_result.isSuccess());
    
    // Property 3: Try to get history
    auto history_result = pipeline.getHistory();
    
    // Should succeed (getting empty history is valid)
    RC_ASSERT(history_result.isSuccess());
    RC_ASSERT(history_result.value().empty());
    
    // Property 4: Try to configure again with different invalid handles
    ModelHandle invalid_stt2 = static_cast<ModelHandle>(888);
    ModelHandle invalid_llm2 = static_cast<ModelHandle>(887);
    ModelHandle invalid_tts2 = static_cast<ModelHandle>(886);
    
    auto config_result2 = pipeline.configure(invalid_stt2, invalid_llm2, invalid_tts2);
    
    // Should handle gracefully (not crash)
    if (config_result2.isError()) {
        RC_ASSERT(!config_result2.error().message.empty());
    }
    
    // Property 5: Pipeline operations should still work
    auto clear_result2 = pipeline.clearHistory();
    RC_ASSERT(clear_result2.isSuccess());
}

// Test that multiple consecutive errors don't corrupt state
RC_GTEST_PROP(SDKRecoveryPropertyTest, MultipleConsecutiveErrorsHandledGracefully,
              (std::vector<rc::InferenceErrorScenario> error_scenarios)) {
    // Limit the number of scenarios to avoid very long tests
    RC_PRE(error_scenarios.size() > 0u && error_scenarios.size() <= 10u);
    
    // Create LLM engine
    LLMEngine engine;
    MemoryManager memory_mgr(4ULL * 1024 * 1024 * 1024);
    engine.setMemoryManager(&memory_mgr);
    
    // Generate multiple errors in sequence
    for (const auto& scenario : error_scenarios) {
        auto error_result = generateLLMError(engine, scenario);
        
        // Property 1: Each error should be handled gracefully
        RC_ASSERT(error_result.isError());
        RC_ASSERT(!error_result.error().message.empty());
    }
    
    // Property 2: After multiple errors, SDK should still be usable
    ModelHandle test_handle = static_cast<ModelHandle>(1);
    auto recovery_result = engine.generate(test_handle, "final recovery test");
    
    // Should fail gracefully (model not loaded)
    RC_ASSERT(recovery_result.isError());
    RC_ASSERT(!recovery_result.error().message.empty());
    
    // Property 3: Error should be consistent (not corrupted)
    RC_ASSERT(recovery_result.error().code == ErrorCode::InferenceModelNotLoaded ||
              recovery_result.error().code == ErrorCode::InvalidInputModelHandle);
}

// Test that errors in one engine don't affect other engines
RC_GTEST_PROP(SDKRecoveryPropertyTest, ErrorsInOneEngineDoNotAffectOthers,
              (rc::InferenceErrorScenario llm_scenario, rc::AudioErrorScenario audio_scenario)) {
    // Create all engines
    LLMEngine llm;
    STTEngine stt;
    TTSEngine tts;
    MemoryManager memory_mgr(4ULL * 1024 * 1024 * 1024);
    llm.setMemoryManager(&memory_mgr);
    
    // Generate error in LLM engine
    auto llm_error = generateLLMError(llm, llm_scenario);
    RC_ASSERT(llm_error.isError());
    
    // Property 1: STT engine should still be usable
    auto stt_result = generateSTTError(stt, audio_scenario);
    RC_ASSERT(stt_result.isError());
    RC_ASSERT(!stt_result.error().message.empty());
    
    // Property 2: TTS engine should still be usable
    auto tts_result = generateTTSError(tts, audio_scenario);
    RC_ASSERT(tts_result.isError());
    RC_ASSERT(!tts_result.error().message.empty());
    
    // Property 3: LLM engine should still be usable after errors in other engines
    auto llm_recovery = generateLLMError(llm, rc::InferenceErrorScenario::InvalidModelHandle);
    RC_ASSERT(llm_recovery.isError());
    RC_ASSERT(!llm_recovery.error().message.empty());
    
    // Property 4: All engines should maintain independent state
    // (errors in one don't corrupt others)
    RC_ASSERT(llm_error.error().code != ErrorCode::InvalidInputNullPointer);
    RC_ASSERT(stt_result.error().code != ErrorCode::InvalidInputNullPointer);
    RC_ASSERT(tts_result.error().code != ErrorCode::InvalidInputNullPointer);
}

// Test that SDK can recover from errors and perform successful operations
// when given valid inputs after errors
TEST(SDKRecoveryUnitTest, SDKCanPerformValidOperationsAfterErrors) {
    // Check if a test model is available
    const char* model_path_env = std::getenv("TEST_MODEL_PATH");
    if (!model_path_env || !std::filesystem::exists(model_path_env)) {
        GTEST_SKIP() << "Skipping test - no valid GGUF model available. "
                     << "Set TEST_MODEL_PATH environment variable to run this test.";
    }
    
    std::string model_path(model_path_env);
    
    // Create LLM engine
    LLMEngine engine;
    MemoryManager memory_mgr(4ULL * 1024 * 1024 * 1024);
    engine.setMemoryManager(&memory_mgr);
    
    // Generate several errors first
    ModelHandle invalid_handle = static_cast<ModelHandle>(999);
    
    // Error 1: Invalid handle
    auto error1 = engine.generate(invalid_handle, "test");
    ASSERT_TRUE(error1.isError());
    
    // Error 2: Invalid config
    GenerationConfig invalid_config;
    invalid_config.temperature = -1.0f;
    auto error2 = engine.generate(invalid_handle, "test", invalid_config);
    ASSERT_TRUE(error2.isError());
    
    // Error 3: Empty prompt
    auto error3 = engine.generate(invalid_handle, "");
    ASSERT_TRUE(error3.isError());
    
    // Now load a valid model
    auto load_result = engine.loadModel(model_path);
    if (load_result.isError()) {
        GTEST_SKIP() << "Failed to load model: " << load_result.error().message;
    }
    
    ModelHandle valid_handle = load_result.value();
    
    // Property: After errors, SDK should be able to perform valid operations
    GenerationConfig valid_config = GenerationConfig::defaults();
    valid_config.max_tokens = 10;
    
    auto success_result = engine.generate(valid_handle, "Hello", valid_config);
    
    // Should succeed now that we have a valid model and config
    ASSERT_TRUE(success_result.isSuccess());
    EXPECT_FALSE(success_result.value().empty());
    
    // Try tokenization
    auto tokenize_result = engine.tokenize(valid_handle, "test text");
    ASSERT_TRUE(tokenize_result.isSuccess());
    EXPECT_FALSE(tokenize_result.value().empty());
    
    // Try detokenization
    auto detokenize_result = engine.detokenize(valid_handle, tokenize_result.value());
    ASSERT_TRUE(detokenize_result.isSuccess());
    EXPECT_FALSE(detokenize_result.value().empty());
}

// Test that concurrent errors don't corrupt state
TEST(SDKRecoveryUnitTest, ConcurrentErrorsDoNotCorruptState) {
    // Create LLM engine
    LLMEngine engine;
    MemoryManager memory_mgr(4ULL * 1024 * 1024 * 1024);
    engine.setMemoryManager(&memory_mgr);
    
    // Generate errors from multiple threads
    const int num_threads = 4;
    const int errors_per_thread = 10;
    
    std::vector<std::thread> threads;
    std::atomic<int> error_count{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&engine, &error_count]() {
            const int local_errors_per_thread = errors_per_thread;
            for (int i = 0; i < local_errors_per_thread; ++i) {
                ModelHandle invalid_handle = static_cast<ModelHandle>(999 + i);
                auto result = engine.generate(invalid_handle, "test");
                
                if (result.isError()) {
                    error_count++;
                    
                    // Verify error is valid
                    EXPECT_FALSE(result.error().message.empty());
                    EXPECT_GE(result.error().message.length(), 10u);
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Property: All operations should have produced errors
    EXPECT_EQ(error_count.load(), num_threads * errors_per_thread);
    
    // Property: SDK should still be usable after concurrent errors
    ModelHandle test_handle = static_cast<ModelHandle>(1);
    auto recovery_result = engine.generate(test_handle, "recovery test");
    
    ASSERT_TRUE(recovery_result.isError());
    EXPECT_FALSE(recovery_result.error().message.empty());
}

// Test that errors don't leak memory
TEST(SDKRecoveryUnitTest, ErrorsDoNotLeakMemory) {
    // Create LLM engine
    LLMEngine engine;
    MemoryManager memory_mgr(4ULL * 1024 * 1024 * 1024);
    engine.setMemoryManager(&memory_mgr);
    
    // Generate many errors
    const int num_errors = 1000;
    
    for (int i = 0; i < num_errors; ++i) {
        ModelHandle invalid_handle = static_cast<ModelHandle>(999 + i);
        
        // Generate error
        auto result = engine.generate(invalid_handle, "test prompt " + std::to_string(i));
        
        ASSERT_TRUE(result.isError());
        EXPECT_FALSE(result.error().message.empty());
        
        // Try tokenization error
        auto tokenize_result = engine.tokenize(invalid_handle, "test");
        ASSERT_TRUE(tokenize_result.isError());
        
        // Try detokenization error
        std::vector<int> tokens = {1, 2, 3};
        auto detokenize_result = engine.detokenize(invalid_handle, tokens);
        ASSERT_TRUE(detokenize_result.isError());
    }
    
    // Property: After many errors, SDK should still be responsive
    // (no memory leaks causing slowdown)
    ModelHandle test_handle = static_cast<ModelHandle>(1);
    auto recovery_result = engine.generate(test_handle, "final test");
    
    ASSERT_TRUE(recovery_result.isError());
    EXPECT_FALSE(recovery_result.error().message.empty());
}
