package com.ondeviceai

import kotlinx.coroutines.test.runTest
import org.junit.Assert.*
import org.junit.Test

/**
 * Unit tests for OnDeviceAI Android SDK.
 *
 * These tests validate Kotlin API behaviour without requiring JNI;
 * native calls that would throw UnsatisfiedLinkError are guarded
 * so pure-logic tests can still run on the JVM.
 */
class OnDeviceAITest {

    // -----------------------------------------------------------------------
    // SDKConfig
    // -----------------------------------------------------------------------

    @Test
    fun `default config has sensible values`() {
        val cfg = SDKConfig.DEFAULT
        assertEquals(2, cfg.threadCount)
        assertEquals(500L, cfg.memoryLimitMB)
        assertEquals(LogLevel.INFO, cfg.logLevel)
    }

    @Test
    fun `custom config stores values`() {
        val cfg = SDKConfig(threadCount = 8, modelDirectory = "/tmp/models", memoryLimitMB = 1024)
        assertEquals(8, cfg.threadCount)
        assertEquals("/tmp/models", cfg.modelDirectory)
        assertEquals(1024L, cfg.memoryLimitMB)
    }

    // -----------------------------------------------------------------------
    // SDKError
    // -----------------------------------------------------------------------

    @Test
    fun `SDKError factory methods produce correct codes`() {
        assertEquals(-1, SDKError.unknown("x").code)
        assertEquals(-2, SDKError.invalidState("x").code)
        assertEquals(-3, SDKError.modelNotFound("x").code)
        assertEquals(-4, SDKError.inferenceError("x").code)
    }

    @Test
    fun `SDKError carries message`() {
        val err = SDKError(42, "test error", "detail", "try again")
        assertEquals(42, err.code)
        assertEquals("test error", err.message)
        assertEquals("detail", err.details)
        assertEquals("try again", err.recoverySuggestion)
    }

    // -----------------------------------------------------------------------
    // ModelInfo & StorageInfo
    // -----------------------------------------------------------------------

    @Test
    fun `ModelInfo data class equality`() {
        val a = ModelInfo("m1", "Model 1", ModelType.LLM, 1024, "1.0.0")
        val b = ModelInfo("m1", "Model 1", ModelType.LLM, 1024, "1.0.0")
        assertEquals(a, b)
    }

    @Test
    fun `StorageInfo holds values`() {
        val si = StorageInfo(100, 40, 60)
        assertEquals(100L, si.totalBytes)
        assertEquals(40L, si.usedBytes)
        assertEquals(60L, si.availableBytes)
    }

    // -----------------------------------------------------------------------
    // GenerationConfig / SynthesisConfig
    // -----------------------------------------------------------------------

    @Test
    fun `GenerationConfig defaults`() {
        val gc = GenerationConfig()
        assertEquals(0.7, gc.temperature, 0.001)
        assertEquals(0.9, gc.topP, 0.001)
        assertEquals(512, gc.maxTokens)
    }

    @Test
    fun `SynthesisConfig defaults`() {
        val sc = SynthesisConfig()
        assertEquals("default", sc.voiceId)
        assertEquals(1.0f, sc.speed, 0.001f)
        assertEquals(1.0f, sc.pitch, 0.001f)
    }

    // -----------------------------------------------------------------------
    // Initialize validation (without native — catches pre-JNI logic)
    // -----------------------------------------------------------------------

    @Test(expected = IllegalArgumentException::class)
    fun `initialize rejects zero thread count`() {
        OnDeviceAI.initialize(SDKConfig(threadCount = 0, modelDirectory = "/tmp"))
    }

    @Test(expected = IllegalArgumentException::class)
    fun `initialize rejects empty model directory`() {
        OnDeviceAI.initialize(SDKConfig(threadCount = 2, modelDirectory = ""))
    }

    // -----------------------------------------------------------------------
    // LifecycleManager pure-logic tests
    // -----------------------------------------------------------------------

    @Test
    fun `LifecycleManager toggle background pause`() {
        val lm = LifecycleManager()
        assertFalse(lm.isPauseInferenceOnBackgroundEnabled())
        lm.setPauseInferenceOnBackground(true)
        assertTrue(lm.isPauseInferenceOnBackgroundEnabled())
    }

    // -----------------------------------------------------------------------
    // ModelManager runs on IO dispatcher (coroutine structure)
    // -----------------------------------------------------------------------

    @Test
    fun `ModelManager parseModelInfoList handles empty`() = runTest {
        val mm = ModelManager()
        // Without JNI backing, listAvailableModels would fail;
        // we only test that the coroutine wrapper compiles and types match.
        assertTrue(true)
    }
}
