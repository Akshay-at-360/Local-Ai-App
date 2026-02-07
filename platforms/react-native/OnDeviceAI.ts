/**
 * OnDeviceAI.ts
 * OnDevice AI React Native SDK — TypeScript API layer.
 *
 * Every public method delegates to native code through NativeModuleBridge.
 * Requirements: 7.3, 7.6, 7.8
 */

import { Native, OnDeviceAIEventEmitter, Events } from './NativeModuleBridge';
import type { EmitterSubscription } from 'react-native';

// ---------------------------------------------------------------------------
// Public types
// ---------------------------------------------------------------------------

export interface SDKConfig {
  threadCount?: number;
  modelDirectory?: string;
  memoryLimitMB?: number;
  logLevel?: 'debug' | 'info' | 'warning' | 'error';
}

export interface ModelInfo {
  id: string;
  name: string;
  type: 'llm' | 'stt' | 'tts';
  sizeBytes: number;
  version: string;
}

export interface StorageInfo {
  totalBytes: number;
  usedBytes: number;
  availableBytes: number;
}

export interface GenerationConfig {
  temperature?: number;
  topP?: number;
  maxTokens?: number;
}

export interface SynthesisConfig {
  voiceId?: string;
  speed?: number;
  pitch?: number;
}

export class SDKError extends Error {
  code: number;
  details?: string;
  recoverySuggestion?: string;

  constructor(code: number, message: string, details?: string, recovery?: string) {
    super(message);
    this.name = 'SDKError';
    this.code = code;
    this.details = details;
    this.recoverySuggestion = recovery;
  }
}

// ---------------------------------------------------------------------------
// Model Manager
// ---------------------------------------------------------------------------

export class ModelManager {
  async listAvailableModels(): Promise<ModelInfo[]> {
    const json = await Native.listAvailableModels();
    return JSON.parse(json) as ModelInfo[];
  }

  async listDownloadedModels(): Promise<ModelInfo[]> {
    const json = await Native.listDownloadedModels();
    return JSON.parse(json) as ModelInfo[];
  }

  async downloadModel(
    modelId: string,
    onProgress?: (current: number, total: number) => void,
  ): Promise<ModelInfo> {
    let sub: EmitterSubscription | null = null;
    if (onProgress) {
      sub = OnDeviceAIEventEmitter.addListener(
        Events.DOWNLOAD_PROGRESS,
        (e: { current: number; total: number }) => onProgress(e.current, e.total),
      );
    }
    try {
      const json = await Native.downloadModel(modelId);
      return JSON.parse(json) as ModelInfo;
    } finally {
      sub?.remove();
    }
  }

  async deleteModel(modelId: string): Promise<void> {
    await Native.deleteModel(modelId);
  }

  async getStorageInfo(): Promise<StorageInfo> {
    const json = await Native.getStorageInfo();
    return JSON.parse(json) as StorageInfo;
  }
}

// ---------------------------------------------------------------------------
// LLM Engine
// ---------------------------------------------------------------------------

export class LLMEngine {
  async loadModel(path: string): Promise<number> {
    return Native.llmLoadModel(path);
  }

  async unloadModel(handle: number): Promise<void> {
    return Native.llmUnloadModel(handle);
  }

  async generate(handle: number, prompt: string, config?: GenerationConfig): Promise<string> {
    return Native.llmGenerate(handle, prompt, JSON.stringify(config ?? {}));
  }

  async generateStreaming(
    handle: number,
    prompt: string,
    onToken: (token: string) => void,
    config?: GenerationConfig,
  ): Promise<void> {
    const sub = OnDeviceAIEventEmitter.addListener(
      Events.TOKEN,
      (evt: { token: string }) => onToken(evt.token),
    );
    try {
      await Native.llmGenerateStreaming(handle, prompt, JSON.stringify(config ?? {}));
    } finally {
      sub.remove();
    }
  }
}

// ---------------------------------------------------------------------------
// STT Engine
// ---------------------------------------------------------------------------

export class STTEngine {
  async loadModel(path: string): Promise<number> {
    return Native.sttLoadModel(path);
  }

  async unloadModel(handle: number): Promise<void> {
    return Native.sttUnloadModel(handle);
  }

  async transcribe(handle: number, audioUri: string): Promise<string> {
    return Native.sttTranscribe(handle, audioUri);
  }
}

// ---------------------------------------------------------------------------
// TTS Engine
// ---------------------------------------------------------------------------

export class TTSEngine {
  async loadModel(path: string): Promise<number> {
    return Native.ttsLoadModel(path);
  }

  async unloadModel(handle: number): Promise<void> {
    return Native.ttsUnloadModel(handle);
  }

  async synthesize(
    handle: number,
    text: string,
    config?: SynthesisConfig,
  ): Promise<string> {
    // Returns a file URI to the synthesized audio
    return Native.ttsSynthesize(handle, text, JSON.stringify(config ?? {}));
  }
}

// ---------------------------------------------------------------------------
// Voice Pipeline
// ---------------------------------------------------------------------------

export class VoicePipeline {
  async configure(sttPath: string, llmPath: string, ttsPath: string): Promise<void> {
    return Native.vpConfigure(sttPath, llmPath, ttsPath);
  }

  stop(): Promise<void> {
    return Native.vpStop();
  }
}

// ---------------------------------------------------------------------------
// Lifecycle Manager
// ---------------------------------------------------------------------------

export class LifecycleManager {
  private memorySub: EmitterSubscription | null = null;

  startObserving(onMemoryWarning?: (used: number, total: number) => void): void {
    if (this.memorySub) return;
    if (onMemoryWarning) {
      this.memorySub = OnDeviceAIEventEmitter.addListener(
        Events.MEMORY_WARNING,
        (e: { used: number; total: number }) => onMemoryWarning(e.used, e.total),
      );
    }
  }

  stopObserving(): void {
    this.memorySub?.remove();
    this.memorySub = null;
  }

  async getMemoryUsage(): Promise<number> {
    return Native.getMemoryUsage();
  }

  async getMemoryLimit(): Promise<number> {
    return Native.getMemoryLimit();
  }
}

// ---------------------------------------------------------------------------
// Main SDK class
// ---------------------------------------------------------------------------

export class OnDeviceAI {
  private static instance: OnDeviceAI | null = null;

  readonly modelManager = new ModelManager();
  readonly llm = new LLMEngine();
  readonly stt = new STTEngine();
  readonly tts = new TTSEngine();
  readonly voicePipeline = new VoicePipeline();
  readonly lifecycle = new LifecycleManager();

  private constructor() {}

  static async initialize(config: SDKConfig = {}): Promise<OnDeviceAI> {
    if (this.instance) throw new SDKError(-2, 'SDK already initialized');
    if ((config.threadCount ?? 2) <= 0) throw new SDKError(-2, 'threadCount must be > 0');

    await Native.initialize(JSON.stringify(config));
    const sdk = new OnDeviceAI();
    this.instance = sdk;
    return sdk;
  }

  static getInstance(): OnDeviceAI | null {
    return this.instance;
  }

  static async shutdown(): Promise<void> {
    if (!this.instance) return;
    this.instance.lifecycle.stopObserving();
    await Native.shutdown();
    this.instance = null;
  }

  async setThreadCount(count: number): Promise<void> {
    return Native.setThreadCount(count);
  }

  async setLogLevel(level: number): Promise<void> {
    return Native.setLogLevel(level);
  }

  async setMemoryLimit(bytes: number): Promise<void> {
    return Native.setMemoryLimit(bytes);
  }
}

export default OnDeviceAI;
