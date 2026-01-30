#include <gtest/gtest.h>
#include "ondeviceai/sha256.hpp"
#include <fstream>
#include <cstdio>

using namespace ondeviceai::crypto;

// Test basic SHA-256 hashing with known test vectors
TEST(SHA256Test, KnownTestVectors) {
    // Test vector 1: Empty string
    {
        auto hash = SHA256::hash("");
        std::string hex = SHA256::toHex(hash);
        EXPECT_EQ(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    }
    
    // Test vector 2: "abc"
    {
        auto hash = SHA256::hash("abc");
        std::string hex = SHA256::toHex(hash);
        EXPECT_EQ(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    }
    
    // Test vector 3: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
    {
        auto hash = SHA256::hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
        std::string hex = SHA256::toHex(hash);
        EXPECT_EQ(hex, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    }
    
    // Test vector 4: "The quick brown fox jumps over the lazy dog"
    {
        auto hash = SHA256::hash("The quick brown fox jumps over the lazy dog");
        std::string hex = SHA256::toHex(hash);
        EXPECT_EQ(hex, "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
    }
}

// Test incremental hashing (update multiple times)
TEST(SHA256Test, IncrementalHashing) {
    SHA256 hasher;
    hasher.update("The quick brown fox ");
    hasher.update("jumps over ");
    hasher.update("the lazy dog");
    
    auto hash = hasher.finalize();
    std::string hex = SHA256::toHex(hash);
    
    EXPECT_EQ(hex, "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
}

// Test that incremental and single-call hashing produce same result
TEST(SHA256Test, IncrementalVsSingleCall) {
    std::string data = "This is a longer test string that we'll hash in different ways";
    
    // Single call
    auto hash1 = SHA256::hash(data);
    
    // Incremental
    SHA256 hasher;
    hasher.update(data.substr(0, 20));
    hasher.update(data.substr(20, 20));
    hasher.update(data.substr(40));
    auto hash2 = hasher.finalize();
    
    EXPECT_EQ(hash1, hash2);
}

// Test hashing binary data
TEST(SHA256Test, BinaryData) {
    uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    
    auto hash = SHA256::hash(data, sizeof(data));
    std::string hex = SHA256::toHex(hash);
    
    // This is the expected SHA-256 of the above bytes
    EXPECT_EQ(hex, "be45cb2605bf36bebde684841a28f0fd43c69850a3dce5fedba69928ee3a8991");
}

// Test file hashing
TEST(SHA256Test, FileHashing) {
    // Create a temporary test file
    const char* test_file = "/tmp/sha256_test_file.txt";
    std::string test_content = "Test file content for SHA-256 hashing";
    {
        std::ofstream file(test_file);
        file << test_content;
        file.close();
    }
    
    // Hash the file
    std::string file_hash = SHA256::hashFile(test_file);
    
    // Also hash the content directly for comparison
    auto content_hash = SHA256::hash(test_content);
    std::string content_hash_hex = SHA256::toHex(content_hash);
    
    // They should match
    EXPECT_EQ(file_hash, content_hash_hex);
    EXPECT_FALSE(file_hash.empty());
    EXPECT_EQ(file_hash.length(), 64); // SHA-256 produces 64 hex characters
    
    // Clean up
    std::remove(test_file);
}

// Test file hashing with larger file
TEST(SHA256Test, LargeFileHashing) {
    // Create a temporary test file with more data
    const char* test_file = "/tmp/sha256_large_test_file.txt";
    {
        std::ofstream file(test_file);
        // Write 10KB of data
        for (int i = 0; i < 1000; ++i) {
            file << "0123456789";
        }
        file.close();
    }
    
    // Hash the file
    std::string hash = SHA256::hashFile(test_file);
    
    // Verify hash is not empty (exact value depends on content)
    EXPECT_FALSE(hash.empty());
    EXPECT_EQ(hash.length(), 64); // SHA-256 produces 64 hex characters
    
    // Clean up
    std::remove(test_file);
}

// Test file hashing with non-existent file
TEST(SHA256Test, NonExistentFile) {
    std::string hash = SHA256::hashFile("/tmp/this_file_does_not_exist_12345.txt");
    EXPECT_TRUE(hash.empty());
}

// Test hex conversion
TEST(SHA256Test, HexConversion) {
    std::array<uint8_t, 32> hash;
    for (int i = 0; i < 32; ++i) {
        hash[i] = static_cast<uint8_t>(i);
    }
    
    std::string hex = SHA256::toHex(hash);
    EXPECT_EQ(hex, "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
}

// Test that finalize can only be called once
TEST(SHA256Test, FinalizeOnce) {
    SHA256 hasher;
    hasher.update("test");
    
    auto hash1 = hasher.finalize();
    auto hash2 = hasher.finalize(); // Should return same hash
    
    EXPECT_EQ(hash1, hash2);
}

// Test empty data hashing
TEST(SHA256Test, EmptyData) {
    auto hash = SHA256::hash(reinterpret_cast<const uint8_t*>(""), 0);
    std::string hex = SHA256::toHex(hash);
    EXPECT_EQ(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

// Test long string (> 64 bytes, requires multiple blocks)
TEST(SHA256Test, LongString) {
    std::string long_string(1000, 'a'); // 1000 'a' characters
    
    auto hash = SHA256::hash(long_string);
    std::string hex = SHA256::toHex(hash);
    
    // This is the expected SHA-256 of 1000 'a' characters
    EXPECT_EQ(hex, "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");
}
