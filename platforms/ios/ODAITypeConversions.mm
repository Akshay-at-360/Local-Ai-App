//
//  ODAITypeConversions.mm
//  OnDeviceAI iOS SDK
//
//  Type conversion implementations between C++ and Objective-C
//  Requirements: 7.1, 7.8
//

#import "ODAITypeConversions.h"

namespace ODAIBridge {

// MARK: - C++ to Objective-C Conversions

NSString* toNSString(const std::string& str) {
    if (str.empty()) {
        return @"";
    }
    return [NSString stringWithUTF8String:str.c_str()];
}

template<typename T>
NSArray* toNSArray(const std::vector<T>& vec) {
    NSMutableArray* array = [NSMutableArray arrayWithCapacity:vec.size()];
    for (const auto& item : vec) {
        [array addObject:@(item)];
    }
    return [array copy];
}

// Specialization for string vectors
template<>
NSArray* toNSArray<std::string>(const std::vector<std::string>& vec) {
    NSMutableArray* array = [NSMutableArray arrayWithCapacity:vec.size()];
    for (const auto& item : vec) {
        [array addObject:toNSString(item)];
    }
    return [array copy];
}

NSDictionary<NSString*, NSString*>* toNSDictionary(const std::map<std::string, std::string>& map) {
    NSMutableDictionary* dict = [NSMutableDictionary dictionaryWithCapacity:map.size()];
    for (const auto& [key, value] : map) {
        dict[toNSString(key)] = toNSString(value);
    }
    return [dict copy];
}

NSError* toNSError(const ondeviceai::Error& error) {
    NSInteger code = static_cast<NSInteger>(error.code);
    NSMutableDictionary* userInfo = [NSMutableDictionary dictionary];
    
    userInfo[NSLocalizedDescriptionKey] = toNSString(error.message);
    
    if (!error.details.empty()) {
        userInfo[NSLocalizedFailureReasonErrorKey] = toNSString(error.details);
    }
    
    if (error.recovery_suggestion.has_value()) {
        userInfo[NSLocalizedRecoverySuggestionErrorKey] = toNSString(error.recovery_suggestion.value());
    }
    
    return [NSError errorWithDomain:ODAIErrorDomain code:code userInfo:userInfo];
}

ODAIModelInfo* toODAIModelInfo(const ondeviceai::ModelInfo& info) {
    ODAIModelInfo* objcInfo = [[ODAIModelInfo alloc] init];
    objcInfo.modelId = toNSString(info.id);
    objcInfo.name = toNSString(info.name);
    objcInfo.type = static_cast<ODAIModelType>(info.type);
    objcInfo.version = toNSString(info.version);
    objcInfo.sizeBytes = info.size_bytes;
    objcInfo.downloadURL = toNSString(info.download_url);
    objcInfo.checksumSHA256 = toNSString(info.checksum_sha256);
    objcInfo.metadata = toNSDictionary(info.metadata);
    
    ODAIDeviceRequirements* reqs = [[ODAIDeviceRequirements alloc] init];
    reqs.minRamBytes = info.requirements.min_ram_bytes;
    reqs.minStorageBytes = info.requirements.min_storage_bytes;
    reqs.supportedPlatforms = toNSArray(info.requirements.supported_platforms);
    objcInfo.requirements = reqs;
    
    return objcInfo;
}

ODAIStorageInfo* toODAIStorageInfo(const ondeviceai::StorageInfo& info) {
    ODAIStorageInfo* objcInfo = [[ODAIStorageInfo alloc] init];
    objcInfo.totalBytes = info.total_bytes;
    objcInfo.availableBytes = info.available_bytes;
    objcInfo.usedByModelsBytes = info.used_by_models_bytes;
    return objcInfo;
}

ODAIAudioData* toODAIAudioData(const ondeviceai::AudioData& audio) {
    ODAIAudioData* objcAudio = [[ODAIAudioData alloc] init];
    
    NSMutableArray<NSNumber*>* samples = [NSMutableArray arrayWithCapacity:audio.samples.size()];
    for (float sample : audio.samples) {
        [samples addObject:@(sample)];
    }
    objcAudio.samples = [samples copy];
    objcAudio.sampleRate = audio.sample_rate;
    
    return objcAudio;
}

ODAITranscription* toODAITranscription(const ondeviceai::Transcription& transcription) {
    ODAITranscription* objcTranscription = [[ODAITranscription alloc] init];
    objcTranscription.text = toNSString(transcription.text);
    objcTranscription.confidence = transcription.confidence;
    objcTranscription.language = toNSString(transcription.language);
    
    NSMutableArray<ODAIWord*>* words = [NSMutableArray arrayWithCapacity:transcription.words.size()];
    for (const auto& word : transcription.words) {
        ODAIWord* objcWord = [[ODAIWord alloc] init];
        objcWord.text = toNSString(word.text);
        objcWord.startTime = word.start_time;
        objcWord.endTime = word.end_time;
        objcWord.confidence = word.confidence;
        [words addObject:objcWord];
    }
    objcTranscription.words = [words copy];
    
    return objcTranscription;
}

ODAIVoiceInfo* toODAIVoiceInfo(const ondeviceai::VoiceInfo& voice) {
    ODAIVoiceInfo* objcVoice = [[ODAIVoiceInfo alloc] init];
    objcVoice.voiceId = toNSString(voice.id);
    objcVoice.name = toNSString(voice.name);
    objcVoice.language = toNSString(voice.language);
    objcVoice.gender = toNSString(voice.gender);
    return objcVoice;
}

ODAIAudioSegment* toODAIAudioSegment(const ondeviceai::AudioSegment& segment) {
    ODAIAudioSegment* objcSegment = [[ODAIAudioSegment alloc] init];
    objcSegment.startTime = segment.start_time;
    objcSegment.endTime = segment.end_time;
    return objcSegment;
}

ODAIConversationTurn* toODAIConversationTurn(const ondeviceai::ConversationTurn& turn) {
    ODAIConversationTurn* objcTurn = [[ODAIConversationTurn alloc] init];
    objcTurn.userText = toNSString(turn.user_text);
    objcTurn.assistantText = toNSString(turn.assistant_text);
    objcTurn.timestamp = turn.timestamp;
    return objcTurn;
}

ODAIDeviceCapabilities* toODAIDeviceCapabilities(const ondeviceai::DeviceCapabilities& caps) {
    ODAIDeviceCapabilities* objcCaps = [[ODAIDeviceCapabilities alloc] init];
    objcCaps.ramBytes = caps.ram_bytes;
    objcCaps.storageBytes = caps.storage_bytes;
    objcCaps.platform = toNSString(caps.platform);
    objcCaps.accelerators = toNSArray(caps.accelerators);
    return objcCaps;
}

// MARK: - Objective-C to C++ Conversions

std::string toCppString(NSString* str) {
    if (!str) {
        return "";
    }
    return std::string([str UTF8String]);
}

template<typename T>
std::vector<T> toCppVector(NSArray* array) {
    std::vector<T> vec;
    vec.reserve([array count]);
    for (NSNumber* num in array) {
        vec.push_back([num doubleValue]);
    }
    return vec;
}

// Specialization for string arrays
template<>
std::vector<std::string> toCppVector<std::string>(NSArray* array) {
    std::vector<std::string> vec;
    vec.reserve([array count]);
    for (NSString* str in array) {
        vec.push_back(toCppString(str));
    }
    return vec;
}

std::map<std::string, std::string> toCppMap(NSDictionary<NSString*, NSString*>* dict) {
    std::map<std::string, std::string> map;
    for (NSString* key in dict) {
        map[toCppString(key)] = toCppString(dict[key]);
    }
    return map;
}

ondeviceai::SDKConfig toCppSDKConfig(ODAISDKConfig* config) {
    ondeviceai::SDKConfig cppConfig;
    cppConfig.thread_count = static_cast<int>(config.threadCount);
    cppConfig.model_directory = toCppString(config.modelDirectory);
    cppConfig.log_level = toCppLogLevel(config.logLevel);
    cppConfig.memory_limit = config.memoryLimit;
    cppConfig.enable_telemetry = config.enableTelemetry;
    cppConfig.callback_thread_count = static_cast<int>(config.callbackThreadCount);
    cppConfig.synchronous_callbacks = config.synchronousCallbacks;
    return cppConfig;
}

ondeviceai::GenerationConfig toCppGenerationConfig(ODAIGenerationConfig* config) {
    ondeviceai::GenerationConfig cppConfig;
    cppConfig.max_tokens = static_cast<int>(config.maxTokens);
    cppConfig.temperature = config.temperature;
    cppConfig.top_p = config.topP;
    cppConfig.top_k = static_cast<int>(config.topK);
    cppConfig.repetition_penalty = config.repetitionPenalty;
    cppConfig.stop_sequences = toCppVector<std::string>(config.stopSequences);
    return cppConfig;
}

ondeviceai::TranscriptionConfig toCppTranscriptionConfig(ODAITranscriptionConfig* config) {
    ondeviceai::TranscriptionConfig cppConfig;
    cppConfig.language = toCppString(config.language);
    cppConfig.translate_to_english = config.translateToEnglish;
    cppConfig.word_timestamps = config.wordTimestamps;
    return cppConfig;
}

ondeviceai::SynthesisConfig toCppSynthesisConfig(ODAISynthesisConfig* config) {
    ondeviceai::SynthesisConfig cppConfig;
    cppConfig.voice_id = toCppString(config.voiceId);
    cppConfig.speed = config.speed;
    cppConfig.pitch = config.pitch;
    return cppConfig;
}

ondeviceai::PipelineConfig toCppPipelineConfig(ODAIPipelineConfig* config) {
    ondeviceai::PipelineConfig cppConfig;
    cppConfig.llm_config = toCppGenerationConfig(config.llmConfig);
    cppConfig.stt_config = toCppTranscriptionConfig(config.sttConfig);
    cppConfig.tts_config = toCppSynthesisConfig(config.ttsConfig);
    cppConfig.enable_vad = config.enableVAD;
    cppConfig.vad_threshold = config.vadThreshold;
    return cppConfig;
}

ondeviceai::AudioData toCppAudioData(ODAIAudioData* audio) {
    ondeviceai::AudioData cppAudio;
    cppAudio.samples.reserve([audio.samples count]);
    for (NSNumber* sample in audio.samples) {
        cppAudio.samples.push_back([sample floatValue]);
    }
    cppAudio.sample_rate = static_cast<int>(audio.sampleRate);
    return cppAudio;
}

ondeviceai::DeviceCapabilities toCppDeviceCapabilities(ODAIDeviceCapabilities* caps) {
    ondeviceai::DeviceCapabilities cppCaps;
    cppCaps.ram_bytes = caps.ramBytes;
    cppCaps.storage_bytes = caps.storageBytes;
    cppCaps.platform = toCppString(caps.platform);
    cppCaps.accelerators = toCppVector<std::string>(caps.accelerators);
    return cppCaps;
}

ondeviceai::ModelType toCppModelType(ODAIModelType type) {
    switch (type) {
        case ODAIModelTypeLLM:
            return ondeviceai::ModelType::LLM;
        case ODAIModelTypeSTT:
            return ondeviceai::ModelType::STT;
        case ODAIModelTypeTTS:
            return ondeviceai::ModelType::TTS;
        case ODAIModelTypeAll:
            return ondeviceai::ModelType::All;
    }
}

ondeviceai::LogLevel toCppLogLevel(ODAILogLevel level) {
    switch (level) {
        case ODAILogLevelDebug:
            return ondeviceai::LogLevel::Debug;
        case ODAILogLevelInfo:
            return ondeviceai::LogLevel::Info;
        case ODAILogLevelWarning:
            return ondeviceai::LogLevel::Warning;
        case ODAILogLevelError:
            return ondeviceai::LogLevel::Error;
    }
}

// MARK: - Callback Wrappers

ondeviceai::ProgressCallback wrapProgressCallback(ODAIProgressCallback block) {
    if (!block) {
        return nullptr;
    }
    
    // Capture block with strong reference (ARC will manage lifetime)
    ODAIProgressCallback capturedBlock = [block copy];
    
    return [capturedBlock](double progress) {
        // Invoke on main thread for UI updates
        dispatch_async(dispatch_get_main_queue(), ^{
            capturedBlock(progress);
        });
    };
}

ondeviceai::TokenCallback wrapTokenCallback(ODAITokenCallback block) {
    if (!block) {
        return nullptr;
    }
    
    ODAITokenCallback capturedBlock = [block copy];
    
    return [capturedBlock](const std::string& token) {
        NSString* nsToken = toNSString(token);
        dispatch_async(dispatch_get_main_queue(), ^{
            capturedBlock(nsToken);
        });
    };
}

ondeviceai::AudioStreamCallback wrapAudioStreamCallback(ODAIAudioStreamCallback block) {
    if (!block) {
        return nullptr;
    }
    
    ODAIAudioStreamCallback capturedBlock = [block copy];
    
    return [capturedBlock]() -> ondeviceai::AudioData {
        __block ODAIAudioData* objcAudio = nil;
        dispatch_sync(dispatch_get_main_queue(), ^{
            objcAudio = capturedBlock();
        });
        
        if (objcAudio) {
            return toCppAudioData(objcAudio);
        }
        return ondeviceai::AudioData();
    };
}

ondeviceai::AudioChunkCallback wrapAudioChunkCallback(ODAIAudioChunkCallback block) {
    if (!block) {
        return nullptr;
    }
    
    ODAIAudioChunkCallback capturedBlock = [block copy];
    
    return [capturedBlock](const ondeviceai::AudioData& chunk) {
        ODAIAudioData* objcChunk = toODAIAudioData(chunk);
        dispatch_async(dispatch_get_main_queue(), ^{
            capturedBlock(objcChunk);
        });
    };
}

ondeviceai::TranscriptionCallback wrapTranscriptionCallback(ODAITranscriptionCallback block) {
    if (!block) {
        return nullptr;
    }
    
    ODAITranscriptionCallback capturedBlock = [block copy];
    
    return [capturedBlock](const std::string& text) {
        NSString* nsText = toNSString(text);
        dispatch_async(dispatch_get_main_queue(), ^{
            capturedBlock(nsText);
        });
    };
}

ondeviceai::TextCallback wrapTextCallback(ODAITextCallback block) {
    if (!block) {
        return nullptr;
    }
    
    ODAITextCallback capturedBlock = [block copy];
    
    return [capturedBlock](const std::string& text) {
        NSString* nsText = toNSString(text);
        dispatch_async(dispatch_get_main_queue(), ^{
            capturedBlock(nsText);
        });
    };
}

} // namespace ODAIBridge
