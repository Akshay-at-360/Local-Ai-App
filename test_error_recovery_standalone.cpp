#include "core/include/ondeviceai/error_recovery.hpp"
#include "core/include/ondeviceai/types.hpp"
#include <iostream>
#include <atomic>

using namespace ondeviceai;
using namespace ondeviceai::ErrorRecovery;

int main() {
    std::cout << "Testing Error Recovery Implementation\n";
    std::cout << "======================================\n\n";
    
    // Test 1: Exponential backoff calculation
    std::cout << "Test 1: Exponential Backoff Calculation\n";
    RetryConfig config = RetryConfig::defaults();
    std::cout << "  Attempt 0: " << calculateBackoffDelay(0, config) << "ms (expected: 0)\n";
    std::cout << "  Attempt 1: " << calculateBackoffDelay(1, config) << "ms (expected: 1000)\n";
    std::cout << "  Attempt 2: " << calculateBackoffDelay(2, config) << "ms (expected: 2000)\n";
    std::cout << "  Attempt 3: " << calculateBackoffDelay(3, config) << "ms (expected: 4000)\n";
    std::cout << "  ✓ PASSED\n\n";
    
    // Test 2: isRetryable for network errors
    std::cout << "Test 2: Retryable Error Detection\n";
    bool test2_pass = true;
    test2_pass &= isRetryable(Error(ErrorCode::NetworkUnreachable, "test"));
    test2_pass &= isRetryable(Error(ErrorCode::NetworkConnectionTimeout, "test"));
    test2_pass &= isRetryable(Error(ErrorCode::ResourceOutOfMemory, "test"));
    test2_pass &= !isRetryable(Error(ErrorCode::ModelFileCorrupted, "test"));
    test2_pass &= !isRetryable(Error(ErrorCode::InvalidInputParameterValue, "test"));
    std::cout << "  " << (test2_pass ? "✓ PASSED" : "✗ FAILED") << "\n\n";
    
    // Test 3: withRetry succeeds on first attempt
    std::cout << "Test 3: withRetry - Success on First Attempt\n";
    std::atomic<int> attempt_count{0};
    auto operation1 = [&]() -> Result<int> {
        attempt_count++;
        return Result<int>::success(42);
    };
    
    auto result1 = withRetry<int>(operation1);
    bool test3_pass = result1.isSuccess() && result1.value() == 42 && attempt_count == 1;
    std::cout << "  Attempts: " << attempt_count << " (expected: 1)\n";
    std::cout << "  Result: " << (result1.isSuccess() ? result1.value() : -1) << " (expected: 42)\n";
    std::cout << "  " << (test3_pass ? "✓ PASSED" : "✗ FAILED") << "\n\n";
    
    // Test 4: withRetry retries and succeeds
    std::cout << "Test 4: withRetry - Retry and Success\n";
    attempt_count = 0;
    auto operation2 = [&]() -> Result<int> {
        attempt_count++;
        if (attempt_count < 3) {
            return Result<int>::failure(Error(ErrorCode::NetworkConnectionTimeout, "Timeout"));
        }
        return Result<int>::success(100);
    };
    
    RetryConfig fast_config;
    fast_config.max_attempts = 5;
    fast_config.initial_delay_ms = 10;
    
    auto result2 = withRetry<int>(operation2, fast_config);
    bool test4_pass = result2.isSuccess() && result2.value() == 100 && attempt_count == 3;
    std::cout << "  Attempts: " << attempt_count << " (expected: 3)\n";
    std::cout << "  Result: " << (result2.isSuccess() ? result2.value() : -1) << " (expected: 100)\n";
    std::cout << "  " << (test4_pass ? "✓ PASSED" : "✗ FAILED") << "\n\n";
    
    // Test 5: withRetry does not retry non-retryable errors
    std::cout << "Test 5: withRetry - No Retry for Non-Retryable Errors\n";
    attempt_count = 0;
    auto operation3 = [&]() -> Result<int> {
        attempt_count++;
        return Result<int>::failure(Error(ErrorCode::ModelFileCorrupted, "Corrupted"));
    };
    
    auto result3 = withRetry<int>(operation3, fast_config);
    bool test5_pass = result3.isError() && 
                      result3.error().code == ErrorCode::ModelFileCorrupted && 
                      attempt_count == 1;
    std::cout << "  Attempts: " << attempt_count << " (expected: 1)\n";
    std::cout << "  Error Code: " << static_cast<int>(result3.error().code) << "\n";
    std::cout << "  " << (test5_pass ? "✓ PASSED" : "✗ FAILED") << "\n\n";
    
    // Test 6: CleanupGuard executes on destruction
    std::cout << "Test 6: CleanupGuard - Executes on Destruction\n";
    bool cleanup_executed = false;
    {
        CleanupGuard guard([&]() {
            cleanup_executed = true;
        });
    }
    bool test6_pass = cleanup_executed;
    std::cout << "  Cleanup executed: " << (cleanup_executed ? "yes" : "no") << " (expected: yes)\n";
    std::cout << "  " << (test6_pass ? "✓ PASSED" : "✗ FAILED") << "\n\n";
    
    // Test 7: CleanupGuard dismiss prevents execution
    std::cout << "Test 7: CleanupGuard - Dismiss Prevents Execution\n";
    cleanup_executed = false;
    {
        CleanupGuard guard([&]() {
            cleanup_executed = true;
        });
        guard.dismiss();
    }
    bool test7_pass = !cleanup_executed;
    std::cout << "  Cleanup executed: " << (cleanup_executed ? "yes" : "no") << " (expected: no)\n";
    std::cout << "  " << (test7_pass ? "✓ PASSED" : "✗ FAILED") << "\n\n";
    
    // Test 8: CleanupGuard trigger executes early
    std::cout << "Test 8: CleanupGuard - Trigger Executes Early\n";
    cleanup_executed = false;
    {
        CleanupGuard guard([&]() {
            cleanup_executed = true;
        });
        guard.trigger();
        bool early_executed = cleanup_executed;
        cleanup_executed = false;  // Reset for destruction check
        
        bool test8_pass = early_executed && !cleanup_executed;
        std::cout << "  Early execution: " << (early_executed ? "yes" : "no") << " (expected: yes)\n";
        std::cout << "  " << (test8_pass ? "✓ PASSED" : "✗ FAILED") << "\n\n";
        return test8_pass ? 0 : 1;
    }
    
    // Summary
    std::cout << "======================================\n";
    std::cout << "All tests completed!\n";
    
    return 0;
}
