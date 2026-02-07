import 'package:flutter_test/flutter_test.dart';
import 'package:ondeviceai/ondeviceai.dart';

void main() {
  // -----------------------------------------------------------------------
  // SDKException
  // -----------------------------------------------------------------------

  group('SDKException', () {
    test('carries code and message', () {
      final e = SDKException(code: 42, message: 'test', details: 'd');
      expect(e.code, 42);
      expect(e.message, 'test');
      expect(e.details, 'd');
    });

    test('toString includes code', () {
      final e = SDKException.unknown('oops');
      expect(e.toString(), contains('-1'));
      expect(e.toString(), contains('oops'));
    });

    test('factory constructors', () {
      expect(SDKException.unknown('x').code, -1);
      expect(SDKException.invalidState('x').code, -2);
    });
  });

  // -----------------------------------------------------------------------
  // GenerationConfig / SynthesisConfig
  // -----------------------------------------------------------------------

  group('GenerationConfig', () {
    test('default values', () {
      const c = GenerationConfig();
      expect(c.temperature, 0.7);
      expect(c.topP, 0.9);
      expect(c.maxTokens, 512);
    });

    test('custom values', () {
      const c = GenerationConfig(temperature: 0.1, maxTokens: 64);
      expect(c.temperature, 0.1);
      expect(c.maxTokens, 64);
    });
  });

  group('SynthesisConfig', () {
    test('default values', () {
      const c = SynthesisConfig();
      expect(c.voiceId, 'default');
      expect(c.speed, 1.0);
      expect(c.pitch, 1.0);
    });
  });

  // -----------------------------------------------------------------------
  // ModelInfo / StorageInfo
  // -----------------------------------------------------------------------

  group('ModelInfo', () {
    test('holds values', () {
      final m = ModelInfo(
          id: 'm1',
          name: 'Model 1',
          type: 'llm',
          sizeBytes: 1024,
          version: '1.0.0');
      expect(m.id, 'm1');
      expect(m.sizeBytes, 1024);
    });
  });

  group('StorageInfo', () {
    test('percentage calculation', () {
      final s =
          StorageInfo(totalBytes: 1000, usedBytes: 400, availableBytes: 600);
      expect(s.availablePercentage, 60);
    });

    test('zero total handled', () {
      final s = StorageInfo(totalBytes: 0, usedBytes: 0, availableBytes: 0);
      expect(s.availablePercentage, 0);
    });
  });

  // -----------------------------------------------------------------------
  // Initialize validation (unit-level, does not load native lib)
  // -----------------------------------------------------------------------

  group('OnDeviceAI.initialize validation', () {
    test('rejects zero thread count', () {
      expect(
        () => OnDeviceAI.initialize(threadCount: 0),
        throwsA(isA<SDKException>()),
      );
    });

    test('rejects negative thread count', () {
      expect(
        () => OnDeviceAI.initialize(threadCount: -1),
        throwsA(isA<SDKException>()),
      );
    });
  });

  // -----------------------------------------------------------------------
  // LifecycleManager (pure-logic tests)
  // -----------------------------------------------------------------------

  group('LifecycleManager', () {
    test('observing toggle', () {
      final lm = LifecycleManager();
      expect(lm.isObserving, false);
      lm.startObserving();
      expect(lm.isObserving, true);
      lm.stopObserving();
      expect(lm.isObserving, false);
    });
  });
}
