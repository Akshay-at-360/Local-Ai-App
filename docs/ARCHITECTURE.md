# Architecture Documentation

This document describes the architecture of the OnDevice AI SDK.

## Overview

The SDK follows a three-tier architecture:

1. **Platform Layer**: Native SDKs in platform-specific languages
2. **Bridge Layer**: Translation between platform languages and C++
3. **Core Layer**: Shared C++ implementation

## Core Components

### SDK Manager
- Central coordinator for initialization and lifecycle
- Manages global configuration
- Provides access to all components

### Model Manager
- Handles model discovery, download, and storage
- Manages model registry (local and remote)
- Verifies model integrity

### LLM Engine
- Wraps llama.cpp for language model inference
- Manages tokenization and generation
- Handles context and conversation history

### STT Engine
- Wraps whisper.cpp for speech recognition
- Handles audio preprocessing
- Implements Voice Activity Detection

### TTS Engine
- Wraps ONNX Runtime for speech synthesis
- Manages voice selection
- Generates audio waveforms

### Voice Pipeline
- Orchestrates STT → LLM → TTS flow
- Manages conversation state
- Handles interruptions and cancellations

### Memory Manager
- Tracks memory usage per model
- Implements LRU cache for model eviction
- Monitors memory pressure

## Threading Model

- **Main Thread**: Initialization, configuration, model management
- **Inference Thread Pool**: Parallel inference execution
- **I/O Thread**: File operations, network downloads
- **Callback Thread**: Streaming callbacks, progress updates

## Error Handling

All operations return `Result<T>` types with detailed error information:
- Error code for programmatic handling
- Human-readable message
- Technical details for debugging
- Optional recovery suggestion

## Memory Management

- Memory mapping for efficient model loading
- Reference counting for automatic cleanup
- LRU cache for model eviction under pressure
- Configurable memory limits

## Platform Integration

Each platform wrapper:
- Exposes consistent API patterns
- Uses platform-idiomatic async patterns
- Integrates with platform memory management
- Leverages platform-specific acceleration

For detailed design information, see `.kiro/specs/on-device-ai-sdk/design.md`
