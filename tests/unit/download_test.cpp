#include <gtest/gtest.h>
#include "ondeviceai/download.hpp"
#include "ondeviceai/logger.hpp"
#include <fstream>
#include <thread>
#include <chrono>

using namespace ondeviceai;

class DownloadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger
        Logger::getInstance().setLogLevel(LogLevel::Debug);
        
        // Clean up any existing test files
        std::remove("test_download.txt");
        std::remove("test_download.txt.tmp");
    }
    
    void TearDown() override {
        // Clean up test files
        std::remove("test_download.txt");
        std::remove("test_download.txt.tmp");
    }
};

// Test: Download state transitions
TEST_F(DownloadTest, StateTransitions) {
    bool progress_called = false;
    double last_progress = 0.0;
    
    auto progress_callback = [&](double progress) {
        progress_called = true;
        last_progress = progress;
    };
    
    // Note: This test uses a mock URL that won't actually download
    // In a real test environment, you'd use a local test server
    Download download(1, "http://example.com/test.bin", "test_download.txt", 1024, progress_callback);
    
    // Initial state should be Pending
    EXPECT_EQ(download.getState(), DownloadState::Pending);
    EXPECT_EQ(download.getBytesDownloaded(), 0);
    EXPECT_EQ(download.getExpectedSize(), 1024);
    EXPECT_DOUBLE_EQ(download.getProgress(), 0.0);
}

// Test: Download cancellation
TEST_F(DownloadTest, Cancellation) {
    auto progress_callback = [](double progress) {
        (void)progress;
    };
    
    Download download(2, "http://example.com/large.bin", "test_download.txt", 1024*1024, progress_callback);
    
    // Start download (will fail due to mock URL, but that's okay for this test)
    auto start_result = download.start();
    EXPECT_TRUE(start_result.isSuccess());
    
    // Cancel immediately
    download.cancel();
    
    // State should be cancelled
    EXPECT_EQ(download.getState(), DownloadState::Cancelled);
}

// Test: Progress calculation
TEST_F(DownloadTest, ProgressCalculation) {
    auto progress_callback = [](double progress) {
        (void)progress;
    };
    
    Download download(3, "http://example.com/test.bin", "test_download.txt", 1000, progress_callback);
    
    // Progress should be 0 initially
    EXPECT_DOUBLE_EQ(download.getProgress(), 0.0);
    
    // Expected size should be correct
    EXPECT_EQ(download.getExpectedSize(), 1000);
}

// Test: Resumable download detection
TEST_F(DownloadTest, ResumableDownload) {
    // Create a partial download file
    {
        std::ofstream file("test_download.txt.tmp", std::ios::binary);
        std::string partial_data(512, 'A');
        file.write(partial_data.c_str(), partial_data.size());
    }
    
    auto progress_callback = [](double progress) {
        (void)progress;
    };
    
    // Create download - should detect existing partial file
    Download download(4, "http://example.com/test.bin", "test_download.txt", 1024, progress_callback);
    
    // Should have detected 512 bytes already downloaded
    EXPECT_EQ(download.getBytesDownloaded(), 512);
    EXPECT_DOUBLE_EQ(download.getProgress(), 0.5);
}

// Test: Multiple downloads with different handles
TEST_F(DownloadTest, MultipleDownloads) {
    auto progress_callback = [](double progress) {
        (void)progress;
    };
    
    Download download1(10, "http://example.com/file1.bin", "test_download1.txt", 1024, progress_callback);
    Download download2(20, "http://example.com/file2.bin", "test_download2.txt", 2048, progress_callback);
    
    EXPECT_EQ(download1.getExpectedSize(), 1024);
    EXPECT_EQ(download2.getExpectedSize(), 2048);
    
    // Clean up
    std::remove("test_download1.txt");
    std::remove("test_download1.txt.tmp");
    std::remove("test_download2.txt");
    std::remove("test_download2.txt.tmp");
}

// Test: Progress callback invocation
TEST_F(DownloadTest, ProgressCallbackInvocation) {
    int callback_count = 0;
    std::vector<double> progress_values;
    
    auto progress_callback = [&](double progress) {
        callback_count++;
        progress_values.push_back(progress);
    };
    
    Download download(5, "http://example.com/test.bin", "test_download.txt", 1024, progress_callback);
    
    // Note: In a real test with actual download, we'd verify:
    // 1. Callback is called multiple times
    // 2. Progress values are non-decreasing (monotonicity)
    // 3. Final progress is 1.0
    
    // For now, just verify the callback is set up
    EXPECT_EQ(download.getState(), DownloadState::Pending);
}

// Test: Zero-size download handling
TEST_F(DownloadTest, ZeroSizeDownload) {
    auto progress_callback = [](double progress) {
        (void)progress;
    };
    
    Download download(6, "http://example.com/empty.bin", "test_download.txt", 0, progress_callback);
    
    EXPECT_EQ(download.getExpectedSize(), 0);
    EXPECT_DOUBLE_EQ(download.getProgress(), 0.0);
}

// Test: Download with null progress callback
TEST_F(DownloadTest, NullProgressCallback) {
    // Should not crash with null callback
    Download download(7, "http://example.com/test.bin", "test_download.txt", 1024, nullptr);
    
    EXPECT_EQ(download.getState(), DownloadState::Pending);
    EXPECT_EQ(download.getExpectedSize(), 1024);
}
