/**
 * NativeModuleBridge.ts
 * React Native ↔ Native (iOS/Android) bridge layer.
 *
 * Imports the Turbo-Module/NativeModules interface and wraps every
 * call so the TypeScript layer never touches raw NativeModules directly.
 *
 * Requirements: 7.3, 7.8
 */

import { NativeModules, NativeEventEmitter, Platform } from 'react-native';

// ---------------------------------------------------------------------------
// Raw native module reference
// ---------------------------------------------------------------------------

const RawModule = NativeModules.OnDeviceAI;

if (!RawModule) {
  throw new Error(
    'OnDeviceAI native module not found. ' +
    'Make sure native modules are linked (run `pod install` on iOS).'
  );
}

// ---------------------------------------------------------------------------
// Typed bridge functions
// ---------------------------------------------------------------------------

export interface NativeBridge {
  // SDK lifecycle
  initialize(config: string): Promise<void>;
  shutdown(): Promise<void>;

  // Configuration
  setThreadCount(count: number): Promise<void>;
  setLogLevel(level: number): Promise<void>;
  setMemoryLimit(bytes: number): Promise<void>;
  setCallbackThreadCount(count: number): Promise<void>;

  // Model management
  listAvailableModels(): Promise<string>;  // JSON-encoded array
  listDownloadedModels(): Promise<string>;
  downloadModel(modelId: string): Promise<string>;
  deleteModel(modelId: string): Promise<void>;
  getStorageInfo(): Promise<string>;

  // LLM
  llmLoadModel(path: string): Promise<number>;
  llmUnloadModel(handle: number): Promise<void>;
  llmGenerate(handle: number, prompt: string, configJson: string): Promise<string>;
  llmGenerateStreaming(handle: number, prompt: string, configJson: string): Promise<void>;

  // STT
  sttLoadModel(path: string): Promise<number>;
  sttUnloadModel(handle: number): Promise<void>;
  sttTranscribe(handle: number, audioUri: string): Promise<string>;

  // TTS
  ttsLoadModel(path: string): Promise<number>;
  ttsUnloadModel(handle: number): Promise<void>;
  ttsSynthesize(handle: number, text: string, configJson: string): Promise<string>;

  // Voice Pipeline
  vpConfigure(sttPath: string, llmPath: string, ttsPath: string): Promise<void>;
  vpStop(): Promise<void>;

  // Memory / lifecycle
  getMemoryUsage(): Promise<number>;
  getMemoryLimit(): Promise<number>;
}

/**
 * Typed reference to the native module.
 * Every function returns a Promise that resolves when the native side
 * completes the operation.
 */
export const Native: NativeBridge = RawModule as NativeBridge;

// ---------------------------------------------------------------------------
// Event emitter
// ---------------------------------------------------------------------------

export const OnDeviceAIEventEmitter = new NativeEventEmitter(RawModule);

export const Events = {
  TOKEN:            'ondeviceai_token',
  TRANSCRIPTION:    'ondeviceai_transcription',
  SYNTHESIS_DONE:   'ondeviceai_synthesis_done',
  MEMORY_WARNING:   'ondeviceai_memory_warning',
  DOWNLOAD_PROGRESS:'ondeviceai_download_progress',
  ERROR:            'ondeviceai_error',
} as const;

export default Native;
