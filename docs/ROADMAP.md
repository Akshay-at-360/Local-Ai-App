# OnDevice AI SDK — Product Roadmap

## Vision
Build the most developer-friendly, privacy-first, on-device AI inference SDK across all mobile and desktop platforms.

---

## v1.0.0 — Foundation Release (Current)
**Status**: Released 2026-02-07

### Core
- [x] LLM inference via llama.cpp (GGUF format)
- [x] STT inference via whisper.cpp
- [x] TTS synthesis via ONNX Runtime (Piper)
- [x] Voice Pipeline (STT → LLM → TTS)
- [x] Model Manager with secure downloads and SHA-256 verification
- [x] Memory Manager with LRU eviction
- [x] Hardware acceleration (Metal, CoreML, NNAPI)
- [x] Comprehensive error handling with recovery

### Platforms
- [x] iOS (Swift + Obj-C++ bridge)
- [x] Android (Kotlin + JNI bridge)
- [x] React Native (TypeScript + Native Modules)
- [x] Flutter (Dart + FFI)

### Quality
- [x] 135+ unit tests, 20+ property tests
- [x] Performance benchmarks
- [x] Security audit
- [x] CI/CD (GitHub Actions)

---

## v1.1.0 — Hardening & Web (Target: Q2 2026)

### New Features
- [ ] **WebAssembly platform** — Run models in the browser via WASM/WebGPU
- [ ] **Batch inference** — Process multiple prompts in parallel
- [ ] **Audio format support** — MP3, FLAC, OGG, M4A input for STT
- [ ] **Model quantization tools** — Convert models to optimized formats on-device
- [ ] **Streaming TTS output** — Real-time audio playback during synthesis

### Security
- [ ] **Certificate pinning** for model download endpoints
- [ ] **Ed25519 model signatures** — Verify model provenance
- [ ] **OSS-Fuzz integration** — Continuous fuzz testing

### Performance
- [ ] **Speculative decoding** for faster LLM inference
- [ ] **KV-cache quantization** — Reduce memory for long contexts
- [ ] **INT4/INT8 optimized kernels** for ARM NEON

### Developer Experience
- [ ] **Swift Package Manager** distribution
- [ ] **Gradle dependency** publication to Maven Central
- [ ] **Interactive documentation** website
- [ ] **Example apps** for each platform (more complete)

---

## v1.2.0 — Advanced Capabilities (Target: Q3 2026)

### New Features
- [ ] **Vision models** — Image captioning, OCR, visual Q&A
- [ ] **Multi-model concurrent inference** — Run LLM + STT simultaneously
- [ ] **On-device fine-tuning** — LoRA adapters for small models
- [ ] **Encrypted model storage** — AES-256 encryption at rest
- [ ] **Model caching** — Smart preloading based on usage patterns

### Platform Enhancements
- [ ] **macOS native** — Optimized Metal compute for Apple Silicon
- [ ] **Windows native** — DirectML acceleration
- [ ] **Linux ARM** — Raspberry Pi and embedded device support

### Voice Pipeline V2
- [ ] **Multi-turn context** — Persistent conversation across sessions
- [ ] **Speaker diarization** — Identify multiple speakers
- [ ] **Emotion detection** — Detect tone and sentiment
- [ ] **Custom wake words** — On-device wake word detection

---

## v2.0.0 — Platform & Ecosystem (Target: Q1 2027)

### Architecture
- [ ] **Plugin architecture** — Third-party model providers
- [ ] **Model marketplace** — Curated model hub with ratings
- [ ] **Cross-device sync** — Share model configurations across devices
- [ ] **Federated learning** — Privacy-preserving model improvement

### Enterprise
- [ ] **MDM integration** — Managed model deployment
- [ ] **Compliance tools** — HIPAA, GDPR audit support
- [ ] **Enterprise support SLA** — 24/7 critical issue response
- [ ] **Custom model training** service

### Research
- [ ] **Benchmark suite** — Standardized on-device AI benchmarks
- [ ] **Model compression research** — Pushing size/quality tradeoffs
- [ ] **Multi-modal models** — Text + image + audio combined inference

---

## Maintenance Policy

| Version | Support Level | Duration |
|---------|--------------|----------|
| v1.0.x | Active (bugfixes + security) | Until v1.2.0 release |
| v1.1.x | Active | Until v2.0.0 release |
| v1.2.x | Active | 12 months after release |
| v2.0.x | Active | 18 months after release |

**Hotfix criteria**: Security vulnerabilities, data loss, crashes affecting >1% of users.

---

## How to Influence the Roadmap

1. **Vote on issues** — React with 👍 on GitHub issues you care about
2. **Open feature requests** — Use the [feature request template](.github/ISSUE_TEMPLATE/feature_request.md)
3. **Contribute** — See [CONTRIBUTING.md](CONTRIBUTING.md)
4. **Discuss** — Join the GitHub Discussions for roadmap conversations
