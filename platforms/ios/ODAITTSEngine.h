//
//  ODAITTSEngine.h
//  OnDeviceAI iOS SDK
//
//  Objective-C++ bridge for TTSEngine
//  Requirements: 7.1, 7.8
//

#import <Foundation/Foundation.h>
#import "OnDeviceAI-Bridging.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * TTS engine for text-to-speech synthesis
 * Supports speech synthesis with configurable voices and parameters
 */
@interface ODAITTSEngine : NSObject

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
 * Synthesize speech from text
 * @param handle Model handle
 * @param text Text to synthesize
 * @param config Synthesis configuration
 * @param error Error pointer for failures
 * @return Audio data or nil on error
 */
- (nullable ODAIAudioData *)synthesize:(uint64_t)handle
                                  text:(NSString *)text
                                config:(ODAISynthesisConfig *)config
                                 error:(NSError **)error;

/**
 * Get available voices for a model
 * @param handle Model handle
 * @param error Error pointer for failures
 * @return Array of voice info or nil on error
 */
- (nullable NSArray<ODAIVoiceInfo *> *)getAvailableVoices:(uint64_t)handle error:(NSError **)error;

// Internal initializer (not for public use)
- (instancetype)initWithCppEngine:(void *)cppEngine NS_DESIGNATED_INITIALIZER;

// Prevent direct instantiation
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
