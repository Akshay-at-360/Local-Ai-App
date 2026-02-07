# OnDevice AI SDK - Build & Distribution Guide

**Version**: 1.0.0

## Table of Contents

1. [Build System Overview](#build-system-overview)
2. [Platform Build Instructions](#platform-build-instructions)
3. [Packaging & Distribution](#packaging--distribution)
4. [Release Process](#release-process)

---

## Build System Overview

The OnDevice AI SDK uses multiple build systems for different platforms:

- **C++ Core**: CMake 3.15+
- **iOS**: Xcode + CMake
- **Android**: Gradle + CMake
- **Web**: Emscripten + CMake
- **React Native**: Metro + native build
- **Flutter**: Dart build system

### Build Dependencies

```bash
# macOS prerequisites
brew install cmake ninja llvm@15

# For Android development
sdk install android-ndk r23b

# For Flutter
flutter pub get

# For React Native
npm install
```

---

## Platform Build Instructions

### iOS Framework Build

#### 1. Generate XCFramework

```bash
cd ios

# Build for iOS device
xcodebuild archive \
  -scheme OnDeviceAI \
  -configuration Release \
  -derivedDataPath build/Release \
  -destinationPlatform iOS

# Build for iOS simulator
xcodebuild archive \
  -scheme OnDeviceAI \
  -configuration Release \
  -derivedDataPath build/Simulator \
  -destinationPlatform iOS \
  -arch arm64

# Create XCFramework
xcodebuild -create-xcframework \
  -framework build/Release.xcarchive/Products/Library/Frameworks/OnDeviceAI.framework \
  -framework build/Simulator.xcarchive/Products/Library/Frameworks/OnDeviceAI.framework \
  -output OnDeviceAI.xcframework
```

#### 2. CocoaPods Distribution

```bash
# Create podspec
cat > OnDeviceAI.podspec << 'EOF'
Pod::Spec.new do |spec|
  spec.name         = "OnDeviceAI"
  spec.version      = "1.0.0"
  spec.license      = "Apache 2.0"
  spec.author       = "360 Labs"
  spec.homepage     = "https://ondeviceai.example.com"
  spec.source       = { git: "https://github.com/ondevice-ai/sdk.git" }
  spec.ios.deployment_target = "12.0"
  
  spec.source_files = "ios/Sources/**/*.{h,swift,mm}"
  spec.vendored_frameworks = "OnDeviceAI.xcframework"
  spec.dependency "ONNX"
end
EOF

# Push to CocoaPods
pod repo push OnDeviceAI OnDeviceAI.podspec
```

### Android AAR Build

#### 1. Build AAR

```bash
cd android

# Build release AAR
./gradlew assembleRelease

# Output: build/outputs/aar/ondeviceai-release.aar
```

#### 2. Maven Publication

```gradle
// build.gradle
publishing {
    publications {
        release(MavenPublication) {
            groupId = "com.ondeviceai"
            artifactId = "sdk"
            version = "1.0.0"
            
            artifact("$buildDir/outputs/aar/ondeviceai-release.aar")
        }
    }
    
    repositories {
        maven {
            url = "https://repo.example.com/maven"
            credentials {
                username = System.getenv("MAVEN_USERNAME")
                password = System.getenv("MAVEN_PASSWORD")
            }
        }
    }
}
```

```bash
# Publish to Maven repository
./gradlew publish
```

### React Native Package Build

#### 1. Build Native Modules

```bash
cd platforms/react-native

# Build for iOS
npm run build:ios

# Build for Android
npm run build:android
```

#### 2. NPM Publication

```bash
# Update package.json version
npm version minor

# Publish to npm
npm publish
```

### Flutter Package Build

#### 1. Build Flutter Package

```bash
cd platforms/flutter

# Generate native bindings
flutter pub get

# Build release libraries
flutter build --release

# Create pub package
flutter pub publish --dry-run
```

#### 2. Pub.dev Publication

```bash
# Publish to pub.dev
flutter pub publish
```

---

## Packaging & Distribution

### Artifact Organization

```
dist/
├── ios/
│   ├── OnDeviceAI-1.0.0.xcframework.zip
│   ├── OnDeviceAI-1.0.0.podspec
│   └── OnDeviceAI-1.0.0-iOS.tar.gz
├── android/
│   ├── ondeviceai-1.0.0-release.aar
│   ├── ondeviceai-1.0.0-javadoc.jar
│   └── ondeviceai-1.0.0-sources.jar
├── web/
│   ├── ondeviceai-1.0.0.wasm
│   ├── ondeviceai-1.0.0.js
│   └── ondeviceai-1.0.0.d.ts
├── react-native/
│   └── @ondeviceai-react-native-1.0.0.tgz
├── flutter/
│   └── ondeviceai-1.0.0.pub
└── docs/
    ├── API_DOCUMENTATION.md
    ├── PLATFORM_GUIDES.md
    └── TROUBLESHOOTING.md
```

### GitHub Release

```bash
# Create release tag
git tag -a v1.0.0 -m "Release v1.0.0"

# Push tag
git push origin v1.0.0

# Create GitHub release with artifacts
gh release create v1.0.0 \
  --title "OnDevice AI v1.0.0" \
  --notes-file RELEASE_NOTES.md \
  dist/ios/*.zip \
  dist/android/*.aar \
  dist/web/*.wasm \
  CHANGELOG.md
```

### Version Scheme

Follows Semantic Versioning: MAJOR.MINOR.PATCH

- **MAJOR**: Breaking API changes
- **MINOR**: New features, backward compatible
- **PATCH**: Bug fixes, no API changes

---

## Release Process

### Pre-Release Checklist

```markdown
## Release Checklist for v1.0.0

- [ ] All tests passing on all platforms
- [ ] Code reviewed and approved
- [ ] Documentation updated
- [ ] API documentation generated
- [ ] Performance benchmarks run
- [ ] Security audit completed
- [ ] Changelog prepared
- [ ] Version bumped appropriately
- [ ] Examples tested on actual devices
```

### Release Steps

#### 1. Prepare Release Branch

```bash
git checkout -b release/v1.0.0

# Update version numbers
sed -i 's/0.9.0/1.0.0/g' CMakeLists.txt
sed -i 's/0.9.0/1.0.0/g' package.json
sed -i 's/0.9.0/1.0.0/g' android/build.gradle
sed -i 's/0.9.0/1.0.0/g' pubspec.yaml

# Commit version update
git add .
git commit -m "Bump version to 1.0.0"
```

#### 2. Build Release Artifacts

```bash
./scripts/build-all.sh

# Verify artifacts
ls -la dist/
```

#### 3. Run Final Tests

```bash
./scripts/test-all.sh

# Test with actual devices
# - iPhone
# - Android phone
# - Web browser
# - React Native simulator
# - Flutter emulator
```

#### 4. Generate Documentation

```bash
./scripts/generate-docs.sh

# Output: docs/generated/
```

#### 5. Create Release

```bash
# Merge to main
git checkout main
git merge --no-ff release/v1.0.0

# Tag release
git tag -a v1.0.0 -m "Release v1.0.0"

# Push to GitHub
git push origin main
git push origin v1.0.0

# Create GitHub release
gh release create v1.0.0 --generate-notes
```

#### 6. Publish Packages

```bash
# iOS
pod repo push OnDeviceAI OnDeviceAI.podspec

# Android
./gradlew publish

# React Native
npm publish

# Flutter
flutter pub publish

# Web (if applicable)
npm publish --registry=https://registry.npmjs.org
```

### Post-Release

#### 1. Update Documentation Site

```bash
# Push documentation to docs site
./scripts/publish-docs.sh
```

#### 2. Announce Release

```bash
# Social media, email, Discord, forums
- "OnDevice AI v1.0.0 released!"
- Link to release notes
- Key features/improvements
```

#### 3. Bug Fix Release Process

If critical bugs found post-release:

```bash
# Create hotfix branch
git checkout -b hotfix/v1.0.1

# Fix bug
# Update tests
git commit -m "Fix critical bug in model loading"

# Merge back to main
git checkout main
git merge --no-ff hotfix/v1.0.1

# Release v1.0.1
git tag v1.0.1
git push origin main v1.0.1
```

---

## Continuous Integration

### CI/CD Pipeline

```yaml
# .github/workflows/release.yml
name: Release

on:
  push:
    tags:
      - 'v*'

jobs:
  build-ios:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build iOS Framework
        run: ./scripts/build-ios.sh
      - name: Upload artifacts
        uses: actions/upload-artifact@v2
        with:
          name: ios-framework
          path: dist/ios/

  build-android:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build Android AAR
        run: ./scripts/build-android.sh
      - name: Upload artifacts
        uses: actions/upload-artifact@v2
        with:
          name: android-aar
          path: dist/android/

  build-web:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build WebAssembly
        run: ./scripts/build-web.sh
      - name: Upload artifacts
        uses: actions/upload-artifact@v2
        with:
          name: web-wasm
          path: dist/web/

  release:
    runs-on: ubuntu-latest
    needs: [build-ios, build-android, build-web]
    steps:
      - uses: actions/checkout@v2
      - name: Download artifacts
        uses: actions/download-artifact@v2
      - name: Create Release
        run: ./scripts/create-release.sh
```

---

## Support

For build issues:

1. Check platform-specific README
2. Review CI logs
3. Open GitHub issue with build output
4. Contact support@ondeviceai.example.com

---

**Version**: 1.0.0 | **Last Updated**: 2024
