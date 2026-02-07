/**
 * OnDeviceAI React Native SDK – unit tests.
 *
 * These tests validate the TypeScript API types and error handling
 * without requiring native module backing (mocked NativeModules).
 */

import { SDKError, GenerationConfig } from '../OnDeviceAI';

// ---------------------------------------------------------------------------
// SDKError
// ---------------------------------------------------------------------------

describe('SDKError', () => {
  it('carries code and message', () => {
    const err = new SDKError(42, 'test', 'detail', 'retry');
    expect(err.code).toBe(42);
    expect(err.message).toBe('test');
    expect(err.details).toBe('detail');
    expect(err.recoverySuggestion).toBe('retry');
    expect(err.name).toBe('SDKError');
  });

  it('is an instance of Error', () => {
    const err = new SDKError(-1, 'oops');
    expect(err).toBeInstanceOf(Error);
  });
});

// ---------------------------------------------------------------------------
// GenerationConfig type
// ---------------------------------------------------------------------------

describe('GenerationConfig type', () => {
  it('has sensible defaults when empty', () => {
    const cfg: GenerationConfig = {};
    expect(cfg.temperature).toBeUndefined();
    expect(cfg.topP).toBeUndefined();
    expect(cfg.maxTokens).toBeUndefined();
  });

  it('accepts custom values', () => {
    const cfg: GenerationConfig = { temperature: 0.5, topP: 0.8, maxTokens: 256 };
    expect(cfg.temperature).toBe(0.5);
    expect(cfg.maxTokens).toBe(256);
  });
});
