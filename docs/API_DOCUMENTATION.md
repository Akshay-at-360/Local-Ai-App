# OnDevice AI SDK - Complete API Documentation

**Version**: 1.0.0  
**Last Updated**: 2024

## Table of Contents

1. [Overview](#overview)
2. [Platform-Specific Guides](#platform-specific-guides)
3. [Core API Reference](#core-api-reference)
4. [Platform APIs](#platform-apis)
5. [Error Handling](#error-handling)
6. [Advanced Usage](#advanced-usage)
7. [Performance Tuning](#performance-tuning)
8. [Troubleshooting](#troubleshooting)

---

## Overview

OnDevice AI is a cross-platform SDK for running AI inference models (LLM, STT, TTS) on edge devices without cloud connectivity.

### Key Features

- **Cross-Platform Support**: iOS, Android, Web, React Native, Flutter
- **Hardware Acceleration**: CoreML/Metal on iOS, NNAPI on Android
- **Async-First Architecture**: Native async/await patterns per platform
- **Memory Safe**: Automatic memory management with lifecycle awareness
- **Type-Safe**: Strong typing in all languages
- **Production Ready**: Full error handling and recovery

### Supported Models

- **LLM**: Llama 2, Mistral (via llama.cpp)
- **STT**: Whisper (via whisper.cpp)
- **TTS**: FastPitch, Glow-TTS (via ONNX Runtime)

---

## Platform-Specific Guides

### iOS (Swift)

#### Installation

```bash
# Via CocoaPods
pod 'OnDeviceAI'

# Or manually add XCFramework
cp OnDeviceAI.xcframework /path/to/project
```

#### Basic Usage

```swift
import OnDeviceAI

// Initialize
let sdk = try await OnDeviceAI.initialize(config: SDKConfig(
    threadCount: 2,
    modelDirectory: "/path/to/models",
    memoryLimitMB: 500
))

// Load model
let modelHandle = try await sdk.llm.loadModel("path/to/model.gguf")

// Generate text
let response = try await sdk.llm.generate(handle: modelHandle, prompt: "Hello")

// Cleanup
try await sdk.llm.unloadModel(handle: modelHandle)
await OnDeviceAI.shutdown()
```

#### iOS-Specific Features

- **Memory Warnings**: Automatic pause/resume on memory pressure
- **Background Transitions**: Suspend inference when app enters background
- **Core ML Integration**: Automatic Metal acceleration for compatible models

### Android (Kotlin)

#### Installation

```gradle
dependencies {
    implementation 'com.example:ondeviceai:1.0.0'
    
    // Required dependencies
    implementation 'org.jetbrains.kotlinx:kotlinx-coroutines-android:latest'
}
```

#### Basic Usage

```kotlin
import com.example.ondeviceai.OnDeviceAI

// Initialize
val sdk = OnDeviceAI.initialize(SDKConfig(
    threadCount = 2,
    modelDirectory = "/path/to/models",
    memoryLimitMB = 500
))

// Load model
val modelHandle = sdk.llm.loadModel("path/to/model.gguf")

// Generate text
val response = sdk.llm.generate(modelHandle, "Hello")

// Cleanup
sdk.llm.unloadModel(modelHandle)
OnDeviceAI.shutdown()
```

#### Android-Specific Features

- **NNAPI Acceleration**: Automatic Neural Network API acceleration
- **Activity Lifecycle**: Integrated with Android lifecycle awareness
- **Coroutine Support**: Full kotlinx.coroutines integration

### React Native (TypeScript)

#### Installation

```bash
npm install @ondeviceai/react-native
react-native link @ondeviceai/react-native
```

#### Basic Usage

```typescript
import OnDeviceAI from '@ondeviceai/react-native';

// Initialize
const sdk = await OnDeviceAI.initialize({
    threadCount: 2,
    memoryLimitMB: 500,
});

// Use lifecycle manager to observe app state
sdk.lifecycle.startObserving();

// Generate text
const response = await sdk.llm.generate(0, "Hello");

// Cleanup
sdk.lifecycle.stopObserving();
OnDeviceAI.shutdown();
```

#### React Native-Specific Features

- **Native Module Bridge**: Direct connection to native implementations
- **Event Emitters**: Listen to SDK events and lifecycle changes
- **Promises & Callbacks**: Standard RN async patterns

### Flutter (Dart)

#### Installation

```yaml
dependencies:
  ondeviceai: ^1.0.0
```

#### Basic Usage

```dart
import 'package:ondeviceai/ondeviceai.dart';

// Initialize
final sdk = await OnDeviceAI.initialize(
  threadCount: 2,
  memoryLimitMB: 500,
);

// Generate text
final response = await sdk.llm.generate(0, "Hello");

// Cleanup
await OnDeviceAI.shutdown();
```

#### Flutter-Specific Features

- **Dart FFI**: Direct C++ bindings via dart:ffi
- **Async/Await**: Dart's Future-based concurrency
- **Platform Channels**: Native plugin integration

---

## Core API Reference

### SDKConfig

Configuration for SDK initialization.

```swift
struct SDKConfig {
    var threadCount: Int = 2           // CPU threads for inference
    var modelDirectory: String = ""    // Directory to store/load models
    var memoryLimitMB: Int = 500       // Maximum memory usage
    var logLevel: String = "info"      // Log verbosity
}
```

### OnDeviceAI (Main SDK)

#### Methods

```swift
// Initialization
static func initialize(config: SDKConfig) async throws -> OnDeviceAI

// Instance
static func getInstance() -> OnDeviceAI?

// Shutdown
static func shutdown() async

// Configuration
func setThreadCount(_ count: Int) throws
func setLogLevel(_ level: String) throws
func setMemoryLimit(_ bytes: Int) throws
```

#### Properties

```swift
var modelManager: ModelManager
var llm: LLMEngine
var stt: STTEngine
var tts: TTSEngine
var voicePipeline: VoicePipeline
var lifecycle: LifecycleManager
```

### LLMEngine

Large Language Model inference.

```swift
// Load model from file
func loadModel(_ path: String) async throws -> Int

// Generate text (blocking)
func generate(handle: Int, prompt: String) async throws -> String

// Generate text (streaming)
func generateStreaming(
    handle: Int,
    prompt: String,
    onToken: @escaping (String) -> Void
) async throws

// Batch generation
func generateBatch(
    handles: [Int],
    prompts: [String]
) async throws -> [String]

// Unload model
func unloadModel(_ handle: Int) async throws
```

### STTEngine

Speech-to-Text.

```swift
// Transcribe audio file
func transcribe(
    modelHandle: Int,
    audioPath: String
) async throws -> String

// Transcribe audio data
func transcribeData(
    modelHandle: Int,
    audioData: Data,
    format: AudioFormat
) async throws -> String
```

### TTSEngine

Text-to-Speech.

```swift
// Synthesize text to audio
func synthesize(
    modelHandle: Int,
    text: String
) async throws -> AudioBuffer

// Stream synthesis
func synthesizeStreaming(
    modelHandle: Int,
    text: String,
    onAudioChunk: @escaping (AudioBuffer) -> Void
) async throws
```

### VoicePipeline

Orchestrate STT → LLM → TTS.

```swift
// Configure pipeline
func configure(
    sttModelPath: String,
    llmModelPath: String,
    ttsModelPath: String
) async throws

// Process voice input end-to-end
func processVoice(
    audioPath: String,
    llmPrompt: String
) async throws -> (transcript: String, response: String, audio: AudioBuffer)
```

### LifecycleManager

Memory and lifecycle management.

```swift
// Start observing lifecycle events
func startObserving()

// Stop observing
func stopObserving()

// Get current memory usage
func getCurrentMemoryUsage() async -> Int

// Get memory limit
func getMemoryLimit() async -> Int

// Get memory summary
func getMemorySummary() async -> String

// Lifecycle event callbacks
func onMemoryWarning(handler: @escaping () -> Void)
func onBackground(handler: @escaping () -> Void)
func onForeground(handler: @escaping () -> Void)
```

---

## Platform APIs

### iOS Additional APIs

```swift
// CoreML acceleration control
func enableCoreMLAcceleration(_ enabled: Bool)
func isCoreMLAvailable() -> Bool

// Metal GPU acceleration
func enableMetalAcceleration(_ enabled: Bool)
func isMetalAvailable() -> Bool

// Neural Engine detection
func hasNeuralEngine() -> Bool
```

### Android Additional APIs

```kotlin
// NNAPI acceleration control
fun enableNNAPIAcceleration(enabled: Boolean)
fun isNNAPIAvailable(): Boolean

// Activity lifecycle binding
fun bindLifecycle(activity: Activity)
fun unbindLifecycle()

// Coroutine scope configuration
fun setCallbackCoroutineScope(scope: CoroutineScope)
```

### React Native Additional APIs

```typescript
// Native event listening
OnDeviceAIEventEmitter.addListener(
    OnDeviceAIEvents.MEMORY_WARNING,
    (event) => { /* handle */ }
);

// Callback thread control
sdk.setCallbackThreadCount(2);
```

### Flutter Additional APIs

```dart
// FFI bridge initialization
await OnDeviceAINative.initializeNative();

// Platform channel configuration
setPlatformChannelCallbacks({
  'onError': (error) => { /* handle */ },
  'onMemoryWarning': (usage) => { /* handle */ },
});
```

---

## Error Handling

### Error Hierarchy

All platforms use consistent error types:

```
SDKException
├── InvalidState (-2): SDK not initialized or invalid operation
├── ModelNotFound (-3): Requested model not found
├── InsufficientMemory (-4): Out of memory
├── IOError (-5): File I/O error
├── InvalidInput (-6): Invalid parameter
└── Unknown (-1): Generic/unknown error
```

### Error Handling Examples

```swift
// iOS
do {
    let response = try await sdk.llm.generate(handle: 0, prompt: "test")
} catch let error as SDKError {
    print("SDK Error (\(error.code)): \(error.message)")
    print("Recovery: \(error.recoverySuggestion ?? "N/A")")
} catch {
    print("Unexpected error: \(error)")
}
```

```kotlin
// Android
try {
    val response = sdk.llm.generate(0, "test")
} catch (e: SDKException) {
    Log.e("SDK", "Error (${e.code}): ${e.message}")
    Log.d("SDK", "Recovery: ${e.recoverySuggestion}")
}
```

```typescript
// React Native / TypeScript
try {
    const response = await sdk.llm.generate(0, "test");
} catch (error) {
    if (error instanceof SDKError) {
        console.error(`SDK Error (${error.code}): ${error.message}`);
        console.log(`Recovery: ${error.recoverySuggestion}`);
    }
}
```

---

## Advanced Usage

### Streaming Generation

```swift
// iOS
try await sdk.llm.generateStreaming(
    handle: modelHandle,
    prompt: "Write a story about",
    onToken: { token in
        print(token, terminator: "")
    }
)
```

### Batch Processing

```kotlin
// Android
val responses = sdk.llm.generateBatch(
    listOf(handle1, handle2),
    listOf("Prompt 1", "Prompt 2")
)
```

### Voice Pipeline (STT → LLM → TTS)

```swift
// iOS
let result = try await sdk.voicePipeline.processVoice(
    audioPath: "/path/to/audio.wav",
    llmPrompt: "Respond to the user"
)

print("Transcript: \(result.transcript)")
print("Response: \(result.response)")
// result.audio can be played or saved
```

### Memory Monitoring

```dart
// Flutter
final sdk = await OnDeviceAI.initialize();

sdk.lifecycle.startObserving();

// Check memory periodically
final summary = await sdk.lifecycle.getMemorySummary();
print(summary);  // "Memory: 256.5 MB / 500.0 MB (51.3%)"
```

---

## Performance Tuning

### Optimizations by Platform

#### iOS

```swift
// Use Metal acceleration for compatible models
sdk.enableMetalAcceleration(true)

// Reduce thread count for lower-end devices
sdk.setThreadCount(1)

// Monitor memory in background
sdk.lifecycle.setPauseInferenceOnBackground(true)
```

#### Android

```kotlin
// Enable NNAPI for hardware acceleration
sdk.enableNNAPIAcceleration(true)

// Adjust thread count based on CPU cores
sdk.setThreadCount(Runtime.getRuntime().availableProcessors())

// Bind to activity lifecycle
sdk.bindLifecycle(activity)
```

### Model Optimization

- Use quantized models (int8, int4)
- Use smaller model sizes for inference
- Batch multiple requests when possible
- Cache model handles during inference sessions

---

## Troubleshooting

### Common Issues

#### Issue: "SDK not initialized"

**Solution**: Ensure `initialize()` returns successfully before using SDK.

```swift
let sdk = try await OnDeviceAI.initialize()
assert(sdk != nil, "SDK initialization failed")
```

#### Issue: "Out of memory"

**Solutions**:
1. Reduce model size or use quantized version
2. Reduce thread count
3. Enable pause-on-background
4. Implement streaming instead of full batch processing

#### Issue: "Model not found"

**Solution**: Verify model file path and permissions:

```swift
let fileExists = FileManager.default.fileExists(atPath: modelPath)
assert(fileExists, "Model file not found at \(modelPath)")
```

#### Issue: "Slow inference"

**Solutions**:
1. Enable hardware acceleration (CoreML/NNAPI)
2. Increase thread count
3. Use optimized/quantized models
4. Check for memory pressure events

### Debug Logging

```swift
// iOS - Enable debug logs
SDK.setLogLevel("debug")

// Check logs in Console.app
```

```kotlin
// Android - Enable debug logs
SDK.setLogLevel("debug")

// View logs with
// adb logcat | grep "OnDeviceAI"
```

### Profiling

#### iOS

```swift
// Profile memory usage
let memoryUsage = await sdk.lifecycle.getCurrentMemoryUsage()
print("Memory: \(memoryUsage / 1024 / 1024) MB")
```

#### Android

```kotlin
// Profile performance
val startTime = System.currentTimeMillis()
val response = sdk.llm.generate(handle, prompt)
val duration = System.currentTimeMillis() - startTime
Log.d("Perf", "Generation took $duration ms")
```

---

## Requirements

### iOS

- iOS 12.0+
- Xcode 13.0+
- Swift 5.5+

### Android

- API Level 21+
- Android Studio 4.0+
- Kotlin 1.4+

### React Native

- React Native 0.60+
- Node.js 12+

### Flutter

- Flutter 2.0+
- Dart 2.12+

---

## Support & Resources

- **Documentation**: https://docs.ondevice-ai.example.com
- **GitHub**: https://github.com/ondevice-ai/sdk
- **Issues**: https://github.com/ondevice-ai/sdk/issues
- **Discord**: https://discord.gg/ondevice-ai

---

**Version**: 1.0.0 | **License**: Apache 2.0
