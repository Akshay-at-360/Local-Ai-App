//
//  TTSEngine.swift
//  OnDeviceAI iOS SDK
//
//  Swift wrapper for TTSEngine with async/await
//  Requirements: 7.1, 7.6, 7.8
//

import Foundation

/// TTS engine for text-to-speech synthesis
public class TTSEngine {
    
    private let objcEngine: ODAITTSEngine
    
    internal init(objcEngine: ODAITTSEngine) {
        self.objcEngine = objcEngine
    }
    
    // MARK: - Model Management
    
    /// Load a model from file path
    /// - Parameter path: Model file path
    /// - Returns: Model handle
    /// - Throws: SDKError if loading fails
    public func loadModel(path: String) async throws -> ModelHandle {
        return try await withCheckedThrowingContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async { [weak self] in
                guard let self = self else {
                    continuation.resume(throwing: SDKError.invalidState("TTSEngine deallocated"))
                    return
                }
                
                var error: NSError?
                let handle = self.objcEngine.loadModel(path, error: &error)
                
                if let error = error {
                    continuation.resume(throwing: SDKError.from(error))
                } else if handle != 0 {
                    continuation.resume(returning: ModelHandle(value: handle))
                } else {
                    continuation.resume(throwing: SDKError.unknown("Failed to load model"))
                }
            }
        }
    }
    
    /// Unload a model
    /// - Parameter handle: Model handle
    /// - Throws: SDKError if unloading fails
    public func unloadModel(_ handle: ModelHandle) throws {
        var error: NSError?
        let success = objcEngine.unloadModel(handle.value, error: &error)
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to unload model")
        }
    }
    
    // MARK: - Speech Synthesis
    
    /// Synthesize speech from text
    /// - Parameters:
    ///   - model: Model handle
    ///   - text: Text to synthesize
    ///   - config: Synthesis configuration (defaults to standard configuration)
    /// - Returns: Audio data
    /// - Throws: SDKError if synthesis fails
    public func synthesize(
        model: ModelHandle,
        text: String,
        config: SynthesisConfig = .default
    ) async throws -> AudioData {
        return try await withCheckedThrowingContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async { [weak self] in
                guard let self = self else {
                    continuation.resume(throwing: SDKError.invalidState("TTSEngine deallocated"))
                    return
                }
                
                var error: NSError?
                let result = self.objcEngine.synthesize(
                    model.value,
                    text: text,
                    config: config.toObjC(),
                    error: &error
                )
                
                if let error = error {
                    continuation.resume(throwing: SDKError.from(error))
                } else if let result = result {
                    continuation.resume(returning: AudioData(objcAudio: result))
                } else {
                    continuation.resume(throwing: SDKError.unknown("Failed to synthesize speech"))
                }
            }
        }
    }
    
    // MARK: - Voice Information
    
    /// Get available voices for a model
    /// - Parameter model: Model handle
    /// - Returns: Array of voice information
    /// - Throws: SDKError if the operation fails
    public func getAvailableVoices(model: ModelHandle) throws -> [VoiceInfo] {
        var error: NSError?
        guard let voices = objcEngine.getAvailableVoices(model.value, error: &error) else {
            throw SDKError.from(error)
        }
        return voices.map { VoiceInfo(objcVoice: $0) }
    }
}
