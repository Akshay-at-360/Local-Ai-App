//
//  ODAIModelManager.mm
//  OnDeviceAI iOS SDK
//
//  Objective-C++ bridge implementation for ModelManager
//  Requirements: 7.1, 7.8, 22.5 (ARC integration)
//

#import "ODAIModelManager.h"
#import "ODAITypeConversions.h"
#include "ondeviceai/model_manager.hpp"

@implementation ODAIModelManager {
    // C++ model manager pointer (not owned, managed by SDK)
    ondeviceai::ModelManager* _cppModelManager;
}

- (instancetype)initWithCppManager:(void *)cppManager {
    self = [super init];
    if (self) {
        _cppModelManager = static_cast<ondeviceai::ModelManager*>(cppManager);
    }
    return self;
}

- (void)dealloc {
    // C++ manager is owned by SDK, not by this wrapper
    _cppModelManager = nullptr;
}

- (nullable NSArray<ODAIModelInfo *> *)listAvailableModelsWithType:(ODAIModelType)type
                                                             device:(ODAIDeviceCapabilities *)device
                                                              error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return nil;
    }
    
    ondeviceai::ModelType cppType = ODAIBridge::toCppModelType(type);
    ondeviceai::DeviceCapabilities cppDevice = ODAIBridge::toCppDeviceCapabilities(device);
    
    auto result = _cppModelManager->listAvailableModels(cppType, cppDevice);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    const auto& models = result.value();
    NSMutableArray<ODAIModelInfo *>* objcModels = [NSMutableArray arrayWithCapacity:models.size()];
    for (const auto& model : models) {
        [objcModels addObject:ODAIBridge::toODAIModelInfo(model)];
    }
    
    return [objcModels copy];
}

- (NSArray<ODAIModelInfo *> *)listDownloadedModels {
    if (!_cppModelManager) {
        return @[];
    }
    
    auto result = _cppModelManager->listDownloadedModels();
    
    if (result.isError()) {
        return @[];
    }
    
    const auto& models = result.value();
    NSMutableArray<ODAIModelInfo *>* objcModels = [NSMutableArray arrayWithCapacity:models.size()];
    for (const auto& model : models) {
        [objcModels addObject:ODAIBridge::toODAIModelInfo(model)];
    }
    
    return [objcModels copy];
}

- (uint64_t)downloadModel:(NSString *)modelId
                 progress:(nullable ODAIProgressCallback)progressCallback
                    error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return 0;
    }
    
    std::string cppModelId = ODAIBridge::toCppString(modelId);
    ondeviceai::ProgressCallback cppCallback = ODAIBridge::wrapProgressCallback(progressCallback);
    
    auto result = _cppModelManager->downloadModel(cppModelId, cppCallback);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return 0;
    }
    
    return result.value();
}

- (BOOL)cancelDownload:(uint64_t)handle error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return NO;
    }
    
    auto result = _cppModelManager->cancelDownload(handle);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return NO;
    }
    
    return YES;
}

- (BOOL)deleteModel:(NSString *)modelId error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return NO;
    }
    
    std::string cppModelId = ODAIBridge::toCppString(modelId);
    auto result = _cppModelManager->deleteModel(cppModelId);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return NO;
    }
    
    return YES;
}

- (nullable ODAIModelInfo *)getModelInfo:(NSString *)modelId error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return nil;
    }
    
    std::string cppModelId = ODAIBridge::toCppString(modelId);
    auto result = _cppModelManager->getModelInfo(cppModelId);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    return ODAIBridge::toODAIModelInfo(result.value());
}

- (BOOL)isModelDownloaded:(NSString *)modelId {
    if (!_cppModelManager) {
        return NO;
    }
    
    std::string cppModelId = ODAIBridge::toCppString(modelId);
    auto result = _cppModelManager->isModelDownloaded(cppModelId);
    
    if (result.isError()) {
        return NO;
    }
    
    return result.value();
}

- (nullable NSString *)getModelPath:(NSString *)modelId error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return nil;
    }
    
    std::string cppModelId = ODAIBridge::toCppString(modelId);
    auto result = _cppModelManager->getModelPath(cppModelId);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    return ODAIBridge::toNSString(result.value());
}

- (nullable NSArray<NSString *> *)getAvailableVersions:(NSString *)modelId error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return nil;
    }
    
    std::string cppModelId = ODAIBridge::toCppString(modelId);
    auto result = _cppModelManager->getAvailableVersions(cppModelId);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    return ODAIBridge::toNSArray(result.value());
}

- (nullable ODAIModelInfo *)checkForUpdates:(NSString *)modelId error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return nil;
    }
    
    std::string cppModelId = ODAIBridge::toCppString(modelId);
    auto result = _cppModelManager->checkForUpdates(cppModelId);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    return ODAIBridge::toODAIModelInfo(result.value());
}

- (BOOL)pinModelVersion:(NSString *)modelId version:(NSString *)version error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return NO;
    }
    
    std::string cppModelId = ODAIBridge::toCppString(modelId);
    std::string cppVersion = ODAIBridge::toCppString(version);
    auto result = _cppModelManager->pinModelVersion(cppModelId, cppVersion);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return NO;
    }
    
    return YES;
}

- (BOOL)unpinModelVersion:(NSString *)modelId error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return NO;
    }
    
    std::string cppModelId = ODAIBridge::toCppString(modelId);
    auto result = _cppModelManager->unpinModelVersion(cppModelId);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return NO;
    }
    
    return YES;
}

- (BOOL)isModelVersionPinned:(NSString *)modelId {
    if (!_cppModelManager) {
        return NO;
    }
    
    std::string cppModelId = ODAIBridge::toCppString(modelId);
    auto result = _cppModelManager->isModelVersionPinned(cppModelId);
    
    if (result.isError()) {
        return NO;
    }
    
    return result.value();
}

- (nullable ODAIStorageInfo *)getStorageInfo:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return nil;
    }
    
    auto result = _cppModelManager->getStorageInfo();
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    return ODAIBridge::toODAIStorageInfo(result.value());
}

- (BOOL)cleanupIncompleteDownloads:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return NO;
    }
    
    auto result = _cppModelManager->cleanupIncompleteDownloads();
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return NO;
    }
    
    return YES;
}

- (nullable NSArray<ODAIModelInfo *> *)recommendModelsWithType:(ODAIModelType)type
                                                         device:(ODAIDeviceCapabilities *)device
                                                          error:(NSError **)error {
    if (!_cppModelManager) {
        if (error) {
            *error = [NSError errorWithDomain:ODAIErrorDomain
                                         code:1501
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model manager not initialized"}];
        }
        return nil;
    }
    
    ondeviceai::ModelType cppType = ODAIBridge::toCppModelType(type);
    ondeviceai::DeviceCapabilities cppDevice = ODAIBridge::toCppDeviceCapabilities(device);
    
    auto result = _cppModelManager->recommendModels(cppType, cppDevice);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    const auto& models = result.value();
    NSMutableArray<ODAIModelInfo *>* objcModels = [NSMutableArray arrayWithCapacity:models.size()];
    for (const auto& model : models) {
        [objcModels addObject:ODAIBridge::toODAIModelInfo(model)];
    }
    
    return [objcModels copy];
}

@end
