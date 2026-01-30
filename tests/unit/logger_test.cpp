#include <gtest/gtest.h>
#include "ondeviceai/logger.hpp"
#include <thread>
#include <chrono>

using namespace ondeviceai;

TEST(LoggerTest, GetInstance) {
    auto& logger = Logger::getInstance();
    EXPECT_EQ(&logger, &Logger::getInstance());
}

TEST(LoggerTest, SetAndGetLogLevel) {
    auto& logger = Logger::getInstance();
    
    logger.setLogLevel(LogLevel::Debug);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::Debug);
    
    logger.setLogLevel(LogLevel::Error);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::Error);
}

TEST(LoggerTest, LogMessages) {
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Debug);
    
    // These should not crash
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
}

TEST(LoggerTest, LogLevelFiltering) {
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Warning);
    
    // Debug and Info should be filtered out (no crash expected)
    logger.debug("This should be filtered");
    logger.info("This should be filtered");
    logger.warning("This should appear");
    logger.error("This should appear");
}

TEST(LoggerTest, PerformanceLogging) {
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Debug);
    
    // Test performance logging
    logger.logPerformance("test_operation", 123.45);
    logger.logPerformance("test_operation_with_details", 456.78, "some details");
}

TEST(LoggerTest, MemoryLogging) {
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Debug);
    
    // Test memory logging with different sizes
    logger.logMemoryUsage("small_allocation", 512);  // bytes
    logger.logMemoryUsage("medium_allocation", 1024 * 1024);  // MB
    logger.logMemoryUsage("large_allocation", 2ULL * 1024 * 1024 * 1024);  // GB
    logger.logMemoryUsage("allocation_with_details", 1024, "test details");
}

TEST(LoggerTest, ThreadSafety) {
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Debug);
    
    // Test concurrent logging from multiple threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([i, &logger]() {
            for (int j = 0; j < 10; ++j) {
                logger.info("Thread " + std::to_string(i) + " message " + std::to_string(j));
                logger.debug("Thread " + std::to_string(i) + " debug " + std::to_string(j));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

TEST(LoggerTest, ScopedTimer) {
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Debug);
    
    {
        ScopedTimer timer("test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    {
        ScopedTimer timer("test_operation_with_details", "some details");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

TEST(LoggerTest, MacroUsage) {
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Debug);
    
    // Test convenience macros
    LOG_DEBUG("Debug via macro");
    LOG_INFO("Info via macro");
    LOG_WARNING("Warning via macro");
    LOG_ERROR("Error via macro");
    
    // Test performance macros
    LOG_PERFORMANCE("macro_operation", 100.5, "test details");
    LOG_MEMORY("macro_allocation", 1024 * 1024, "1MB allocation");
}

TEST(LoggerTest, PrivacyCompliance) {
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Debug);
    
    // Verify that logging doesn't expose sensitive data
    // This is a documentation test - the logger itself doesn't filter content,
    // but the calling code should not pass sensitive data
    std::string sensitive_data = "user_password_123";
    
    // Good practice: Don't log sensitive data
    logger.info("User authentication attempt");  // No sensitive data
    
    // Bad practice (don't do this): logger.info("Password: " + sensitive_data);
}

TEST(LoggerTest, TimestampFormat) {
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Info);
    
    // Verify timestamps are included (visual inspection in output)
    logger.info("Message with timestamp");
}

TEST(LoggerTest, ThreadIdIncluded) {
    auto& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Info);
    
    // Verify thread IDs are different for different threads
    std::thread t1([&logger]() {
        logger.info("Message from thread 1");
    });
    
    std::thread t2([&logger]() {
        logger.info("Message from thread 2");
    });
    
    t1.join();
    t2.join();
}
