//
//  ODAILLMEngine.h
//  OnDeviceAI iOS SDK
//
//  Objective-C++ bridge for LLMEngine
//  Requirements: 7.1, 7.8
//

#import <Foundation/Foundation.h>
#import "OnDeviceAI-Bridging.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * LLM engine for language model inference
 * Supports model loading, text generation, and context management
 */
@interface ODAILLMEngine : NSObject

/**
 * Load a model from file path
 * @param path Model file path
 * @param error Error pointer for failures
 * @return Model handle or 0 on error
 */
- (uint64_t)loadModel:(NSString *)path error:(NSError **)error;

/**
 * Unload a model
 * @param handle Model handle
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)unloadModel:(uint64_t)handle error:(NSError **)error;

/**
 * Check if a model is loaded
 * @param handle Model handle
 * @return YES if loaded, NO otherwise
 */
- (BOOL)isModelLoaded:(uint64_t)handle;

/**
 * Generate text synchronously
 * @param handle Model handle
 * @param prompt Input prompt
 * @param config Generation configuration
 * @param error Error pointer for failures
 * @return Generated text or nil on error
 */
- (nullable NSString *)generate:(uint64_t)handle
                         prompt:(NSString *)prompt
                         config:(ODAIGenerationConfig *)config
                          error:(NSError **)error;

/**
 * Generate text with streaming callbacks
 * @param handle Model handle
 * @param prompt Input prompt
 * @param callback Token callback (called for each generated token)
 * @param config Generation configuration
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)generateStreaming:(uint64_t)handle
                   prompt:(NSString *)prompt
                 callback:(ODAITokenCallback)callback
                   config:(ODAIGenerationConfig *)config
                    error:(NSError **)error;

/**
 * Clear conversation context
 * @param handle Model handle
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)clearContext:(uint64_t)handle error:(NSError **)error;

/**
 * Get conversation history
 * @param handle Model handle
 * @param error Error pointer for failures
 * @return Array of conversation turns or nil on error
 */
- (nullable NSArray<NSString *> *)getConversationHistory:(uint64_t)handle error:(NSError **)error;

/**
 * Tokenize text
 * @param handle Model handle
 * @param text Text to tokenize
 * @param error Error pointer for failures
 * @return Array of token IDs or nil on error
 */
- (nullable NSArray<NSNumber *> *)tokenize:(uint64_t)handle text:(NSString *)text error:(NSError **)error;

/**
 * Detokenize token IDs
 * @param handle Model handle
 * @param tokens Array of token IDs
 * @param error Error pointer for failures
 * @return Detokenized text or nil on error
 */
- (nullable NSString *)detokenize:(uint64_t)handle tokens:(NSArray<NSNumber *> *)tokens error:(NSError **)error;

// Internal initializer (not for public use)
- (instancetype)initWithCppEngine:(void *)cppEngine NS_DESIGNATED_INITIALIZER;

// Prevent direct instantiation
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
