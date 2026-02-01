//
//  STTEngine.swift
//  OnDeviceAI iOS SDK
//
//  Swift wrapper for STTEngine with async/await
//  Requirements: 7.1, 7.6, 7.8
//

import Foundation

/// STT engine for speech-to-text transcription
public class STTEngine {
    
    private let objcEngine: ODAISTTEngine
    
    internal init(objcEngine: ODAISTTEngine) {
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
                    continuation.resume(throwing: SDKError.invalidState("STTEngine deallocated"))
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
    
    // MARK: - Transcription
    
    /// Transcribe audio
    /// - Parameters:
    ///   - model: Model handle
    ///   - audio: Audio data
    ///   - config: Transcription configuration (defaults to standard configuration)
    /// - Returns: Transcription result
    /// - Throws: SDKError if transcription fails
    public func transcribe(
        model: ModelHandle,
        audio: AudioData,
        config: TranscriptionConfig = .default
    ) async throws -> Transcription {
        return try await withCheckedThrowingContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async { [weak self] in
                guard let self = self else {
                    continuation.resume(throwing: SDKError.invalidState("STTEngine deallocated"))
                    return
                }
                
                var error: NSError?
                let result = self.objcEngine.transcribe(
                    model.value,
                    audio: audio.toObjC(),
                    config: config.toObjC(),
                    error: &error
                )
                
                if let error = error {
                    continuation.resume(throwing: SDKError.from(error))
                } else if let result = result {
                    continuation.resume(returning: Transcription(objcTranscription: result))
                } else {
                    continuation.resume(throwing: SDKError.unknown("Failed to transcribe audio"))
                }
            }
        }
    }
    
    // MARK: - Voice Activity Detection
    
    /// Detect voice activity in audio
    /// - Parameters:
    ///   - audio: Audio data
    ///   - threshold: VAD threshold (0.0 to 1.0, defaults to 0.5)
    /// - Returns: Array of audio segments with speech
    /// - Throws: SDKError if detection fails
    public func detectVoiceActivity(
        audio: AudioData,
        threshold: Float = 0.5
    ) throws -> [AudioSegment] {
        var error: NSError?
        guard let segments = objcEngine.detectVoiceActivity(
            audio.toObjC(),
            threshold: threshold,
            error: &error
        ) else {
            throw SDKError.from(error)
        }
        return segments.map { AudioSegment(objcSegment: $0) }
    }
}
