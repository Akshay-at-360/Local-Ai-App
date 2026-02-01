//
//  ODAISDKManager.mm
//  OnDeviceAI iOS SDK
//
//  Objective-C++ bridge implementation for SDKManager
//  Requirements: 7.1, 7.8, 22.5 (ARC integration)
//

#import "ODAISDKManager.h"
#import "ODAITypeConversions.h"
#import "ODAIModelManager.h"
#import "ODAILLMEngine.h"
#import "ODAISTTEngine.h"
#import "ODAITTSEngine.h"
#import "ODAIVoicePipeline.h"
#include "ondeviceai/sdk_manager.hpp"

@implementation ODAISDKManager {
    // C++ SDK manager pointer (not owned, managed by C++ singleton)
    ondeviceai::SDKManager* _cppSDKManager;
    
    // Objective-C wrapper instances (owned by ARC)
    ODAIModelManager* _modelManager;
    ODAILLMEngine* _llmEngine;
    ODAISTTEngine* _sttEngine;
    ODAITTSEngine* _ttsEngine;
    ODAIVoicePipeline* _voicePipeline;
}

static ODAISDKManager* _sharedInstance = nil;
static dispatch_once_t _onceToken;

+ (nullable instancetype)initializeWithConfig:(ODAISDKConfig *)config error:(NSError **)error {
    __block ODAISDKManager* instance = nil;
    __block NSError* initError = nil;
    
    dispatch_once(&_onceToken, ^{
        // Convert Objective-C config to C++
        ondeviceai::SDKConfig cppConfig = ODAIBridge::toCppSDKConfig(config);
        
        // Initialize C++ SDK
        auto result = ondeviceai::SDKManager::initialize(cppConfig);
        
        if (result.isError()) {
            initError = ODAIBridge::toNSError(result.error());
            return;
        }
        
        // Create Objective-C wrapper
        instance = [[ODAISDKManager alloc] initWithCppManager:result.value()];
        _sharedInstance = instance;
    });
    
    if (initError && error) {
        *error = initError;
    }
    
    return instance;
}

+ (nullable instancetype)getInstance {
    return _sharedInstance;
}

+ (void)shutdown {
    if (_sharedInstance) {
        // Shutdown C++ SDK
        ondeviceai::SDKManager::shutdown();
        
        // Clear Objective-C wrapper (ARC will handle deallocation)
        _sharedInstance = nil;
        
        // Reset once token to allow re-initialization
        _onceToken = 0;
    }
}

- (instancetype)initWithCppManager:(ondeviceai::SDKManager*)cppManager {
    self = [super init];
    if (self) {
        _cppSDKManager = cppManager;
        
        // Create wrapper instances for components
        // These are lazily created on first access
        _modelManager = nil;
        _llmEngine = nil;
        _sttEngine = nil;
        _ttsEngine = nil;
        _voicePipeline = nil;
    }
    return self;
}

- (void)dealloc {
    // ARC will automatically release all Objective-C objects
    // C++ SDK manager is managed by its own singleton, not owned by this wrapper
    _cppSDKManager = nullptr;
}

- (void)setThreadCount:(NSInteger)count {
    if (_cppSDKManager) {
        _cppSDKManager->setThreadCount(static_cast<int>(count));
    }
}

- (void)setModelDirectory:(NSString *)path {
    if (_cppSDKManager) {
        _cppSDKManager->setModelDirectory(ODAIBridge::toCppString(path));
    }
}

- (void)setLogLevel:(ODAILogLevel)level {
    if (_cppSDKManager) {
        _cppSDKManager->setLogLevel(ODAIBridge::toCppLogLevel(level));
    }
}

- (void)setMemoryLimit:(NSUInteger)bytes {
    if (_cppSDKManager) {
        _cppSDKManager->setMemoryLimit(bytes);
    }
}

- (void)setCallbackThreadCount:(NSInteger)count {
    if (_cppSDKManager) {
        _cppSDKManager->setCallbackThreadCount(static_cast<int>(count));
    }
}

- (void)setSynchronousCallbacks:(BOOL)synchronous {
    if (_cppSDKManager) {
        _cppSDKManager->setSynchronousCallbacks(synchronous);
    }
}

- (ODAIModelManager *)modelManager {
    if (!_modelManager && _cppSDKManager) {
        _modelManager = [[ODAIModelManager alloc] initWithCppManager:_cppSDKManager->getModelManager()];
    }
    return _modelManager;
}

- (ODAILLMEngine *)llmEngine {
    if (!_llmEngine && _cppSDKManager) {
        _llmEngine = [[ODAILLMEngine alloc] initWithCppEngine:_cppSDKManager->getLLMEngine()];
    }
    return _llmEngine;
}

- (ODAISTTEngine *)sttEngine {
    if (!_sttEngine && _cppSDKManager) {
        _sttEngine = [[ODAISTTEngine alloc] initWithCppEngine:_cppSDKManager->getSTTEngine()];
    }
    return _sttEngine;
}

- (ODAITTSEngine *)ttsEngine {
    if (!_ttsEngine && _cppSDKManager) {
        _ttsEngine = [[ODAITTSEngine alloc] initWithCppEngine:_cppSDKManager->getTTSEngine()];
    }
    return _ttsEngine;
}

- (ODAIVoicePipeline *)voicePipeline {
    if (!_voicePipeline && _cppSDKManager) {
        _voicePipeline = [[ODAIVoicePipeline alloc] initWithCppPipeline:_cppSDKManager->getVoicePipeline()];
    }
    return _voicePipeline;
}

@end
