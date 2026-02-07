# OnDeviceAI iOS Example Application

## Overview

This example application demonstrates the capabilities of the OnDeviceAI SDK on iOS. It provides a simple UI to interact with the SDK's core functionality including initialization, configuration, lifecycle management, and memory monitoring.

## Features

### 1. SDK Initialization
- Initialize the SDK with custom configuration
- Set model directory, thread count, and memory limits
- Handle initialization errors gracefully

### 2. Configuration
- Set thread count for inference
- Configure logging levels
- Manage callback threads
- Enable/disable synchronous callbacks

### 3. Lifecycle Management
- Start/stop observing iOS lifecycle events
- Monitor memory warnings
- Handle background/foreground transitions
- Automatic model unloading on background

### 4. Memory Monitoring
- View current memory usage
- Check memory limits
- Monitor memory pressure
- Get formatted memory summaries

### 5. Error Handling
- Graceful error detection and reporting
- User-friendly error messages
- Recovery suggestions

## Getting Started

### Prerequisites

- iOS 12.0 or later
- Xcode 14.0 or later
- OnDeviceAI SDK framework

### Building the Example

1. Open the project in Xcode:
```bash
cd platforms/ios
open Example.xcodeproj
```

2. Select the Example target

3. Build and run on an iOS device or simulator:
```bash
⌘ + R (or Product > Run)
```

## Using the Example App

### Initialize SDK
1. Tap "Initialize SDK" button
2. Watch the log output for status messages
3. Status should change to "✅ Initialized"

### Configure SDK
1. Tap "Configure" button
2. This sets various SDK parameters:
   - Thread count: 3
   - Log level: debug
   - Callback threads: 2
   - Synchronous callbacks: disabled

### Enable Lifecycle Events
1. Tap "Lifecycle Events" button
2. This enables:
   - Memory warning monitoring
   - Background transition handling
   - Automatic model unloading

### View Memory Information
1. Tap "Memory Info" button
2. View:
   - Current memory usage
   - Memory limit
   - Usage percentage
   - Memory pressure status

### Shutdown SDK
1. Tap "Shutdown" button
2. Cleanly release all resources
3. Ready for re-initialization

## Code Examples

### Basic Initialization

```swift
import OnDeviceAI

// Configure the SDK
var config = SDKConfig.default
config.threadCount = 2
config.modelDirectory = NSTemporaryDirectory() + "models"
config.memoryLimitBytes = 500 * 1024 * 1024 // 500 MB

// Initialize
let sdk = try OnDeviceAI.initialize(config: config)
```

### Memory Management

```swift
// Check memory usage
let currentUsage = sdk.lifecycle.getCurrentMemoryUsage()
let limit = sdk.lifecycle.getMemoryLimit()
let percentage = sdk.lifecycle.getMemoryUsagePercentage()

print("Using \(percentage ?? 0)% of available memory")
```

### Lifecycle Management

```swift
// Start observing iOS lifecycle events
sdk.startObservingLifecycleEvents()

// Enable automatic model unloading on background
sdk.setAutoUnloadModelsOnBackground(true)

// Later: stop observing
sdk.stopObservingLifecycleEvents()
```

### Error Handling

```swift
do {
    let sdk = try OnDeviceAI.initialize(config: config)
    // Use SDK
} catch let error as SDKError {
    print("SDK Error: \(error.message)")
    print("Recovery: \(error.recoverySuggestion)")
} catch {
    print("Unknown error: \(error)")
}
```

## Architecture

### UI Structure

```
ExampleViewController
├── Navigation Bar
├── ScrollView
│   └── ContentView
│       └── StackView
│           ├── Title Label
│           ├── Status Label
│           ├── Memory Label
│           ├── Button Stack
│           │   ├── Initialize Button
│           │   ├── Configure Button
│           │   ├── Lifecycle Button
│           │   ├── Memory Button
│           │   └── Shutdown Button
│           └── Output TextViewA
```

### Lifecycle Flow

```
App Launch
   ↓
Initialize SDK
   ↓
Configure Options
   ↓
Enable Lifecycle Events
   ↓
Monitor Memory
   ↓
Background → Pause Inference
   ↓
Foreground → Resume Inference
   ↓
Shutdown
```

## Testing

### Unit Tests

Run included tests:
```bash
⌘ + U (or Product > Test)
```

Test suites included:
- Initialization tests
- Configuration tests
- Lifecycle management tests
- Memory monitoring tests
- Error handling tests
- Thread safety tests

### Manual Testing

1. **Test Initialization**
   - Launch app
   - Tap "Initialize SDK"
   - Verify status changes to ✅

2. **Test Memory Monitoring**
   - Initialize SDK
   - Tap "Memory Info"
   - Verify memory values are displayed

3. **Test Lifecycle Events**
   - Initialize SDK
   - Tap "Lifecycle Events"
   - Put app in background (Home key)
   - Return to app
   - Memory should be managed automatically

4. **Test Error Handling**
   - Try operations without initializing
   - Verify error messages are clear

## Troubleshooting

### SDK Initialization Fails
- Check model directory permissions
- Verify memory limit is reasonable (500MB+)
- Check console logs for detailed error messages

### Memory Not Decreasing
- This is normal - memory may be retained for performance
- Force memory info update by tapping "Memory Info"
- Manually unload unused models if needed

### Lifecycle Events Not Firing
- Ensure "Lifecycle Events" button has been tapped
- Check that auto-unload is enabled
- Test by putting app in background

### Performance Issues
- Reduce thread count if needed
- Increase memory limit for larger models
- Use lower quantization levels for models (Q4 vs Q8)

## Advanced Usage

### Custom Configuration

```swift
var config = SDKConfig.default
config.threadCount = 4                    // Use 4 threads
config.logLevel = .debug                  // Verbose logging
config.memoryLimitBytes = 1_000_000_000  // 1 GB limit
sdk = try OnDeviceAI.initialize(config: config)
```

### Real-time Monitoring

```swift
// Check memory periodically
DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
    let percentage = self.sdk?.lifecycle.getMemoryUsagePercentage() ?? 0
    print("Memory: \(percentage)%")
}
```

### Background Processing

```swift
// Perform work while in background
Task {
    // This will be paused if auto-unload is enabled
    // Re-enable for longer background sessions
    self.sdk?.setAutoUnloadModelsOnBackground(false)
}
```

## Platform Considerations

### iOS 12-14
- Full support
- Metal acceleration available
- Core ML available (iOS 11+)

### iOS 15+
- All features supported
- Neural Engine fully available
- Better memory management

### iPadOS
- All iOS features supported
- Can use larger models with more memory
- Split-screen considerations for lifecycle

## Performance Tips

1. **Thread Count**: Match device CPU cores
   - iPhone 12: 6 cores → set to 3-4
   - iPhone 13+: 6-8 cores → set to 4

2. **Memory Setup**: Leave room for OS
   - iPhone with 4GB RAM: limit to 2GB
   - iPhone with 6GB+ RAM: limit to 3-4GB

3. **Lifecycle Events**: Enable for apps that need background stability
   - Disable for apps requiring continuous processing

4. **Callback Threads**: 2 is usually optimal
   - 1 for single-threaded apps
   - 2-4 for multi-threaded apps

## Next Steps

- **Load Models**: Implement model loading in ModelManager
- **Run Inference**: Add LLM inference examples
- **Voice Interaction**: Implement STT/TTS pipeline
- **Custom UI**: Extend with your app's design

## Resources

- [OnDeviceAI SDK Documentation](../docs/)
- [iOS Best Practices](../README.md)
- [Error Codes Reference](../docs/ERROR_CODES.md)

## Support

For issues or questions:
1. Check the troubleshooting section
2. Review error messages in console
3. Check SDK requirements and compatibility
4. Review example code in this app

---

**Example App Version**: 1.0  
**SDK Version**: 0.1.0  
**Minimum iOS**: 12.0
