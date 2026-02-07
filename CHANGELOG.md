# Changelog

All notable changes to the OnDevice AI SDK will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-02-07

### Added

#### Core SDK
- **SDK Manager**: Thread-safe singleton with configurable initialization, shutdown, and component lifecycle management
- **Model Manager**: Model discovery, download with progress tracking, SHA-256 integrity verification, and local registry persistence
- **Memory Manager**: Per-model memory tracking, LRU cache eviction, reference counting, configurable memory limits, and pressure callbacks
- **Logger**: Thread-safe logging with configurable levels (Debug, Info, Warning, Error), timestamps, and thread IDs
- **Callback Dispatcher**: Thread-safe callback routing from inference threads to user-specified threads, supporting both synchronous and asynchronous dispatch modes
- **Error Recovery**: Automatic retry with exponential backoff for transient failures, graceful degradation for hardware acceleration
- **Hardware Acceleration**: Runtime detection and configuration for Metal, CoreML, NNAPI, Vulkan, and CPU SIMD

#### LLM Engine (llama.cpp)
- GGUF model loading with memory-mapped I/O
- Synchronous and streaming token generation with configurable sampling (temperature, top_p, top_k, repetition_penalty, stop sequences)
- Context management with KV-cache, context window tracking, and conversation history
- Tokenization and detokenization APIs

#### STT Engine (whisper.cpp)
- Whisper model loading for speech-to-text transcription
- Voice Activity Detection (VAD) for efficient audio processing
- Multi-language support with auto-detection and English translation mode
- Word-level timestamps and confidence scores

#### TTS Engine (ONNX Runtime / Piper)
- ONNX model loading for text-to-speech synthesis
- Multi-voice support with voice configuration (speed, pitch)
- Synchronous and streaming audio generation (PCM float32, 22050 Hz)
- Phoneme-based synthesis pipeline

#### Voice Pipeline
- Full-duplex STT → LLM → TTS orchestration
- Conversation state management and turn-taking
- Interruption and cancellation support mid-generation
- VAD integration for natural conversation flow

#### iOS Platform (Swift + Objective-C++)
- Swift 5.5+ async/await API for all SDK operations
- Objective-C++ bridge layer (30 files) translating between Swift and C++ core
- CoreML and Metal hardware acceleration integration
- System memory pressure handling via `didReceiveMemoryWarning`
- App lifecycle management (background/foreground transitions)
- XCTest suite: 25+ lifecycle tests, 34+ integration tests
- Example iOS application with full SDK integration

#### Android Platform (Kotlin + JNI)
- Kotlin coroutine-based API with `suspend` functions
- JNI bridge (OnDeviceAI_jni.cpp) routing all operations to C++ core
- NNAPI hardware acceleration detection and configuration
- `ComponentCallbacks2` integration for system memory pressure
- NDK build for arm64-v8a, armeabi-v7a, x86_64
- JUnit test suite: 12+ tests
- Example Android application with streaming chat UI

#### React Native Platform (TypeScript + Native Modules)
- TypeScript API with full type definitions
- iOS native module (RCTEventEmitter subclass) bridging to C++ core
- Android native module (ReactContextBaseJavaModule) bridging through Kotlin SDK
- NativeEventEmitter for streaming tokens, download progress, errors
- Promise-based async for all operations
- Jest test suite
- Example React Native application (350+ lines)

#### Flutter Platform (Dart + FFI)
- Dart API with FFI bindings using `package:ffi`
- `Pointer<Utf8>` string marshalling for cross-language communication
- Platform-aware `DynamicLibrary` loading (iOS `.process()`, Android `.open()`)
- `flutter_test` suite
- Example Flutter application with chat UI

#### Testing Infrastructure
- **Unit Tests**: 33 test files covering all core components (Google Test)
- **Property Tests**: 10 property-based test suites with 100+ iterations each (RapidCheck)
- **Performance Benchmarks**: Model loading, inference speed, memory usage, concurrent access
- **Memory Stress Tests**: Leak detection, LRU eviction, thread safety, error path cleanup
- **Security Tests**: Input validation, path traversal, corruption detection, boundary values
- **Cross-Platform Tests**: 6 test suites verifying API consistency (TypeScript)
- **CI/CD**: GitHub Actions for Linux, macOS, Windows + AddressSanitizer + ThreadSanitizer

#### Documentation
- API Documentation (675+ lines) — Full API reference for all platforms
- Architecture Documentation (350+ lines) — System design, data flow, threading model
- Build & Distribution Guide (468+ lines) — Build instructions, dependency management
- Security Audit Report — On-device processing, PII protection, model integrity verification
- Pre-Release Checklist — Comprehensive validation checklist

### Security
- All inference runs entirely on-device — no user data transmitted during inference
- SHA-256 checksum verification for all model downloads
- HTTPS-only model downloads with retry logic
- No PII collection, logging, or transmission
- RAII memory management throughout (no raw `new`/`delete` in public API)
- Input validation on all public APIs (prevents injection, overflow, path traversal)
- AddressSanitizer and ThreadSanitizer in CI pipeline

### Known Issues
- **Duplicate ggml symbols**: llama.cpp and whisper.cpp both include ggml, causing 372 duplicate symbols when linked together. Workaround: separate test executables per engine.
- **Web platform**: Only CMakeLists.txt stub — WebAssembly support deferred to v1.1.0.
- **Cross-platform test stubs**: Platform-specific test methods return hardcoded results when not on real devices (by design).

---

## [Unreleased]

### Planned for v1.1.0
- WebAssembly (WASM) platform support
- Certificate pinning for model downloads
- Ed25519 model signature verification
- OSS-Fuzz integration for continuous fuzz testing
- Batch inference API for LLM
- Audio file format support (MP3, FLAC, OGG) for STT

### Planned for v1.2.0
- Encrypted model storage option
- Quantization-aware training support
- Vision model support (image captioning, OCR)
- Multi-model concurrent inference
- On-device fine-tuning for small models
