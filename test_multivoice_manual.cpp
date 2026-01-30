#include "core/include/ondeviceai/tts_engine.hpp"
#include <iostream>
#include <fstream>

using namespace ondeviceai;

int main() {
    std::cout << "=== Testing TTS Multi-Voice Support ===" << std::endl;
    
    // Create TTS engine
    TTSEngine engine;
    
    // Create a dummy model file
    std::string model_path = "test_model.onnx";
    {
        std::ofstream file(model_path);
        file << "dummy model";
    }
    
    // Load model
    std::cout << "\n1. Loading model..." << std::endl;
    auto load_result = engine.loadModel(model_path);
    if (load_result.isError()) {
        std::cerr << "Failed to load model: " << load_result.error().message << std::endl;
        std::remove(model_path.c_str());
        return 1;
    }
    std::cout << "   Model loaded successfully with handle: " << load_result.value() << std::endl;
    
    ModelHandle handle = load_result.value();
    
    // Get available voices
    std::cout << "\n2. Getting available voices..." << std::endl;
    auto voices_result = engine.getAvailableVoices(handle);
    if (voices_result.isError()) {
        std::cerr << "Failed to get voices: " << voices_result.error().message << std::endl;
        std::remove(model_path.c_str());
        return 1;
    }
    
    auto& voices = voices_result.value();
    std::cout << "   Found " << voices.size() << " voice(s):" << std::endl;
    for (const auto& voice : voices) {
        std::cout << "     - " << voice.name << " (" << voice.id << ")" << std::endl;
        std::cout << "       Language: " << voice.language << ", Gender: " << voice.gender << std::endl;
    }
    
    // Test synthesis with default voice
    std::cout << "\n3. Testing synthesis with default voice..." << std::endl;
    SynthesisConfig config1 = SynthesisConfig::defaults();
    config1.voice_id = "";  // Use default
    
    auto synth1 = engine.synthesize(handle, "Hello world", config1);
    if (synth1.isError()) {
        std::cerr << "Synthesis failed: " << synth1.error().message << std::endl;
    } else {
        std::cout << "   Synthesis successful: " << synth1.value().samples.size() 
                  << " samples at " << synth1.value().sample_rate << " Hz" << std::endl;
    }
    
    // Test synthesis with specific voice
    if (!voices.empty()) {
        std::cout << "\n4. Testing synthesis with specific voice (" << voices[0].name << ")..." << std::endl;
        SynthesisConfig config2 = SynthesisConfig::defaults();
        config2.voice_id = voices[0].id;
        
        auto synth2 = engine.synthesize(handle, "Testing specific voice", config2);
        if (synth2.isError()) {
            std::cerr << "Synthesis failed: " << synth2.error().message << std::endl;
        } else {
            std::cout << "   Synthesis successful: " << synth2.value().samples.size() 
                      << " samples" << std::endl;
        }
    }
    
    // Test synthesis with invalid voice
    std::cout << "\n5. Testing synthesis with invalid voice (should fail)..." << std::endl;
    SynthesisConfig config3 = SynthesisConfig::defaults();
    config3.voice_id = "invalid-voice-id";
    
    auto synth3 = engine.synthesize(handle, "This should fail", config3);
    if (synth3.isError()) {
        std::cout << "   Expected error occurred: " << synth3.error().message << std::endl;
        std::cout << "   Details: " << synth3.error().details << std::endl;
    } else {
        std::cerr << "   ERROR: Should have failed with invalid voice!" << std::endl;
    }
    
    // Test synthesis with different language voices
    if (voices.size() > 1) {
        std::cout << "\n6. Testing synthesis with different language voices..." << std::endl;
        
        // Find two voices with different languages
        for (size_t i = 0; i < voices.size(); ++i) {
            for (size_t j = i + 1; j < voices.size(); ++j) {
                if (voices[i].language != voices[j].language) {
                    std::cout << "   Testing " << voices[i].name << " (" << voices[i].language << ")..." << std::endl;
                    SynthesisConfig config_a = SynthesisConfig::defaults();
                    config_a.voice_id = voices[i].id;
                    auto synth_a = engine.synthesize(handle, "Test", config_a);
                    
                    std::cout << "   Testing " << voices[j].name << " (" << voices[j].language << ")..." << std::endl;
                    SynthesisConfig config_b = SynthesisConfig::defaults();
                    config_b.voice_id = voices[j].id;
                    auto synth_b = engine.synthesize(handle, "Test", config_b);
                    
                    if (synth_a.isSuccess() && synth_b.isSuccess()) {
                        std::cout << "   Both voices synthesized successfully!" << std::endl;
                    }
                    
                    goto done_multi_lang_test;
                }
            }
        }
        done_multi_lang_test:;
    }
    
    // Unload model
    std::cout << "\n7. Unloading model..." << std::endl;
    auto unload_result = engine.unloadModel(handle);
    if (unload_result.isError()) {
        std::cerr << "Failed to unload model: " << unload_result.error().message << std::endl;
    } else {
        std::cout << "   Model unloaded successfully" << std::endl;
    }
    
    // Clean up
    std::remove(model_path.c_str());
    
    std::cout << "\n=== All tests completed ===" << std::endl;
    return 0;
}
