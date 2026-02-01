//
//  OnDeviceAI-Bridging.h
//  OnDeviceAI iOS SDK
//
//  Objective-C++ bridge header exposing C++ API to Objective-C
//  Requirements: 7.1, 7.8
//

#import <Foundation/Foundation.h>

// Forward declarations for Objective-C classes
@class ODAISDKManager;
@class ODAIModelManager;
@class ODAILLMEngine;
@class ODAISTTEngine;
@class ODAITTSEngine;
@class ODAIVoicePipeline;

// Error domain
extern NSErrorDomain const ODAIErrorDomain;

// Enums matching C++ enums
typedef NS_ENUM(NSInteger, ODAILogLevel) {
    ODAILogLevelDebug = 0,
    ODAILogLevelInfo = 1,
    ODAILogLevelWarning = 2,
    ODAILogLevelError = 3
};

typedef NS_ENUM(NSInteger, ODAIModelType) {
    ODAIModelTypeLLM = 0,
    ODAIModelTypeSTT = 1,
    ODAIModelTypeTTS = 2,
    ODAIModelTypeAll = 3
};

// Configuration classes
@interface ODAISDKConfig : NSObject
@property (nonatomic, assign) NSInteger threadCount;
@property (nonatomic, copy) NSString *modelDirectory;
@property (nonatomic, assign) ODAILogLevel logLevel;
@property (nonatomic, assign) NSUInteger memoryLimit;
@property (nonatomic, assign) BOOL enableTelemetry;
@property (nonatomic, assign) NSInteger callbackThreadCount;
@property (nonatomic, assign) BOOL synchronousCallbacks;

+ (instancetype)defaultConfig;
@end

@interface ODAIDeviceCapabilities : NSObject
@property (nonatomic, assign) NSUInteger ramBytes;
@property (nonatomic, assign) NSUInteger storageBytes;
@property (nonatomic, copy) NSString *platform;
@property (nonatomic, copy) NSArray<NSString *> *accelerators;

+ (instancetype)current;
@end

@interface ODAIDeviceRequirements : NSObject
@property (nonatomic, assign) NSUInteger minRamBytes;
@property (nonatomic, assign) NSUInteger minStorageBytes;
@property (nonatomic, copy) NSArray<NSString *> *supportedPlatforms;
@end

@interface ODAIModelInfo : NSObject
@property (nonatomic, copy) NSString *modelId;
@property (nonatomic, copy) NSString *name;
@property (nonatomic, assign) ODAIModelType type;
@property (nonatomic, copy) NSString *version;
@property (nonatomic, assign) NSUInteger sizeBytes;
@property (nonatomic, copy) NSString *downloadURL;
@property (nonatomic, copy) NSString *checksumSHA256;
@property (nonatomic, copy) NSDictionary<NSString *, NSString *> *metadata;
@property (nonatomic, strong) ODAIDeviceRequirements *requirements;
@end

@interface ODAIStorageInfo : NSObject
@property (nonatomic, assign) NSUInteger totalBytes;
@property (nonatomic, assign) NSUInteger availableBytes;
@property (nonatomic, assign) NSUInteger usedByModelsBytes;
@end

// Audio data structures
@interface ODAIAudioData : NSObject
@property (nonatomic, copy) NSArray<NSNumber *> *samples; // Float32 PCM samples
@property (nonatomic, assign) NSInteger sampleRate;

+ (instancetype)fromFile:(NSString *)path error:(NSError **)error;
+ (instancetype)fromWAVData:(NSData *)wavData error:(NSError **)error;
- (NSData *)toWAVWithBitsPerSample:(NSInteger)bitsPerSample error:(NSError **)error;
- (instancetype)resampleToRate:(NSInteger)targetSampleRate error:(NSError **)error;
- (instancetype)normalize:(NSError **)error;
@end

@interface ODAIAudioSegment : NSObject
@property (nonatomic, assign) float startTime;
@property (nonatomic, assign) float endTime;
@end

@interface ODAIWord : NSObject
@property (nonatomic, copy) NSString *text;
@property (nonatomic, assign) float startTime;
@property (nonatomic, assign) float endTime;
@property (nonatomic, assign) float confidence;
@end

@interface ODAITranscription : NSObject
@property (nonatomic, copy) NSString *text;
@property (nonatomic, assign) float confidence;
@property (nonatomic, copy) NSString *language;
@property (nonatomic, copy) NSArray<ODAIWord *> *words;
@end

@interface ODAIVoiceInfo : NSObject
@property (nonatomic, copy) NSString *voiceId;
@property (nonatomic, copy) NSString *name;
@property (nonatomic, copy) NSString *language;
@property (nonatomic, copy) NSString *gender;
@end

// Configuration structures
@interface ODAIGenerationConfig : NSObject
@property (nonatomic, assign) NSInteger maxTokens;
@property (nonatomic, assign) float temperature;
@property (nonatomic, assign) float topP;
@property (nonatomic, assign) NSInteger topK;
@property (nonatomic, assign) float repetitionPenalty;
@property (nonatomic, copy) NSArray<NSString *> *stopSequences;

+ (instancetype)defaultConfig;
@end

@interface ODAITranscriptionConfig : NSObject
@property (nonatomic, copy) NSString *language;
@property (nonatomic, assign) BOOL translateToEnglish;
@property (nonatomic, assign) BOOL wordTimestamps;

+ (instancetype)defaultConfig;
@end

@interface ODAISynthesisConfig : NSObject
@property (nonatomic, copy) NSString *voiceId;
@property (nonatomic, assign) float speed;
@property (nonatomic, assign) float pitch;

+ (instancetype)defaultConfig;
@end

@interface ODAIPipelineConfig : NSObject
@property (nonatomic, strong) ODAIGenerationConfig *llmConfig;
@property (nonatomic, strong) ODAITranscriptionConfig *sttConfig;
@property (nonatomic, strong) ODAISynthesisConfig *ttsConfig;
@property (nonatomic, assign) BOOL enableVAD;
@property (nonatomic, assign) float vadThreshold;

+ (instancetype)defaultConfig;
@end

@interface ODAIConversationTurn : NSObject
@property (nonatomic, copy) NSString *userText;
@property (nonatomic, copy) NSString *assistantText;
@property (nonatomic, assign) float timestamp;
@end

// Callback blocks
typedef void (^ODAIProgressCallback)(double progress);
typedef void (^ODAITokenCallback)(NSString *token);
typedef ODAIAudioData * _Nullable (^ODAIAudioStreamCallback)(void);
typedef void (^ODAIAudioChunkCallback)(ODAIAudioData *chunk);
typedef void (^ODAITranscriptionCallback)(NSString *text);
typedef void (^ODAITextCallback)(NSString *text);
