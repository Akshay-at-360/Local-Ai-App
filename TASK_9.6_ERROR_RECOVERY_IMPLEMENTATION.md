# Task 9.6: Error Recovery and Cleanup Implementation

## Overview

Implemented comprehensive error recovery and cleanup mechanisms to ensure the SDK remains usable after errors, properly cleans up resources on error paths, and implements retry logic for transient errors.

**Validates**: Requirements 13.4, 13.6

## Implementation Summary

### 1. Error Recovery Utilities (`core/include/ondeviceai/error_recovery.hpp`)

Created a comprehensive error recovery framework with the following components:

#### RetryConfig
- Configurable retry behavior with exponential backoff
- Default, aggressive, and conservative presets
- Parameters:
  - `max_attempts`: Maximum retry attempts (default: 3)
  - `initial_delay_ms`: Initial delay (default: 1000ms)
  - `max_delay_ms`: Maximum delay cap (default: 30000ms)
  - `backoff_multiplier`: Exponential multiplier (default: 2.0)

#### Retry Logic Functions

**`calculateBackoffDelay()`**
- Calculates exponential backoff delays
- Formula: `initial_delay * (multiplier ^ (attempt - 1))`
- Capped at `max_delay_ms`

**`isRetryable()`**
- Determines if an error represents a transient condition
- Retryable errors:
  - Network errors (unreachable, timeout, DNS failure)
  - Resource exhaustion (memory, thread pool, GPU memory)
  - File locked errors
- Non-retryable errors:
  - Validation errors
  - Corrupted files
  - Permission denied
  - Cancelled operations

**`withRetry<T>()`**
- Template function for automatic retry with exponential backoff
- Executes operation, retries on transient errors
- Invokes optional callback before each retry
- Returns success or final error after exhausting retries

#### CleanupGuard (RAII)
- Ensures cleanup code runs on scope exit
- Prevents resource leaks even with exceptions or early returns
- Methods:
  - `dismiss()`: Disable cleanup (call when operation succeeds)
  - `trigger()`: Execute cleanup early
- Exception-safe: suppresses exceptions in cleanup code

### 2. Enhanced LLM Engine Error Recovery

#### Model Loading (`loadModel()`)
- **Cleanup Guard**: Automatically frees llama.cpp resources on error
  - Frees context if allocation fails
  - Frees model if context creation fails
  - Prevents partial model state
  
- **Retry Logic for Model Loading**:
  - 2 attempts with 500ms initial delay
  - Handles transient file lock errors
  - Logs retry attempts with error details

- **Retry Logic for Context Creation**:
  - 2 attempts with 1000ms initial delay
  - Attempts to free additional memory between retries
  - Evicts more models if memory exhaustion occurs

- **Error Handling Improvements**:
  - Catches filesystem errors when getting file size
  - Provides detailed error messages with recovery suggestions
  - Ensures SDK remains in valid state after failures

#### Inference Operations (`generate()` and `generateStreaming()`)

- **Reference Count Cleanup Guards**:
  - Automatically decrements reference count on all error paths
  - Prevents memory manager state corruption
  - Eliminates manual cleanup code duplication

- **Sampler Cleanup Guards**:
  - Automatically frees llama.cpp sampler chains
  - Prevents resource leaks in generation loops
  - Handles exceptions in token callbacks safely

- **Simplified Error Paths**:
  - Removed 15+ manual cleanup calls
  - Cleanup guards handle all error scenarios
  - Code is more maintainable and less error-prone

### 3. Existing Error Recovery (Already Implemented)

#### Download Class
- Already has retry logic with exponential backoff
- Retries network errors up to 3 times
- Delays: 1s, 2s, 4s (capped at 30s)
- Resumable downloads for interrupted transfers

#### Model Manager
- Checksum verification with automatic cleanup on failure
- Storage space checking before downloads
- Atomic file moves to prevent corruption
- Cleanup of incomplete downloads on startup

## Testing

### Unit Tests (`tests/unit/error_recovery_test.cpp`)

Comprehensive test suite covering:

1. **Exponential Backoff Calculation**
   - Verifies correct delay calculation
   - Tests maximum delay capping

2. **Retryable Error Detection**
   - Network errors (retryable)
   - Resource errors (retryable)
   - Validation errors (non-retryable)
   - Permanent errors (non-retryable)

3. **withRetry Functionality**
   - Success on first attempt
   - Retry and eventual success
   - No retry for non-retryable errors
   - Exhausting all retries
   - Retry callback invocation
   - Void return type support

4. **CleanupGuard Functionality**
   - Executes on destruction
   - Dismiss prevents execution
   - Early trigger
   - Exception suppression
   - LIFO execution order

5. **Retry Configurations**
   - Default configuration
   - Aggressive configuration
   - Conservative configuration

### Standalone Test Results

```
Testing Error Recovery Implementation
======================================

Test 1: Exponential Backoff Calculation
  Attempt 0: 0ms (expected: 0)
  Attempt 1: 1000ms (expected: 1000)
  Attempt 2: 2000ms (expected: 2000)
  Attempt 3: 4000ms (expected: 4000)
  ✓ PASSED

Test 2: Retryable Error Detection
  ✓ PASSED

Test 3: withRetry - Success on First Attempt
  Attempts: 1 (expected: 1)
  Result: 42 (expected: 42)
  ✓ PASSED

Test 4: withRetry - Retry and Success
  Attempts: 3 (expected: 3)
  Result: 100 (expected: 100)
  ✓ PASSED

Test 5: withRetry - No Retry for Non-Retryable Errors
  Attempts: 1 (expected: 1)
  Error Code: 1101
  ✓ PASSED

Test 6: CleanupGuard - Executes on Destruction
  Cleanup executed: yes (expected: yes)
  ✓ PASSED

Test 7: CleanupGuard - Dismiss Prevents Execution
  Cleanup executed: no (expected: no)
  ✓ PASSED

Test 8: CleanupGuard - Trigger Executes Early
  Early execution: yes (expected: yes)
  ✓ PASSED
```

**All tests passed successfully!**

## Requirements Validation

### Requirement 13.4: SDK Remains Usable After Errors

✅ **Implemented**:
- CleanupGuard ensures resources are freed on all error paths
- Reference counting properly managed with automatic cleanup
- Model loading failures don't leave partial state
- Inference errors don't corrupt model state
- Memory manager state remains consistent after errors

**Evidence**:
- Cleanup guards in `loadModel()` free llama.cpp resources
- Reference count guards in `generate()` and `generateStreaming()`
- Sampler cleanup guards prevent resource leaks
- All error paths return SDK to valid state

### Requirement 13.6: Retry Logic for Transient Errors

✅ **Implemented**:
- Network errors: Automatic retry with exponential backoff (Download class)
- Resource exhaustion: Retry with memory cleanup attempts (LLM engine)
- File locked: Retry with short delays (Model loading)
- Configurable retry behavior with presets

**Evidence**:
- `withRetry()` template function for automatic retries
- `isRetryable()` identifies transient vs permanent errors
- Exponential backoff: 1s, 2s, 4s, 8s... (capped at 30s)
- Model loading retries on transient errors
- Context creation retries with memory cleanup

## Error Recovery Strategies

### 1. Network Errors
- **Strategy**: Retry with exponential backoff
- **Max Attempts**: 3
- **Delays**: 1s, 2s, 4s
- **Applies To**: Model downloads, registry queries

### 2. Resource Exhaustion
- **Strategy**: Cleanup and retry
- **Actions**: Evict LRU models, free memory
- **Max Attempts**: 2
- **Applies To**: Model loading, context creation

### 3. Transient File Errors
- **Strategy**: Short delay and retry
- **Max Attempts**: 2
- **Delays**: 500ms, 1s
- **Applies To**: Model file access

### 4. Permanent Errors
- **Strategy**: Fail immediately, no retry
- **Examples**: Validation errors, corrupted files, permission denied
- **Applies To**: All operations

## Code Quality Improvements

### Before (Manual Cleanup)
```cpp
if (error_condition) {
    if (memory_manager_) {
        memory_manager_->decrementRefCount(handle);
    }
    llama_sampler_free(smpl);
    return Result<T>::failure(error);
}
```

### After (Automatic Cleanup)
```cpp
ErrorRecovery::CleanupGuard ref_guard([&]() {
    if (memory_manager_) {
        memory_manager_->decrementRefCount(handle);
    }
});

ErrorRecovery::CleanupGuard sampler_guard([&smpl]() {
    if (smpl) {
        llama_sampler_free(smpl);
    }
});

if (error_condition) {
    return Result<T>::failure(error);
}
// Guards automatically clean up
```

**Benefits**:
- Eliminated 15+ manual cleanup calls
- Impossible to forget cleanup on new error paths
- Exception-safe (guards work even with exceptions)
- More maintainable and less error-prone

## Files Modified

1. **core/include/ondeviceai/error_recovery.hpp** (NEW)
   - Error recovery utilities
   - Retry logic with exponential backoff
   - CleanupGuard RAII class

2. **core/src/llm_engine.cpp**
   - Enhanced `loadModel()` with retry and cleanup guards
   - Enhanced `generate()` with cleanup guards
   - Enhanced `generateStreaming()` with cleanup guards
   - Removed manual cleanup code

3. **tests/unit/error_recovery_test.cpp** (NEW)
   - Comprehensive unit tests
   - 15 test cases covering all functionality

4. **tests/CMakeLists.txt**
   - Added error_recovery_test.cpp to test suite

5. **test_error_recovery_standalone.cpp** (NEW)
   - Standalone test for verification
   - 8 test cases with visual output

## Performance Impact

- **Minimal overhead**: Cleanup guards are lightweight RAII objects
- **No runtime cost when successful**: Guards dismissed on success
- **Retry delays**: Only occur on actual errors (transient conditions)
- **Memory**: Cleanup guards use stack allocation (no heap overhead)

## Future Enhancements

1. **Cancellation Tokens**: Add support for cancelling retry operations
2. **Jitter**: Add random jitter to backoff delays to prevent thundering herd
3. **Circuit Breaker**: Implement circuit breaker pattern for repeated failures
4. **Metrics**: Track retry statistics for monitoring
5. **Adaptive Backoff**: Adjust backoff based on error patterns

## Conclusion

Task 9.6 successfully implements comprehensive error recovery and cleanup mechanisms:

✅ SDK remains usable after errors (Requirement 13.4)
✅ Resources cleaned up on all error paths
✅ Retry logic for transient errors (Requirement 13.6)
✅ Exponential backoff for network and resource errors
✅ RAII cleanup guards prevent resource leaks
✅ All tests pass successfully

The implementation significantly improves SDK robustness and reliability, ensuring that transient errors are handled gracefully and resources are always properly cleaned up.
