//
//  ODAILifecycleManager.h
//  OnDeviceAI iOS SDK
//
//  iOS lifecycle management for memory warnings and background transitions
//  Requirements: 22.1 (memory warnings), 22.5 (ARC integration)
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class ODAISDKManager;

/**
 * Manages iOS-specific lifecycle events for the SDK
 * Handles memory warnings and background/foreground transitions
 */
@interface ODAILifecycleManager : NSObject

/**
 * Initialize lifecycle manager with SDK manager reference
 * @param sdkManager The SDK manager to notify of lifecycle events
 * @return Initialized lifecycle manager
 */
- (instancetype)initWithSDKManager:(ODAISDKManager *)sdkManager;

/**
 * Start observing lifecycle events
 * Registers for memory warnings and app state notifications
 */
- (void)startObserving;

/**
 * Stop observing lifecycle events
 * Unregisters all notifications
 */
- (void)stopObserving;

/**
 * Enable or disable automatic model unloading on background
 * @param enabled YES to unload models when app goes to background, NO to keep them loaded
 */
- (void)setPauseInferenceOnBackground:(BOOL)enabled;

/**
 * Check if inference pausing on background is enabled
 * @return YES if enabled, NO otherwise
 */
- (BOOL)isPauseInferenceOnBackgroundEnabled;

// Prevent direct instantiation without SDK manager
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
