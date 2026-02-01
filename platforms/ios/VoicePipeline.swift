//
//  VoicePipeline.swift
//  OnDeviceAI iOS SDK
//
//  Swift wrapper for VoicePipeline with async/await
//  Requirements: 7.1, 7.6, 7.8
//

import Foundation

/// Voice pipeline for orchestrating STT → LLM → TTS conversations
public class VoicePipeline {
    
    private let objcPipeline: ODAIVoicePipeline
    
    internal init(objcPipeline: ODAIVoicePipeline) {
        self.objcPipeline = objcPipeline
    }
    
    // MARK: - Configuration
    
    /// Configure the pipeline with models
    /// - Parameters:
    ///   - sttModel: STT model handle
    ///   - llmModel: LLM model handle
    ///   - ttsModel: TTS model handle
    ///   - config: Pipeline configuration (defaults to standard configuration)
    /// - Throws: SDKError if configuration fails
    public func configure(
        sttModel: ModelHandle,
        llmModel: ModelHandle,
        ttsModel: ModelHandle,
        config: PipelineConfig = .default
    ) throws {
        var error: NSError?
        let success = objcPipeline.configureSttModel(
            sttModel.value,
            llmModel: llmModel.value,
            ttsModel: ttsModel.value,
            config: config.toObjC(),
            error: &error
        )
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to configure pipeline")
        }
    }
    
    // MARK: - Conversation
    
    /// Start a conversation
    /// - Parameters:
    ///   - audioInput: Audio input callback (returns nil to stop)
    ///   - audioOutput: Audio output callback
    ///   - onTranscription: Transcription callback
    ///   - onResponse: LLM response callback
    /// - Throws: SDKError if the conversation fails to start
    public func startConversation(
        audioInput: @escaping () async -> AudioData?,
        audioOutput: @escaping (AudioData) -> Void,
        onTranscription: @escaping (String) -> Void,
        onResponse: @escaping (String) -> Void
    ) async throws {
        return try await withCheckedThrowingContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async { [weak self] in
                guard let self = self else {
                    continuation.resume(throwing: SDKError.invalidState("VoicePipeline deallocated"))
                    return
                }
                
                var error: NSError?
                let success = self.objcPipeline.startConversation(
                    withAudioInput: {
                        // Bridge async Swift callback to sync Objective-C callback
                        // Note: This is a simplified approach. In production, you'd want
                        // to use a proper async-to-sync bridge with a semaphore or similar
                        var result: ODAIAudioData?
                        let semaphore = DispatchSemaphore(value: 0)
                        
                        Task {
                            if let audio = await audioInput() {
                                result = audio.toObjC()
                            }
                            semaphore.signal()
                        }
                        
                        semaphore.wait()
                        return result
                    },
                    audioOutput: { objcAudio in
                        let audio = AudioData(objcAudio: objcAudio)
                        audioOutput(audio)
                    },
                    transcriptionCallback: { text in
                        onTranscription(text)
                    },
                    llmResponseCallback: { text in
                        onResponse(text)
                    },
                    error: &error
                )
                
                if let error = error {
                    continuation.resume(throwing: SDKError.from(error))
                } else if success {
                    continuation.resume()
                } else {
                    continuation.resume(throwing: SDKError.unknown("Failed to start conversation"))
                }
            }
        }
    }
    
    /// Stop the current conversation
    /// - Throws: SDKError if stopping fails
    public func stopConversation() throws {
        var error: NSError?
        let success = objcPipeline.stopConversation(&error)
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to stop conversation")
        }
    }
    
    /// Interrupt current TTS playback
    /// - Throws: SDKError if interruption fails
    public func interrupt() throws {
        var error: NSError?
        let success = objcPipeline.interrupt(&error)
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to interrupt")
        }
    }
    
    // MARK: - Context Management
    
    /// Clear conversation history
    /// - Throws: SDKError if clearing fails
    public func clearHistory() throws {
        var error: NSError?
        let success = objcPipeline.clearHistory(&error)
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to clear history")
        }
    }
    
    /// Get conversation history
    /// - Returns: Array of conversation turns
    /// - Throws: SDKError if the operation fails
    public func getHistory() throws -> [ConversationTurn] {
        var error: NSError?
        guard let history = objcPipeline.getHistory(&error) else {
            throw SDKError.from(error)
        }
        return history.map { ConversationTurn(objcTurn: $0) }
    }
}
