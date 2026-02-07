//
//  ExampleViewController.swift
//  OnDeviceAI Example App
//
//  Example iOS application demonstrating OnDeviceAI SDK usage
//  Requirements: 20.5, 20.7
//

import UIKit

/// Main view controller demonstrating OnDeviceAI capabilities
class ExampleViewController: UIViewController {
    
    // MARK: - Properties
    
    private var sdk: OnDeviceAI?
    private var isSDKInitialized = false
    
    // MARK: - UI Elements
    
    private let scrollView = UIScrollView()
    private let contentView = UIView()
    private let stackView = UIStackView()
    
    private let titleLabel = UILabel()
    private let statusLabel = UILabel()
    private let memoryLabel = UILabel()
    
    private let initButton = UIButton(type: .system)
    private let configButton = UIButton(type: .system)
    private let lifecycleButton = UIButton(type: .system)
    private let memoryButton = UIButton(type: .system)
    private let shutdownButton = UIButton(type: .system)
    
    private let outputTextView = UITextView()
    
    // MARK: - Lifecycle
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "OnDeviceAI Example"
        view.backgroundColor = .systemBackground
        
        setupUI()
        appendLog("🚀 OnDeviceAI Example App Started")
    }
    
    deinit {
        if isSDKInitialized {
            OnDeviceAI.shutdown()
        }
    }
    
    // MARK: - UI Setup
    
    private func setupUI() {
        // Configure scroll view
        scrollView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(scrollView)
        
        contentView.translatesAutoresizingMaskIntoConstraints = false
        scrollView.addSubview(contentView)
        
        // Configure stack view
        stackView.axis = .vertical
        stackView.spacing = 12
        stackView.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(stackView)
        
        // Title
        titleLabel.text = "OnDeviceAI SDK Demo"
        titleLabel.font = .systemFont(ofSize: 24, weight: .bold)
        titleLabel.textAlignment = .center
        stackView.addArrangedSubview(titleLabel)
        
        // Status
        statusLabel.text = "Status: Not Initialized"
        statusLabel.font = .systemFont(ofSize: 14, weight: .semibold)
        statusLabel.textAlignment = .center
        statusLabel.textColor = .secondaryLabel
        stackView.addArrangedSubview(statusLabel)
        
        // Memory info
        memoryLabel.text = "Memory: --"
        memoryLabel.font = .systemFont(ofSize: 12)
        memoryLabel.textAlignment = .center
        memoryLabel.textColor = .tertiaryLabel
        stackView.addArrangedSubview(memoryLabel)
        
        // Buttons
        let buttonStackView = UIStackView()
        buttonStackView.axis = .vertical
        buttonStackView.spacing = 8
        
        setupButton(initButton, title: "Initialize SDK", action: #selector(initializeSDK))
        setupButton(configButton, title: "Configure", action: #selector(configureSDK))
        setupButton(lifecycleButton, title: "Lifecycle Events", action: #selector(toggleLifecycleEvents))
        setupButton(memoryButton, title: "Memory Info", action: #selector(showMemoryInfo))
        setupButton(shutdownButton, title: "Shutdown", action: #selector(shutdownSDK))
        
        buttonStackView.addArrangedSubview(initButton)
        buttonStackView.addArrangedSubview(configButton)
        buttonStackView.addArrangedSubview(lifecycleButton)
        buttonStackView.addArrangedSubview(memoryButton)
        buttonStackView.addArrangedSubview(shutdownButton)
        
        stackView.addArrangedSubview(buttonStackView)
        
        // Output text view
        outputTextView.isEditable = false
        outputTextView.font = .monospacedSystemFont(ofSize: 12, weight: .regular)
        outputTextView.layer.borderColor = UIColor.separator.cgColor
        outputTextView.layer.borderWidth = 1
        outputTextView.layer.cornerRadius = 8
        outputTextView.translatesAutoresizingMaskIntoConstraints = false
        stackView.addArrangedSubview(outputTextView)
        
        // Layout constraints
        NSLayoutConstraint.activate([
            scrollView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
            scrollView.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor),
            scrollView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            scrollView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            
            contentView.topAnchor.constraint(equalTo: scrollView.topAnchor),
            contentView.bottomAnchor.constraint(equalTo: scrollView.bottomAnchor),
            contentView.leadingAnchor.constraint(equalTo: scrollView.leadingAnchor, constant: 16),
            contentView.trailingAnchor.constraint(equalTo: scrollView.trailingAnchor, constant: -16),
            contentView.widthAnchor.constraint(equalTo: scrollView.widthAnchor, constant: -32),
            
            stackView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 16),
            stackView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -16),
            stackView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor),
            stackView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor),
            
            outputTextView.heightAnchor.constraint(greaterThanOrEqualToConstant: 200),
        ])
    }
    
    private func setupButton(_ button: UIButton, title: String, action: Selector) {
        button.setTitle(title, for: .normal)
        button.addTarget(self, action: action, for: .touchUpInside)
        button.backgroundColor = .systemBlue
        button.setTitleColor(.white, for: .normal)
        button.layer.cornerRadius = 8
        button.heightAnchor.constraint(equalToConstant: 44).isActive = true
    }
    
    // MARK: - Actions
    
    @objc private func initializeSDK() {
        appendLog("📱 Initializing SDK...")
        
        do {
            var config = SDKConfig.default
            config.threadCount = 2
            config.modelDirectory = NSTemporaryDirectory() + "ondeviceai_example"
            config.memoryLimitBytes = 500 * 1024 * 1024 // 500 MB
            config.logLevel = .info
            
            sdk = try OnDeviceAI.initialize(config: config)
            isSDKInitialized = true
            
            appendLog("✅ SDK initialized successfully")
            updateStatus()
        } catch {
            appendLog("❌ Initialization failed: \(error)")
        }
    }
    
    @objc private func configureSDK() {
        guard isSDKInitialized, let sdk = sdk else {
            appendLog("⚠️ SDK not initialized")
            return
        }
        
        appendLog("⚙️ Configuring SDK...")
        
        sdk.setThreadCount(3)
        sdk.setLogLevel(.debug)
        sdk.setCallbackThreadCount(2)
        sdk.setSynchronousCallbacks(false)
        
        appendLog("✅ Configuration complete:")
        appendLog("   - Thread count: 3")
        appendLog("   - Log level: debug")
        appendLog("   - Callback threads: 2")
        appendLog("   - Synchronous callbacks: disabled")
    }
    
    @objc private func toggleLifecycleEvents() {
        guard isSDKInitialized, let sdk = sdk else {
            appendLog("⚠️ SDK not initialized")
            return
        }
        
        let isEnabled = sdk.lifecycle.isPauseInferenceOnBackgroundEnabled()
        
        if isEnabled {
            appendLog("🔇 Disabling lifecycle events...")
            sdk.stopObservingLifecycleEvents()
            sdk.setAutoUnloadModelsOnBackground(false)
            appendLog("✅ Lifecycle events disabled")
        } else {
            appendLog("🔊 Enabling lifecycle events...")
            sdk.startObservingLifecycleEvents()
            sdk.setAutoUnloadModelsOnBackground(true)
            appendLog("✅ Lifecycle events enabled:")
            appendLog("   - Memory warnings: monitored")
            appendLog("   - Background transitions: monitored")
            appendLog("   - Auto-unload on background: enabled")
        }
    }
    
    @objc private func showMemoryInfo() {
        guard isSDKInitialized, let sdk = sdk else {
            appendLog("⚠️ SDK not initialized")
            return
        }
        
        appendLog("📊 Memory Information:")
        
        let current = sdk.lifecycle.getCurrentMemoryUsage()
        let limit = sdk.lifecycle.getMemoryLimit()
        let isPressure = sdk.lifecycle.isMemoryPressure()
        
        appendLog(String(format: "   Current Usage: %.1f MB", Double(current) / 1024.0 / 1024.0))
        appendLog(String(format: "   Memory Limit: %.1f MB", Double(limit) / 1024.0 / 1024.0))
        
        if let percentage = sdk.lifecycle.getMemoryUsagePercentage() {
            appendLog(String(format: "   Usage: %.1f%%", percentage))
        }
        
        appendLog("   Memory Pressure: " + (isPressure ? "⚠️ YES" : "✅ NO"))
        
        let summary = sdk.lifecycle.getMemorySummary()
        appendLog("   Summary: " + summary)
    }
    
    @objc private func shutdownSDK() {
        guard isSDKInitialized else {
            appendLog("⚠️ SDK not initialized")
            return
        }
        
        appendLog("🔌 Shutting down SDK...")
        
        sdk?.lifecycle.stopObserving()
        OnDeviceAI.shutdown()
        
        sdk = nil
        isSDKInitialized = false
        
        appendLog("✅ SDK shutdown complete")
        updateStatus()
    }
    
    // MARK: - Helper Methods
    
    private func appendLog(_ message: String) {
        let timestamp = DateFormatter.localizedString(from: Date(), dateStyle: .none, timeStyle: .mediumStyle)
        let logMessage = "[\(timestamp)] \(message)\n"
        
        DispatchQueue.main.async {
            self.outputTextView.text.append(logMessage)
            
            // Scroll to bottom
            let range = NSRange(location: self.outputTextView.text.count - logMessage.count, length: logMessage.count)
            self.outputTextView.scrollRangeToVisible(range)
        }
    }
    
    private func updateStatus() {
        DispatchQueue.main.async {
            if self.isSDKInitialized {
                self.statusLabel.text = "Status: ✅ Initialized"
                self.statusLabel.textColor = .systemGreen
                
                if let percentage = self.sdk?.lifecycle.getMemoryUsagePercentage() {
                    self.memoryLabel.text = String(format: "Memory: %.1f%%", percentage)
                }
            } else {
                self.statusLabel.text = "Status: ⚠️ Not Initialized"
                self.statusLabel.textColor = .systemOrange
                self.memoryLabel.text = "Memory: --"
            }
        }
    }
}
