//
//  ODAIModelManager.h
//  OnDeviceAI iOS SDK
//
//  Objective-C++ bridge for ModelManager
//  Requirements: 7.1, 7.8
//

#import <Foundation/Foundation.h>
#import "OnDeviceAI-Bridging.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * Model manager for discovering, downloading, and managing AI models
 * Handles model lifecycle including downloads, verification, and storage
 */
@interface ODAIModelManager : NSObject

/**
 * List available models from remote registry
 * @param type Model type filter (LLM, STT, TTS, or All)
 * @param device Device capabilities for filtering
 * @param error Error pointer for failures
 * @return Array of available models or nil on error
 */
- (nullable NSArray<ODAIModelInfo *> *)listAvailableModelsWithType:(ODAIModelType)type
                                                             device:(ODAIDeviceCapabilities *)device
                                                              error:(NSError **)error;

/**
 * List models that have been downloaded locally
 * @return Array of downloaded models
 */
- (NSArray<ODAIModelInfo *> *)listDownloadedModels;

/**
 * Download a model from the remote registry
 * @param modelId Model identifier
 * @param progressCallback Progress callback (0.0 to 1.0)
 * @param error Error pointer for failures
 * @return Download handle for tracking/cancellation, or 0 on error
 */
- (uint64_t)downloadModel:(NSString *)modelId
                 progress:(nullable ODAIProgressCallback)progressCallback
                    error:(NSError **)error;

/**
 * Cancel an ongoing download
 * @param handle Download handle from downloadModel
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)cancelDownload:(uint64_t)handle error:(NSError **)error;

/**
 * Delete a downloaded model
 * @param modelId Model identifier
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)deleteModel:(NSString *)modelId error:(NSError **)error;

/**
 * Get information about a specific model
 * @param modelId Model identifier
 * @param error Error pointer for failures
 * @return Model info or nil on error
 */
- (nullable ODAIModelInfo *)getModelInfo:(NSString *)modelId error:(NSError **)error;

/**
 * Check if a model is downloaded
 * @param modelId Model identifier
 * @return YES if downloaded, NO otherwise
 */
- (BOOL)isModelDownloaded:(NSString *)modelId;

/**
 * Get the local file path for a downloaded model
 * @param modelId Model identifier
 * @param error Error pointer for failures
 * @return File path or nil on error
 */
- (nullable NSString *)getModelPath:(NSString *)modelId error:(NSError **)error;

/**
 * Get available versions for a model
 * @param modelId Model identifier
 * @param error Error pointer for failures
 * @return Array of version strings or nil on error
 */
- (nullable NSArray<NSString *> *)getAvailableVersions:(NSString *)modelId error:(NSError **)error;

/**
 * Check for updates to a model
 * @param modelId Model identifier
 * @param error Error pointer for failures
 * @return Updated model info or nil if no update available/error
 */
- (nullable ODAIModelInfo *)checkForUpdates:(NSString *)modelId error:(NSError **)error;

/**
 * Pin a specific version of a model
 * @param modelId Model identifier
 * @param version Version string to pin
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)pinModelVersion:(NSString *)modelId version:(NSString *)version error:(NSError **)error;

/**
 * Unpin a model version
 * @param modelId Model identifier
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)unpinModelVersion:(NSString *)modelId error:(NSError **)error;

/**
 * Check if a model version is pinned
 * @param modelId Model identifier
 * @return YES if pinned, NO otherwise
 */
- (BOOL)isModelVersionPinned:(NSString *)modelId;

/**
 * Get storage information
 * @param error Error pointer for failures
 * @return Storage info or nil on error
 */
- (nullable ODAIStorageInfo *)getStorageInfo:(NSError **)error;

/**
 * Clean up incomplete downloads
 * @param error Error pointer for failures
 * @return YES on success, NO on error
 */
- (BOOL)cleanupIncompleteDownloads:(NSError **)error;

/**
 * Get recommended models for the current device
 * @param type Model type
 * @param device Device capabilities
 * @param error Error pointer for failures
 * @return Array of recommended models or nil on error
 */
- (nullable NSArray<ODAIModelInfo *> *)recommendModelsWithType:(ODAIModelType)type
                                                         device:(ODAIDeviceCapabilities *)device
                                                          error:(NSError **)error;

// Internal initializer (not for public use)
- (instancetype)initWithCppManager:(void *)cppManager NS_DESIGNATED_INITIALIZER;

// Prevent direct instantiation
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
