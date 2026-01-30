#include <gtest/gtest.h>
#include "ondeviceai/types.hpp"
#include "ondeviceai/error_utils.hpp"

using namespace ondeviceai;

// ============================================================================
// Test Fixture
// ============================================================================

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// ============================================================================
// Error Structure Tests
// ============================================================================

TEST_F(ErrorHandlingTest, ErrorConstructor) {
    Error error(
        ErrorCode::ModelFileNotFound,
        "Test message",
        "Test details",
        "Test recovery"
    );
    
    EXPECT_EQ(error.code, ErrorCode::ModelFileNotFound);
    EXPECT_EQ(error.message, "Test message");
    EXPECT_EQ(error.details, "Test details");
    ASSERT_TRUE(error.recovery_suggestion.has_value());
    EXPECT_EQ(error.recovery_suggestion.value(), "Test recovery");
}

TEST_F(ErrorHandlingTest, ErrorWithoutRecoverySuggestion) {
    Error error(
        ErrorCode::OperationCancelled,
        "Operation cancelled",
        "User cancelled the operation"
    );
    
    EXPECT_EQ(error.code, ErrorCode::OperationCancelled);
    EXPECT_EQ(error.message, "Operation cancelled");
    EXPECT_EQ(error.details, "User cancelled the operation");
    EXPECT_FALSE(error.recovery_suggestion.has_value());
}

// ============================================================================
// Result<T> Tests
// ============================================================================

TEST_F(ErrorHandlingTest, ResultSuccessInt) {
    auto result = Result<int>::success(42);
    
    EXPECT_TRUE(result.isSuccess());
    EXPECT_FALSE(result.isError());
    EXPECT_EQ(result.value(), 42);
}

TEST_F(ErrorHandlingTest, ResultSuccessString) {
    auto result = Result<std::string>::success("Hello, World!");
    
    EXPECT_TRUE(result.isSuccess());
    EXPECT_FALSE(result.isError());
    EXPECT_EQ(result.value(), "Hello, World!");
}

TEST_F(ErrorHandlingTest, ResultFailure) {
    Error error(ErrorCode::ModelFileNotFound, "File not found", "Details");
    auto result = Result<int>::failure(error);
    
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::ModelFileNotFound);
    EXPECT_EQ(result.error().message, "File not found");
}

TEST_F(ErrorHandlingTest, ResultVoidSuccess) {
    auto result = Result<void>::success();
    
    EXPECT_TRUE(result.isSuccess());
    EXPECT_FALSE(result.isError());
}

TEST_F(ErrorHandlingTest, ResultVoidFailure) {
    Error error(ErrorCode::InvalidInputConfiguration, "Invalid config", "Details");
    auto result = Result<void>::failure(error);
    
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidInputConfiguration);
}

TEST_F(ErrorHandlingTest, ResultMoveSemantics) {
    // Test that Result properly handles move semantics
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto result = Result<std::vector<int>>::success(std::move(data));
    
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value().size(), 5);
    EXPECT_EQ(result.value()[0], 1);
}

// ============================================================================
// Error Code Categories Tests
// ============================================================================

TEST_F(ErrorHandlingTest, ErrorCodeCategories) {
    // ModelNotFound category (1000-1099)
    EXPECT_EQ(static_cast<int>(ErrorCode::ModelNotFoundInRegistry), 1001);
    EXPECT_EQ(static_cast<int>(ErrorCode::ModelFileNotFound), 1002);
    EXPECT_EQ(static_cast<int>(ErrorCode::ModelVersionNotAvailable), 1003);
    
    // ModelLoadError category (1100-1199)
    EXPECT_EQ(static_cast<int>(ErrorCode::ModelFileCorrupted), 1101);
    EXPECT_EQ(static_cast<int>(ErrorCode::ModelIncompatibleArchitecture), 1102);
    EXPECT_EQ(static_cast<int>(ErrorCode::ModelInsufficientMemory), 1103);
    
    // InferenceError category (1200-1299)
    EXPECT_EQ(static_cast<int>(ErrorCode::InferenceModelNotLoaded), 1201);
    EXPECT_EQ(static_cast<int>(ErrorCode::InferenceInvalidInput), 1202);
    EXPECT_EQ(static_cast<int>(ErrorCode::InferenceContextWindowExceeded), 1203);
    
    // NetworkError category (1300-1399)
    EXPECT_EQ(static_cast<int>(ErrorCode::NetworkUnreachable), 1301);
    EXPECT_EQ(static_cast<int>(ErrorCode::NetworkConnectionTimeout), 1302);
    EXPECT_EQ(static_cast<int>(ErrorCode::NetworkHTTPError), 1303);
    
    // StorageError category (1400-1499)
    EXPECT_EQ(static_cast<int>(ErrorCode::StorageInsufficientSpace), 1401);
    EXPECT_EQ(static_cast<int>(ErrorCode::StoragePermissionDenied), 1402);
    EXPECT_EQ(static_cast<int>(ErrorCode::StorageReadError), 1403);
    
    // InvalidInput category (1500-1599)
    EXPECT_EQ(static_cast<int>(ErrorCode::InvalidInputNullPointer), 1501);
    EXPECT_EQ(static_cast<int>(ErrorCode::InvalidInputParameterValue), 1502);
    EXPECT_EQ(static_cast<int>(ErrorCode::InvalidInputConfiguration), 1503);
    
    // ResourceExhausted category (1600-1699)
    EXPECT_EQ(static_cast<int>(ErrorCode::ResourceOutOfMemory), 1601);
    EXPECT_EQ(static_cast<int>(ErrorCode::ResourceTooManyOpenFiles), 1602);
    EXPECT_EQ(static_cast<int>(ErrorCode::ResourceThreadPoolExhausted), 1603);
    
    // Cancelled category (1700-1799)
    EXPECT_EQ(static_cast<int>(ErrorCode::OperationCancelled), 1701);
    EXPECT_EQ(static_cast<int>(ErrorCode::OperationTimeout), 1702);
    EXPECT_EQ(static_cast<int>(ErrorCode::OperationInterrupted), 1703);
}

// ============================================================================
// ErrorUtils Helper Functions Tests
// ============================================================================

TEST_F(ErrorHandlingTest, ModelNotFoundInRegistryHelper) {
    auto error = ErrorUtils::modelNotFoundInRegistry("test-model-id");
    
    EXPECT_EQ(error.code, ErrorCode::ModelNotFoundInRegistry);
    EXPECT_FALSE(error.message.empty());
    EXPECT_FALSE(error.details.empty());
    EXPECT_TRUE(error.recovery_suggestion.has_value());
    EXPECT_NE(error.message.find("test-model-id"), std::string::npos);
}

TEST_F(ErrorHandlingTest, ModelFileNotFoundHelper) {
    auto error = ErrorUtils::modelFileNotFound("/path/to/model.gguf");
    
    EXPECT_EQ(error.code, ErrorCode::ModelFileNotFound);
    EXPECT_NE(error.message.find("/path/to/model.gguf"), std::string::npos);
    EXPECT_TRUE(error.recovery_suggestion.has_value());
}

TEST_F(ErrorHandlingTest, ModelInsufficientMemoryHelper) {
    size_t required = 3 * 1024 * 1024 * 1024ULL; // 3 GB
    size_t available = 2 * 1024 * 1024 * 1024ULL; // 2 GB
    
    auto error = ErrorUtils::modelInsufficientMemory(required, available);
    
    EXPECT_EQ(error.code, ErrorCode::ModelInsufficientMemory);
    EXPECT_FALSE(error.message.empty());
    EXPECT_FALSE(error.details.empty());
    EXPECT_TRUE(error.recovery_suggestion.has_value());
}

TEST_F(ErrorHandlingTest, InferenceModelNotLoadedHelper) {
    ModelHandle handle = 12345;
    auto error = ErrorUtils::inferenceModelNotLoaded(handle);
    
    EXPECT_EQ(error.code, ErrorCode::InferenceModelNotLoaded);
    EXPECT_NE(error.message.find("12345"), std::string::npos);
    EXPECT_TRUE(error.recovery_suggestion.has_value());
}

TEST_F(ErrorHandlingTest, InferenceContextWindowExceededHelper) {
    auto error = ErrorUtils::inferenceContextWindowExceeded(5000, 4096);
    
    EXPECT_EQ(error.code, ErrorCode::InferenceContextWindowExceeded);
    EXPECT_NE(error.details.find("5000"), std::string::npos);
    EXPECT_NE(error.details.find("4096"), std::string::npos);
    EXPECT_TRUE(error.recovery_suggestion.has_value());
}

TEST_F(ErrorHandlingTest, NetworkUnreachableHelper) {
    auto error = ErrorUtils::networkUnreachable("example.com");
    
    EXPECT_EQ(error.code, ErrorCode::NetworkUnreachable);
    EXPECT_NE(error.message.find("example.com"), std::string::npos);
    EXPECT_TRUE(error.recovery_suggestion.has_value());
}

TEST_F(ErrorHandlingTest, NetworkHTTPErrorHelper) {
    auto error = ErrorUtils::networkHTTPError(404, "Not Found");
    
    EXPECT_EQ(error.code, ErrorCode::NetworkHTTPError);
    EXPECT_NE(error.message.find("404"), std::string::npos);
    EXPECT_TRUE(error.recovery_suggestion.has_value());
}

TEST_F(ErrorHandlingTest, StorageInsufficientSpaceHelper) {
    size_t required = 5 * 1024 * 1024 * 1024ULL; // 5 GB
    size_t available = 1 * 1024 * 1024 * 1024ULL; // 1 GB
    
    auto error = ErrorUtils::storageInsufficientSpace(required, available);
    
    EXPECT_EQ(error.code, ErrorCode::StorageInsufficientSpace);
    EXPECT_FALSE(error.details.empty());
    EXPECT_TRUE(error.recovery_suggestion.has_value());
}

TEST_F(ErrorHandlingTest, InvalidInputNullPointerHelper) {
    auto error = ErrorUtils::invalidInputNullPointer("callback");
    
    EXPECT_EQ(error.code, ErrorCode::InvalidInputNullPointer);
    EXPECT_NE(error.message.find("callback"), std::string::npos);
    EXPECT_TRUE(error.recovery_suggestion.has_value());
}

TEST_F(ErrorHandlingTest, InvalidInputModelHandleHelper) {
    ModelHandle handle = 999;
    auto error = ErrorUtils::invalidInputModelHandle(handle);
    
    EXPECT_EQ(error.code, ErrorCode::InvalidInputModelHandle);
    EXPECT_NE(error.message.find("999"), std::string::npos);
    EXPECT_TRUE(error.recovery_suggestion.has_value());
}

TEST_F(ErrorHandlingTest, ResourceOutOfMemoryHelper) {
    auto error = ErrorUtils::resourceOutOfMemory("model loading");
    
    EXPECT_EQ(error.code, ErrorCode::ResourceOutOfMemory);
    EXPECT_NE(error.message.find("model loading"), std::string::npos);
    EXPECT_TRUE(error.recovery_suggestion.has_value());
}

TEST_F(ErrorHandlingTest, OperationCancelledHelper) {
    auto error = ErrorUtils::operationCancelled("download");
    
    EXPECT_EQ(error.code, ErrorCode::OperationCancelled);
    EXPECT_NE(error.message.find("download"), std::string::npos);
    EXPECT_FALSE(error.recovery_suggestion.has_value()); // Cancellation doesn't need recovery
}

// ============================================================================
// Error Category Helper Tests
// ============================================================================

TEST_F(ErrorHandlingTest, GetErrorCategory) {
    EXPECT_EQ(ErrorUtils::getErrorCategory(ErrorCode::ModelFileNotFound), "ModelNotFound");
    EXPECT_EQ(ErrorUtils::getErrorCategory(ErrorCode::ModelFileCorrupted), "ModelLoadError");
    EXPECT_EQ(ErrorUtils::getErrorCategory(ErrorCode::InferenceModelNotLoaded), "InferenceError");
    EXPECT_EQ(ErrorUtils::getErrorCategory(ErrorCode::NetworkUnreachable), "NetworkError");
    EXPECT_EQ(ErrorUtils::getErrorCategory(ErrorCode::StorageInsufficientSpace), "StorageError");
    EXPECT_EQ(ErrorUtils::getErrorCategory(ErrorCode::InvalidInputNullPointer), "InvalidInput");
    EXPECT_EQ(ErrorUtils::getErrorCategory(ErrorCode::ResourceOutOfMemory), "ResourceExhausted");
    EXPECT_EQ(ErrorUtils::getErrorCategory(ErrorCode::OperationCancelled), "Cancelled");
}

TEST_F(ErrorHandlingTest, IsRetryable) {
    EXPECT_TRUE(ErrorUtils::isRetryable(ErrorCode::NetworkUnreachable));
    EXPECT_TRUE(ErrorUtils::isRetryable(ErrorCode::NetworkConnectionTimeout));
    EXPECT_TRUE(ErrorUtils::isRetryable(ErrorCode::NetworkDNSFailure));
    EXPECT_TRUE(ErrorUtils::isRetryable(ErrorCode::ResourceThreadPoolExhausted));
    
    EXPECT_FALSE(ErrorUtils::isRetryable(ErrorCode::ModelFileNotFound));
    EXPECT_FALSE(ErrorUtils::isRetryable(ErrorCode::InvalidInputNullPointer));
    EXPECT_FALSE(ErrorUtils::isRetryable(ErrorCode::OperationCancelled));
}

TEST_F(ErrorHandlingTest, IsNetworkError) {
    EXPECT_TRUE(ErrorUtils::isNetworkError(ErrorCode::NetworkUnreachable));
    EXPECT_TRUE(ErrorUtils::isNetworkError(ErrorCode::NetworkConnectionTimeout));
    EXPECT_TRUE(ErrorUtils::isNetworkError(ErrorCode::NetworkHTTPError));
    EXPECT_TRUE(ErrorUtils::isNetworkError(ErrorCode::NetworkDNSFailure));
    EXPECT_TRUE(ErrorUtils::isNetworkError(ErrorCode::NetworkSSLError));
    
    EXPECT_FALSE(ErrorUtils::isNetworkError(ErrorCode::ModelFileNotFound));
    EXPECT_FALSE(ErrorUtils::isNetworkError(ErrorCode::StorageInsufficientSpace));
}

TEST_F(ErrorHandlingTest, IsStorageError) {
    EXPECT_TRUE(ErrorUtils::isStorageError(ErrorCode::StorageInsufficientSpace));
    EXPECT_TRUE(ErrorUtils::isStorageError(ErrorCode::StoragePermissionDenied));
    EXPECT_TRUE(ErrorUtils::isStorageError(ErrorCode::StorageReadError));
    EXPECT_TRUE(ErrorUtils::isStorageError(ErrorCode::StorageWriteError));
    EXPECT_TRUE(ErrorUtils::isStorageError(ErrorCode::StorageDiskFull));
    
    EXPECT_FALSE(ErrorUtils::isStorageError(ErrorCode::NetworkUnreachable));
    EXPECT_FALSE(ErrorUtils::isStorageError(ErrorCode::InvalidInputNullPointer));
}

TEST_F(ErrorHandlingTest, IsValidationError) {
    EXPECT_TRUE(ErrorUtils::isValidationError(ErrorCode::InvalidInputNullPointer));
    EXPECT_TRUE(ErrorUtils::isValidationError(ErrorCode::InvalidInputParameterValue));
    EXPECT_TRUE(ErrorUtils::isValidationError(ErrorCode::InvalidInputConfiguration));
    EXPECT_TRUE(ErrorUtils::isValidationError(ErrorCode::InvalidInputAudioFormat));
    EXPECT_TRUE(ErrorUtils::isValidationError(ErrorCode::InvalidInputModelHandle));
    
    EXPECT_FALSE(ErrorUtils::isValidationError(ErrorCode::NetworkUnreachable));
    EXPECT_FALSE(ErrorUtils::isValidationError(ErrorCode::ModelFileNotFound));
}

// ============================================================================
// Error Message Quality Tests (Requirement 13.1)
// ============================================================================

TEST_F(ErrorHandlingTest, AllErrorsHaveNonEmptyMessages) {
    // Test that all error helper functions produce non-empty messages
    std::vector<Error> errors = {
        ErrorUtils::modelNotFoundInRegistry("test"),
        ErrorUtils::modelFileNotFound("test.gguf"),
        ErrorUtils::modelVersionNotAvailable("test", "1.0"),
        ErrorUtils::modelFileCorrupted("test.gguf"),
        ErrorUtils::modelIncompatibleArchitecture("GGUF", "ONNX"),
        ErrorUtils::modelInsufficientMemory(1000, 500),
        ErrorUtils::modelUnsupportedQuantization("Q2_K"),
        ErrorUtils::modelFileLocked("test.gguf"),
        ErrorUtils::inferenceModelNotLoaded(123),
        ErrorUtils::inferenceInvalidInput("test"),
        ErrorUtils::inferenceContextWindowExceeded(5000, 4096),
        ErrorUtils::inferenceTimeout(30),
        ErrorUtils::inferenceHardwareAccelerationFailure("test"),
        ErrorUtils::networkUnreachable("example.com"),
        ErrorUtils::networkConnectionTimeout("example.com"),
        ErrorUtils::networkHTTPError(404),
        ErrorUtils::networkDNSFailure("example.com"),
        ErrorUtils::networkSSLError("test"),
        ErrorUtils::storageInsufficientSpace(1000, 500),
        ErrorUtils::storagePermissionDenied("/path"),
        ErrorUtils::storageReadError("/path"),
        ErrorUtils::storageWriteError("/path"),
        ErrorUtils::storageDiskFull("/path"),
        ErrorUtils::invalidInputNullPointer("param"),
        ErrorUtils::invalidInputParameterValue("param", "details"),
        ErrorUtils::invalidInputConfiguration("details"),
        ErrorUtils::invalidInputAudioFormat("details"),
        ErrorUtils::invalidInputModelHandle(123),
        ErrorUtils::resourceOutOfMemory("operation"),
        ErrorUtils::resourceTooManyOpenFiles(),
        ErrorUtils::resourceThreadPoolExhausted(),
        ErrorUtils::resourceGPUMemoryExhausted("details"),
        ErrorUtils::operationCancelled("operation"),
        ErrorUtils::operationTimeout("operation", 30),
        ErrorUtils::operationInterrupted("operation")
    };
    
    for (const auto& error : errors) {
        EXPECT_FALSE(error.message.empty()) 
            << "Error code " << static_cast<int>(error.code) << " has empty message";
        EXPECT_FALSE(error.details.empty())
            << "Error code " << static_cast<int>(error.code) << " has empty details";
    }
}

// ============================================================================
// Error-Specific Failure Reasons Tests (Requirement 13.3)
// ============================================================================

TEST_F(ErrorHandlingTest, DifferentModelLoadFailuresHaveDifferentCodes) {
    auto not_found = ErrorUtils::modelFileNotFound("test.gguf");
    auto corrupted = ErrorUtils::modelFileCorrupted("test.gguf");
    auto insufficient_memory = ErrorUtils::modelInsufficientMemory(1000, 500);
    auto incompatible = ErrorUtils::modelIncompatibleArchitecture("GGUF", "ONNX");
    
    // All should have different error codes
    EXPECT_NE(not_found.code, corrupted.code);
    EXPECT_NE(not_found.code, insufficient_memory.code);
    EXPECT_NE(not_found.code, incompatible.code);
    EXPECT_NE(corrupted.code, insufficient_memory.code);
    EXPECT_NE(corrupted.code, incompatible.code);
    EXPECT_NE(insufficient_memory.code, incompatible.code);
    
    // All should have different messages
    EXPECT_NE(not_found.message, corrupted.message);
    EXPECT_NE(not_found.message, insufficient_memory.message);
    EXPECT_NE(not_found.message, incompatible.message);
}

TEST_F(ErrorHandlingTest, DifferentNetworkFailuresHaveDifferentCodes) {
    auto unreachable = ErrorUtils::networkUnreachable("example.com");
    auto timeout = ErrorUtils::networkConnectionTimeout("example.com");
    auto http_error = ErrorUtils::networkHTTPError(404);
    auto dns_failure = ErrorUtils::networkDNSFailure("example.com");
    
    // All should have different error codes
    EXPECT_NE(unreachable.code, timeout.code);
    EXPECT_NE(unreachable.code, http_error.code);
    EXPECT_NE(unreachable.code, dns_failure.code);
    EXPECT_NE(timeout.code, http_error.code);
    EXPECT_NE(timeout.code, dns_failure.code);
    EXPECT_NE(http_error.code, dns_failure.code);
}

// ============================================================================
// Recovery Suggestion Tests
// ============================================================================

TEST_F(ErrorHandlingTest, RecoverySuggestionsAreHelpful) {
    // Test that recovery suggestions provide actionable guidance
    auto insufficient_memory = ErrorUtils::modelInsufficientMemory(3000000000, 2000000000);
    ASSERT_TRUE(insufficient_memory.recovery_suggestion.has_value());
    EXPECT_FALSE(insufficient_memory.recovery_suggestion.value().empty());
    
    auto insufficient_storage = ErrorUtils::storageInsufficientSpace(5000000000, 1000000000);
    ASSERT_TRUE(insufficient_storage.recovery_suggestion.has_value());
    EXPECT_FALSE(insufficient_storage.recovery_suggestion.value().empty());
    
    auto model_not_found = ErrorUtils::modelFileNotFound("test.gguf");
    ASSERT_TRUE(model_not_found.recovery_suggestion.has_value());
    EXPECT_FALSE(model_not_found.recovery_suggestion.value().empty());
}

TEST_F(ErrorHandlingTest, CancelledOperationsNoRecoverySuggestion) {
    // Cancelled operations typically don't need recovery suggestions
    auto cancelled = ErrorUtils::operationCancelled("download");
    EXPECT_FALSE(cancelled.recovery_suggestion.has_value());
}

// ============================================================================
// Integration Tests with Result<T>
// ============================================================================

TEST_F(ErrorHandlingTest, ResultPropagation) {
    // Simulate a function that returns Result<int>
    auto divide = [](int a, int b) -> Result<int> {
        if (b == 0) {
            return Result<int>::failure(
                ErrorUtils::invalidInputParameterValue("divisor", "Cannot divide by zero")
            );
        }
        return Result<int>::success(a / b);
    };
    
    auto result1 = divide(10, 2);
    EXPECT_TRUE(result1.isSuccess());
    EXPECT_EQ(result1.value(), 5);
    
    auto result2 = divide(10, 0);
    EXPECT_TRUE(result2.isError());
    EXPECT_EQ(result2.error().code, ErrorCode::InvalidInputParameterValue);
}

TEST_F(ErrorHandlingTest, ResultChaining) {
    // Test error propagation through multiple operations
    auto step1 = []() -> Result<int> {
        return Result<int>::success(42);
    };
    
    auto step2 = [](int value) -> Result<std::string> {
        if (value < 0) {
            return Result<std::string>::failure(
                ErrorUtils::invalidInputParameterValue("value", "Must be non-negative")
            );
        }
        return Result<std::string>::success("Value: " + std::to_string(value));
    };
    
    auto result1 = step1();
    ASSERT_TRUE(result1.isSuccess());
    
    auto result2 = step2(result1.value());
    EXPECT_TRUE(result2.isSuccess());
    EXPECT_EQ(result2.value(), "Value: 42");
}
