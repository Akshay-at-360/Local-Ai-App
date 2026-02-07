//
//  LifecycleManager.swift
//  OnDeviceAI iOS SDK
//
//  Swift wrapper for iOS lifecycle management
//  Requirements: 22.1 (memory warnings), 22.5 (ARC integration)
//

import Foundation

/// Manages iOS lifecycle events including memory warnings and background transitions
///
/// This class handles:
/// - Memory warning notifications
/// - Background/foreground transitions
/// - Automatic model unloading to free memory
/// - Optional inference pausing when app is in background
public class LifecycleManager {
    
    // MARK: - Properties
    
    private let objcLifecycleManager: ODAILifecycleManager
    private let sdkManager: ODAISDKManager
    
    /// Closure called when memory warning is received
    public var onMemoryWarning: (() -> Void)?
    
    /// Closure called when app enters background
    public var onEnterBackground: (() -> Void)?
    
    /// Closure called when app enters foreground
    public var onEnterForeground: (() -> Void)?
    
    // MARK: - Initialization
    
    internal init(objcLifecycleManager: ODAILifecycleManager, sdkManager: ODAISDKManager) {
        self.objcLifecycleManager = objcLifecycleManager
        self.sdkManager = sdkManager
    }
    
    // MARK: - Observation Control
    
    /// Start observing iOS lifecycle events
    ///
    /// This registers the lifecycle manager to receive:
    /// - UIApplicationDidReceiveMemoryWarningNotification
    /// - UIApplicationDidEnterBackgroundNotification
    /// - UIApplicationWillEnterForegroundNotification
    ///
    /// Should be called early in the app's lifecycle, typically after SDK initialization
    public func startObserving() {
        objcLifecycleManager.startObserving()
    }
    
    /// Stop observing iOS lifecycle events
    ///
    /// This unregisters all event handlers. Call this when you no longer need
    /// to handle lifecycle events or before shutting down the SDK.
    public func stopObserving() {
        objcLifecycleManager.stopObserving()
    }
    
    // MARK: - Configuration
    
    /// Enable or disable automatic model unloading when app goes to background
    ///
    /// When enabled, all loaded models will be unloaded when the app enters background,
    /// freeing up memory for other apps. Models will need to be reloaded when the app
    /// returns to foreground.
    ///
    /// - Parameter enabled: true to unload models on background, false to keep them
    public func setPauseInferenceOnBackground(_ enabled: Bool) {
        objcLifecycleManager.setPauseInferenceOnBackground(enabled)
    }
    
    /// Check if automatic model unloading on background is enabled
    ///
    /// - Returns: true if pausing on background is enabled, false otherwise
    public func isPauseInferenceOnBackgroundEnabled() -> Bool {
        return objcLifecycleManager.isPauseInferenceOnBackgroundEnabled()
    }
    
    // MARK: - Memory Information
    
    /// Get current memory usage
    ///
    /// - Returns: Memory usage in bytes
    public func getCurrentMemoryUsage() -> UInt {
        return sdkManager.getCurrentMemoryUsage()
    }
    
    /// Get memory limit
    ///
    /// - Returns: Memory limit in bytes, or 0 if no limit is set
    public func getMemoryLimit() -> UInt {
        return sdkManager.getMemoryLimit()
    }
    
    /// Check if memory is under pressure
    ///
    /// Returns true if current memory usage is above the pressure threshold
    ///
    /// - Returns: true if memory is under pressure, false otherwise
    public func isMemoryPressure() -> Bool {
        return sdkManager.isMemoryPressure()
    }
    
    /// Get memory usage percentage
    ///
    /// - Returns: Memory usage as a percentage (0-100), or nil if no memory limit is set
    public func getMemoryUsagePercentage() -> Double? {
        let limit = getMemoryLimit()
        guard limit > 0 else { return nil }
        
        let current = getCurrentMemoryUsage()
        return Double(current) / Double(limit) * 100.0
    }
    
    // MARK: - Manual Memory Management
    
    /// Manually unload all currently loaded models
    ///
    /// This can be called when you receive a memory warning or want to free up memory
    /// for other operations. After calling this, models will need to be reloaded
    /// before they can be used for inference again.
    ///
    /// - Returns: true if models were unloaded, false if none were loaded
    @discardableResult
    public func unloadAllModels() -> Bool {
        return sdkManager.unloadAllModels()
    }
    
    // MARK: - Utilities
    
    /// Get a human-readable memory usage summary
    ///
    /// - Returns: String describing current memory usage
    public func getMemorySummary() -> String {
        let current = getCurrentMemoryUsage()
        let limit = getMemoryLimit()
        
        if limit > 0 {
            let percentage = Double(current) / Double(limit) * 100.0
            return String(format: "Memory: %.1f MB / %.1f MB (%.1f%%)",
                         Double(current) / 1024.0 / 1024.0,
                         Double(limit) / 1024.0 / 1024.0,
                         percentage)
        } else {
            return String(format: "Memory: %.1f MB (no limit)",
                         Double(current) / 1024.0 / 1024.0)
        }
    }
    
    /// Get the state of the lifecycle manager
    ///
    /// - Returns: Dictionary with state information
    public func getState() -> [String: Any] {
        return [
            "isObserving": objcLifecycleManager.isPauseInferenceOnBackgroundEnabled(), // Placeholder
            "pauseInferenceOnBackground": isPauseInferenceOnBackgroundEnabled(),
            "currentMemoryUsage": getCurrentMemoryUsage(),
            "memoryLimit": getMemoryLimit(),
            "isMemoryPressure": isMemoryPressure()
        ]
    }
}
