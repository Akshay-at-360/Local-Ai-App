//
//  Types.swift
//  OnDeviceAI iOS SDK
//
//  Swift type definitions and conversions
//  Requirements: 7.1, 7.6, 7.8
//

import Foundation

// MARK: - Model Handle

/// Opaque handle representing a loaded model instance
public struct ModelHandle: Equatable, Hashable {
    internal let value: UInt64
    
    internal init(value: UInt64) {
        self.value = value
    }
}

// MARK: - Enums

/// Log level for SDK logging
public enum LogLevel {
    case debug
    case info
    case warning
    case error
    
    internal func toObjC() -> ODAILogLevel {
        switch self {
        case .debug: return .debug
        case .info: return .info
        case .warning: return .warning
        case .error: return .error
        }
    }
}

/// Model type classification
public enum ModelType {
    case llm
    case stt
    case tts
    case all
    
    internal func toObjC() -> ODAIModelType {
        switch self {
        case .llm: return .LLM
        case .stt: return .STT
        case .tts: return .TTS
        case .all: return .all
        }
    }
}

// MARK: - Configuration

/// SDK configuration
public struct SDKConfig {
    /// Number of threads for inference operations
    public var threadCount: Int
    
    /// Model storage directory path
    public var modelDirectory: String
    
    /// Logging level
    public var logLevel: LogLevel
    
    /// Memory limit in bytes (0 = no limit)
    public var memoryLimit: UInt
    
    /// Enable telemetry collection
    public var enableTelemetry: Bool
    
    /// Number of callback threads
    public var callbackThreadCount: Int
    
    /// Whether callbacks should be synchronous
    public var synchronousCallbacks: Bool
    
    /// Default configuration
    public static let `default` = SDKConfig(
        threadCount: 4,
        modelDirectory: NSTemporaryDirectory() + "ondeviceai_models",
        logLevel: .info,
        memoryLimit: 0,
        enableTelemetry: false,
        callbackThreadCount: 2,
        synchronousCallbacks: false
    )
    
    public init(
        threadCount: Int = 4,
        modelDirectory: String = NSTemporaryDirectory() + "ondeviceai_models",
        logLevel: LogLevel = .info,
        memoryLimit: UInt = 0,
        enableTelemetry: Bool = false,
        callbackThreadCount: Int = 2,
        synchronousCallbacks: Bool = false
    ) {
        self.threadCount = threadCount
        self.modelDirectory = modelDirectory
        self.logLevel = logLevel
        self.memoryLimit = memoryLimit
        self.enableTelemetry = enableTelemetry
        self.callbackThreadCount = callbackThreadCount
        self.synchronousCallbacks = synchronousCallbacks
    }
    
    internal func toObjC() -> ODAISDKConfig {
        let config = ODAISDKConfig.defaultConfig()
        config.threadCount = threadCount
        config.modelDirectory = modelDirectory
        config.logLevel = logLevel.toObjC()
        config.memoryLimit = memoryLimit
        config.enableTelemetry = enableTelemetry
        config.callbackThreadCount = callbackThreadCount
        config.synchronousCallbacks = synchronousCallbacks
        return config
    }
}

/// Device capabilities for model filtering
public struct DeviceCapabilities {
    /// RAM in bytes
    public var ramBytes: UInt
    
    /// Storage in bytes
    public var storageBytes: UInt
    
    /// Platform identifier
    public var platform: String
    
    /// Available hardware accelerators
    public var accelerators: [String]
    
    /// Current device capabilities
    public static let current = DeviceCapabilities(
        ramBytes: UInt(ProcessInfo.processInfo.physicalMemory),
        storageBytes: 0, // Would need platform-specific code to get this
        platform: "iOS",
        accelerators: ["CoreML", "Metal"]
    )
    
    public init(ramBytes: UInt, storageBytes: UInt, platform: String, accelerators: [String]) {
        self.ramBytes = ramBytes
        self.storageBytes = storageBytes
        self.platform = platform
        self.accelerators = accelerators
    }
    
    internal func toObjC() -> ODAIDeviceCapabilities {
        let caps = ODAIDeviceCapabilities.current()
        caps.ramBytes = ramBytes
        caps.storageBytes = storageBytes
        caps.platform = platform
        caps.accelerators = accelerators
        return caps
    }
}

/// Device requirements for a model
public struct DeviceRequirements {
    /// Minimum RAM in bytes
    public let minRamBytes: UInt
    
    /// Minimum storage in bytes
    public let minStorageBytes: UInt
    
    /// Supported platforms
    public let supportedPlatforms: [String]
    
    internal init(objcRequirements: ODAIDeviceRequirements) {
        self.minRamBytes = objcRequirements.minRamBytes
        self.minStorageBytes = objcRequirements.minStorageBytes
        self.supportedPlatforms = objcRequirements.supportedPlatforms
    }
}

/// Model information
public struct ModelInfo {
    /// Model identifier
    public let modelId: String
    
    /// Human-readable name
    public let name: String
    
    /// Model type
    public let type: ModelType
    
    /// Version string
    public let version: String
    
    /// Size in bytes
    public let sizeBytes: UInt
    
    /// Download URL
    public let downloadURL: String
    
    /// SHA-256 checksum
    public let checksumSHA256: String
    
    /// Additional metadata
    public let metadata: [String: String]
    
    /// Device requirements
    public let requirements: DeviceRequirements
    
    internal init(objcInfo: ODAIModelInfo) {
        self.modelId = objcInfo.modelId
        self.name = objcInfo.name
        self.type = {
            switch objcInfo.type {
            case .LLM: return .llm
            case .STT: return .stt
            case .TTS: return .tts
            case .all: return .all
            @unknown default: return .all
            }
        }()
        self.version = objcInfo.version
        self.sizeBytes = objcInfo.sizeBytes
        self.downloadURL = objcInfo.downloadURL
        self.checksumSHA256 = objcInfo.checksumSHA256
        self.metadata = objcInfo.metadata
        self.requirements = DeviceRequirements(objcRequirements: objcInfo.requirements)
    }
}

/// Storage information
public struct StorageInfo {
    /// Total storage in bytes
    public let totalBytes: UInt
    
    /// Available storage in bytes
    public let availableBytes: UInt
    
    /// Storage used by models in bytes
    public let usedByModelsBytes: UInt
    
    internal init(objcInfo: ODAIStorageInfo) {
        self.totalBytes = objcInfo.totalBytes
        self.availableBytes = objcInfo.availableBytes
        self.usedByModelsBytes = objcInfo.usedByModelsBytes
    }
}

// MARK: - Audio

/// Audio data structure
public struct AudioData {
    /// PCM samples (Float32, mono, normalized to [-1.0, 1.0])
    public var samples: [Float]
    
    /// Sample rate in Hz
    public var sampleRate: Int
    
    public init(samples: [Float], sampleRate: Int) {
        self.samples = samples
        self.sampleRate = sampleRate
    }
    
    internal init(objcAudio: ODAIAudioData) {
        self.samples = objcAudio.samples.map { $0.floatValue }
        self.sampleRate = objcAudio.sampleRate
    }
    
    internal func toObjC() -> ODAIAudioData {
        let audio = ODAIAudioData()
        audio.samples = samples.map { NSNumber(value: $0) }
        audio.sampleRate = sampleRate
        return audio
    }
    
    /// Load audio from file
    public static func fromFile(_ path: String) throws -> AudioData {
        var error: NSError?
        guard let objcAudio = ODAIAudioData.fromFile(path, error: &error) else {
            throw SDKError.from(error)
        }
        return AudioData(objcAudio: objcAudio)
    }
    
    /// Load audio from WAV data
    public static func fromWAVData(_ data: Data) throws -> AudioData {
        var error: NSError?
        guard let objcAudio = ODAIAudioData.fromWAVData(data, error: &error) else {
            throw SDKError.from(error)
        }
        return AudioData(objcAudio: objcAudio)
    }
    
    /// Convert to WAV data
    public func toWAV(bitsPerSample: Int = 16) throws -> Data {
        var error: NSError?
        guard let data = toObjC().toWAV(withBitsPerSample: bitsPerSample, error: &error) else {
            throw SDKError.from(error)
        }
        return data
    }
    
    /// Resample to target sample rate
    public func resample(to targetSampleRate: Int) throws -> AudioData {
        var error: NSError?
        guard let objcAudio = toObjC().resample(toRate: targetSampleRate, error: &error) else {
            throw SDKError.from(error)
        }
        return AudioData(objcAudio: objcAudio)
    }
    
    /// Normalize audio samples
    public func normalize() throws -> AudioData {
        var error: NSError?
        guard let objcAudio = toObjC().normalize(&error) else {
            throw SDKError.from(error)
        }
        return AudioData(objcAudio: objcAudio)
    }
}

/// Audio segment with timestamps
public struct AudioSegment {
    /// Start time in seconds
    public let startTime: Float
    
    /// End time in seconds
    public let endTime: Float
    
    internal init(objcSegment: ODAIAudioSegment) {
        self.startTime = objcSegment.startTime
        self.endTime = objcSegment.endTime
    }
}

/// Word with timing information
public struct Word {
    /// Word text
    public let text: String
    
    /// Start time in seconds
    public let startTime: Float
    
    /// End time in seconds
    public let endTime: Float
    
    /// Confidence score [0.0, 1.0]
    public let confidence: Float
    
    internal init(objcWord: ODAIWord) {
        self.text = objcWord.text
        self.startTime = objcWord.startTime
        self.endTime = objcWord.endTime
        self.confidence = objcWord.confidence
    }
}

/// Transcription result
public struct Transcription {
    /// Transcribed text
    public let text: String
    
    /// Overall confidence score [0.0, 1.0]
    public let confidence: Float
    
    /// Detected or specified language
    public let language: String
    
    /// Word-level timestamps (if enabled)
    public let words: [Word]
    
    internal init(objcTranscription: ODAITranscription) {
        self.text = objcTranscription.text
        self.confidence = objcTranscription.confidence
        self.language = objcTranscription.language
        self.words = objcTranscription.words.map { Word(objcWord: $0) }
    }
}

/// Voice information
public struct VoiceInfo {
    /// Voice identifier
    public let voiceId: String
    
    /// Human-readable name
    public let name: String
    
    /// Language code
    public let language: String
    
    /// Gender
    public let gender: String
    
    internal init(objcVoice: ODAIVoiceInfo) {
        self.voiceId = objcVoice.voiceId
        self.name = objcVoice.name
        self.language = objcVoice.language
        self.gender = objcVoice.gender
    }
}

// MARK: - Generation Configuration

/// LLM generation configuration
public struct GenerationConfig {
    /// Maximum tokens to generate
    public var maxTokens: Int
    
    /// Temperature for sampling [0.0, 2.0]
    public var temperature: Float
    
    /// Top-p for nucleus sampling [0.0, 1.0]
    public var topP: Float
    
    /// Top-k for sampling
    public var topK: Int
    
    /// Repetition penalty [1.0, 2.0]
    public var repetitionPenalty: Float
    
    /// Stop sequences
    public var stopSequences: [String]
    
    /// Default configuration
    public static let `default` = GenerationConfig(
        maxTokens: 512,
        temperature: 0.7,
        topP: 0.9,
        topK: 40,
        repetitionPenalty: 1.1,
        stopSequences: []
    )
    
    public init(
        maxTokens: Int = 512,
        temperature: Float = 0.7,
        topP: Float = 0.9,
        topK: Int = 40,
        repetitionPenalty: Float = 1.1,
        stopSequences: [String] = []
    ) {
        self.maxTokens = maxTokens
        self.temperature = temperature
        self.topP = topP
        self.topK = topK
        self.repetitionPenalty = repetitionPenalty
        self.stopSequences = stopSequences
    }
    
    internal func toObjC() -> ODAIGenerationConfig {
        let config = ODAIGenerationConfig.defaultConfig()
        config.maxTokens = maxTokens
        config.temperature = temperature
        config.topP = topP
        config.topK = topK
        config.repetitionPenalty = repetitionPenalty
        config.stopSequences = stopSequences
        return config
    }
}

/// STT transcription configuration
public struct TranscriptionConfig {
    /// Language code or "auto" for detection
    public var language: String
    
    /// Whether to translate to English
    public var translateToEnglish: Bool
    
    /// Whether to include word timestamps
    public var wordTimestamps: Bool
    
    /// Default configuration
    public static let `default` = TranscriptionConfig(
        language: "auto",
        translateToEnglish: false,
        wordTimestamps: false
    )
    
    public init(
        language: String = "auto",
        translateToEnglish: Bool = false,
        wordTimestamps: Bool = false
    ) {
        self.language = language
        self.translateToEnglish = translateToEnglish
        self.wordTimestamps = wordTimestamps
    }
    
    internal func toObjC() -> ODAITranscriptionConfig {
        let config = ODAITranscriptionConfig.defaultConfig()
        config.language = language
        config.translateToEnglish = translateToEnglish
        config.wordTimestamps = wordTimestamps
        return config
    }
}

/// TTS synthesis configuration
public struct SynthesisConfig {
    /// Voice identifier
    public var voiceId: String
    
    /// Speech speed multiplier [0.5, 2.0]
    public var speed: Float
    
    /// Pitch adjustment [-1.0, 1.0]
    public var pitch: Float
    
    /// Default configuration
    public static let `default` = SynthesisConfig(
        voiceId: "",
        speed: 1.0,
        pitch: 0.0
    )
    
    public init(
        voiceId: String = "",
        speed: Float = 1.0,
        pitch: Float = 0.0
    ) {
        self.voiceId = voiceId
        self.speed = speed
        self.pitch = pitch
    }
    
    internal func toObjC() -> ODAISynthesisConfig {
        let config = ODAISynthesisConfig.defaultConfig()
        config.voiceId = voiceId
        config.speed = speed
        config.pitch = pitch
        return config
    }
}

/// Voice pipeline configuration
public struct PipelineConfig {
    /// LLM generation configuration
    public var llmConfig: GenerationConfig
    
    /// STT transcription configuration
    public var sttConfig: TranscriptionConfig
    
    /// TTS synthesis configuration
    public var ttsConfig: SynthesisConfig
    
    /// Enable voice activity detection
    public var enableVAD: Bool
    
    /// VAD threshold [0.0, 1.0]
    public var vadThreshold: Float
    
    /// Default configuration
    public static let `default` = PipelineConfig(
        llmConfig: .default,
        sttConfig: .default,
        ttsConfig: .default,
        enableVAD: true,
        vadThreshold: 0.5
    )
    
    public init(
        llmConfig: GenerationConfig = .default,
        sttConfig: TranscriptionConfig = .default,
        ttsConfig: SynthesisConfig = .default,
        enableVAD: Bool = true,
        vadThreshold: Float = 0.5
    ) {
        self.llmConfig = llmConfig
        self.sttConfig = sttConfig
        self.ttsConfig = ttsConfig
        self.enableVAD = enableVAD
        self.vadThreshold = vadThreshold
    }
    
    internal func toObjC() -> ODAIPipelineConfig {
        let config = ODAIPipelineConfig.defaultConfig()
        config.llmConfig = llmConfig.toObjC()
        config.sttConfig = sttConfig.toObjC()
        config.ttsConfig = ttsConfig.toObjC()
        config.enableVAD = enableVAD
        config.vadThreshold = vadThreshold
        return config
    }
}

/// Conversation turn
public struct ConversationTurn {
    /// User's text input
    public let userText: String
    
    /// Assistant's text response
    public let assistantText: String
    
    /// Timestamp
    public let timestamp: Float
    
    internal init(objcTurn: ODAIConversationTurn) {
        self.userText = objcTurn.userText
        self.assistantText = objcTurn.assistantText
        self.timestamp = objcTurn.timestamp
    }
}
