/**
 * OnDeviceAIModule.mm
 * React Native iOS Native Module — Objective-C++ bridge to C++ core.
 *
 * This module exposes the C++ SDK to React Native's JavaScript thread
 * via RCTBridgeModule.
 *
 * Requirements: 7.3, 7.8
 */

#import <React/RCTBridgeModule.h>
#import <React/RCTEventEmitter.h>
#include "ondeviceai/sdk_manager.hpp"
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/stt_engine.hpp"
#include "ondeviceai/tts_engine.hpp"
#include "ondeviceai/voice_pipeline.hpp"
#include "ondeviceai/memory_manager.hpp"
#include "ondeviceai/model_manager.hpp"
#include "ondeviceai/types.hpp"
#include <string>

using namespace ondeviceai;

// ---------------------------------------------------------------------------
@interface OnDeviceAIModule : RCTEventEmitter <RCTBridgeModule>
@end

@implementation OnDeviceAIModule

RCT_EXPORT_MODULE(OnDeviceAI);

+ (BOOL)requiresMainQueueSetup { return NO; }

- (NSArray<NSString *> *)supportedEvents {
    return @[
        @"ondeviceai_token",
        @"ondeviceai_transcription",
        @"ondeviceai_synthesis_done",
        @"ondeviceai_memory_warning",
        @"ondeviceai_download_progress",
        @"ondeviceai_error"
    ];
}

// ---------------------------------------------------------------------------
// SDK lifecycle
// ---------------------------------------------------------------------------

RCT_EXPORT_METHOD(initialize:(NSString *)configJson
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        // Parse JSON config (minimal for now; production would use NSJSONSerialization)
        SDKConfig config;
        config.thread_count = 2;
        config.memory_limit = 500 * 1024 * 1024;
        config.log_level    = LogLevel::Info;

        // If the caller sent a model directory, pass it through
        NSData *data = [configJson dataUsingEncoding:NSUTF8StringEncoding];
        if (data) {
            NSError *err = nil;
            NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:data
                                                                options:0
                                                                  error:&err];
            if (dict[@"threadCount"])
                config.thread_count = [dict[@"threadCount"] intValue];
            if (dict[@"modelDirectory"])
                config.model_directory = [dict[@"modelDirectory"] UTF8String];
            if (dict[@"memoryLimitMB"])
                config.memory_limit = [dict[@"memoryLimitMB"] longLongValue] * 1024 * 1024;
        }

        auto result = SDKManager::initialize(config);
        if (result.isError()) {
            reject(@"ERR_INIT", @(result.error().message.c_str()), nil);
        } else {
            resolve(nil);
        }
    });
}

RCT_EXPORT_METHOD(shutdown:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    SDKManager::shutdown();
    resolve(nil);
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

RCT_EXPORT_METHOD(setThreadCount:(int)count
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
    mgr->setThreadCount(count);
    resolve(nil);
}

RCT_EXPORT_METHOD(setLogLevel:(int)level
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
    mgr->setLogLevel(static_cast<LogLevel>(level));
    resolve(nil);
}

RCT_EXPORT_METHOD(setMemoryLimit:(double)bytes
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
    mgr->setMemoryLimit(static_cast<size_t>(bytes));
    resolve(nil);
}

RCT_EXPORT_METHOD(setCallbackThreadCount:(int)count
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
    mgr->setCallbackThreadCount(count);
    resolve(nil);
}

// ---------------------------------------------------------------------------
// LLM
// ---------------------------------------------------------------------------

RCT_EXPORT_METHOD(llmLoadModel:(NSString *)path
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        auto *mgr = SDKManager::getInstance();
        if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
        auto result = mgr->getLLMEngine()->loadModel([path UTF8String]);
        if (result.isError()) {
            reject(@"ERR_LLM_LOAD", @(result.error().message.c_str()), nil);
        } else {
            resolve(@(static_cast<double>(result.value())));
        }
    });
}

RCT_EXPORT_METHOD(llmUnloadModel:(double)handle
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
    auto result = mgr->getLLMEngine()->unloadModel(static_cast<ModelHandle>(handle));
    if (result.isError()) {
        reject(@"ERR_LLM_UNLOAD", @(result.error().message.c_str()), nil);
    } else {
        resolve(nil);
    }
}

RCT_EXPORT_METHOD(llmGenerate:(double)handle
                  prompt:(NSString *)prompt
                  configJson:(NSString *)configJson
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        auto *mgr = SDKManager::getInstance();
        if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }

        GenerationConfig config;
        // Parse configJson for temperature/topP/maxTokens
        NSData *data = [configJson dataUsingEncoding:NSUTF8StringEncoding];
        if (data) {
            NSDictionary *d = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            if (d[@"temperature"]) config.temperature = [d[@"temperature"] floatValue];
            if (d[@"topP"]) config.top_p = [d[@"topP"] floatValue];
            if (d[@"maxTokens"]) config.max_tokens = [d[@"maxTokens"] intValue];
        }

        auto result = mgr->getLLMEngine()->generate(
            static_cast<ModelHandle>(handle), [prompt UTF8String], config);
        if (result.isError()) {
            reject(@"ERR_LLM_GEN", @(result.error().message.c_str()), nil);
        } else {
            resolve(@(result.value().c_str()));
        }
    });
}

RCT_EXPORT_METHOD(llmGenerateStreaming:(double)handle
                  prompt:(NSString *)prompt
                  configJson:(NSString *)configJson
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        auto *mgr = SDKManager::getInstance();
        if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }

        GenerationConfig config;
        NSData *data = [configJson dataUsingEncoding:NSUTF8StringEncoding];
        if (data) {
            NSDictionary *d = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            if (d[@"temperature"]) config.temperature = [d[@"temperature"] floatValue];
            if (d[@"topP"]) config.top_p = [d[@"topP"] floatValue];
            if (d[@"maxTokens"]) config.max_tokens = [d[@"maxTokens"] intValue];
        }

        auto result = mgr->getLLMEngine()->generateStreaming(
            static_cast<ModelHandle>(handle), [prompt UTF8String], config,
            [self](const std::string& token) -> bool {
                [self sendEventWithName:@"ondeviceai_token"
                                   body:@{@"token": @(token.c_str())}];
                return true;
            });

        if (result.isError()) {
            reject(@"ERR_LLM_STREAM", @(result.error().message.c_str()), nil);
        } else {
            resolve(nil);
        }
    });
}

// ---------------------------------------------------------------------------
// STT
// ---------------------------------------------------------------------------

RCT_EXPORT_METHOD(sttLoadModel:(NSString *)path
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        auto *mgr = SDKManager::getInstance();
        if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
        auto result = mgr->getSTTEngine()->loadModel([path UTF8String]);
        if (result.isError()) {
            reject(@"ERR_STT_LOAD", @(result.error().message.c_str()), nil);
        } else {
            resolve(@(static_cast<double>(result.value())));
        }
    });
}

RCT_EXPORT_METHOD(sttUnloadModel:(double)handle
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
    mgr->getSTTEngine()->unloadModel(static_cast<ModelHandle>(handle));
    resolve(nil);
}

RCT_EXPORT_METHOD(sttTranscribe:(double)handle
                  audioUri:(NSString *)audioUri
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        auto *mgr = SDKManager::getInstance();
        if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }

        // Load audio from URI — simplified; production would use AVAudioFile
        AudioData audio;
        audio.sample_rate = 16000;
        audio.channels    = 1;
        // TODO: decode audio file at audioUri into PCM samples

        TranscriptionConfig tCfg;
        auto result = mgr->getSTTEngine()->transcribe(
            static_cast<ModelHandle>(handle), audio, tCfg);
        if (result.isError()) {
            reject(@"ERR_STT", @(result.error().message.c_str()), nil);
        } else {
            resolve(@(result.value().text.c_str()));
        }
    });
}

// ---------------------------------------------------------------------------
// TTS
// ---------------------------------------------------------------------------

RCT_EXPORT_METHOD(ttsLoadModel:(NSString *)path
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        auto *mgr = SDKManager::getInstance();
        if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
        auto result = mgr->getTTSEngine()->loadModel([path UTF8String]);
        if (result.isError()) {
            reject(@"ERR_TTS_LOAD", @(result.error().message.c_str()), nil);
        } else {
            resolve(@(static_cast<double>(result.value())));
        }
    });
}

RCT_EXPORT_METHOD(ttsUnloadModel:(double)handle
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
    mgr->getTTSEngine()->unloadModel(static_cast<ModelHandle>(handle));
    resolve(nil);
}

RCT_EXPORT_METHOD(ttsSynthesize:(double)handle
                  text:(NSString *)text
                  configJson:(NSString *)configJson
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        auto *mgr = SDKManager::getInstance();
        if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }

        SynthesisConfig sCfg;
        NSData *data = [configJson dataUsingEncoding:NSUTF8StringEncoding];
        if (data) {
            NSDictionary *d = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            if (d[@"speed"]) sCfg.speed = [d[@"speed"] floatValue];
            if (d[@"pitch"]) sCfg.pitch = [d[@"pitch"] floatValue];
        }

        auto result = mgr->getTTSEngine()->synthesize(
            static_cast<ModelHandle>(handle), [text UTF8String], sCfg);
        if (result.isError()) {
            reject(@"ERR_TTS", @(result.error().message.c_str()), nil);
        } else {
            // Write audio to temp file and return URI
            NSString *tmpDir = NSTemporaryDirectory();
            NSString *filePath = [tmpDir stringByAppendingPathComponent:
                [NSString stringWithFormat:@"tts_%u.wav", arc4random()]];
            // TODO: write WAV header + PCM samples to filePath
            resolve(filePath);
        }
    });
}

// ---------------------------------------------------------------------------
// Voice Pipeline
// ---------------------------------------------------------------------------

RCT_EXPORT_METHOD(vpConfigure:(NSString *)sttPath
                  llmPath:(NSString *)llmPath
                  ttsPath:(NSString *)ttsPath
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }

    VoicePipelineConfig vpCfg;
    vpCfg.stt_model_path = [sttPath UTF8String];
    vpCfg.llm_model_path = [llmPath UTF8String];
    vpCfg.tts_model_path = [ttsPath UTF8String];

    auto result = mgr->getVoicePipeline()->configure(vpCfg);
    if (result.isError()) {
        reject(@"ERR_VP", @(result.error().message.c_str()), nil);
    } else {
        resolve(nil);
    }
}

RCT_EXPORT_METHOD(vpStop:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
    mgr->getVoicePipeline()->stopConversation();
    resolve(nil);
}

// ---------------------------------------------------------------------------
// Memory helpers
// ---------------------------------------------------------------------------

RCT_EXPORT_METHOD(getMemoryUsage:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { resolve(@(0)); return; }
    resolve(@(static_cast<double>(mgr->getMemoryManager()->getCurrentUsage())));
}

RCT_EXPORT_METHOD(getMemoryLimit:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { resolve(@(0)); return; }
    resolve(@(static_cast<double>(mgr->getMemoryManager()->getMemoryLimit())));
}

// ---------------------------------------------------------------------------
// Model management
// ---------------------------------------------------------------------------

RCT_EXPORT_METHOD(listAvailableModels:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        auto *mgr = SDKManager::getInstance();
        if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
        // Return JSON string of models
        resolve(@"[]"); // Placeholder — full implementation serializes from C++ ModelManager
    });
}

RCT_EXPORT_METHOD(listDownloadedModels:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        auto *mgr = SDKManager::getInstance();
        if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
        resolve(@"[]"); // Placeholder
    });
}

RCT_EXPORT_METHOD(downloadModel:(NSString *)modelId
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        auto *mgr = SDKManager::getInstance();
        if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
        // Trigger download through C++ ModelManager
        resolve(@"{}"); // Placeholder
    });
}

RCT_EXPORT_METHOD(deleteModel:(NSString *)modelId
                  resolve:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
    resolve(nil); // Placeholder
}

RCT_EXPORT_METHOD(getStorageInfo:(RCTPromiseResolveBlock)resolve
                  reject:(RCTPromiseRejectBlock)reject)
{
    auto *mgr = SDKManager::getInstance();
    if (!mgr) { reject(@"ERR", @"SDK not initialized", nil); return; }
    resolve(@"{\"totalBytes\":0,\"usedBytes\":0,\"availableBytes\":0}"); // Placeholder
}

@end
