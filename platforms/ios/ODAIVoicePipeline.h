//
//  ODAIVoicePipeline.h
//  OnDeviceAI iOS SDK
//
//  Objective-C++ bridge for VoicePipeline
//  Requirements: 7.1, 7.8
//

#import <Foundation/Foundation.h>
#import "OnDeviceAI-Bridging.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * Voice pipeline for orchestrating STT → LLM → TTS conversations
 * Manages end-to-end voice conversation flow
 */
@interface ODAIVoicePipeline : NSObject

/**
 * Configure the pipeline with models
 * @param sttModel STT model handle
 * @param llmModel LLM model handle
 * @param ttsModel TTS model handle
 * @param config Pipeline configuration
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)configureSttModel:(uint64_t)sttModel
                 llmModel:(uint64_t)llmModel
                 ttsModel:(uint64_t)ttsModel
                   config:(ODAIPipelineConfig *)config
                    error:(NSError **)error;

/**
 * Start a conversation
 * @param audioInput Audio input callback
 * @param audioOutput Audio output callback
 * @param transcriptionCallback Transcription callback
 * @param llmResponseCallback LLM response callback
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)startConversationWithAudioInput:(ODAIAudioStreamCallback)audioInput
                            audioOutput:(ODAIAudioChunkCallback)audioOutput
                   transcriptionCallback:(ODAITranscriptionCallback)transcriptionCallback
                      llmResponseCallback:(ODAITextCallback)llmResponseCallback
                                    error:(NSError **)error;

/**
 * Stop the current conversation
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)stopConversation:(NSError **)error;

/**
 * Interrupt current TTS playback
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)interrupt:(NSError **)error;

/**
 * Clear conversation history
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)clearHistory:(NSError **)error;

/**
 * Get conversation history
 * @param error Error pointer for failures
 * @return Array of conversation turns or nil on error
 */
- (nullable NSArray<ODAIConversationTurn *> *)getHistory:(NSError **)error;

// Internal initializer (not for public use)
- (instancetype)initWithCppPipeline:(void *)cppPipeline NS_DESIGNATED_INITIALIZER;

// Prevent direct instantiation
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
