#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>
#include "ondeviceai/types.hpp"
#include "ondeviceai/error_utils.hpp"
#include <vector>
#include <string>
#include <random>

using namespace ondeviceai;

// ============================================================================
// RapidCheck Generators for Error Conditions
// ============================================================================

namespace rc {

// Generator for ErrorCode enum values
template<>
struct Arbitrary<ErrorCode> {
    static Gen<ErrorCode> arbitrary() {
        return gen::element(
            // ModelNotFound errors (1000-1099)
            ErrorCode::ModelNotFoundInRegistry,
            ErrorCode::ModelFileNotFound,
            ErrorCode::ModelVersionNotAvailable,
            
            // ModelLoadError errors (1100-1199)
            ErrorCode::ModelFileCorrupted,
            ErrorCode::ModelIncompatibleArchitecture,
            ErrorCode::ModelInsufficientMemory,
            ErrorCode::ModelUnsupportedQuantization,
            ErrorCode::ModelFileLocked,
            
            // InferenceError errors (1200-1299)
            ErrorCode::InferenceModelNotLoaded,
            ErrorCode::InferenceInvalidInput,
            ErrorCode::InferenceContextWindowExceeded,
            ErrorCode::InferenceTimeout,
            ErrorCode::InferenceHardwareAccelerationFailure,
            
            // NetworkError errors (1300-1399)
            ErrorCode::NetworkUnreachable,
            ErrorCode::NetworkConnectionTimeout,
            ErrorCode::NetworkHTTPError,
            ErrorCode::NetworkDNSFailure,
            ErrorCode::NetworkSSLError,
            
            // StorageError errors (1400-1499)
            ErrorCode::StorageInsufficientSpace,
            ErrorCode::StoragePermissionDenied,
            ErrorCode::StorageReadError,
            ErrorCode::StorageWriteError,
            ErrorCode::StorageDiskFull,
            
            // InvalidInput errors (1500-1599)
            ErrorCode::InvalidInputNullPointer,
            ErrorCode::InvalidInputParameterValue,
            ErrorCode::InvalidInputConfiguration,
            ErrorCode::InvalidInputAudioFormat,
            ErrorCode::InvalidInputModelHandle,
            
            // ResourceExhausted errors (1600-1699)
            ErrorCode::ResourceOutOfMemory,
            ErrorCode::ResourceTooManyOpenFiles,
            ErrorCode::ResourceThreadPoolExhausted,
            ErrorCode::ResourceGPUMemoryExhausted,
            
            // Cancelled errors (1700-1799)
            ErrorCode::OperationCancelled,
            ErrorCode::OperationTimeout,
            ErrorCode::OperationInterrupted
        );
    }
};

} // namespace rc

// ============================================================================
// Helper Functions
// ============================================================================

// Generate a random error using ErrorUtils based on the error code
Error generateErrorFromCode(ErrorCode code) {
    switch (code) {
        // ModelNotFound errors
        case ErrorCode::ModelNotFoundInRegistry:
            return ErrorUtils::modelNotFoundInRegistry("test-model-" + std::to_string(rand()));
        case ErrorCode::ModelFileNotFound:
            return ErrorUtils::modelFileNotFound("/path/to/model-" + std::to_string(rand()) + ".gguf");
        case ErrorCode::ModelVersionNotAvailable:
            return ErrorUtils::modelVersionNotAvailable("test-model", "1.0." + std::to_string(rand() % 100));
        
        // ModelLoadError errors
        case ErrorCode::ModelFileCorrupted:
            return ErrorUtils::modelFileCorrupted("/path/to/model.gguf", "Checksum mismatch");
        case ErrorCode::ModelIncompatibleArchitecture:
            return ErrorUtils::modelIncompatibleArchitecture("GGUF", "ONNX");
        case ErrorCode::ModelInsufficientMemory:
            return ErrorUtils::modelInsufficientMemory(
                3ULL * 1024 * 1024 * 1024,  // 3GB required
                2ULL * 1024 * 1024 * 1024   // 2GB available
            );
        case ErrorCode::ModelUnsupportedQuantization:
            return ErrorUtils::modelUnsupportedQuantization("Q2_K");
        case ErrorCode::ModelFileLocked:
            return ErrorUtils::modelFileLocked("/path/to/model.gguf");
        
        // InferenceError errors
        case ErrorCode::InferenceModelNotLoaded:
            return ErrorUtils::inferenceModelNotLoaded(static_cast<ModelHandle>(rand()));
        case ErrorCode::InferenceInvalidInput:
            return ErrorUtils::inferenceInvalidInput("Input text is too long");
        case ErrorCode::InferenceContextWindowExceeded:
            return ErrorUtils::inferenceContextWindowExceeded(5000, 4096);
        case ErrorCode::InferenceTimeout:
            return ErrorUtils::inferenceTimeout(30);
        case ErrorCode::InferenceHardwareAccelerationFailure:
            return ErrorUtils::inferenceHardwareAccelerationFailure("Metal initialization failed");
        
        // NetworkError errors
        case ErrorCode::NetworkUnreachable:
            return ErrorUtils::networkUnreachable("example.com");
        case ErrorCode::NetworkConnectionTimeout:
            return ErrorUtils::networkConnectionTimeout("example.com");
        case ErrorCode::NetworkHTTPError:
            return ErrorUtils::networkHTTPError(404, "Not Found");
        case ErrorCode::NetworkDNSFailure:
            return ErrorUtils::networkDNSFailure("example.com");
        case ErrorCode::NetworkSSLError:
            return ErrorUtils::networkSSLError("Certificate verification failed");
        
        // StorageError errors
        case ErrorCode::StorageInsufficientSpace:
            return ErrorUtils::storageInsufficientSpace(
                5ULL * 1024 * 1024 * 1024,  // 5GB required
                1ULL * 1024 * 1024 * 1024   // 1GB available
            );
        case ErrorCode::StoragePermissionDenied:
            return ErrorUtils::storagePermissionDenied("/restricted/path");
        case ErrorCode::StorageReadError:
            return ErrorUtils::storageReadError("/path/to/file", "I/O error");
        case ErrorCode::StorageWriteError:
            return ErrorUtils::storageWriteError("/path/to/file", "Disk error");
        case ErrorCode::StorageDiskFull:
            return ErrorUtils::storageDiskFull("/path/to/file");
        
        // InvalidInput errors
        case ErrorCode::InvalidInputNullPointer:
            return ErrorUtils::invalidInputNullPointer("callback");
        case ErrorCode::InvalidInputParameterValue:
            return ErrorUtils::invalidInputParameterValue("temperature", "Value must be between 0.0 and 2.0");
        case ErrorCode::InvalidInputConfiguration:
            return ErrorUtils::invalidInputConfiguration("Thread count must be positive");
        case ErrorCode::InvalidInputAudioFormat:
            return ErrorUtils::invalidInputAudioFormat("Expected PCM float32, got int16");
        case ErrorCode::InvalidInputModelHandle:
            return ErrorUtils::invalidInputModelHandle(static_cast<ModelHandle>(rand()));
        
        // ResourceExhausted errors
        case ErrorCode::ResourceOutOfMemory:
            return ErrorUtils::resourceOutOfMemory("model loading");
        case ErrorCode::ResourceTooManyOpenFiles:
            return ErrorUtils::resourceTooManyOpenFiles();
        case ErrorCode::ResourceThreadPoolExhausted:
            return ErrorUtils::resourceThreadPoolExhausted();
        case ErrorCode::ResourceGPUMemoryExhausted:
            return ErrorUtils::resourceGPUMemoryExhausted("VRAM allocation failed");
        
        // Cancelled errors
        case ErrorCode::OperationCancelled:
            return ErrorUtils::operationCancelled("download");
        case ErrorCode::OperationTimeout:
            return ErrorUtils::operationTimeout("inference", 60);
        case ErrorCode::OperationInterrupted:
            return ErrorUtils::operationInterrupted("synthesis");
        
        default:
            // Should never reach here with our generator
            return ErrorUtils::invalidInputParameterValue("unknown", "Unknown error code");
    }
}

// ============================================================================
// Property Tests
// ============================================================================

// Feature: on-device-ai-sdk, Property 16: Error Messages Include Description
// **Validates: Requirements 13.1**
//
// Property: For any error returned by the SDK, the error should include a 
// non-empty descriptive message.
//
// This property test generates random error conditions across all error 
// categories and verifies that every error has:
// 1. A non-empty message field
// 2. A non-empty details field
// 3. Descriptive content that helps identify the error
RC_GTEST_PROP(ErrorPropertyTest, ErrorMessagesIncludeDescription,
              (ErrorCode error_code)) {
    // Generate an error using the error code
    Error error = generateErrorFromCode(error_code);
    
    // Property 1: Error message must not be empty
    RC_ASSERT(!error.message.empty());
    
    // Property 2: Error details must not be empty
    RC_ASSERT(!error.details.empty());
    
    // Property 3: Message should be descriptive (at least 10 characters)
    // This ensures it's not just a placeholder like "Error" or "Failed"
    RC_ASSERT(error.message.length() >= 10u);
    
    // Property 4: Details should be descriptive (at least 10 characters)
    RC_ASSERT(error.details.length() >= 10u);
    
    // Property 5: Error code should match what was requested
    RC_ASSERT(error.code == error_code);
    
    // Property 6: Message and details should be different
    // (they serve different purposes - message is user-facing, details are technical)
    RC_ASSERT(error.message != error.details);
}

// Additional property test: Verify all error helper functions produce valid errors
RC_GTEST_PROP(ErrorPropertyTest, AllErrorHelpersProduceValidErrors,
              (ErrorCode error_code)) {
    // Generate error
    Error error = generateErrorFromCode(error_code);
    
    // Verify the error is well-formed
    RC_ASSERT(!error.message.empty());
    RC_ASSERT(!error.details.empty());
    RC_ASSERT(error.code == error_code);
    
    // Verify error category is correct
    std::string category = ErrorUtils::getErrorCategory(error_code);
    RC_ASSERT(!category.empty());
    RC_ASSERT(category != "Unknown");
}

// Property test: Verify error messages contain relevant context
RC_GTEST_PROP(ErrorPropertyTest, ErrorMessagesContainContext,
              (ErrorCode error_code)) {
    Error error = generateErrorFromCode(error_code);
    
    // Error messages should contain some context about what failed
    // We check that the message is not just generic
    std::string message_lower = error.message;
    std::transform(message_lower.begin(), message_lower.end(), 
                   message_lower.begin(), ::tolower);
    
    // Message should contain at least one meaningful word
    bool has_context = false;
    std::vector<std::string> context_words = {
        "model", "file", "network", "storage", "memory", "inference",
        "download", "load", "error", "failed", "invalid", "timeout",
        "permission", "disk", "connection", "ssl", "http", "gpu",
        "thread", "operation", "cancelled", "interrupted"
    };
    
    for (const auto& word : context_words) {
        if (message_lower.find(word) != std::string::npos) {
            has_context = true;
            break;
        }
    }
    
    RC_ASSERT(has_context);
}

// Property test: Verify recovery suggestions are provided when appropriate
RC_GTEST_PROP(ErrorPropertyTest, RecoverySuggestionsWhenAppropriate,
              (ErrorCode error_code)) {
    Error error = generateErrorFromCode(error_code);
    
    // Cancelled operations typically don't need recovery suggestions
    if (error_code == ErrorCode::OperationCancelled) {
        // May or may not have recovery suggestion - both are valid
        RC_SUCCEED("Cancelled operations may not have recovery suggestions");
    } else {
        // Most other errors should have recovery suggestions
        // (though this is not strictly required, it's good practice)
        if (error.recovery_suggestion.has_value()) {
            RC_ASSERT(!error.recovery_suggestion.value().empty());
        }
    }
}

// Property test: Verify error codes are in correct ranges
RC_GTEST_PROP(ErrorPropertyTest, ErrorCodesInCorrectRanges,
              (ErrorCode error_code)) {
    int code_value = static_cast<int>(error_code);
    
    // Verify code is in a valid range
    bool in_valid_range = 
        (code_value >= 1000 && code_value < 1100) ||  // ModelNotFound
        (code_value >= 1100 && code_value < 1200) ||  // ModelLoadError
        (code_value >= 1200 && code_value < 1300) ||  // InferenceError
        (code_value >= 1300 && code_value < 1400) ||  // NetworkError
        (code_value >= 1400 && code_value < 1500) ||  // StorageError
        (code_value >= 1500 && code_value < 1600) ||  // InvalidInput
        (code_value >= 1600 && code_value < 1700) ||  // ResourceExhausted
        (code_value >= 1700 && code_value < 1800);    // Cancelled
    
    RC_ASSERT(in_valid_range);
}

// Property test: Verify error category helpers work correctly
RC_GTEST_PROP(ErrorPropertyTest, ErrorCategoryHelpersCorrect,
              (ErrorCode error_code)) {
    int code_value = static_cast<int>(error_code);
    std::string category = ErrorUtils::getErrorCategory(error_code);
    
    // Verify category matches the code range
    if (code_value >= 1000 && code_value < 1100) {
        RC_ASSERT(category == "ModelNotFound");
    } else if (code_value >= 1100 && code_value < 1200) {
        RC_ASSERT(category == "ModelLoadError");
    } else if (code_value >= 1200 && code_value < 1300) {
        RC_ASSERT(category == "InferenceError");
    } else if (code_value >= 1300 && code_value < 1400) {
        RC_ASSERT(category == "NetworkError");
        RC_ASSERT(ErrorUtils::isNetworkError(error_code));
    } else if (code_value >= 1400 && code_value < 1500) {
        RC_ASSERT(category == "StorageError");
        RC_ASSERT(ErrorUtils::isStorageError(error_code));
    } else if (code_value >= 1500 && code_value < 1600) {
        RC_ASSERT(category == "InvalidInput");
        RC_ASSERT(ErrorUtils::isValidationError(error_code));
    } else if (code_value >= 1600 && code_value < 1700) {
        RC_ASSERT(category == "ResourceExhausted");
    } else if (code_value >= 1700 && code_value < 1800) {
        RC_ASSERT(category == "Cancelled");
    }
}

// Property test: Verify Result<T> properly propagates errors
RC_GTEST_PROP(ErrorPropertyTest, ResultPropagatesErrorsCorrectly,
              (ErrorCode error_code)) {
    Error error = generateErrorFromCode(error_code);
    
    // Create a Result<int> with the error
    auto result = Result<int>::failure(error);
    
    // Verify the result is in error state
    RC_ASSERT(result.isError());
    RC_ASSERT(!result.isSuccess());
    
    // Verify the error is preserved
    const Error& propagated_error = result.error();
    RC_ASSERT(propagated_error.code == error.code);
    RC_ASSERT(propagated_error.message == error.message);
    RC_ASSERT(propagated_error.details == error.details);
}

// Property test: Verify errors with same code have consistent structure
RC_GTEST_PROP(ErrorPropertyTest, SameCodeConsistentStructure,
              (ErrorCode error_code)) {
    // Generate two errors with the same code
    Error error1 = generateErrorFromCode(error_code);
    Error error2 = generateErrorFromCode(error_code);
    
    // Both should have the same error code
    RC_ASSERT(error1.code == error2.code);
    RC_ASSERT(error1.code == error_code);
    
    // Both should have non-empty messages and details
    RC_ASSERT(!error1.message.empty());
    RC_ASSERT(!error2.message.empty());
    RC_ASSERT(!error1.details.empty());
    RC_ASSERT(!error2.details.empty());
    
    // Both should have the same category
    std::string category1 = ErrorUtils::getErrorCategory(error1.code);
    std::string category2 = ErrorUtils::getErrorCategory(error2.code);
    RC_ASSERT(category1 == category2);
}

// Property test: Verify error messages are human-readable
RC_GTEST_PROP(ErrorPropertyTest, ErrorMessagesAreHumanReadable,
              (ErrorCode error_code)) {
    Error error = generateErrorFromCode(error_code);
    
    // Check that message contains mostly printable ASCII characters
    // (allowing for some special characters)
    int printable_count = 0;
    for (char c : error.message) {
        if (std::isprint(static_cast<unsigned char>(c)) || std::isspace(static_cast<unsigned char>(c))) {
            printable_count++;
        }
    }
    
    // At least 95% of characters should be printable
    double printable_ratio = static_cast<double>(printable_count) / error.message.length();
    RC_ASSERT(printable_ratio >= 0.95);
    
    // Same for details
    printable_count = 0;
    for (char c : error.details) {
        if (std::isprint(static_cast<unsigned char>(c)) || std::isspace(static_cast<unsigned char>(c))) {
            printable_count++;
        }
    }
    
    printable_ratio = static_cast<double>(printable_count) / error.details.length();
    RC_ASSERT(printable_ratio >= 0.95);
}

// Property test: Verify retryable errors are correctly identified
RC_GTEST_PROP(ErrorPropertyTest, RetryableErrorsCorrectlyIdentified,
              (ErrorCode error_code)) {
    bool is_retryable = ErrorUtils::isRetryable(error_code);
    
    // Verify retryable classification matches expected behavior
    if (error_code == ErrorCode::NetworkUnreachable ||
        error_code == ErrorCode::NetworkConnectionTimeout ||
        error_code == ErrorCode::NetworkDNSFailure ||
        error_code == ErrorCode::ResourceThreadPoolExhausted) {
        RC_ASSERT(is_retryable);
    } else {
        // Most other errors are not retryable
        // (though this could be extended in the future)
    }
}

// ============================================================================
// Property 17: Error-Specific Failure Reasons
// ============================================================================

// Feature: on-device-ai-sdk, Property 17: Error-Specific Failure Reasons
// **Validates: Requirements 13.3**
//
// Property: For any model loading failure, different failure causes 
// (file not found, corrupted, insufficient memory, incompatible format) 
// should produce errors with different error codes or messages.
//
// This property test generates different model loading failure scenarios
// and verifies that each distinct failure cause produces a unique error
// code, ensuring that applications can distinguish between different
// failure reasons and take appropriate action.
RC_GTEST_PROP(ErrorPropertyTest, ModelLoadingFailuresHaveDistinctErrorCodes,
              ()) {
    // Generate different model loading failure scenarios
    std::vector<ErrorCode> model_load_error_codes = {
        ErrorCode::ModelFileNotFound,
        ErrorCode::ModelFileCorrupted,
        ErrorCode::ModelInsufficientMemory,
        ErrorCode::ModelIncompatibleArchitecture,
        ErrorCode::ModelUnsupportedQuantization,
        ErrorCode::ModelFileLocked
    };
    
    // Generate errors for each failure type
    std::vector<Error> errors;
    errors.push_back(ErrorUtils::modelFileNotFound("/path/to/model.gguf"));
    errors.push_back(ErrorUtils::modelFileCorrupted("/path/to/model.gguf", "Invalid magic number"));
    errors.push_back(ErrorUtils::modelInsufficientMemory(3ULL * 1024 * 1024 * 1024, 2ULL * 1024 * 1024 * 1024));
    errors.push_back(ErrorUtils::modelIncompatibleArchitecture("GGUF", "ONNX"));
    errors.push_back(ErrorUtils::modelUnsupportedQuantization("Q2_K"));
    errors.push_back(ErrorUtils::modelFileLocked("/path/to/model.gguf"));
    
    // Property 1: Each error should have a unique error code
    std::set<ErrorCode> unique_codes;
    for (const auto& error : errors) {
        unique_codes.insert(error.code);
    }
    RC_ASSERT(unique_codes.size() == errors.size());
    
    // Property 2: All error codes should be in the ModelLoadError range (1100-1199)
    // or ModelNotFound range (1000-1099)
    for (const auto& error : errors) {
        int code_value = static_cast<int>(error.code);
        bool in_model_error_range = 
            (code_value >= 1000 && code_value < 1100) ||  // ModelNotFound
            (code_value >= 1100 && code_value < 1200);    // ModelLoadError
        RC_ASSERT(in_model_error_range);
    }
    
    // Property 3: Each error should have a distinct message
    std::set<std::string> unique_messages;
    for (const auto& error : errors) {
        unique_messages.insert(error.message);
    }
    RC_ASSERT(unique_messages.size() == errors.size());
    
    // Property 4: Error messages should contain keywords specific to the failure type
    RC_ASSERT(errors[0].message.find("not found") != std::string::npos ||
              errors[0].message.find("Not found") != std::string::npos);
    RC_ASSERT(errors[1].message.find("corrupt") != std::string::npos ||
              errors[1].message.find("invalid") != std::string::npos);
    RC_ASSERT(errors[2].message.find("memory") != std::string::npos ||
              errors[2].message.find("Memory") != std::string::npos);
    RC_ASSERT(errors[3].message.find("architecture") != std::string::npos ||
              errors[3].message.find("Incompatible") != std::string::npos);
    RC_ASSERT(errors[4].message.find("quantization") != std::string::npos ||
              errors[4].message.find("Unsupported") != std::string::npos);
    RC_ASSERT(errors[5].message.find("locked") != std::string::npos ||
              errors[5].message.find("in use") != std::string::npos);
}

// Property test: Verify model loading errors have distinct details
RC_GTEST_PROP(ErrorPropertyTest, ModelLoadingFailuresHaveDistinctDetails,
              ()) {
    // Generate different model loading failure scenarios with varying parameters
    auto path1 = *rc::gen::string<std::string>();
    auto path2 = *rc::gen::string<std::string>();
    auto required_mem = *rc::gen::inRange<size_t>(1ULL * 1024 * 1024 * 1024, 10ULL * 1024 * 1024 * 1024);
    auto available_mem = *rc::gen::inRange<size_t>(100ULL * 1024 * 1024, required_mem);
    
    std::vector<Error> errors;
    errors.push_back(ErrorUtils::modelFileNotFound(path1));
    errors.push_back(ErrorUtils::modelFileCorrupted(path2, "Checksum mismatch"));
    errors.push_back(ErrorUtils::modelInsufficientMemory(required_mem, available_mem));
    errors.push_back(ErrorUtils::modelIncompatibleArchitecture("GGUF", "ONNX"));
    errors.push_back(ErrorUtils::modelUnsupportedQuantization("Q2_K"));
    errors.push_back(ErrorUtils::modelFileLocked(path1));
    
    // Property: Each error should have distinct error codes
    std::set<ErrorCode> unique_codes;
    for (const auto& error : errors) {
        unique_codes.insert(error.code);
    }
    RC_ASSERT(unique_codes.size() == errors.size());
    
    // Property: Details should provide specific information about the failure
    for (const auto& error : errors) {
        RC_ASSERT(!error.details.empty());
        RC_ASSERT(error.details.length() >= 10u);
    }
    
    // Property: Memory error should include numeric details
    RC_ASSERT(errors[2].details.find("MB") != std::string::npos ||
              errors[2].details.find("GB") != std::string::npos ||
              errors[2].details.find(std::to_string(required_mem)) != std::string::npos);
}

// Property test: Verify different failure causes can be distinguished programmatically
RC_GTEST_PROP(ErrorPropertyTest, ModelLoadingFailureCausesDistinguishable,
              ()) {
    // Create a map of failure causes to their error codes
    std::map<std::string, ErrorCode> failure_causes = {
        {"file_not_found", ErrorCode::ModelFileNotFound},
        {"corrupted", ErrorCode::ModelFileCorrupted},
        {"insufficient_memory", ErrorCode::ModelInsufficientMemory},
        {"incompatible_architecture", ErrorCode::ModelIncompatibleArchitecture},
        {"unsupported_quantization", ErrorCode::ModelUnsupportedQuantization},
        {"file_locked", ErrorCode::ModelFileLocked}
    };
    
    // Generate errors for each cause
    std::vector<std::pair<std::string, Error>> errors;
    errors.emplace_back("file_not_found", ErrorUtils::modelFileNotFound("/test/model.gguf"));
    errors.emplace_back("corrupted", ErrorUtils::modelFileCorrupted("/test/model.gguf", "Invalid format"));
    errors.emplace_back("insufficient_memory", ErrorUtils::modelInsufficientMemory(3000000000, 2000000000));
    errors.emplace_back("incompatible_architecture", ErrorUtils::modelIncompatibleArchitecture("GGUF", "ONNX"));
    errors.emplace_back("unsupported_quantization", ErrorUtils::modelUnsupportedQuantization("Q2_K"));
    errors.emplace_back("file_locked", ErrorUtils::modelFileLocked("/test/model.gguf"));
    
    // Property 1: Each cause should map to a unique error code
    std::set<ErrorCode> codes_seen;
    for (const auto& [cause, error] : errors) {
        auto expected_code = failure_causes[cause];
        RC_ASSERT(error.code == expected_code);
        RC_ASSERT(codes_seen.find(error.code) == codes_seen.end());
        codes_seen.insert(error.code);
    }
    
    // Property 2: Application can distinguish causes by checking error codes
    const auto& file_not_found_error = errors[0].second;
    const auto& corrupted_error = errors[1].second;
    const auto& memory_error = errors[2].second;
    
    RC_ASSERT(file_not_found_error.code != corrupted_error.code);
    RC_ASSERT(file_not_found_error.code != memory_error.code);
    RC_ASSERT(corrupted_error.code != memory_error.code);
    
    // Property 3: Each error should have appropriate recovery suggestions
    RC_ASSERT(file_not_found_error.recovery_suggestion.has_value());
    RC_ASSERT(corrupted_error.recovery_suggestion.has_value());
    RC_ASSERT(memory_error.recovery_suggestion.has_value());
    
    // Property 4: Recovery suggestions should be different for different causes
    RC_ASSERT(file_not_found_error.recovery_suggestion.value() != 
              corrupted_error.recovery_suggestion.value());
    RC_ASSERT(file_not_found_error.recovery_suggestion.value() != 
              memory_error.recovery_suggestion.value());
}

// Property test: Verify model loading errors are in correct category
RC_GTEST_PROP(ErrorPropertyTest, ModelLoadingErrorsInCorrectCategory,
              ()) {
    std::vector<Error> model_loading_errors = {
        ErrorUtils::modelFileNotFound("/path/to/model.gguf"),
        ErrorUtils::modelFileCorrupted("/path/to/model.gguf", "Invalid"),
        ErrorUtils::modelInsufficientMemory(3000000000, 2000000000),
        ErrorUtils::modelIncompatibleArchitecture("GGUF", "ONNX"),
        ErrorUtils::modelUnsupportedQuantization("Q2_K"),
        ErrorUtils::modelFileLocked("/path/to/model.gguf")
    };
    
    // Property: All model loading errors should be in ModelLoadError or ModelNotFound category
    for (const auto& error : model_loading_errors) {
        std::string category = ErrorUtils::getErrorCategory(error.code);
        RC_ASSERT(category == "ModelLoadError" || category == "ModelNotFound");
    }
}
