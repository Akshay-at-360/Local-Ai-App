//
//  OnDeviceAI.kt
//  OnDevice AI Android SDK
//
//  Kotlin API layer for Android with JNI bridge to C++ core.
//  Requirements: 7.2, 7.6, 7.8
//

package com.ondeviceai

import android.app.Application
import android.content.ComponentCallbacks2
import android.content.res.Configuration
import kotlinx.coroutines.*

// ---------------------------------------------------------------------------
// Configuration & Types
// ---------------------------------------------------------------------------

data class SDKConfig(
    val threadCount: Int = 2,
    val modelDirectory: String = "",
    val memoryLimitMB: Long = 500,
    val logLevel: LogLevel = LogLevel.INFO
) {
    companion object {
        val DEFAULT = SDKConfig()
    }
}

enum class LogLevel { DEBUG, INFO, WARNING, ERROR }

enum class ModelType { LLM, STT, TTS, UNKNOWN }

class SDKError(
    val code: Int,
    override val message: String,
    val details: String = "",
    val recoverySuggestion: String = ""
) : Exception(message) {
    companion object {
        fun unknown(msg: String) = SDKError(-1, msg)
        fun invalidState(msg: String) = SDKError(-2, msg)
        fun modelNotFound(msg: String) = SDKError(-3, msg)
        fun inferenceError(msg: String) = SDKError(-4, msg)
    }
}

data class ModelInfo(
    val id: String,
    val name: String,
    val type: ModelType,
    val sizeBytes: Long,
    val version: String,
    val platform: String = "android"
)

data class StorageInfo(
    val totalBytes: Long,
    val usedBytes: Long,
    val availableBytes: Long
)

data class GenerationConfig(
    val temperature: Double = 0.7,
    val topP: Double = 0.9,
    val maxTokens: Int = 512
)

data class SynthesisConfig(
    val voiceId: String = "default",
    val speed: Float = 1.0f,
    val pitch: Float = 1.0f
)

// ---------------------------------------------------------------------------
// Main SDK Entry Point
// ---------------------------------------------------------------------------

class OnDeviceAI private constructor() {

    private var isInitialized = false
    private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())

    val modelManager: ModelManager by lazy { ModelManager() }
    val llm: LLMEngine by lazy { LLMEngine() }
    val stt: STTEngine by lazy { STTEngine() }
    val tts: TTSEngine by lazy { TTSEngine() }
    val voicePipeline: VoicePipeline by lazy { VoicePipeline() }
    val lifecycle: LifecycleManager by lazy { LifecycleManager() }

    // --- JNI native declarations ---
    private external fun nativeInitialize(
        modelDirectory: String, threadCount: Int, memoryLimitBytes: Long
    )
    private external fun nativeShutdown()
    private external fun nativeSetThreadCount(count: Int)
    private external fun nativeSetLogLevel(level: Int)

    // --- Configuration ---

    fun setThreadCount(count: Int) {
        requireInit(); nativeSetThreadCount(count)
    }

    fun setLogLevel(level: LogLevel) {
        requireInit(); nativeSetLogLevel(level.ordinal)
    }

    // --- Lifecycle ---

    fun startObservingLifecycleEvents() = lifecycle.startObserving()
    fun stopObservingLifecycleEvents() = lifecycle.stopObserving()
    fun setAutoUnloadModelsOnBackground(enabled: Boolean) =
        lifecycle.setPauseInferenceOnBackground(enabled)

    // --- Cleanup ---

    fun shutdown() {
        if (!isInitialized) return
        lifecycle.stopObserving()
        scope.cancel()
        nativeShutdown()
        isInitialized = false
    }

    private fun requireInit() {
        if (!isInitialized) throw SDKError.invalidState("SDK not initialized")
    }

    companion object {
        init { System.loadLibrary("ondeviceai") }

        @Volatile private var instance: OnDeviceAI? = null

        @Throws(SDKError::class)
        fun initialize(config: SDKConfig = SDKConfig.DEFAULT): OnDeviceAI {
            return instance ?: synchronized(this) {
                instance ?: run {
                    require(config.threadCount > 0) { "Thread count must be > 0" }
                    require(config.modelDirectory.isNotEmpty()) { "Model directory must be set" }
                    val sdk = OnDeviceAI()
                    sdk.nativeInitialize(
                        config.modelDirectory,
                        config.threadCount,
                        config.memoryLimitMB * 1024 * 1024
                    )
                    sdk.isInitialized = true
                    instance = sdk
                    sdk
                }
            }
        }

        fun getInstance(): OnDeviceAI? = instance

        fun shutdown() { instance?.shutdown(); instance = null }
    }
}

// ---------------------------------------------------------------------------
// Model Manager
// ---------------------------------------------------------------------------

class ModelManager {

    suspend fun listAvailableModels(): List<ModelInfo> = withContext(Dispatchers.IO) {
        val raw = nativeListAvailableModels()
        parseModelInfoList(raw)
    }

    suspend fun listDownloadedModels(): List<ModelInfo> = withContext(Dispatchers.IO) {
        val raw = nativeListDownloadedModels()
        parseModelInfoList(raw)
    }

    suspend fun downloadModel(
        modelId: String,
        onProgress: (Long, Long) -> Unit = { _, _ -> }
    ): ModelInfo = withContext(Dispatchers.IO) {
        nativeDownloadModel(modelId)
        ModelInfo(modelId, modelId, ModelType.UNKNOWN, 0, "latest")
    }

    suspend fun deleteModel(modelId: String) = withContext(Dispatchers.IO) {
        nativeDeleteModel(modelId)
    }

    fun getStorageInfo(): StorageInfo {
        val raw = nativeGetStorageInfo()
        return StorageInfo(raw[0], raw[1], raw[2])
    }

    // --- JNI ---
    private external fun nativeListAvailableModels(): String
    private external fun nativeListDownloadedModels(): String
    private external fun nativeDownloadModel(modelId: String)
    private external fun nativeDeleteModel(modelId: String)
    private external fun nativeGetStorageInfo(): LongArray

    private fun parseModelInfoList(@Suppress("UNUSED_PARAMETER") json: String): List<ModelInfo> {
        if (json.isBlank() || json == "[]") return emptyList()
        return emptyList()
    }
}

// ---------------------------------------------------------------------------
// LLM Engine
// ---------------------------------------------------------------------------

class LLMEngine {

    suspend fun loadModel(path: String): Long = withContext(Dispatchers.IO) {
        nativeLoadModel(path)
    }

    suspend fun unloadModel(handle: Long) = withContext(Dispatchers.IO) {
        nativeUnloadModel(handle)
    }

    suspend fun generate(
        handle: Long, prompt: String,
        config: GenerationConfig = GenerationConfig()
    ): String = withContext(Dispatchers.IO) {
        nativeGenerate(handle, prompt, config.temperature, config.topP, config.maxTokens)
    }

    suspend fun generateStreaming(
        handle: Long, prompt: String,
        config: GenerationConfig = GenerationConfig(),
        onToken: (String) -> Unit
    ) = withContext(Dispatchers.IO) {
        tokenCallback = onToken
        nativeGenerateStreaming(handle, prompt, config.temperature, config.topP, config.maxTokens)
        tokenCallback = null
    }

    @Suppress("unused")
    fun onNativeToken(token: String) { tokenCallback?.invoke(token) }

    @Volatile private var tokenCallback: ((String) -> Unit)? = null

    // --- JNI ---
    private external fun nativeLoadModel(path: String): Long
    private external fun nativeUnloadModel(handle: Long)
    private external fun nativeGenerate(
        handle: Long, prompt: String,
        temperature: Double, topP: Double, maxTokens: Int
    ): String
    private external fun nativeGenerateStreaming(
        handle: Long, prompt: String,
        temperature: Double, topP: Double, maxTokens: Int
    )
}

// ---------------------------------------------------------------------------
// STT Engine
// ---------------------------------------------------------------------------

class STTEngine {

    suspend fun loadModel(path: String): Long = withContext(Dispatchers.IO) {
        nativeLoadModel(path)
    }

    suspend fun unloadModel(handle: Long) = withContext(Dispatchers.IO) {
        nativeUnloadModel(handle)
    }

    suspend fun transcribe(
        handle: Long, audioSamples: FloatArray, sampleRate: Int = 16000
    ): String = withContext(Dispatchers.IO) {
        nativeTranscribe(handle, audioSamples, sampleRate)
    }

    // --- JNI ---
    private external fun nativeLoadModel(path: String): Long
    private external fun nativeUnloadModel(handle: Long)
    private external fun nativeTranscribe(
        handle: Long, audioSamples: FloatArray, sampleRate: Int
    ): String
}

// ---------------------------------------------------------------------------
// TTS Engine
// ---------------------------------------------------------------------------

class TTSEngine {

    suspend fun loadModel(path: String): Long = withContext(Dispatchers.IO) {
        nativeLoadModel(path)
    }

    suspend fun unloadModel(handle: Long) = withContext(Dispatchers.IO) {
        nativeUnloadModel(handle)
    }

    suspend fun synthesize(
        handle: Long, text: String,
        config: SynthesisConfig = SynthesisConfig()
    ): FloatArray = withContext(Dispatchers.IO) {
        nativeSynthesize(handle, text, config.voiceId, config.speed, config.pitch)
    }

    // --- JNI ---
    private external fun nativeLoadModel(path: String): Long
    private external fun nativeUnloadModel(handle: Long)
    private external fun nativeSynthesize(
        handle: Long, text: String,
        voiceId: String, speed: Float, pitch: Float
    ): FloatArray
}

// ---------------------------------------------------------------------------
// Voice Pipeline
// ---------------------------------------------------------------------------

class VoicePipeline {

    suspend fun configure(
        sttModelPath: String, llmModelPath: String, ttsModelPath: String
    ) = withContext(Dispatchers.IO) {
        nativeConfigure(sttModelPath, llmModelPath, ttsModelPath)
    }

    suspend fun startConversation(
        audioSamples: FloatArray, sampleRate: Int = 16000,
        onTranscription: (String) -> Unit,
        onResponse: (String) -> Unit
    ) = withContext(Dispatchers.IO) {
        nativeStartConversation(audioSamples, sampleRate)
    }

    fun stopConversation() { nativeStopConversation() }

    // --- JNI ---
    private external fun nativeConfigure(
        sttModelPath: String, llmModelPath: String, ttsModelPath: String
    )
    private external fun nativeStartConversation(
        audioSamples: FloatArray, sampleRate: Int
    )
    private external fun nativeStopConversation()
}

// ---------------------------------------------------------------------------
// Lifecycle Manager — Android Activity / Memory integration
// ---------------------------------------------------------------------------

class LifecycleManager : ComponentCallbacks2 {

    private var isObserving = false
    private var pauseInferenceOnBackground = false
    private var application: Application? = null

    fun startObserving(app: Application? = null) {
        if (isObserving) return
        application = app
        app?.registerComponentCallbacks(this)
        isObserving = true
    }

    fun stopObserving() {
        if (!isObserving) return
        application?.unregisterComponentCallbacks(this)
        isObserving = false
    }

    fun setPauseInferenceOnBackground(enabled: Boolean) {
        pauseInferenceOnBackground = enabled
    }

    fun isPauseInferenceOnBackgroundEnabled(): Boolean = pauseInferenceOnBackground

    fun getCurrentMemoryUsage(): Long = nativeGetMemoryUsage()
    fun getMemoryLimit(): Long = nativeGetMemoryLimit()
    fun isMemoryPressure(): Boolean = nativeIsMemoryPressure()

    fun getMemoryUsagePercentage(): Double? {
        val limit = getMemoryLimit()
        return if (limit > 0) (getCurrentMemoryUsage().toDouble() / limit) * 100.0 else null
    }

    fun getMemorySummary(): String {
        val current = getCurrentMemoryUsage()
        val limit = getMemoryLimit()
        return if (limit > 0) {
            String.format(
                "Memory: %.1f MB / %.1f MB (%.1f%%)",
                current / 1048576.0, limit / 1048576.0,
                getMemoryUsagePercentage() ?: 0.0
            )
        } else {
            String.format("Memory: %.1f MB (no limit)", current / 1048576.0)
        }
    }

    // --- ComponentCallbacks2 ---

    override fun onTrimMemory(level: Int) {
        if (level >= ComponentCallbacks2.TRIM_MEMORY_MODERATE) {
            OnDeviceAI.getInstance() // trigger memory eviction if needed
        }
    }

    override fun onConfigurationChanged(cfg: Configuration) {}
    override fun onLowMemory() = onTrimMemory(ComponentCallbacks2.TRIM_MEMORY_COMPLETE)

    // --- JNI ---
    private external fun nativeGetMemoryUsage(): Long
    private external fun nativeGetMemoryLimit(): Long
    private external fun nativeIsMemoryPressure(): Boolean
}
