//
//  OnDeviceAI.swift
//  OnDeviceAI iOS SDK
//
//  Swift API layer for OnDeviceAI SDK
//  Requirements: 7.1, 7.6, 7.8
//

import Foundation

/// Main entry point for the OnDeviceAI SDK
/// Provides access to all SDK components and manages initialization
public class OnDeviceAI {
    
    // MARK: - Properties
    
    private let objcManager: ODAISDKManager
    
    /// Model manager for discovering and downloading models
    public private(set) lazy var modelManager: ModelManager = {
        ModelManager(objcManager: objcManager.modelManager())
    }()
    
    /// LLM engine for language model inference
    public private(set) lazy var llm: LLMEngine = {
        LLMEngine(objcEngine: objcManager.llmEngine())
    }()
    
    /// STT engine for speech-to-text transcription
    public private(set) lazy var stt: STTEngine = {
        STTEngine(objcEngine: objcManager.sttEngine())
    }()
    
    /// TTS engine for text-to-speech synthesis
    public private(set) lazy var tts: TTSEngine = {
        TTSEngine(objcEngine: objcManager.ttsEngine())
    }()
    
    /// Voice pipeline for orchestrating voice conversations
    public private(set) lazy var voicePipeline: VoicePipeline = {
        VoicePipeline(objcPipeline: objcManager.voicePipeline())
    }()
    
    // MARK: - Initialization
    
    private init(objcManager: ODAISDKManager) {
        self.objcManager = objcManager
    }
    
    /// Initialize the SDK with configuration
    /// - Parameter config: SDK configuration (defaults to standard configuration)
    /// - Throws: SDKError if initialization fails
    /// - Returns: Initialized OnDeviceAI instance
    public static func initialize(config: SDKConfig = .default) throws -> OnDeviceAI {
        let objcConfig = config.toObjC()
        
        var error: NSError?
        guard let objcManager = ODAISDKManager.initialize(with: objcConfig, error: &error) else {
            throw SDKError.from(error)
        }
        
        return OnDeviceAI(objcManager: objcManager)
    }
    
    /// Get the current SDK instance (must be initialized first)
    /// - Returns: Current SDK instance or nil if not initialized
    public static func getInstance() -> OnDeviceAI? {
        guard let objcManager = ODAISDKManager.getInstance() else {
            return nil
        }
        return OnDeviceAI(objcManager: objcManager)
    }
    
    /// Shutdown the SDK and release all resources
    /// After shutdown, initialize must be called again to use the SDK
    public static func shutdown() {
        ODAISDKManager.shutdown()
    }
    
    // MARK: - Configuration
    
    /// Set the number of threads for inference operations
    /// - Parameter count: Number of threads (must be > 0)
    public func setThreadCount(_ count: Int) {
        objcManager.setThreadCount(count)
    }
    
    /// Set the model storage directory
    /// - Parameter path: Directory path for model storage
    public func setModelDirectory(_ path: String) {
        objcManager.setModelDirectory(path)
    }
    
    /// Set the logging level
    /// - Parameter level: Log level
    public func setLogLevel(_ level: LogLevel) {
        objcManager.setLogLevel(level.toObjC())
    }
    
    /// Set memory limit for the SDK
    /// - Parameter bytes: Memory limit in bytes (0 = no limit)
    public func setMemoryLimit(_ bytes: UInt) {
        objcManager.setMemoryLimit(bytes)
    }
    
    /// Set the number of callback threads
    /// - Parameter count: Number of callback threads
    public func setCallbackThreadCount(_ count: Int) {
        objcManager.setCallbackThreadCount(count)
    }
    
    /// Set whether callbacks should be synchronous
    /// - Parameter synchronous: true for synchronous callbacks, false for asynchronous
    public func setSynchronousCallbacks(_ synchronous: Bool) {
        objcManager.setSynchronousCallbacks(synchronous)
    }
}
