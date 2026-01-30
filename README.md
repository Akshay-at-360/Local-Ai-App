# OnDevice AI SDK

A comprehensive multi-platform SDK for on-device AI capabilities including LLM inference, Speech-to-Text, Text-to-Speech, and voice conversation pipelines.

## Features

- **LLM Inference**: Run language models (100M-5B parameters) completely on-device
- **Speech-to-Text**: Transcribe audio using Whisper models
- **Text-to-Speech**: Synthesize natural speech from text
- **Voice Pipeline**: End-to-end voice conversation orchestration
- **Cross-Platform**: iOS, Android, React Native, Flutter, and Web support
- **Privacy-First**: All processing happens on-device, no data transmission
- **Offline Operation**: Works without internet connectivity
- **Hardware Acceleration**: Leverages GPU/NPU when available

## Supported Platforms

- **iOS**: Swift API with Core ML acceleration
- **Android**: Kotlin API with NNAPI acceleration
- **React Native**: TypeScript API for cross-platform mobile
- **Flutter**: Dart API for cross-platform mobile
- **Web**: JavaScript API with WebGPU acceleration

## Project Structure

```
.
├── core/                   # C++ core library
│   ├── include/           # Public headers
│   └── src/               # Implementation
├── platforms/             # Platform-specific wrappers
│   ├── ios/              # iOS/Swift SDK
│   ├── android/          # Android/Kotlin SDK
│   ├── react-native/     # React Native/TypeScript SDK
│   ├── flutter/          # Flutter/Dart SDK
│   └── web/              # Web/JavaScript SDK
├── tests/                 # Test suite
│   ├── unit/             # Unit tests
│   └── property/         # Property-based tests
├── examples/              # Example applications
└── docs/                  # Documentation

```

## Building

### Prerequisites

- CMake 3.20 or higher
- C++17 compatible compiler
- Ninja (optional, recommended)

### Build Instructions

```bash
# Configure
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run tests
cd build && ctest --output-on-failure
```

### Build Options

- `BUILD_TESTS`: Build test suite (default: ON)
- `BUILD_IOS`: Build iOS framework (default: OFF)
- `BUILD_ANDROID`: Build Android library (default: OFF)
- `BUILD_WASM`: Build WebAssembly (default: OFF)
- `ENABLE_ASAN`: Enable AddressSanitizer (default: OFF)
- `ENABLE_TSAN`: Enable ThreadSanitizer (default: OFF)

## Quick Start

```cpp
#include <ondeviceai/ondeviceai.hpp>

using namespace ondeviceai;

int main() {
    // Initialize SDK
    auto config = SDKConfig::defaults();
    config.model_directory = "./models";
    auto sdk_result = SDKManager::initialize(config);
    
    if (sdk_result.isError()) {
        std::cerr << "Failed to initialize SDK: " 
                  << sdk_result.error().message << std::endl;
        return 1;
    }
    
    auto* sdk = sdk_result.value();
    
    // Load a model
    auto* llm = sdk->getLLMEngine();
    auto model_result = llm->loadModel("./models/llama-3b-q4.gguf");
    
    if (model_result.isSuccess()) {
        auto handle = model_result.value();
        
        // Generate text
        auto response = llm->generate(handle, "Hello, how are you?");
        if (response.isSuccess()) {
            std::cout << "Response: " << response.value() << std::endl;
        }
        
        llm->unloadModel(handle);
    }
    
    // Cleanup
    SDKManager::shutdown();
    return 0;
}
```

## Development Status

This project is currently in early development. The infrastructure and core architecture are in place, but model integration (llama.cpp, whisper.cpp, ONNX Runtime) is pending.

### Completed
- ✅ Project structure and build system
- ✅ Core C++ architecture
- ✅ Type system and error handling
- ✅ SDK Manager and component lifecycle
- ✅ Memory management with LRU cache
- ✅ Logging system
- ✅ Basic test infrastructure
- ✅ CI/CD pipeline

### In Progress
- 🚧 Model Manager implementation
- 🚧 LLM Engine (llama.cpp integration)
- 🚧 STT Engine (whisper.cpp integration)
- 🚧 TTS Engine (ONNX Runtime integration)

### Planned
- 📋 Platform wrappers (iOS, Android, React Native, Flutter, Web)
- 📋 Property-based testing
- 📋 Example applications
- 📋 Documentation

## Testing

The project uses Google Test for unit testing and will include property-based testing with RapidCheck.

```bash
# Run all tests
cd build && ctest --output-on-failure

# Run with sanitizers
cmake -B build -DENABLE_ASAN=ON
cmake --build build
cd build && ctest --output-on-failure
```

## Code Quality

The project uses:
- **clang-format**: Code formatting
- **clang-tidy**: Static analysis
- **AddressSanitizer**: Memory error detection
- **ThreadSanitizer**: Thread safety verification

## Contributing

Contributions are welcome! Please ensure:
- Code follows the project style (clang-format)
- All tests pass
- New features include tests
- Documentation is updated

## License

[License TBD]

## Contact

[Contact information TBD]
