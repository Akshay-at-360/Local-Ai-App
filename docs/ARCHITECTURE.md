# Architecture Documentation

This document describes the architecture of the OnDevice AI SDK — a cross-platform, privacy-first AI inference framework that runs LLM, STT, and TTS models entirely on-device.

---

## Overview

The SDK follows a **three-tier architecture** ensuring code reuse across platforms while exposing idiomatic APIs to each ecosystem:

```
┌─────────────────────────────────────────────────────────┐
│                    Platform Layer                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────┐  │
│  │   iOS     │ │ Android  │ │  React   │ │  Flutter  │  │
│  │  Swift    │ │  Kotlin  │ │  Native  │ │   Dart    │  │
│  │ async/    │ │ coroutin │ │  TS +    │ │  FFI +    │  │
│  │  await    │ │ es + JNI │ │  Native  │ │  Pointer  │  │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬──────┘  │
│       │             │            │             │         │
├───────┼─────────────┼────────────┼─────────────┼────────┤
│       │       Bridge Layer       │             │         │
│  ┌────┴─────┐ ┌────┴─────┐ ┌────┴─────┐ ┌────┴──────┐  │
│  │  ObjC++  │ │   JNI    │ │ ObjC++/  │ │  dart:ffi │  │
│  │  Bridge  │ │  Bridge  │ │ JNI Mod  │ │ bindings  │  │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬──────┘  │
│       │             │            │             │         │
├───────┴─────────────┴────────────┴─────────────┴────────┤
│                     Core Layer (C++)                     │
│  ┌───────────────────────────────────────────────────┐   │
│  │  SDKManager  ModelManager  MemoryManager  Logger  │   │
│  │  LLMEngine   STTEngine    TTSEngine               │   │
│  │  VoicePipeline  CallbackDispatcher  ErrorRecovery │   │
│  │  HardwareAcceleration  DownloadManager            │   │
│  └───────────────────────────────────────────────────┘   │
│            │              │              │                │
│       ┌────┴───┐    ┌────┴───┐    ┌─────┴──────┐        │
│       │llama.  │    │whisper │    │ONNX Runtime│        │
│       │  cpp   │    │  .cpp  │    │  (Piper)   │        │
│       └────────┘    └────────┘    └────────────┘        │
└─────────────────────────────────────────────────────────┘
```

---

## Core Components

### SDK Manager (`sdk_manager.cpp` — 327 lines)
- Central coordinator: initialization, shutdown, and global lifecycle
- Singleton pattern with thread-safe `std::call_once`
- Owns instances of all engine components
- Validates configuration before propagation

### Model Manager (`model_manager.cpp`)
- Model discovery: scans local storage and remote registries
- Secure downloads with SHA-256 verification
- Manages model registry (local manifest + remote endpoints)
- Supports GGUF (LLM), whisper (STT), and ONNX (TTS) formats

### LLM Engine (`llm_engine.cpp` — 1296 lines)
- Wraps **llama.cpp** for transformer-based text generation
- GGUF model loading with memory-mapped I/O
- Streaming token generation with callback support
- Context management with KV-cache
- Configurable sampling: temperature, top_p, top_k, repeat penalty, stop tokens

### STT Engine (`stt_engine.cpp` — 495 lines)
- Wraps **whisper.cpp** for speech-to-text transcription
- 16kHz mono PCM audio preprocessing
- Voice Activity Detection (VAD) for efficient processing
- Supports multiple languages and translation mode
- Returns segments with timestamps

### TTS Engine (`tts_engine.cpp` — 793 lines)
- Wraps **ONNX Runtime** running Piper TTS models
- Phoneme-based synthesis pipeline
- Multi-voice support with voice configuration
- Produces raw PCM audio (22050 Hz, float32)
- Configurable speed and speaking rate

### Voice Pipeline (`voice_pipeline.cpp`)
- Full-duplex orchestration: STT → LLM → TTS
- Manages conversation state and turn-taking
- Supports interruption and cancellation mid-generation
- Integrates VAD for natural conversation flow

### Memory Manager (`memory_manager.cpp`)
- Tracks memory usage per loaded model
- LRU eviction when approaching configurable limits
- Responds to system memory pressure callbacks (iOS/Android)
- Reports per-component and total memory usage

### Callback Dispatcher (`callback_dispatcher.cpp`)
- Thread-safe callback routing from inference threads to user callbacks
- Ensures callbacks respect platform threading rules
- Supports streaming tokens, progress updates, error notifications

### Hardware Acceleration (`hardware_acceleration.cpp`)
- Detects available accelerators at runtime (Metal, CoreML, NNAPI, GPU)
- Configures backend-specific optimizations
- Falls back gracefully when acceleration unavailable

---

## Data Flow

### Text Generation (LLM)

```
User Prompt
    │
    ▼
┌───────────────┐     ┌──────────────┐     ┌──────────────┐
│  Platform API  │────▶│ Bridge Layer │────▶│  LLM Engine  │
│ generate(...)  │     │  (JNI/ObjC/  │     │  (llama.cpp) │
│                │     │  FFI bridge) │     │              │
└───────────────┘     └──────────────┘     └──────┬───────┘
                                                   │
                                          ┌────────▼────────┐
                                          │   Tokenize       │
                                          │   Sample tokens   │
                                          │   Stream callback │
                                          │   Detokenize      │
                                          └────────┬─────────┘
                                                   │
                                          ┌────────▼────────┐
                                          │ Callback         │
                                    ◀─────│ Dispatcher       │
    Token-by-token streaming              └─────────────────┘
```

### Voice Pipeline (STT → LLM → TTS)

```
Microphone Audio (PCM 16kHz)
    │
    ▼
┌─────────┐   transcript   ┌─────────┐   tokens    ┌─────────┐
│   STT   │───────────────▶│   LLM   │────────────▶│   TTS   │
│ Engine  │                 │ Engine  │             │ Engine  │
└─────────┘                 └─────────┘             └────┬────┘
                                                         │
                                                    Audio PCM
                                                    (22050 Hz)
                                                         │
                                                         ▼
                                                     Speaker
```

**Interruption flow**: When new audio is detected while TTS is playing, the Voice Pipeline cancels the current TTS generation, feeds new audio to STT, and restarts the cycle.

---

## Threading Model

```
┌────────────────────────────────────────────────────┐
│                    Main Thread                      │
│   init() / shutdown() / configure() / model mgmt   │
└──────────────────────┬─────────────────────────────┘
                       │ dispatch
    ┌──────────────────┼──────────────────┐
    │                  │                  │
┌───▼────┐       ┌────▼────┐       ┌─────▼─────┐
│Inference│       │  I/O    │       │ Callback  │
│ Thread  │       │ Thread  │       │  Thread   │
│ Pool    │       │         │       │           │
│         │       │download │       │ streaming │
│ LLM gen │       │file ops │       │ tokens    │
│ STT tx  │       │model    │       │ progress  │
│ TTS syn │       │ load    │       │ errors    │
└─────────┘       └─────────┘       └───────────┘
```

- **Main Thread**: Initialization, configuration, model management
- **Inference Thread Pool**: Parallel inference execution (one thread per active engine)
- **I/O Thread**: File operations, network downloads, model I/O
- **Callback Thread**: Streaming callbacks, progress updates (ensures no blocking of inference)
- **Synchronization**: `std::mutex` + `std::condition_variable` for cross-thread coordination

---

## Error Handling

All operations return `Result<T>` types with structured error information:

```cpp
struct Error {
    ErrorCode code;         // Programmatic handling
    std::string message;    // Human-readable description
    std::string details;    // Technical debugging info
    std::string suggestion; // Optional recovery hint
};
```

Error categories:
| Category | Examples |
|----------|---------|
| `initialization_failed` | Missing config, invalid paths |
| `model_not_found` | Model file missing or corrupted |
| `model_load_failed` | OOM, unsupported format |
| `inference_failed` | Context overflow, generation error |
| `download_failed` | Network error, checksum mismatch |
| `memory_pressure` | Insufficient memory for model |
| `hardware_unavailable` | Accelerator not supported |

Automatic error recovery handles transient failures (retries for downloads, fallback for HW acceleration).

---

## Memory Management

- **Memory-mapped I/O**: Models loaded via `mmap` for efficient memory use, allowing OS to page sections in/out
- **Reference counting**: `std::shared_ptr` semantics for automatic cleanup when engines are released
- **LRU cache**: Least-recently-used eviction when total memory exceeds configurable threshold
- **Platform pressure callbacks**: Responds to `didReceiveMemoryWarning` (iOS) / `onTrimMemory` (Android)
- **Configurable limits**: Per-model and total SDK memory caps set via `SDKConfig`

---

## Platform Bridge Details

### iOS (Swift + Objective-C++ Bridge)

```
Swift API (async/await)
    │
    ▼
ObjC++ Headers (ODAILLMEngine.h)
    │
    ▼
ObjC++ Implementation (.mm files)
    │  ┌─ Type conversions (NSString ↔ std::string)
    │  ├─ NSError creation from C++ Error
    │  └─ dispatch_async for callbacks
    ▼
C++ Core (direct #include and function calls)
```

- 30 files: 9 Swift API + 11 ObjC++ bridge + tests + example
- Uses `withCheckedThrowingContinuation` for async wrapping
- CoreML and Metal acceleration via `hardware_acceleration.cpp`

### Android (Kotlin + JNI Bridge)

```
Kotlin API (coroutines)
    │  external fun declarations
    ▼
JNI Bridge (OnDeviceAI_jni.cpp)
    │  JNI_OnLoad, jstring ↔ std::string
    │  GetMethodID for callbacks
    ▼
C++ Core (linked via CMake)
```

- `System.loadLibrary("ondeviceai")` loads the native shared library
- All blocking operations wrapped in `withContext(Dispatchers.IO)` for coroutine safety
- Implements `ComponentCallbacks2` for system memory pressure
- NNAPI acceleration detection at build time

### React Native (TypeScript + Native Modules)

```
TypeScript API
    │  NativeModules.OnDeviceAI
    ▼
┌────────────────────┬────────────────────┐
│ iOS Native Module  │ Android Native Mod │
│ (RCTEventEmitter)  │ (ReactContext      │
│  ObjC++ → C++      │  BaseJavaModule)   │
│                    │  Kotlin → JNI → C++│
└────────────────────┴────────────────────┘
```

- `NativeEventEmitter` for streaming tokens, download progress, errors
- JSON serialization for complex config objects across the bridge
- Promise-based async for all operations

### Flutter (Dart + FFI)

```
Dart API
    │  NativeFunctions.*
    ▼
dart:ffi bindings (platform_channel.dart)
    │  Pointer<Utf8> marshalling
    │  DynamicLibrary.open() / .process()
    ▼
C shared library symbols (C API)
```

- `package:ffi` for `Pointer<Utf8>` ↔ `String` conversion
- `calloc.allocate()` / `calloc.free()` for native memory
- `lookupFunction<NativeFunc, DartFunc>` for type-safe binding
- Platform-aware library loading (iOS `.process()`, Android `.open()`)

---

## Hardware Acceleration

| Platform | Accelerator | Framework | Detection |
|----------|------------|-----------|-----------|
| iOS | GPU | Metal | Always available (A7+) |
| iOS | Neural Engine | CoreML | A11+ Bionic |
| Android | NPU/DSP | NNAPI | API 27+, runtime probe |
| Android | GPU | Vulkan Compute | Runtime probe |
| Desktop | GPU | CUDA / ROCm | Build-time config |
| All | CPU | SIMD (NEON/AVX) | Compile-time detection |

Fallback chain: Preferred accelerator → Next available → CPU (always works).

---

## Build System

```
CMakeLists.txt (root)
├── core/CMakeLists.txt          # C++ core library
│   ├── Links: llama.cpp, whisper.cpp, onnxruntime
│   └── Exports: ondeviceai_core static/shared lib
├── platforms/ios/CMakeLists.txt  # iOS framework
├── platforms/android/CMakeLists.txt  # Android .so via NDK
└── tests/
    ├── unit/CMakeLists.txt       # Google Test suite
    └── property/CMakeLists.txt   # Property-based tests
```

- **C++17** standard required
- **Conan** for dependency management (`conanfile.txt`)
- **NDK** cross-compilation for Android (arm64-v8a, armeabi-v7a, x86_64)
- **Xcode** integration for iOS framework builds

---

## Security

- **On-device only**: No data leaves the device during inference
- **Model integrity**: SHA-256 verification on download and load
- **Secure downloads**: HTTPS-only with certificate pinning support
- **Memory safety**: RAII patterns, no raw `new`/`delete` in public API
- **Input validation**: All public APIs validate parameters before processing

---

## File Structure Reference

```
core/
├── include/ondeviceai/   # 19 public headers
│   ├── ondeviceai.h      # Umbrella header
│   ├── sdk_manager.h     # SDKManager, SDKConfig
│   ├── llm_engine.h      # LLMEngine, GenerationConfig
│   ├── stt_engine.h      # STTEngine, TranscriptionConfig
│   ├── tts_engine.h      # TTSEngine, SynthesisConfig
│   ├── voice_pipeline.h  # VoicePipeline
│   ├── model_manager.h   # ModelManager, ModelInfo
│   ├── memory_manager.h  # MemoryManager, MemoryStats
│   └── types.h           # Shared types, Result<T>, Error
├── src/                   # 17 implementation files
└── CMakeLists.txt

platforms/
├── ios/          # 30 files (Swift + ObjC++ bridge)
├── android/      # Kotlin + JNI + CMake + Gradle
├── react-native/ # TypeScript + iOS/Android native modules
├── flutter/      # Dart FFI + pubspec
└── web/          # (minimal — future)

tests/
├── unit/         # 33 Google Test files
├── property/     # 10 property-based test files
└── CrossPlatformTests.ts

docs/
├── API_DOCUMENTATION.md    # 675 lines — full API reference
├── BUILD_AND_DISTRIBUTION.md # 468 lines — build & release guide
└── ARCHITECTURE.md           # This file
```

For detailed design specs, see `.kiro/specs/on-device-ai-sdk/design.md`.
