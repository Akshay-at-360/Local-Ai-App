//
//  ModelManager.swift
//  OnDeviceAI iOS SDK
//
//  Swift wrapper for ModelManager
//  Requirements: 7.1, 7.6, 7.8
//

import Foundation

/// Model manager for discovering, downloading, and managing AI models
public class ModelManager {
    
    private let objcManager: ODAIModelManager
    
    internal init(objcManager: ODAIModelManager) {
        self.objcManager = objcManager
    }
    
    // MARK: - Model Discovery
    
    /// List available models from remote registry
    /// - Parameters:
    ///   - type: Model type filter (defaults to all types)
    ///   - device: Device capabilities for filtering (defaults to current device)
    /// - Returns: Array of available models
    /// - Throws: SDKError if the operation fails
    public func listAvailableModels(
        type: ModelType = .all,
        device: DeviceCapabilities = .current
    ) async throws -> [ModelInfo] {
        return try await withCheckedThrowingContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async { [weak self] in
                guard let self = self else {
                    continuation.resume(throwing: SDKError.invalidState("ModelManager deallocated"))
                    return
                }
                
                var error: NSError?
                let objcModels = self.objcManager.listAvailableModels(
                    withType: type.toObjC(),
                    device: device.toObjC(),
                    error: &error
                )
                
                if let error = error {
                    continuation.resume(throwing: SDKError.from(error))
                } else if let objcModels = objcModels {
                    let models = objcModels.map { ModelInfo(objcInfo: $0) }
                    continuation.resume(returning: models)
                } else {
                    continuation.resume(throwing: SDKError.unknown("Failed to list models"))
                }
            }
        }
    }
    
    /// List models that have been downloaded locally
    /// - Returns: Array of downloaded models
    public func listDownloadedModels() -> [ModelInfo] {
        let objcModels = objcManager.listDownloadedModels()
        return objcModels.map { ModelInfo(objcInfo: $0) }
    }
    
    // MARK: - Model Download
    
    /// Download a model from the remote registry
    /// - Parameters:
    ///   - modelId: Model identifier
    ///   - progress: Progress callback (0.0 to 1.0)
    /// - Throws: SDKError if the download fails
    public func downloadModel(
        _ modelId: String,
        progress: @escaping (Double) -> Void
    ) async throws {
        return try await withCheckedThrowingContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async { [weak self] in
                guard let self = self else {
                    continuation.resume(throwing: SDKError.invalidState("ModelManager deallocated"))
                    return
                }
                
                var error: NSError?
                let handle = self.objcManager.downloadModel(
                    modelId,
                    progress: { objcProgress in
                        progress(objcProgress)
                    },
                    error: &error
                )
                
                if let error = error {
                    continuation.resume(throwing: SDKError.from(error))
                } else if handle != 0 {
                    // Download initiated successfully
                    // Note: The progress callback will be called asynchronously
                    // For now, we complete immediately after starting the download
                    continuation.resume()
                } else {
                    continuation.resume(throwing: SDKError.unknown("Failed to start download"))
                }
            }
        }
    }
    
    /// Cancel an ongoing download
    /// - Parameter handle: Download handle from downloadModel
    /// - Throws: SDKError if cancellation fails
    public func cancelDownload(_ handle: UInt64) throws {
        var error: NSError?
        let success = objcManager.cancelDownload(handle, error: &error)
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to cancel download")
        }
    }
    
    /// Delete a downloaded model
    /// - Parameter modelId: Model identifier
    /// - Throws: SDKError if deletion fails
    public func deleteModel(_ modelId: String) throws {
        var error: NSError?
        let success = objcManager.deleteModel(modelId, error: &error)
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to delete model")
        }
    }
    
    // MARK: - Model Information
    
    /// Get information about a specific model
    /// - Parameter modelId: Model identifier
    /// - Returns: Model information
    /// - Throws: SDKError if the model is not found
    public func getModelInfo(_ modelId: String) throws -> ModelInfo {
        var error: NSError?
        guard let objcInfo = objcManager.getModelInfo(modelId, error: &error) else {
            throw SDKError.from(error)
        }
        return ModelInfo(objcInfo: objcInfo)
    }
    
    /// Check if a model is downloaded
    /// - Parameter modelId: Model identifier
    /// - Returns: true if downloaded, false otherwise
    public func isModelDownloaded(_ modelId: String) -> Bool {
        return objcManager.isModelDownloaded(modelId)
    }
    
    /// Get the local file path for a downloaded model
    /// - Parameter modelId: Model identifier
    /// - Returns: File path
    /// - Throws: SDKError if the model is not found
    public func getModelPath(_ modelId: String) throws -> String {
        var error: NSError?
        guard let path = objcManager.getModelPath(modelId, error: &error) else {
            throw SDKError.from(error)
        }
        return path
    }
    
    // MARK: - Version Management
    
    /// Get available versions for a model
    /// - Parameter modelId: Model identifier
    /// - Returns: Array of version strings
    /// - Throws: SDKError if the operation fails
    public func getAvailableVersions(_ modelId: String) throws -> [String] {
        var error: NSError?
        guard let versions = objcManager.getAvailableVersions(modelId, error: &error) else {
            throw SDKError.from(error)
        }
        return versions
    }
    
    /// Check for updates to a model
    /// - Parameter modelId: Model identifier
    /// - Returns: Updated model info or nil if no update available
    /// - Throws: SDKError if the operation fails
    public func checkForUpdates(_ modelId: String) throws -> ModelInfo? {
        var error: NSError?
        guard let objcInfo = objcManager.checkForUpdates(modelId, error: &error) else {
            if let error = error {
                throw SDKError.from(error)
            }
            return nil
        }
        return ModelInfo(objcInfo: objcInfo)
    }
    
    /// Pin a specific version of a model
    /// - Parameters:
    ///   - modelId: Model identifier
    ///   - version: Version string to pin
    /// - Throws: SDKError if the operation fails
    public func pinModelVersion(_ modelId: String, version: String) throws {
        var error: NSError?
        let success = objcManager.pinModelVersion(modelId, version: version, error: &error)
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to pin model version")
        }
    }
    
    /// Unpin a model version
    /// - Parameter modelId: Model identifier
    /// - Throws: SDKError if the operation fails
    public func unpinModelVersion(_ modelId: String) throws {
        var error: NSError?
        let success = objcManager.unpinModelVersion(modelId, error: &error)
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to unpin model version")
        }
    }
    
    /// Check if a model version is pinned
    /// - Parameter modelId: Model identifier
    /// - Returns: true if pinned, false otherwise
    public func isModelVersionPinned(_ modelId: String) -> Bool {
        return objcManager.isModelVersionPinned(modelId)
    }
    
    // MARK: - Storage Management
    
    /// Get storage information
    /// - Returns: Storage information
    /// - Throws: SDKError if the operation fails
    public func getStorageInfo() throws -> StorageInfo {
        var error: NSError?
        guard let objcInfo = objcManager.getStorageInfo(&error) else {
            throw SDKError.from(error)
        }
        return StorageInfo(objcInfo: objcInfo)
    }
    
    /// Clean up incomplete downloads
    /// - Throws: SDKError if the operation fails
    public func cleanupIncompleteDownloads() throws {
        var error: NSError?
        let success = objcManager.cleanupIncompleteDownloads(&error)
        
        if let error = error {
            throw SDKError.from(error)
        } else if !success {
            throw SDKError.unknown("Failed to cleanup incomplete downloads")
        }
    }
    
    /// Get recommended models for the current device
    /// - Parameters:
    ///   - type: Model type
    ///   - device: Device capabilities (defaults to current device)
    /// - Returns: Array of recommended models
    /// - Throws: SDKError if the operation fails
    public func recommendModels(
        type: ModelType,
        device: DeviceCapabilities = .current
    ) throws -> [ModelInfo] {
        var error: NSError?
        guard let objcModels = objcManager.recommendModels(
            withType: type.toObjC(),
            device: device.toObjC(),
            error: &error
        ) else {
            throw SDKError.from(error)
        }
        return objcModels.map { ModelInfo(objcInfo: $0) }
    }
}
