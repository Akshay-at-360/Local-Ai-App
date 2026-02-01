//
//  LLMEngine.swift
//  OnDeviceAI iOS SDK
//
//  Swift wrapper for LLMEngine with async/await
//  Requirements: 7.1, 7.6, 7.8
//

import Foundation

/// LLM engine for language model inference
public class LLMEngine {
    
    private let objcEngine: ODAILLMEngine
    
    internal init(objcEngine: ODAILLMEngine) {
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
                    continuation.resume(throwing: SDKError.invalidState("LLMEngine deallocated"))
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
    
    /// Check if a model is loaded
    /// - Parameter handle: Model handle
    /// - Returns: true if loaded, false otherwise
    public func isModelLoaded(_ handle: ModelHandle) -> Bool {
        return objcEngine.isModelLoaded(handle.value)
    }
    
    // MARK: - Text Generation
    
    /// Generate text synchronously
    /// - Parameters:
    ///   - model: Model handle
    ///   - prompt: Input prompt
    ///   - config: Generation configuration (defaults to standard configuration)
    /// - Returns: Generated text
    /// - Throws: SDKError if generation fails
    public func generate(
        model: ModelHandle,
        prompt: String,
        config: GenerationConfig = .default
    ) async throws -> String {
        return try await withCheckedThrowingContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async { [weak self] in
                guard let self = self else {
                    continuation.resume(throwing: SDKError.invalidState("LLMEngine deallocated"))
                    return
                }
                
                var error: NSError?
                let result = self.objcEngine.generate(
                    model.value,
                    prompt: prompt,
                    config: config.toObjC(),
                    error: &error
                )
                
                if let error = error {
                    continuation.resume(throwing: SDKError.from(error))
                } else if let result = result {
                    continuation.resume(returning: result)
                } else {
                    continuation.resume(throwing: SDKError.unknown("Failed to generate text"))
                }
            }
        }
    }
    
    /// Generate text with streaming callbacks
    /// - Parameters:
    ///   - model: Model handle
    ///   - prompt: Input prompt
    ///   - config: Generation configuration (defaults to standard configuration)
    ///   - onToken: Token callback (called for each generated token)
    /// - Throws: SDKError if generation fails
    public func generateStreaming(
        model: ModelHandle,
        prompt: String,
        config: GenerationConfig = .default,
        onToken: @escaping (String) -> Void
    ) async throws {
        return try await withCheckedThrowingContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async { [weak self] in
                guard let self = self else {
                    continuation.resume(throwing: SDKError.invalidState("LLMEngine deallocated"))
                    return
                }
                
                var error: NSError?
                let success = self.objcEngine.generateStreaming(
                    model.value,
                    prompt: prompt,
                    callback: { token in
                        onToken(token)
                    },
                    config: config.toObjC(),
                    error: &error
                )
                
                if let error = error {
                    continuation.resume(throwing: SDKError.from(error))
                } else if success {
                    continuation.resume()
                } else {
                    continuation.resume(throwing: SDKError.unknown("Failed to generate streaming text"))
                }
            }
        }
    }
    
    // MARK: - Context Management
    
    /// Clear conversation context
    /// - Parameter model: Model handle
    /// - Throws: SDKError if the operation fails
    public func clearContext(model: ModelHandle) throws {
        var error: NSError?
        let success = objcEngine.clearContext(model.value, error: &error)
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to clear context")
        }
    }
    
    /// Get conversation history
    /// - Parameter model: Model handle
    /// - Returns: Array of conversation turns
    /// - Throws: SDKError if the operation fails
    public func getConversationHistory(model: ModelHandle) throws -> [String] {
        var error: NSError?
        guard let history = objcEngine.getConversationHistory(model.value, error: &error) else {
            throw SDKError.from(error)
        }
        return history
    }
    
    // MARK: - Tokenization
    
    /// Tokenize text
    /// - Parameters:
    ///   - model: Model handle
    ///   - text: Text to tokenize
    /// - Returns: Array of token IDs
    /// - Throws: SDKError if tokenization fails
    public func tokenize(model: ModelHandle, text: String) throws -> [Int] {
        var error: NSError?
        guard let tokens = objcEngine.tokenize(model.value, text: text, error: &error) else {
            throw SDKError.from(error)
        }
        return tokens.map { $0.intValue }
    }
    
    /// Detokenize token IDs
    /// - Parameters:
    ///   - model: Model handle
    ///   - tokens: Array of token IDs
    /// - Returns: Detokenized text
    /// - Throws: SDKError if detokenization fails
    public func detokenize(model: ModelHandle, tokens: [Int]) throws -> String {
        var error: NSError?
        let nsTokens = tokens.map { NSNumber(value: $0) }
        guard let text = objcEngine.detokenize(model.value, tokens: nsTokens, error: &error) else {
            throw SDKError.from(error)
        }
        return text
    }
}
