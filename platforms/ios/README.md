# OnDeviceAI iOS Bridge Layer

## Overview

This directory contains the Objective-C++ bridge layer that exposes the C++ core API to Objective-C/Swift on iOS. The bridge handles type conversions, memory management integration with ARC, and callback marshaling between C++ and Objective-C.

**Requirements**: 7.1, 7.8

## Architecture

The bridge layer consists of three main components:

### 1. Type Conversions (`ODAITypeConversions.h/mm`)

Provides bidirectional type conversion utilities between C++ and Objective-C:

- **C++ → Objective-C**: Converts C++ types (std::string, std::vector, Result<T>) to Objective-C types (NSString, NSArray, NSError)
- **Objective-C → C++**: Converts Objective-C types to C++ equivalents
- **Callback Wrappers**: Wraps Objective-C blocks as C++ std::function callbacks with proper memory management

### 2. Bridge Types (`ODAIBridgeTypes.mm`)

Implements Objective-C classes that mirror C++ data structures:

- Configuration classes (ODAISDKConfig, ODAIGenerationConfig, etc.)
- Data structures (ODAIAudioData, ODAITranscription, ODAIModelInfo, etc.)
- Device capabilities detection for iOS

### 3. Component Bridges

Objective-C++ wrapper classes for each C++ component:

- **ODAISDKManager**: Wraps `ondeviceai::SDKManager`
- **ODAIModelManager**: Wraps `ondeviceai::ModelManager`
- **ODAILLMEngine**: Wraps `ondeviceai::LLMEngine`
- **ODAISTTEngine**: Wraps `ondeviceai::STTEngine`
- **ODAITTSEngine**: Wraps `ondeviceai::TTSEngine`
- **ODAIVoicePipeline**: Wraps `ondeviceai::VoicePipeline`

## Memory Management

The bridge layer integrates C++ memory management with iOS Automatic Reference Counting (ARC):

### Ownership Model

1. **C++ Objects**: Managed by C++ SDK (singleton pattern for SDKManager, component ownership)
2. **Objective-C Wrappers**: Managed by ARC, hold non-owning pointers to C++ objects
3. **Callbacks**: Objective-C blocks are captured with strong references and wrapped as C++ callbacks

### Lifecycle

```
Initialize SDK → Create C++ SDKManager (owned by C++ singleton)
                ↓
                Create ObjC wrapper (ARC-managed, non-owning pointer)
                ↓
                Access components → Create component wrappers (ARC-managed)
                ↓
Shutdown SDK → Destroy C++ objects
              ↓
              ARC releases ObjC wrappers automatically
```

### Thread Safety

- C++ SDK handles thread safety internally
- Callbacks are dispatched to main queue for UI updates
- Objective-C wrappers are thread-safe for concurrent access

## Type Conversion Examples

### C++ to Objective-C

```cpp
// C++ Error → NSError
ondeviceai::Error cppError(ErrorCode::ModelNotFound, "Model not found");
NSError* nsError = ODAIBridge::toNSError(cppError);

// C++ ModelInfo → ODAIModelInfo
ondeviceai::ModelInfo cppInfo = {...};
ODAIModelInfo* objcInfo = ODAIBridge::toODAIModelInfo(cppInfo);

// C++ vector<string> → NSArray<NSString*>
std::vector<std::string> cppStrings = {"a", "b", "c"};
NSArray<NSString*>* nsArray = ODAIBridge::toNSArray(cppStrings);
```

### Objective-C to C++

```objc
// NSString → std::string
NSString* nsString = @"Hello";
std::string cppString = ODAIBridge::toCppString(nsString);

// ODAIGenerationConfig → GenerationConfig
ODAIGenerationConfig* objcConfig = [ODAIGenerationConfig defaultConfig];
ondeviceai::GenerationConfig cppConfig = ODAIBridge::toCppGenerationConfig(objcConfig);

// Block → std::function
ODAITokenCallback block = ^(NSString* token) {
    NSLog(@"Token: %@", token);
};
ondeviceai::TokenCallback cppCallback = ODAIBridge::wrapTokenCallback(block);
```

## Callback Handling

Callbacks are automatically marshaled to the main queue for UI updates:

```objc
// Objective-C block
ODAIProgressCallback progressBlock = ^(double progress) {
    // This executes on main queue
    self.progressBar.progress = progress;
};

// Wrapped as C++ callback
ondeviceai::ProgressCallback cppCallback = ODAIBridge::wrapProgressCallback(progressBlock);

// C++ SDK invokes callback on inference thread
// Bridge automatically dispatches to main queue
```

## Error Handling

C++ `Result<T>` types are converted to Objective-C error patterns:

```objc
NSError* error = nil;
NSString* result = [llmEngine generate:handle
                                prompt:@"Hello"
                                config:config
                                 error:&error];

if (error) {
    // Error domain: ODAIErrorDomain
    // Error code: matches C++ ErrorCode enum
    // User info: contains message, details, recovery suggestion
    NSLog(@"Error: %@", error.localizedDescription);
}
```

## Building

The bridge layer is built as part of the iOS platform target:

```bash
cd build
cmake .. -DCMAKE_SYSTEM_NAME=iOS
make ondeviceai_ios_bridge
```

## Usage Example

```objc
#import "ODAISDKManager.h"

// Initialize SDK
ODAISDKConfig* config = [ODAISDKConfig defaultConfig];
NSError* error = nil;
ODAISDKManager* sdk = [ODAISDKManager initializeWithConfig:config error:&error];

if (error) {
    NSLog(@"Failed to initialize: %@", error);
    return;
}

// Access components
ODAIModelManager* modelManager = [sdk modelManager];
ODAILLMEngine* llmEngine = [sdk llmEngine];

// Download model
uint64_t downloadHandle = [modelManager downloadModel:@"llama-3b-q4"
                                             progress:^(double progress) {
    NSLog(@"Download progress: %.1f%%", progress * 100);
}
                                                error:&error];

// Load and use model
uint64_t modelHandle = [llmEngine loadModel:@"/path/to/model.gguf" error:&error];
NSString* response = [llmEngine generate:modelHandle
                                  prompt:@"Hello, world!"
                                  config:[ODAIGenerationConfig defaultConfig]
                                   error:&error];

// Cleanup
[ODAISDKManager shutdown];
```

## Next Steps

Task 15.2 will implement the Swift API layer on top of this bridge, providing:
- Swift-native async/await patterns
- Swift-friendly naming conventions
- Type-safe Swift enums and structs
- Integration with Swift concurrency

## Files

- `OnDeviceAI-Bridging.h` - Main bridge header with Objective-C declarations
- `ODAITypeConversions.h/mm` - Type conversion utilities
- `ODAIBridgeTypes.mm` - Objective-C type implementations
- `ODAISDKManager.h/mm` - SDK manager bridge
- `ODAIModelManager.h/mm` - Model manager bridge
- `ODAILLMEngine.h` - LLM engine bridge header
- `ODAISTTEngine.h` - STT engine bridge header
- `ODAITTSEngine.h` - TTS engine bridge header
- `ODAIVoicePipeline.h` - Voice pipeline bridge header
- `CMakeLists.txt` - Build configuration

## Notes

- All bridge classes use ARC for memory management
- C++ objects are not owned by Objective-C wrappers
- Callbacks are automatically dispatched to main queue
- Error handling follows Objective-C conventions (NSError**)
- Thread safety is handled by C++ core, wrappers are thread-safe


## Swift API Layer

Task 15.2 has implemented the Swift API layer on top of the Objective-C++ bridge, providing:

### Features

- **Swift-native async/await patterns**: All asynchronous operations use Swift concurrency
- **Idiomatic Swift naming**: Follows Swift API design guidelines
- **Type-safe Swift types**: Enums, structs, and error handling
- **Automatic memory management**: Integrates with ARC
- **Comprehensive error handling**: Swift Error protocol with detailed error types

### Swift Files

- `OnDeviceAI.swift` - Main SDK entry point
- `ModelManager.swift` - Model discovery and management with async/await
- `LLMEngine.swift` - Language model inference with async/await
- `STTEngine.swift` - Speech-to-text transcription with async/await
- `TTSEngine.swift` - Text-to-speech synthesis with async/await
- `VoicePipeline.swift` - Voice conversation orchestration with async/await
- `Types.swift` - Swift type definitions and conversions
- `SDKError.swift` - Swift error types

### Swift Usage Example

```swift
import OnDeviceAI

// Initialize the SDK
let sdk = try OnDeviceAI.initialize()

// List available models (async)
let models = try await sdk.modelManager.listAvailableModels(type: .llm)

// Download a model (async with progress)
try await sdk.modelManager.downloadModel("llama-3b-q4") { progress in
    print("Download progress: \(progress * 100)%")
}

// Load and use the model (async)
let modelPath = try sdk.modelManager.getModelPath("llama-3b-q4")
let modelHandle = try await sdk.llm.loadModel(path: modelPath)

// Generate text (async)
let response = try await sdk.llm.generate(
    model: modelHandle,
    prompt: "Hello, how are you?",
    config: .default
)
print("Response: \(response)")

// Streaming generation (async)
try await sdk.llm.generateStreaming(
    model: modelHandle,
    prompt: "Tell me a story",
    config: .default
) { token in
    print(token, terminator: "")
}

// Cleanup
try sdk.llm.unloadModel(modelHandle)
OnDeviceAI.shutdown()
```

### Swift Error Handling

```swift
do {
    let response = try await sdk.llm.generate(model: handle, prompt: "Hello")
} catch SDKError.modelNotFound(let message) {
    print("Model not found: \(message)")
} catch SDKError.inferenceError(let message) {
    print("Inference failed: \(message)")
} catch {
    print("Unknown error: \(error)")
}
```

### Swift Concurrency

All async operations use Swift's structured concurrency:

```swift
// Concurrent model loading
async let sttModel = sdk.stt.loadModel(path: sttPath)
async let llmModel = sdk.llm.loadModel(path: llmPath)
async let ttsModel = sdk.tts.loadModel(path: ttsPath)

let (stt, llm, tts) = try await (sttModel, llmModel, ttsModel)

// Configure voice pipeline
try sdk.voicePipeline.configure(
    sttModel: stt,
    llmModel: llm,
    ttsModel: tts
)
```

### Integration

The Swift API layer can be integrated into iOS projects using:

1. **CocoaPods**: Add to Podfile
2. **Swift Package Manager**: Add as dependency
3. **Manual Integration**: Add Swift files and link with bridge library

### Requirements

- iOS 13.0+ (for async/await support)
- Xcode 14.0+
- Swift 5.7+

## Status Update

- [x] Task 15.1: Objective-C++ bridge layer (completed)
- [x] Task 15.2: Swift API layer (completed)
- [ ] Task 15.3: Core ML acceleration
- [ ] Task 15.4: iOS lifecycle management
- [ ] Task 15.5: iOS-specific tests
- [ ] Task 15.6: iOS example application
