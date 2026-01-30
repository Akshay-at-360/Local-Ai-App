#include <gtest/gtest.h>
#include "ondeviceai/error_recovery.hpp"
#include "ondeviceai/types.hpp"
#include <atomic>

using namespace ondeviceai;
using namespace ondeviceai::ErrorRecovery;

// Test fixture for error recovery tests
class ErrorRecoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        attempt_count = 0;
    }
    
    std::atomic<int> attempt_count{0};
};

// Test: calculateBackoffDelay produces exponential delays
TEST_F(ErrorRecoveryTest, CalculateBackoffDelayExponential) {
    RetryConfig config = RetryConfig::defaults();
    
    // First attempt (0) should have no delay
    EXPECT_EQ(0, calculateBackoffDelay(0, config));
    
    // Subsequent attempts should have exponential delays
    int delay1 = calculateBackoffDelay(1, config);
    int delay2 = calculateBackoffDelay(2, config);
    int delay3 = calculateBackoffDelay(3, config);
    
    EXPECT_EQ(1000, delay1);  // 1000 * 2^0 = 1000
    EXPECT_EQ(2000, delay2);  // 1000 * 2^1 = 2000
    EXPECT_EQ(4000, delay3);  // 1000 * 2^2 = 4000
}

// Test: calculateBackoffDelay respects maximum delay
TEST_F(ErrorRecoveryTest, CalculateBackoffDelayMaxCap) {
    RetryConfig config = RetryConfig::defaults();
    config.max_delay_ms = 5000;
    
    // Large attempt number should be capped at max_delay_ms
    int delay = calculateBackoffDelay(10, config);
    EXPECT_LE(delay, config.max_delay_ms);
    EXPECT_EQ(5000, delay);
}

// Test: isRetryable correctly identifies retryable errors
TEST_F(ErrorRecoveryTest, IsRetryableNetworkErrors) {
    // Network errors should be retryable
    EXPECT_TRUE(isRetryable(Error(ErrorCode::NetworkUnreachable, "test")));
    EXPECT_TRUE(isRetryable(Error(ErrorCode::NetworkConnectionTimeout, "test")));
    EXPECT_TRUE(isRetryable(Error(ErrorCode::NetworkDNSFailure, "test")));
}

// Test: isRetryable correctly identifies retryable resource errors
TEST_F(ErrorRecoveryTest, IsRetryableResourceErrors) {
    // Resource exhaustion errors should be retryable
    EXPECT_TRUE(isRetryable(Error(ErrorCode::ResourceThreadPoolExhausted, "test")));
    EXPECT_TRUE(isRetryable(Error(ErrorCode::ResourceOutOfMemory, "test")));
    EXPECT_TRUE(isRetryable(Error(ErrorCode::ResourceGPUMemoryExhausted, "test")));
}

// Test: isRetryable correctly identifies non-retryable errors
TEST_F(ErrorRecoveryTest, IsRetryableNonRetryableErrors) {
    // Permanent errors should not be retryable
    EXPECT_FALSE(isRetryable(Error(ErrorCode::ModelNotFoundInRegistry, "test")));
    EXPECT_FALSE(isRetryable(Error(ErrorCode::ModelFileCorrupted, "test")));
    EXPECT_FALSE(isRetryable(Error(ErrorCode::InvalidInputParameterValue, "test")));
    EXPECT_FALSE(isRetryable(Error(ErrorCode::StorageInsufficientSpace, "test")));
    EXPECT_FALSE(isRetryable(Error(ErrorCode::OperationCancelled, "test")));
}

// Test: withRetry succeeds on first attempt
TEST_F(ErrorRecoveryTest, WithRetrySucceedsFirstAttempt) {
    auto operation = [&]() -> Result<int> {
        attempt_count++;
        return Result<int>::success(42);
    };
    
    auto result = withRetry<int>(operation);
    
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(42, result.value());
    EXPECT_EQ(1, attempt_count);
}

// Test: withRetry retries on transient error and succeeds
TEST_F(ErrorRecoveryTest, WithRetryRetriesAndSucceeds) {
    auto operation = [&]() -> Result<int> {
        attempt_count++;
        if (attempt_count < 3) {
            return Result<int>::failure(Error(ErrorCode::NetworkConnectionTimeout, "Timeout"));
        }
        return Result<int>::success(42);
    };
    
    RetryConfig config;
    config.max_attempts = 5;
    config.initial_delay_ms = 10;  // Short delay for testing
    
    auto result = withRetry<int>(operation, config);
    
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(42, result.value());
    EXPECT_EQ(3, attempt_count);
}

// Test: withRetry does not retry on non-retryable error
TEST_F(ErrorRecoveryTest, WithRetryDoesNotRetryNonRetryable) {
    auto operation = [&]() -> Result<int> {
        attempt_count++;
        return Result<int>::failure(Error(ErrorCode::ModelFileCorrupted, "Corrupted"));
    };
    
    RetryConfig config;
    config.max_attempts = 5;
    
    auto result = withRetry<int>(operation, config);
    
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(ErrorCode::ModelFileCorrupted, result.error().code);
    EXPECT_EQ(1, attempt_count);  // Should not retry
}

// Test: withRetry exhausts retries and returns last error
TEST_F(ErrorRecoveryTest, WithRetryExhaustsRetries) {
    auto operation = [&]() -> Result<int> {
        attempt_count++;
        return Result<int>::failure(Error(ErrorCode::NetworkConnectionTimeout, "Timeout"));
    };
    
    RetryConfig config;
    config.max_attempts = 3;
    config.initial_delay_ms = 10;  // Short delay for testing
    
    auto result = withRetry<int>(operation, config);
    
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(ErrorCode::NetworkConnectionTimeout, result.error().code);
    EXPECT_EQ(3, attempt_count);
}

// Test: withRetry invokes retry callback
TEST_F(ErrorRecoveryTest, WithRetryInvokesCallback) {
    std::vector<int> retry_attempts;
    std::vector<std::string> retry_messages;
    
    auto operation = [&]() -> Result<int> {
        attempt_count++;
        if (attempt_count < 3) {
            return Result<int>::failure(Error(
                ErrorCode::NetworkConnectionTimeout, 
                "Timeout " + std::to_string(attempt_count.load())
            ));
        }
        return Result<int>::success(42);
    };
    
    auto on_retry = [&](int attempt, const Error& error) {
        retry_attempts.push_back(attempt);
        retry_messages.push_back(error.message);
    };
    
    RetryConfig config;
    config.max_attempts = 5;
    config.initial_delay_ms = 10;
    
    auto result = withRetry<int>(operation, config, on_retry);
    
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(2, retry_attempts.size());
    EXPECT_EQ(1, retry_attempts[0]);
    EXPECT_EQ(2, retry_attempts[1]);
    EXPECT_EQ("Timeout 1", retry_messages[0]);
    EXPECT_EQ("Timeout 2", retry_messages[1]);
}

// Test: CleanupGuard executes cleanup on destruction
TEST_F(ErrorRecoveryTest, CleanupGuardExecutesOnDestruction) {
    bool cleanup_executed = false;
    
    {
        CleanupGuard guard([&]() {
            cleanup_executed = true;
        });
        
        EXPECT_FALSE(cleanup_executed);
    }
    
    EXPECT_TRUE(cleanup_executed);
}

// Test: CleanupGuard does not execute when dismissed
TEST_F(ErrorRecoveryTest, CleanupGuardDismiss) {
    bool cleanup_executed = false;
    
    {
        CleanupGuard guard([&]() {
            cleanup_executed = true;
        });
        
        guard.dismiss();
    }
    
    EXPECT_FALSE(cleanup_executed);
}

// Test: CleanupGuard can be triggered early
TEST_F(ErrorRecoveryTest, CleanupGuardTriggerEarly) {
    bool cleanup_executed = false;
    
    {
        CleanupGuard guard([&]() {
            cleanup_executed = true;
        });
        
        EXPECT_FALSE(cleanup_executed);
        guard.trigger();
        EXPECT_TRUE(cleanup_executed);
        
        // Reset for second check
        cleanup_executed = false;
    }
    
    // Should not execute again on destruction
    EXPECT_FALSE(cleanup_executed);
}

// Test: CleanupGuard suppresses exceptions in cleanup
TEST_F(ErrorRecoveryTest, CleanupGuardSuppressesExceptions) {
    {
        CleanupGuard guard([]() {
            throw std::runtime_error("Cleanup error");
        });
        
        // Should not throw when guard is destroyed
    }
    
    // If we get here, exception was suppressed
    EXPECT_TRUE(true);
}

// Test: Aggressive retry config
TEST_F(ErrorRecoveryTest, AggressiveRetryConfig) {
    auto config = RetryConfig::aggressive();
    
    EXPECT_EQ(5, config.max_attempts);
    EXPECT_EQ(500, config.initial_delay_ms);
    EXPECT_EQ(15000, config.max_delay_ms);
    EXPECT_DOUBLE_EQ(1.5, config.backoff_multiplier);
}

// Test: Conservative retry config
TEST_F(ErrorRecoveryTest, ConservativeRetryConfig) {
    auto config = RetryConfig::conservative();
    
    EXPECT_EQ(2, config.max_attempts);
    EXPECT_EQ(2000, config.initial_delay_ms);
    EXPECT_EQ(60000, config.max_delay_ms);
    EXPECT_DOUBLE_EQ(3.0, config.backoff_multiplier);
}

// Test: withRetry with void return type
TEST_F(ErrorRecoveryTest, WithRetryVoidReturnType) {
    auto operation = [&]() -> Result<void> {
        attempt_count++;
        if (attempt_count < 2) {
            return Result<void>::failure(Error(ErrorCode::ResourceThreadPoolExhausted, "Busy"));
        }
        return Result<void>::success();
    };
    
    RetryConfig config;
    config.max_attempts = 3;
    config.initial_delay_ms = 10;
    
    auto result = withRetry<void>(operation, config);
    
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(2, attempt_count);
}

// Test: Multiple cleanup guards execute in reverse order (LIFO)
TEST_F(ErrorRecoveryTest, MultipleCleanupGuardsLIFO) {
    std::vector<int> execution_order;
    
    {
        CleanupGuard guard1([&]() {
            execution_order.push_back(1);
        });
        
        CleanupGuard guard2([&]() {
            execution_order.push_back(2);
        });
        
        CleanupGuard guard3([&]() {
            execution_order.push_back(3);
        });
    }
    
    // Guards should execute in reverse order (LIFO)
    ASSERT_EQ(3, execution_order.size());
    EXPECT_EQ(3, execution_order[0]);
    EXPECT_EQ(2, execution_order[1]);
    EXPECT_EQ(1, execution_order[2]);
}
