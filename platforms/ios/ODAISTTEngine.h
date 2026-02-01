//
//  ODAISTTEngine.h
//  OnDeviceAI iOS SDK
//
//  Objective-C++ bridge for STTEngine
//  Requirements: 7.1, 7.8
//

#import <Foundation/Foundation.h>
#import "OnDeviceAI-Bridging.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * STT engine for speech-to-text transcription
 * Supports audio transcription and voice activity detection
 */
@interface ODAISTTEngine : NSObject

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
 * Transcribe audio
 * @param handle Model handle
 * @param audio Audio data
 * @param config Transcription configuration
 * @param error Error pointer for failures
 * @return Transcription result or nil on error
 */
- (nullable ODAITranscription *)transcribe:(uint64_t)handle
                                     audio:(ODAIAudioData *)audio
                                    config:(ODAITranscriptionConfig *)config
                                     error:(NSError **)error;

/**
 * Detect voice activity in audio
 * @param audio Audio data
 * @param threshold VAD threshold (0.0 to 1.0)
 * @param error Error pointer for failures
 * @return Array of audio segments with speech or nil on error
 */
- (nullable NSArray<ODAIAudioSegment *> *)detectVoiceActivity:(ODAIAudioData *)audio
                                                     threshold:(float)threshold
                                                         error:(NSError **)error;

// Internal initializer (not for public use)
- (instancetype)initWithCppEngine:(void *)cppEngine NS_DESIGNATED_INITIALIZER;

// Prevent direct instantiation
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
