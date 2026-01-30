#include "ondeviceai/tts_engine.hpp"
#include "ondeviceai/logger.hpp"
#include <iostream>
#include <fstream>

using namespace ondeviceai;

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "  TTS Engine Complete Synthesis Test" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    TTSEngine engine;
    
    // Create a dummy model file (will work in fallback mode)
    std::string model_path = "test_tts_model_complete.onnx";
    {
        std::ofstream file(model_path);
        file << "dummy model";
    }
    
    std::cout << "\n1. Loading model..." << std::endl;
    auto load_result = engine.loadModel(model_path);
    
    if (load_result.isSuccess()) {
        ModelHandle handle = load_result.value();
        std::cout << "✓ Model loaded with handle: " << handle << std::endl;
        
        // Test 1: Basic synthesis
        std::cout << "\n2. Testing basic synthesis..." << std::endl;
        auto synth_result = engine.synthesize(handle, "Hello, world!");
        if (synth_result.isSuccess()) {
            auto& audio = synth_result.value();
            std::cout << "✓ Synthesis succeeded" << std::endl;
            std::cout << "  Sample rate: " << audio.sample_rate << " Hz" << std::endl;
            std::cout << "  Samples: " << audio.samples.size() << std::endl;
            
            // Test 2: Convert to WAV
            std::cout << "\n3. Converting to WAV format..." << std::endl;
            auto wav_result = audio.toWAV(16);
            if (wav_result.isSuccess()) {
                std::cout << "✓ WAV conversion succeeded" << std::endl;
                std::cout << "  WAV size: " << wav_result.value().size() << " bytes" << std::endl;
                
                // Save to file
                std::ofstream wav_file("test_output.wav", std::ios::binary);
                wav_file.write(reinterpret_cast<const char*>(wav_result.value().data()), 
                              wav_result.value().size());
                wav_file.close();
                std::cout << "✓ WAV file saved as test_output.wav" << std::endl;
            } else {
                std::cout << "✗ WAV conversion failed: " << wav_result.error().message << std::endl;
            }
        } else {
            std::cout << "✗ Synthesis failed: " << synth_result.error().message << std::endl;
        }
        
        // Test 3: Synthesis with speed modification
        std::cout << "\n4. Testing synthesis with speed = 1.5..." << std::endl;
        SynthesisConfig config_speed;
        config_speed.speed = 1.5f;
        auto synth_speed = engine.synthesize(handle, "Testing speed modification", config_speed);
        if (synth_speed.isSuccess()) {
            std::cout << "✓ Speed modification succeeded" << std::endl;
            std::cout << "  Samples: " << synth_speed.value().samples.size() << std::endl;
        } else {
            std::cout << "✗ Speed modification failed: " << synth_speed.error().message << std::endl;
        }
        
        // Test 4: Synthesis with pitch modification
        std::cout << "\n5. Testing synthesis with pitch = 0.5..." << std::endl;
        SynthesisConfig config_pitch;
        config_pitch.pitch = 0.5f;
        auto synth_pitch = engine.synthesize(handle, "Testing pitch modification", config_pitch);
        if (synth_pitch.isSuccess()) {
            std::cout << "✓ Pitch modification succeeded" << std::endl;
            std::cout << "  Samples: " << synth_pitch.value().samples.size() << std::endl;
        } else {
            std::cout << "✗ Pitch modification failed: " << synth_pitch.error().message << std::endl;
        }
        
        // Test 5: Streaming synthesis
        std::cout << "\n6. Testing streaming synthesis..." << std::endl;
        bool callback_invoked = false;
        size_t total_samples = 0;
        
        auto callback = [&](const AudioData& chunk) {
            callback_invoked = true;
            total_samples += chunk.samples.size();
            std::cout << "  Received chunk: " << chunk.samples.size() << " samples" << std::endl;
        };
        
        auto stream_result = engine.synthesizeStreaming(handle, "Streaming test", callback);
        if (stream_result.isSuccess()) {
            std::cout << "✓ Streaming synthesis succeeded" << std::endl;
            std::cout << "  Callback invoked: " << (callback_invoked ? "yes" : "no") << std::endl;
            std::cout << "  Total samples: " << total_samples << std::endl;
        } else {
            std::cout << "✗ Streaming synthesis failed: " << stream_result.error().message << std::endl;
        }
        
        // Test 6: Get available voices
        std::cout << "\n7. Getting available voices..." << std::endl;
        auto voices_result = engine.getAvailableVoices(handle);
        if (voices_result.isSuccess()) {
            std::cout << "✓ Retrieved " << voices_result.value().size() << " voice(s)" << std::endl;
            for (const auto& voice : voices_result.value()) {
                std::cout << "  - " << voice.name << " (" << voice.id << ")" << std::endl;
                std::cout << "    Language: " << voice.language << std::endl;
                std::cout << "    Gender: " << voice.gender << std::endl;
            }
        } else {
            std::cout << "✗ Failed to get voices: " << voices_result.error().message << std::endl;
        }
        
        // Test 7: Unload model
        std::cout << "\n8. Unloading model..." << std::endl;
        auto unload_result = engine.unloadModel(handle);
        if (unload_result.isSuccess()) {
            std::cout << "✓ Model unloaded successfully" << std::endl;
        } else {
            std::cout << "✗ Failed to unload model: " << unload_result.error().message << std::endl;
        }
        
    } else {
        std::cout << "✗ Failed to load model: " << load_result.error().message << std::endl;
    }
    
    // Clean up
    std::remove(model_path.c_str());
    
    std::cout << "\n==================================================" << std::endl;
    std::cout << "  Test completed!" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    return 0;
}
