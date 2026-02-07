package com.ondeviceai.example

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.lifecycle.lifecycleScope
import com.ondeviceai.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * Example Android activity demonstrating LLM chat, STT, TTS,
 * and voice pipeline with the OnDeviceAI SDK.
 */
class MainActivity : AppCompatActivity() {

    companion object {
        private const val TAG = "OnDeviceAI-Example"
        private const val REQUEST_RECORD = 100
    }

    private lateinit var sdk: OnDeviceAI
    private lateinit var logView: TextView
    private lateinit var promptInput: EditText
    private lateinit var sendButton: Button
    private lateinit var statusText: TextView

    private var llmHandle: Long = -1

    // -----------------------------------------------------------------------
    // Activity lifecycle
    // -----------------------------------------------------------------------

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // --- Build UI programmatically (no XML dependency) ---
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(24, 24, 24, 24)
        }

        statusText = TextView(this).apply { text = "Initializing…" }
        root.addView(statusText)

        logView = TextView(this).apply {
            maxLines = 30
            scrollBarStyle = SCROLLBARS_INSIDE_OVERLAY
            isVerticalScrollBarEnabled = true
        }
        root.addView(logView, LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f
        ))

        promptInput = EditText(this).apply { hint = "Type a prompt…" }
        root.addView(promptInput)

        sendButton = Button(this).apply {
            text = "Send"
            isEnabled = false
            setOnClickListener { onSendClicked() }
        }
        root.addView(sendButton)

        setContentView(root)

        // --- Initialise SDK ---
        initSDK()

        // --- Microphone permission for STT ---
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
            != PackageManager.PERMISSION_GRANTED
        ) {
            ActivityCompat.requestPermissions(
                this, arrayOf(Manifest.permission.RECORD_AUDIO), REQUEST_RECORD
            )
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        lifecycleScope.launch(Dispatchers.IO) {
            if (llmHandle >= 0) sdk.llm.unloadModel(llmHandle)
            OnDeviceAI.shutdown()
        }
    }

    // -----------------------------------------------------------------------
    // SDK setup
    // -----------------------------------------------------------------------

    private fun initSDK() {
        lifecycleScope.launch {
            try {
                val modelDir = filesDir.resolve("models").also { it.mkdirs() }.absolutePath
                sdk = OnDeviceAI.initialize(
                    SDKConfig(threadCount = 4, modelDirectory = modelDir)
                )
                sdk.startObservingLifecycleEvents()
                appendLog("SDK initialized  ✓  model dir: $modelDir")
                statusText.text = "SDK ready – load a model to begin"

                // Attempt to load the first available LLM
                loadDefaultModel()
            } catch (e: Exception) {
                appendLog("Init failed: ${e.message}")
                statusText.text = "Init error"
            }
        }
    }

    private suspend fun loadDefaultModel() {
        try {
            val models = sdk.modelManager.listDownloadedModels()
            val llmModel = models.firstOrNull { it.type == ModelType.LLM }
            if (llmModel != null) {
                appendLog("Loading model ${llmModel.name}…")
                llmHandle = sdk.llm.loadModel(llmModel.id)
                appendLog("Model loaded  ✓")
                withContext(Dispatchers.Main) {
                    sendButton.isEnabled = true
                    statusText.text = "Ready – ask anything"
                }
            } else {
                appendLog("No downloaded LLM models found. Download one first.")
            }
        } catch (e: Exception) {
            appendLog("Model load error: ${e.message}")
        }
    }

    // -----------------------------------------------------------------------
    // Chat
    // -----------------------------------------------------------------------

    private fun onSendClicked() {
        val prompt = promptInput.text.toString().trim()
        if (prompt.isEmpty() || llmHandle < 0) return
        promptInput.text.clear()
        sendButton.isEnabled = false
        appendLog("You: $prompt")

        lifecycleScope.launch {
            try {
                val sb = StringBuilder()
                sdk.llm.generateStreaming(llmHandle, prompt) { token ->
                    sb.append(token)
                    // Update UI on each token
                    lifecycleScope.launch(Dispatchers.Main) {
                        logView.append(token)
                    }
                }
                appendLog("\nAssistant: $sb")
            } catch (e: Exception) {
                appendLog("Generation error: ${e.message}")
            } finally {
                withContext(Dispatchers.Main) { sendButton.isEnabled = true }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Logging helper
    // -----------------------------------------------------------------------

    private fun appendLog(msg: String) {
        Log.d(TAG, msg)
        lifecycleScope.launch(Dispatchers.Main) {
            logView.append("$msg\n")
        }
    }
}
