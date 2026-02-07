package com.ondeviceai.reactnative

import com.facebook.react.bridge.*
import com.facebook.react.modules.core.DeviceEventManagerModule
import com.ondeviceai.*
import kotlinx.coroutines.*

/**
 * React Native Android native module – bridges JS promises to Kotlin SDK.
 *
 * Requirements: 7.3, 7.8
 */
class OnDeviceAIModule(private val reactContext: ReactApplicationContext) :
    ReactContextBaseJavaModule(reactContext) {

    override fun getName() = "OnDeviceAI"

    private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())

    private fun emit(event: String, params: WritableMap) {
        reactContext.getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter::class.java)
            .emit(event, params)
    }

    // -----------------------------------------------------------------------
    // SDK lifecycle
    // -----------------------------------------------------------------------

    @ReactMethod
    fun initialize(configJson: String, promise: Promise) {
        scope.launch {
            try {
                val config = SDKConfig(
                    threadCount = 2,
                    modelDirectory = "",
                    memoryLimitMB = 500
                )
                // Production: parse configJson via org.json.JSONObject
                OnDeviceAI.initialize(config)
                promise.resolve(null)
            } catch (e: Exception) {
                promise.reject("ERR_INIT", e.message, e)
            }
        }
    }

    @ReactMethod
    fun shutdown(promise: Promise) {
        OnDeviceAI.shutdown()
        promise.resolve(null)
    }

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    @ReactMethod
    fun setThreadCount(count: Int, promise: Promise) {
        try {
            OnDeviceAI.getInstance()?.setThreadCount(count)
            promise.resolve(null)
        } catch (e: Exception) { promise.reject("ERR", e.message, e) }
    }

    @ReactMethod
    fun setLogLevel(level: Int, promise: Promise) {
        try {
            OnDeviceAI.getInstance()?.setLogLevel(LogLevel.entries[level])
            promise.resolve(null)
        } catch (e: Exception) { promise.reject("ERR", e.message, e) }
    }

    @ReactMethod
    fun setMemoryLimit(bytes: Double, promise: Promise) {
        promise.resolve(null)
    }

    @ReactMethod
    fun setCallbackThreadCount(count: Int, promise: Promise) {
        promise.resolve(null)
    }

    // -----------------------------------------------------------------------
    // LLM
    // -----------------------------------------------------------------------

    @ReactMethod
    fun llmLoadModel(path: String, promise: Promise) {
        scope.launch {
            try {
                val handle = OnDeviceAI.getInstance()!!.llm.loadModel(path)
                promise.resolve(handle.toDouble())
            } catch (e: Exception) { promise.reject("ERR_LLM_LOAD", e.message, e) }
        }
    }

    @ReactMethod
    fun llmUnloadModel(handle: Double, promise: Promise) {
        scope.launch {
            try {
                OnDeviceAI.getInstance()!!.llm.unloadModel(handle.toLong())
                promise.resolve(null)
            } catch (e: Exception) { promise.reject("ERR_LLM_UNLOAD", e.message, e) }
        }
    }

    @ReactMethod
    fun llmGenerate(handle: Double, prompt: String, configJson: String, promise: Promise) {
        scope.launch {
            try {
                val result = OnDeviceAI.getInstance()!!.llm.generate(handle.toLong(), prompt)
                promise.resolve(result)
            } catch (e: Exception) { promise.reject("ERR_LLM_GEN", e.message, e) }
        }
    }

    @ReactMethod
    fun llmGenerateStreaming(handle: Double, prompt: String, configJson: String, promise: Promise) {
        scope.launch {
            try {
                OnDeviceAI.getInstance()!!.llm.generateStreaming(
                    handle.toLong(), prompt
                ) { token ->
                    val params = Arguments.createMap().apply { putString("token", token) }
                    emit("ondeviceai_token", params)
                }
                promise.resolve(null)
            } catch (e: Exception) { promise.reject("ERR_LLM_STREAM", e.message, e) }
        }
    }

    // -----------------------------------------------------------------------
    // STT
    // -----------------------------------------------------------------------

    @ReactMethod
    fun sttLoadModel(path: String, promise: Promise) {
        scope.launch {
            try {
                val handle = OnDeviceAI.getInstance()!!.stt.loadModel(path)
                promise.resolve(handle.toDouble())
            } catch (e: Exception) { promise.reject("ERR_STT_LOAD", e.message, e) }
        }
    }

    @ReactMethod
    fun sttUnloadModel(handle: Double, promise: Promise) {
        scope.launch {
            try {
                OnDeviceAI.getInstance()!!.stt.unloadModel(handle.toLong())
                promise.resolve(null)
            } catch (e: Exception) { promise.reject("ERR", e.message, e) }
        }
    }

    @ReactMethod
    fun sttTranscribe(handle: Double, audioUri: String, promise: Promise) {
        scope.launch {
            try {
                // Production: decode audio file at audioUri into FloatArray
                val samples = FloatArray(0)
                val result = OnDeviceAI.getInstance()!!.stt.transcribe(
                    handle.toLong(), samples, 16000
                )
                promise.resolve(result)
            } catch (e: Exception) { promise.reject("ERR_STT", e.message, e) }
        }
    }

    // -----------------------------------------------------------------------
    // TTS
    // -----------------------------------------------------------------------

    @ReactMethod
    fun ttsLoadModel(path: String, promise: Promise) {
        scope.launch {
            try {
                val handle = OnDeviceAI.getInstance()!!.tts.loadModel(path)
                promise.resolve(handle.toDouble())
            } catch (e: Exception) { promise.reject("ERR_TTS_LOAD", e.message, e) }
        }
    }

    @ReactMethod
    fun ttsUnloadModel(handle: Double, promise: Promise) {
        scope.launch {
            try {
                OnDeviceAI.getInstance()!!.tts.unloadModel(handle.toLong())
                promise.resolve(null)
            } catch (e: Exception) { promise.reject("ERR", e.message, e) }
        }
    }

    @ReactMethod
    fun ttsSynthesize(handle: Double, text: String, configJson: String, promise: Promise) {
        scope.launch {
            try {
                val audio = OnDeviceAI.getInstance()!!.tts.synthesize(handle.toLong(), text)
                // Production: write audio to temp WAV and return file path
                promise.resolve("")
            } catch (e: Exception) { promise.reject("ERR_TTS", e.message, e) }
        }
    }

    // -----------------------------------------------------------------------
    // Voice Pipeline
    // -----------------------------------------------------------------------

    @ReactMethod
    fun vpConfigure(sttPath: String, llmPath: String, ttsPath: String, promise: Promise) {
        scope.launch {
            try {
                OnDeviceAI.getInstance()!!.voicePipeline.configure(sttPath, llmPath, ttsPath)
                promise.resolve(null)
            } catch (e: Exception) { promise.reject("ERR_VP", e.message, e) }
        }
    }

    @ReactMethod
    fun vpStop(promise: Promise) {
        OnDeviceAI.getInstance()?.voicePipeline?.stopConversation()
        promise.resolve(null)
    }

    // -----------------------------------------------------------------------
    // Memory
    // -----------------------------------------------------------------------

    @ReactMethod
    fun getMemoryUsage(promise: Promise) {
        val usage = OnDeviceAI.getInstance()?.lifecycle?.getCurrentMemoryUsage() ?: 0L
        promise.resolve(usage.toDouble())
    }

    @ReactMethod
    fun getMemoryLimit(promise: Promise) {
        val limit = OnDeviceAI.getInstance()?.lifecycle?.getMemoryLimit() ?: 0L
        promise.resolve(limit.toDouble())
    }

    // -----------------------------------------------------------------------
    // Model management
    // -----------------------------------------------------------------------

    @ReactMethod
    fun listAvailableModels(promise: Promise) {
        scope.launch {
            try {
                val models = OnDeviceAI.getInstance()!!.modelManager.listAvailableModels()
                promise.resolve("[]") // Placeholder serialization
            } catch (e: Exception) { promise.reject("ERR", e.message, e) }
        }
    }

    @ReactMethod
    fun listDownloadedModels(promise: Promise) {
        scope.launch {
            try {
                val models = OnDeviceAI.getInstance()!!.modelManager.listDownloadedModels()
                promise.resolve("[]")
            } catch (e: Exception) { promise.reject("ERR", e.message, e) }
        }
    }

    @ReactMethod
    fun downloadModel(modelId: String, promise: Promise) {
        scope.launch {
            try {
                OnDeviceAI.getInstance()!!.modelManager.downloadModel(modelId) { current, total ->
                    val params = Arguments.createMap().apply {
                        putDouble("current", current.toDouble())
                        putDouble("total", total.toDouble())
                    }
                    emit("ondeviceai_download_progress", params)
                }
                promise.resolve("{}")
            } catch (e: Exception) { promise.reject("ERR", e.message, e) }
        }
    }

    @ReactMethod
    fun deleteModel(modelId: String, promise: Promise) {
        scope.launch {
            try {
                OnDeviceAI.getInstance()!!.modelManager.deleteModel(modelId)
                promise.resolve(null)
            } catch (e: Exception) { promise.reject("ERR", e.message, e) }
        }
    }

    @ReactMethod
    fun getStorageInfo(promise: Promise) {
        try {
            val info = OnDeviceAI.getInstance()!!.modelManager.getStorageInfo()
            promise.resolve("{\"totalBytes\":${info.totalBytes},\"usedBytes\":${info.usedBytes},\"availableBytes\":${info.availableBytes}}")
        } catch (e: Exception) { promise.reject("ERR", e.message, e) }
    }
}
