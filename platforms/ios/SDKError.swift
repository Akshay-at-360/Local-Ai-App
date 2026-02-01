//
//  SDKError.swift
//  OnDeviceAI iOS SDK
//
//  Error types for Swift API
//  Requirements: 7.1, 7.6, 7.8
//

import Foundation

/// SDK error type
public enum SDKError: Error, LocalizedError {
    /// Model not found
    case modelNotFound(String)
    
    /// Model loading error
    case modelLoadError(String)
    
    /// Inference error
    case inferenceError(String)
    
    /// Network error
    case networkError(String)
    
    /// Storage error
    case storageError(String)
    
    /// Invalid input
    case invalidInput(String)
    
    /// Resource exhausted
    case resourceExhausted(String)
    
    /// Operation cancelled
    case cancelled(String)
    
    /// Invalid state
    case invalidState(String)
    
    /// Unknown error
    case unknown(String)
    
    public var errorDescription: String? {
        switch self {
        case .modelNotFound(let message):
            return "Model not found: \(message)"
        case .modelLoadError(let message):
            return "Model load error: \(message)"
        case .inferenceError(let message):
            return "Inference error: \(message)"
        case .networkError(let message):
            return "Network error: \(message)"
        case .storageError(let message):
            return "Storage error: \(message)"
        case .invalidInput(let message):
            return "Invalid input: \(message)"
        case .resourceExhausted(let message):
            return "Resource exhausted: \(message)"
        case .cancelled(let message):
            return "Operation cancelled: \(message)"
        case .invalidState(let message):
            return "Invalid state: \(message)"
        case .unknown(let message):
            return "Unknown error: \(message)"
        }
    }
    
    /// Convert NSError to SDKError
    internal static func from(_ error: NSError?) -> SDKError {
        guard let error = error else {
            return .unknown("Unknown error occurred")
        }
        
        let message = error.localizedDescription
        let code = error.code
        
        // Map error codes to SDK error types
        // Based on error code ranges from design document
        switch code {
        case 1000...1099:
            return .modelNotFound(message)
        case 1100...1199:
            return .modelLoadError(message)
        case 1200...1299:
            return .inferenceError(message)
        case 1300...1399:
            return .networkError(message)
        case 1400...1499:
            return .storageError(message)
        case 1500...1599:
            return .invalidInput(message)
        case 1600...1699:
            return .resourceExhausted(message)
        case 1700...1799:
            return .cancelled(message)
        default:
            return .unknown(message)
        }
    }
}
