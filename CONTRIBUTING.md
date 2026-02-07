# Contributing to OnDevice AI SDK

Thank you for your interest in contributing to the OnDevice AI SDK!

## Getting Started

1. **Fork** the repository
2. **Clone** your fork: `git clone https://github.com/YOUR_USERNAME/Local-Ai-App.git`
3. **Create a branch**: `git checkout -b feature/your-feature-name`
4. **Build**: `cmake -B build && cmake --build build`
5. **Test**: `cd build && ctest --output-on-failure`

## Development Setup

### Prerequisites
- C++17 compiler (GCC 9+, Clang 12+, MSVC 2019+)
- CMake 3.20+
- Git

### Optional (for platform development)
- **iOS**: macOS + Xcode 14+, Swift 5.5+
- **Android**: Android NDK r25+, Kotlin 1.8+
- **React Native**: Node.js 18+, React Native 0.72+
- **Flutter**: Flutter 3.x, Dart 3.0+

### Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build --parallel $(nproc)
cd build && ctest --output-on-failure
```

## Code Guidelines

### C++ Core
- **Standard**: C++17
- **Style**: Follow existing code patterns (RAII, smart pointers, no raw new/delete)
- **Headers**: `.hpp` extension, include guards via `#pragma once`
- **Errors**: Return `Result<T>` types, never throw exceptions across API boundary
- **Thread safety**: Protect shared state with `std::mutex`, use `std::atomic` for flags
- **Naming**: `snake_case` for functions/variables, `PascalCase` for types, `UPPER_CASE` for constants

### Platform Wrappers
- Follow platform-idiomatic conventions (Swift naming, Kotlin conventions, etc.)
- Bridge code should be minimal — delegate to C++ core
- Never duplicate business logic in platform code

### Tests
- Every new public API method needs at least one unit test
- Property tests for invariants (with RapidCheck)
- Use appropriate test fixture (`::testing::Test` subclass)
- Name tests clearly: `TEST(ComponentTest, WhatItVerifies)`

## Pull Request Process

1. **Ensure all tests pass**: `cd build && ctest --output-on-failure`
2. **Run sanitizers**: Build with `-DENABLE_ASAN=ON` or `-DENABLE_TSAN=ON`
3. **Update documentation** if you changed any public API
4. **Write a clear PR description** explaining what and why
5. **Link related issues** using `Fixes #123` or `Closes #123`

## Reporting Issues

- Use the appropriate [issue template](.github/ISSUE_TEMPLATE/)
- Include SDK version, platform, device, and steps to reproduce
- For performance issues, include measurements and device specs

## Security

If you discover a security vulnerability, please report it privately via
GitHub Security Advisories rather than opening a public issue.

## License

By contributing, you agree that your contributions will be licensed under
the same license as the project.
