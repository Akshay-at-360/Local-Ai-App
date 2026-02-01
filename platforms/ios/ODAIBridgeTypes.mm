//
//  ODAIBridgeTypes.mm
//  OnDeviceAI iOS SDK
//
//  Implementation of Objective-C bridge types
//  Requirements: 7.1, 7.8
//

#import "OnDeviceAI-Bridging.h"
#import "ODAITypeConversions.h"
#include "ondeviceai/types.hpp"
#include <sys/sysctl.h>
#include <mach/mach.h>

// Error domain
NSErrorDomain const ODAIErrorDomain = @"com.ondeviceai.error";

// MARK: - ODAISDKConfig

@implementation ODAISDKConfig

- (instancetype)init {
    self = [super init];
    if (self) {
        _threadCount = 4;
        _modelDirectory = @"";
        _logLevel = ODAILogLevelInfo;
        _memoryLimit = 0;
        _enableTelemetry = NO;
        _callbackThreadCount = 2;
        _synchronousCallbacks = NO;
    }
    return self;
}

+ (instancetype)defaultConfig {
    ODAISDKConfig* config = [[ODAISDKConfig alloc] init];
    
    // Set default model directory to app's documents directory
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDirectory = [paths firstObject];
    config.modelDirectory = [documentsDirectory stringByAppendingPathComponent:@"OnDeviceAI/models"];
    
    return config;
}

@end

// MARK: - ODAIDeviceCapabilities

@implementation ODAIDeviceCapabilities

+ (instancetype)current {
    ODAIDeviceCapabilities* caps = [[ODAIDeviceCapabilities alloc] init];
    
    // Get RAM size
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t memsize = 0;
    size_t length = sizeof(memsize);
    sysctl(mib, 2, &memsize, &length, NULL, 0);
    caps.ramBytes = memsize;
    
    // Get storage size
    NSError* error = nil;
    NSDictionary* attrs = [[NSFileManager defaultManager] attributesOfFileSystemForPath:NSHomeDirectory() error:&error];
    if (attrs) {
        caps.storageBytes = [[attrs objectForKey:NSFileSystemSize] unsignedLongLongValue];
    } else {
        caps.storageBytes = 0;
    }
    
    // Platform
    caps.platform = @"iOS";
    
    // Accelerators - iOS devices have Neural Engine and Metal
    NSMutableArray* accelerators = [NSMutableArray array];
    [accelerators addObject:@"Metal"];
    
    // Check for Neural Engine (available on A11 and later)
    // For simplicity, assume all modern iOS devices have it
    [accelerators addObject:@"Neural Engine"];
    
    caps.accelerators = [accelerators copy];
    
    return caps;
}

@end

// MARK: - ODAIDeviceRequirements

@implementation ODAIDeviceRequirements
@end

// MARK: - ODAIModelInfo

@implementation ODAIModelInfo
@end

// MARK: - ODAIStorageInfo

@implementation ODAIStorageInfo
@end

// MARK: - ODAIAudioData

@implementation ODAIAudioData

+ (instancetype)fromFile:(NSString *)path error:(NSError **)error {
    ondeviceai::AudioData cppAudio;
    auto result = ondeviceai::AudioData::fromFile(ODAIBridge::toCppString(path));
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    return ODAIBridge::toODAIAudioData(result.value());
}

+ (instancetype)fromWAVData:(NSData *)wavData error:(NSError **)error {
    std::vector<uint8_t> cppData((const uint8_t*)[wavData bytes], 
                                  (const uint8_t*)[wavData bytes] + [wavData length]);
    
    auto result = ondeviceai::AudioData::fromWAV(cppData);
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    return ODAIBridge::toODAIAudioData(result.value());
}

- (NSData *)toWAVWithBitsPerSample:(NSInteger)bitsPerSample error:(NSError **)error {
    ondeviceai::AudioData cppAudio = ODAIBridge::toCppAudioData(self);
    auto result = cppAudio.toWAV(static_cast<int>(bitsPerSample));
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    const auto& wavData = result.value();
    return [NSData dataWithBytes:wavData.data() length:wavData.size()];
}

- (instancetype)resampleToRate:(NSInteger)targetSampleRate error:(NSError **)error {
    ondeviceai::AudioData cppAudio = ODAIBridge::toCppAudioData(self);
    auto result = cppAudio.resample(static_cast<int>(targetSampleRate));
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    return ODAIBridge::toODAIAudioData(result.value());
}

- (instancetype)normalize:(NSError **)error {
    ondeviceai::AudioData cppAudio = ODAIBridge::toCppAudioData(self);
    auto result = cppAudio.normalize();
    
    if (result.isError()) {
        if (error) {
            *error = ODAIBridge::toNSError(result.error());
        }
        return nil;
    }
    
    return ODAIBridge::toODAIAudioData(result.value());
}

@end

// MARK: - ODAIAudioSegment

@implementation ODAIAudioSegment
@end

// MARK: - ODAIWord

@implementation ODAIWord
@end

// MARK: - ODAITranscription

@implementation ODAITranscription
@end

// MARK: - ODAIVoiceInfo

@implementation ODAIVoiceInfo
@end

// MARK: - ODAIGenerationConfig

@implementation ODAIGenerationConfig

- (instancetype)init {
    self = [super init];
    if (self) {
        _maxTokens = 512;
        _temperature = 0.7f;
        _topP = 0.9f;
        _topK = 40;
        _repetitionPenalty = 1.1f;
        _stopSequences = @[];
    }
    return self;
}

+ (instancetype)defaultConfig {
    return [[ODAIGenerationConfig alloc] init];
}

@end

// MARK: - ODAITranscriptionConfig

@implementation ODAITranscriptionConfig

- (instancetype)init {
    self = [super init];
    if (self) {
        _language = @"auto";
        _translateToEnglish = NO;
        _wordTimestamps = NO;
    }
    return self;
}

+ (instancetype)defaultConfig {
    return [[ODAITranscriptionConfig alloc] init];
}

@end

// MARK: - ODAISynthesisConfig

@implementation ODAISynthesisConfig

- (instancetype)init {
    self = [super init];
    if (self) {
        _voiceId = @"";
        _speed = 1.0f;
        _pitch = 1.0f;
    }
    return self;
}

+ (instancetype)defaultConfig {
    return [[ODAISynthesisConfig alloc] init];
}

@end

// MARK: - ODAIPipelineConfig

@implementation ODAIPipelineConfig

- (instancetype)init {
    self = [super init];
    if (self) {
        _llmConfig = [ODAIGenerationConfig defaultConfig];
        _sttConfig = [ODAITranscriptionConfig defaultConfig];
        _ttsConfig = [ODAISynthesisConfig defaultConfig];
        _enableVAD = YES;
        _vadThreshold = 0.5f;
    }
    return self;
}

+ (instancetype)defaultConfig {
    return [[ODAIPipelineConfig alloc] init];
}

@end

// MARK: - ODAIConversationTurn

@implementation ODAIConversationTurn
@end
