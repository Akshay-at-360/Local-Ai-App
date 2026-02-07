//
//  ODAILifecycleManager.mm
//  OnDeviceAI iOS SDK
//
//  iOS lifecycle management for memory warnings and background transitions
//  Requirements: 22.1 (memory warnings), 22.5 (ARC integration)
//

#import "ODAILifecycleManager.h"
#import "ODAISDKManager.h"
#import "ODAILLMEngine.h"
#import "ODAISTTEngine.h"
#import "ODAITTSEngine.h"
#import "ODAIVoicePipeline.h"
#import <UIKit/UIKit.h>
#import <os/log.h>

// Logging helper
static os_log_t lifecycle_log = os_log_create("com.ondeviceai.ios", "lifecycle");
#define LOG_INFO(format, ...) os_log(lifecycle_log, "%{public}@", [NSString stringWithFormat:format, ##__VA_ARGS__])
#define LOG_ERROR(format, ...) os_log_error(lifecycle_log, "%{public}@", [NSString stringWithFormat:format, ##__VA_ARGS__])

@implementation ODAILifecycleManager {
    ODAISDKManager *_sdkManager;
    BOOL _pauseInferenceOnBackground;
    BOOL _isInBackground;
    BOOL _isObserving;
}

- (instancetype)initWithSDKManager:(ODAISDKManager *)sdkManager {
    NSParameterAssert(sdkManager);
    
    self = [super init];
    if (self) {
        _sdkManager = sdkManager;
        _pauseInferenceOnBackground = NO;
        _isInBackground = NO;
        _isObserving = NO;
    }
    return self;
}

- (void)dealloc {
    [self stopObserving];
}

- (void)startObserving {
    if (_isObserving) {
        return;
    }
    
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    
    // Memory warning notification
    [notificationCenter addObserver:self
                           selector:@selector(didReceiveMemoryWarning)
                               name:UIApplicationDidReceiveMemoryWarningNotification
                             object:nil];
    
    // Background/Foreground notifications
    [notificationCenter addObserver:self
                           selector:@selector(didEnterBackground)
                               name:UIApplicationDidEnterBackgroundNotification
                             object:nil];
    
    [notificationCenter addObserver:self
                           selector:@selector(willEnterForeground)
                               name:UIApplicationWillEnterForegroundNotification
                             object:nil];
    
    [notificationCenter addObserver:self
                           selector:@selector(didFinishLaunching)
                               name:UIApplicationDidFinishLaunchingNotification
                             object:nil];
    
    _isObserving = YES;
    LOG_INFO(@"Lifecycle observer started");
}

- (void)stopObserving {
    if (!_isObserving) {
        return;
    }
    
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    [notificationCenter removeObserver:self];
    
    _isObserving = NO;
    LOG_INFO(@"Lifecycle observer stopped");
}

- (void)setPauseInferenceOnBackground:(BOOL)enabled {
    _pauseInferenceOnBackground = enabled;
    LOG_INFO(@"Pause inference on background: %@", enabled ? @"enabled" : @"disabled");
}

- (BOOL)isPauseInferenceOnBackgroundEnabled {
    return _pauseInferenceOnBackground;
}

#pragma mark - Lifecycle Event Handlers

- (void)didReceiveMemoryWarning {
    LOG_ERROR(@"Memory warning received");
    
    // Attempt to reduce memory usage
    [self handleMemoryWarning];
}

- (void)didEnterBackground {
    LOG_INFO(@"App entered background");
    _isInBackground = YES;
    
    if (_pauseInferenceOnBackground) {
        [self pauseAllInference];
    }
}

- (void)willEnterForeground {
    LOG_INFO(@"App entering foreground");
    _isInBackground = NO;
    
    if (_pauseInferenceOnBackground) {
        [self resumeAllInference];
    }
}

- (void)didFinishLaunching {
    LOG_INFO(@"App finished launching");
    // Any special setup on app launch
}

#pragma mark - Memory Management

- (void)handleMemoryWarning {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        if (!_sdkManager) return;
        
        // Try to unload models to free memory
        BOOL didFreeMemory = [self unloadUnusedModels];
        
        if (didFreeMemory) {
            LOG_INFO(@"Freed memory by unloading unused models");
        } else {
            LOG_ERROR(@"Unable to free more memory");
        }
    });
}

- (BOOL)unloadUnusedModels {
    if (!_sdkManager) return NO;
    
    // Get the memory manager to find candidates for eviction
    // This is handled through the C++ API which tracks LRU models
    
    // For now, we implement a simple strategy:
    // Try to unload all currently loaded models except those actively in use
    
    // Get all engines
    ODAILLMEngine *llmEngine = _sdkManager.llmEngine;
    ODAISTTEngine *sttEngine = _sdkManager.sttEngine;
    ODAITTSEngine *ttsEngine = _sdkManager.ttsEngine;
    
    BOOL didUnload = NO;
    
    // In a real implementation, we would track which models are loaded
    // and unload the least recently used ones
    // For now, we provide the capability for implementing this
    
    // The Swift layer will have better access to tracked models
    
    return didUnload;
}

- (void)pauseAllInference {
    LOG_INFO(@"Pausing all inference");
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        if (!_sdkManager) return;
        
        // Note: This is a placeholder for pause logic
        // The actual pause mechanism would depend on how the C++ SDK
        // manages inference state. For now, we log this event
        // and can extend it in the future.
    });
}

- (void)resumeAllInference {
    LOG_INFO(@"Resuming all inference");
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        if (!_sdkManager) return;
        
        // Placeholder for resume logic
    });
}

@end
