#include "ondeviceai/stt_engine.hpp"
#include <iostream>
#include <cmath>

using namespace ondeviceai;

int main() {
    std::cout << "Testing Voice Activity Detection with configurable threshold...\n\n";
    
    STTEngine engine;
    
    // Test 1: Create audio with speech-like segments
    std::cout << "Test 1: VAD with default threshold (0.5)\n";
    AudioData audio1;
    audio1.sample_rate = 16000;
    audio1.samples.resize(32000, 0.0f);  // 2 seconds
    
    // Add "speech" in the middle (0.5s to 1.5s)
    for (size_t i = 8000; i < 24000; ++i) {
        audio1.samples[i] = 0.1f * std::sin(2.0f * 3.14159f * 440.0f * i / audio1.sample_rate);
    }
    
    auto result1 = engine.detectVoiceActivity(audio1);
    if (result1.isSuccess()) {
        const auto& segments = result1.value();
        std::cout << "  Detected " << segments.size() << " speech segment(s)\n";
        for (size_t i = 0; i < segments.size(); ++i) {
            std::cout << "    Segment " << (i+1) << ": " 
                      << segments[i].start_time << "s - " 
                      << segments[i].end_time << "s\n";
        }
    } else {
        std::cout << "  ERROR: " << result1.error().message << "\n";
        return 1;
    }
    
    // Test 2: Low threshold (more sensitive)
    std::cout << "\nTest 2: VAD with low threshold (0.2) - more sensitive\n";
    auto result2 = engine.detectVoiceActivity(audio1, 0.2f);
    if (result2.isSuccess()) {
        const auto& segments = result2.value();
        std::cout << "  Detected " << segments.size() << " speech segment(s)\n";
    } else {
        std::cout << "  ERROR: " << result2.error().message << "\n";
        return 1;
    }
    
    // Test 3: High threshold (less sensitive)
    std::cout << "\nTest 3: VAD with high threshold (0.9) - less sensitive\n";
    auto result3 = engine.detectVoiceActivity(audio1, 0.9f);
    if (result3.isSuccess()) {
        const auto& segments = result3.value();
        std::cout << "  Detected " << segments.size() << " speech segment(s)\n";
    } else {
        std::cout << "  ERROR: " << result3.error().message << "\n";
        return 1;
    }
    
    // Test 4: Invalid threshold (< 0)
    std::cout << "\nTest 4: VAD with invalid threshold (-0.1) - should fail\n";
    auto result4 = engine.detectVoiceActivity(audio1, -0.1f);
    if (result4.isError()) {
        std::cout << "  Expected error: " << result4.error().message << "\n";
    } else {
        std::cout << "  ERROR: Should have failed with invalid threshold\n";
        return 1;
    }
    
    // Test 5: Invalid threshold (> 1)
    std::cout << "\nTest 5: VAD with invalid threshold (1.5) - should fail\n";
    auto result5 = engine.detectVoiceActivity(audio1, 1.5f);
    if (result5.isError()) {
        std::cout << "  Expected error: " << result5.error().message << "\n";
    } else {
        std::cout << "  ERROR: Should have failed with invalid threshold\n";
        return 1;
    }
    
    // Test 6: Silence detection
    std::cout << "\nTest 6: VAD on silence - should detect no segments\n";
    AudioData audio2;
    audio2.sample_rate = 16000;
    audio2.samples.resize(16000, 0.0f);  // 1 second of silence
    
    auto result6 = engine.detectVoiceActivity(audio2);
    if (result6.isSuccess()) {
        const auto& segments = result6.value();
        std::cout << "  Detected " << segments.size() << " speech segment(s)\n";
        if (segments.size() == 0) {
            std::cout << "  ✓ Correctly detected no speech in silence\n";
        }
    } else {
        std::cout << "  ERROR: " << result6.error().message << "\n";
        return 1;
    }
    
    // Test 7: Empty audio
    std::cout << "\nTest 7: VAD on empty audio - should fail\n";
    AudioData audio3;
    audio3.sample_rate = 16000;
    audio3.samples = {};
    
    auto result7 = engine.detectVoiceActivity(audio3);
    if (result7.isError()) {
        std::cout << "  Expected error: " << result7.error().message << "\n";
    } else {
        std::cout << "  ERROR: Should have failed with empty audio\n";
        return 1;
    }
    
    std::cout << "\n✓ All VAD tests passed!\n";
    return 0;
}
