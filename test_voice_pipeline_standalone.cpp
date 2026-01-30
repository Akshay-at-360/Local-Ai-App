#include "core/include/ondeviceai/voice_pipeline.hpp"
#include "core/include/ondeviceai/stt_engine.hpp"
#include "core/include/ondeviceai/llm_engine.hpp"
#include "core/include/ondeviceai/tts_engine.hpp"
#include <iostream>
#include <cassert>

using namespace ondeviceai;

// Simple test to verify Voice Pipeline implementation
int main() {
    std::cout << "Testing Voice Pipeline Implementation..." << std::endl;
    
    // Create engine instances
    STTEngine stt_engine;
    LLMEngine llm_engine;
    TTSEngine tts_engine;
    
    // Create voice pipeline
    VoicePipeline pipeline(&stt_engine, &llm_engine, &tts_engine);
    std::cout << "✓ VoicePipeline created successfully" << std::endl;
    
    // Test 1: Configure with valid parameters
    {
        PipelineConfig config = PipelineConfig::defaults();
        auto result = pipeline.configure(1, 2, 3, config);
        assert(result.isSuccess());
        std::cout << "✓ Configure with valid parameters succeeded" << std::endl;
    }
    
    // Test 2: Configure with invalid model handles
    {
        PipelineConfig config = PipelineConfig::defaults();
        auto result = pipeline.configure(0, 2, 3, config);
        assert(result.isError());
        assert(result.error().code == ErrorCode::InvalidInputModelHandle);
        std::cout << "✓ Configure with invalid model handle failed as expected" << std::endl;
    }
    
    // Test 3: Configure with invalid VAD threshold
    {
        PipelineConfig config = PipelineConfig::defaults();
        config.vad_threshold = 1.5f; // Invalid: > 1.0
        auto result = pipeline.configure(1, 2, 3, config);
        assert(result.isError());
        assert(result.error().code == ErrorCode::InvalidInputParameterValue);
        std::cout << "✓ Configure with invalid VAD threshold failed as expected" << std::endl;
    }
    
    // Test 4: Start conversation without configuration
    {
        VoicePipeline pipeline2(&stt_engine, &llm_engine, &tts_engine);
        auto result = pipeline2.startConversation(
            []() { return AudioData(); },
            [](const AudioData&) {},
            nullptr,
            nullptr
        );
        assert(result.isError());
        assert(result.error().code == ErrorCode::InvalidInputConfiguration);
        std::cout << "✓ Start conversation without configuration failed as expected" << std::endl;
    }
    
    // Test 5: Start conversation with null callbacks
    {
        PipelineConfig config = PipelineConfig::defaults();
        pipeline.configure(1, 2, 3, config);
        
        auto result = pipeline.startConversation(
            nullptr,  // null audio input
            nullptr,  // null audio output
            nullptr,
            nullptr
        );
        assert(result.isError());
        assert(result.error().code == ErrorCode::InvalidInputNullPointer);
        std::cout << "✓ Start conversation with null callbacks failed as expected" << std::endl;
    }
    
    // Test 6: Clear history
    {
        auto result = pipeline.clearHistory();
        assert(result.isSuccess());
        std::cout << "✓ Clear history succeeded" << std::endl;
    }
    
    // Test 7: Get history (should be empty)
    {
        auto result = pipeline.getHistory();
        assert(result.isSuccess());
        assert(result.value().empty());
        std::cout << "✓ Get history returned empty list" << std::endl;
    }
    
    // Test 8: Stop conversation (when not active)
    {
        auto result = pipeline.stopConversation();
        assert(result.isSuccess());
        std::cout << "✓ Stop conversation (not active) succeeded" << std::endl;
    }
    
    // Test 9: Interrupt (when not active)
    {
        auto result = pipeline.interrupt();
        assert(result.isSuccess());
        std::cout << "✓ Interrupt (not active) succeeded" << std::endl;
    }
    
    // Test 10: Configure while conversation is active
    {
        // This test would require actually starting a conversation,
        // which needs real models. Skipping for now.
        std::cout << "⊘ Skipping test: Configure while active (requires real models)" << std::endl;
    }
    
    std::cout << "\n✅ All Voice Pipeline tests passed!" << std::endl;
    return 0;
}
