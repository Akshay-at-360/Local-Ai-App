#include <gtest/gtest.h>
#include "ondeviceai/types.hpp"
#include <cmath>
#include <fstream>

using namespace ondeviceai;

// Helper function to create a simple WAV file in memory
std::vector<uint8_t> createSimpleWAV(int sample_rate, int num_samples, int bits_per_sample = 16) {
    std::vector<uint8_t> wav_data;
    
    // RIFF header
    wav_data.push_back('R');
    wav_data.push_back('I');
    wav_data.push_back('F');
    wav_data.push_back('F');
    
    // File size - 8 (will be filled later)
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
    
    // Byte rate (sample_rate * num_channels * bits_per_sample / 8)
    uint32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
    wav_data.push_back(byte_rate & 0xFF);
    wav_data.push_back((byte_rate >> 8) & 0xFF);
    wav_data.push_back((byte_rate >> 16) & 0xFF);
    wav_data.push_back((byte_rate >> 24) & 0xFF);
    
    // Block align (num_channels * bits_per_sample / 8)
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
        } else if (bits_per_sample == 8) {
            uint8_t pcm_sample = static_cast<uint8_t>((sample + 1.0f) * 127.5f);
            wav_data.push_back(pcm_sample);
        }
    }
    
    return wav_data;
}

// Test WAV parsing with valid 16-bit PCM
TEST(AudioPreprocessingTest, ParseWAV16BitPCM) {
    auto wav_data = createSimpleWAV(16000, 1000, 16);
    
    auto result = AudioData::fromWAV(wav_data);
    ASSERT_TRUE(result.isSuccess()) << "Failed to parse WAV: " << result.error().message;
    
    const AudioData& audio = result.value();
    
    EXPECT_EQ(audio.sample_rate, 16000);
    EXPECT_EQ(audio.samples.size(), 1000);
    
    // Verify samples are normalized to [-1.0, 1.0]
    for (float sample : audio.samples) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

// Test WAV parsing with 8-bit PCM
TEST(AudioPreprocessingTest, ParseWAV8BitPCM) {
    auto wav_data = createSimpleWAV(8000, 500, 8);
    
    auto result = AudioData::fromWAV(wav_data);
    ASSERT_TRUE(result.isSuccess()) << "Failed to parse WAV: " << result.error().message;
    
    const AudioData& audio = result.value();
    
    EXPECT_EQ(audio.sample_rate, 8000);
    EXPECT_EQ(audio.samples.size(), 500);
    
    // Verify samples are normalized
    for (float sample : audio.samples) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

// Test WAV parsing with invalid data
TEST(AudioPreprocessingTest, ParseInvalidWAV) {
    std::vector<uint8_t> invalid_data = {0x00, 0x01, 0x02, 0x03};
    
    auto result = AudioData::fromWAV(invalid_data);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputAudioFormat);
}

// Test WAV parsing with too small data
TEST(AudioPreprocessingTest, ParseTooSmallWAV) {
    std::vector<uint8_t> small_data(40, 0);  // Less than 44 bytes
    
    auto result = AudioData::fromWAV(small_data);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputAudioFormat);
}

// Test audio resampling - upsampling
TEST(AudioPreprocessingTest, ResampleUpsampling) {
    AudioData audio;
    audio.sample_rate = 8000;
    audio.samples.resize(800);  // 0.1 seconds
    
    // Fill with a simple pattern
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        audio.samples[i] = std::sin(2.0f * 3.14159f * 440.0f * i / audio.sample_rate);
    }
    
    // Resample to 16000 Hz
    auto result = audio.resample(16000);
    ASSERT_TRUE(result.isSuccess()) << "Resampling failed: " << result.error().message;
    
    const AudioData& resampled = result.value();
    
    EXPECT_EQ(resampled.sample_rate, 16000);
    EXPECT_NEAR(resampled.samples.size(), 1600, 10);  // Should be approximately 2x
    
    // Verify samples are still in valid range
    for (float sample : resampled.samples) {
        EXPECT_GE(sample, -1.5f);  // Allow some margin for interpolation
        EXPECT_LE(sample, 1.5f);
    }
}

// Test audio resampling - downsampling
TEST(AudioPreprocessingTest, ResampleDownsampling) {
    AudioData audio;
    audio.sample_rate = 48000;
    audio.samples.resize(4800);  // 0.1 seconds
    
    // Fill with a simple pattern
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        audio.samples[i] = std::sin(2.0f * 3.14159f * 440.0f * i / audio.sample_rate);
    }
    
    // Resample to 16000 Hz
    auto result = audio.resample(16000);
    ASSERT_TRUE(result.isSuccess()) << "Resampling failed: " << result.error().message;
    
    const AudioData& resampled = result.value();
    
    EXPECT_EQ(resampled.sample_rate, 16000);
    EXPECT_NEAR(resampled.samples.size(), 1600, 10);  // Should be approximately 1/3
}

// Test resampling with same sample rate
TEST(AudioPreprocessingTest, ResampleSameRate) {
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
    
    auto result = audio.resample(16000);
    ASSERT_TRUE(result.isSuccess());
    
    const AudioData& resampled = result.value();
    
    EXPECT_EQ(resampled.sample_rate, 16000);
    EXPECT_EQ(resampled.samples.size(), audio.samples.size());
    
    // Should be identical
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        EXPECT_FLOAT_EQ(resampled.samples[i], audio.samples[i]);
    }
}

// Test resampling with invalid sample rate
TEST(AudioPreprocessingTest, ResampleInvalidRate) {
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {0.1f, 0.2f, 0.3f};
    
    auto result = audio.resample(-1000);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputParameterValue);
}

// Test resampling empty audio
TEST(AudioPreprocessingTest, ResampleEmptyAudio) {
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {};
    
    auto result = audio.resample(8000);
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputAudioFormat);
}

// Test audio normalization
TEST(AudioPreprocessingTest, NormalizeAudio) {
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {0.1f, 0.5f, 1.5f, -2.0f, 0.3f};  // Max abs = 2.0
    
    auto result = audio.normalize();
    ASSERT_TRUE(result.isSuccess()) << "Normalization failed: " << result.error().message;
    
    const AudioData& normalized = result.value();
    
    EXPECT_EQ(normalized.sample_rate, 16000);
    EXPECT_EQ(normalized.samples.size(), audio.samples.size());
    
    // Find max absolute value
    float max_abs = 0.0f;
    for (float sample : normalized.samples) {
        max_abs = std::max(max_abs, std::abs(sample));
    }
    
    // Should be normalized to 1.0
    EXPECT_NEAR(max_abs, 1.0f, 0.01f);
    
    // All samples should be in [-1.0, 1.0]
    for (float sample : normalized.samples) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

// Test normalization of already normalized audio
TEST(AudioPreprocessingTest, NormalizeAlreadyNormalized) {
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {0.1f, 0.5f, -0.8f, 0.3f};  // Max abs = 0.8
    
    auto result = audio.normalize();
    ASSERT_TRUE(result.isSuccess());
    
    const AudioData& normalized = result.value();
    
    // Should be unchanged (already normalized)
    EXPECT_EQ(normalized.samples.size(), audio.samples.size());
    for (size_t i = 0; i < audio.samples.size(); ++i) {
        EXPECT_FLOAT_EQ(normalized.samples[i], audio.samples[i]);
    }
}

// Test normalization of silent audio
TEST(AudioPreprocessingTest, NormalizeSilentAudio) {
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {0.0f, 0.0f, 0.0f, 0.0f};
    
    auto result = audio.normalize();
    ASSERT_TRUE(result.isSuccess());
    
    const AudioData& normalized = result.value();
    
    // Should remain silent
    for (float sample : normalized.samples) {
        EXPECT_FLOAT_EQ(sample, 0.0f);
    }
}

// Test normalization of empty audio
TEST(AudioPreprocessingTest, NormalizeEmptyAudio) {
    AudioData audio;
    audio.sample_rate = 16000;
    audio.samples = {};
    
    auto result = audio.normalize();
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputAudioFormat);
}

// Test combined preprocessing: WAV -> resample -> normalize
TEST(AudioPreprocessingTest, CombinedPreprocessing) {
    // Create a WAV file at 8000 Hz
    auto wav_data = createSimpleWAV(8000, 800, 16);
    
    // Parse WAV
    auto parse_result = AudioData::fromWAV(wav_data);
    ASSERT_TRUE(parse_result.isSuccess());
    
    AudioData audio = parse_result.value();
    
    // Resample to 16000 Hz
    auto resample_result = audio.resample(16000);
    ASSERT_TRUE(resample_result.isSuccess());
    
    audio = resample_result.value();
    EXPECT_EQ(audio.sample_rate, 16000);
    
    // Normalize
    auto normalize_result = audio.normalize();
    ASSERT_TRUE(normalize_result.isSuccess());
    
    audio = normalize_result.value();
    
    // Verify final audio is properly preprocessed
    EXPECT_EQ(audio.sample_rate, 16000);
    EXPECT_GT(audio.samples.size(), 0);
    
    // All samples should be in [-1.0, 1.0]
    for (float sample : audio.samples) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

// Test fromFile with non-existent file
TEST(AudioPreprocessingTest, FromFileNonExistent) {
    auto result = AudioData::fromFile("/nonexistent/audio.wav");
    ASSERT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::StorageReadError);
}

// Test fromFile with valid WAV file
TEST(AudioPreprocessingTest, FromFileValidWAV) {
    // Create a temporary WAV file
    std::string temp_file = "test_audio_temp.wav";
    auto wav_data = createSimpleWAV(16000, 1000, 16);
    
    std::ofstream file(temp_file, std::ios::binary);
    file.write(reinterpret_cast<const char*>(wav_data.data()), wav_data.size());
    file.close();
    
    // Load from file
    auto result = AudioData::fromFile(temp_file);
    ASSERT_TRUE(result.isSuccess()) << "Failed to load from file: " << result.error().message;
    
    const AudioData& audio = result.value();
    
    EXPECT_EQ(audio.sample_rate, 16000);
    EXPECT_EQ(audio.samples.size(), 1000);
    
    // Clean up
    std::remove(temp_file.c_str());
}
