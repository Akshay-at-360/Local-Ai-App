//
//  ODAISDKManager.h
//  OnDeviceAI iOS SDK
//
//  Objective-C++ bridge for SDKManager
//  Requirements: 7.1, 7.8
//

#import <Foundation/Foundation.h>
#import "OnDeviceAI-Bridging.h"

@class ODAIModelManager;
@class ODAILLMEngine;
@class ODAISTTEngine;
@class ODAITTSEngine;
@class ODAIVoicePipeline;

NS_ASSUME_NONNULL_BEGIN

/**
 * Main SDK manager class for OnDeviceAI iOS SDK
 * Manages initialization, configuration, and component access
 * Thread-safe singleton pattern with ARC memory management
 */
@interface ODAISDKManager : NSObject

/**
 * Initialize the SDK with configuration
 * @param config SDK configuration
 * @param error Error pointer for initialization failures
 * @return Initialized SDK manager instance or nil on error
 */
+ (nullable instancetype)initializeWithConfig:(ODAISDKConfig *)config error:(NSError **)error;

/**
 * Get the current SDK instance (must be initialized first)
 * @return Current SDK instance or nil if not initialized
 */
+ (nullable instancetype)getInstance;

/**
 * Shutdown the SDK and release all resources
 * After shutdown, initialize must be called again to use the SDK
 */
+ (void)shutdown;

/**
 * Set the number of threads for inference operations
 * @param count Number of threads (must be > 0)
 */
- (void)setThreadCount:(NSInteger)count;

/**
 * Set the model storage directory
 * @param path Directory path for model storage
 */
- (void)setModelDirectory:(NSString *)path;

/**
 * Set the logging level
 * @param level Log level
 */
- (void)setLogLevel:(ODAILogLevel)level;

/**
 * Set memory limit for the SDK
 * @param bytes Memory limit in bytes (0 = no limit)
 */
- (void)setMemoryLimit:(NSUInteger)bytes;

/**
 * Set the number of callback threads
 * @param count Number of callback threads
 */
- (void)setCallbackThreadCount:(NSInteger)count;

/**
 * Set whether callbacks should be synchronous
 * @param synchronous YES for synchronous callbacks, NO for asynchronous
 */
- (void)setSynchronousCallbacks:(BOOL)synchronous;

/**
 * Get the model manager component
 * @return Model manager instance
 */
- (ODAIModelManager *)modelManager;

/**
 * Get the LLM engine component
 * @return LLM engine instance
 */
- (ODAILLMEngine *)llmEngine;

/**
 * Get the STT engine component
 * @return STT engine instance
 */
- (ODAISTTEngine *)sttEngine;

/**
 * Get the TTS engine component
 * @return TTS engine instance
 */
- (ODAITTSEngine *)ttsEngine;

/**
 * Get the voice pipeline component
 * @return Voice pipeline instance
 */
- (ODAIVoicePipeline *)voicePipeline;

// Prevent direct instantiation
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
