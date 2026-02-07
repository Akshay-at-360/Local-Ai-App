/*
 * OnDeviceAI_jni.cpp
 * OnDevice AI Android SDK - JNI Bridge Implementation
 *
 * Routes JNI calls from Kotlin/Java to C++ core SDK.
 * Requirements: 7.2, 7.8
 */

#include "../OnDeviceAI.h"
#include "ondeviceai/sdk_manager.hpp"
#include "ondeviceai/model_manager.hpp"
#include "ondeviceai/llm_engine.hpp"
#include "ondeviceai/stt_engine.hpp"
#include "ondeviceai/tts_engine.hpp"
#include "ondeviceai/voice_pipeline.hpp"
#include "ondeviceai/memory_manager.hpp"
#include "ondeviceai/logger.hpp"
#include "ondeviceai/types.hpp"

#include <jni.h>
#include <string>
#include <vector>
#include <mutex>
#include <android/log.h>

#define LOG_TAG "OnDeviceAI_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace ondeviceai;

// ---------------------------------------------------------------------------
// Utility helpers
// ---------------------------------------------------------------------------

namespace {

/// Convert JNI string to std::string (handles null)
std::string jstringToString(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

/// Create a Java string from std::string
jstring stringToJstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

/// Throw a Java exception wrapping an SDK error
void throwSDKError(JNIEnv* env, const Error& error) {
    jclass cls = env->FindClass("com/ondeviceai/SDKError");
    if (cls) {
        jmethodID ctor = env->GetMethodID(cls, "<init>",
            "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        if (ctor) {
            jstring msg  = stringToJstring(env, error.message);
            jstring det  = stringToJstring(env, error.details);
            jstring rec  = stringToJstring(env, error.recovery_suggestion);
            jthrowable ex = static_cast<jthrowable>(
                env->NewObject(cls, ctor,
                    static_cast<jint>(error.code), msg, det, rec));
            env->Throw(ex);
            env->DeleteLocalRef(msg);
            env->DeleteLocalRef(det);
            env->DeleteLocalRef(rec);
            return;
        }
    }
    // Fallback: plain RuntimeException
    jclass rte = env->FindClass("java/lang/RuntimeException");
    env->ThrowNew(rte, error.message.c_str());
}

/// Throw a generic RuntimeException
void throwRuntime(JNIEnv* env, const char* msg) {
    jclass rte = env->FindClass("java/lang/RuntimeException");
    env->ThrowNew(rte, msg);
}

/// Global reference to the cached JavaVM (set in JNI_OnLoad)
JavaVM* g_jvm = nullptr;

/// Get JNIEnv for the current thread
JNIEnv* getEnv() {
    JNIEnv* env = nullptr;
    if (g_jvm) {
        g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    }
    return env;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// JNI_OnLoad / JNI_OnUnload
// ---------------------------------------------------------------------------

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    g_jvm = vm;
    LOGI("OnDeviceAI JNI library loaded");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* /*vm*/, void* /*reserved*/) {
    LOGI("OnDeviceAI JNI library unloading");
    SDKManager::shutdown();
    g_jvm = nullptr;
}

// ---------------------------------------------------------------------------
// SDK Manager
// ---------------------------------------------------------------------------

JNIEXPORT void JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeInitialize(
    JNIEnv* env, jobject /*obj*/,
    jstring modelDirectory, jint threadCount, jlong memoryLimitBytes)
{
    LOGI("nativeInitialize: threads=%d memLimit=%lld", threadCount, (long long)memoryLimitBytes);

    SDKConfig config;
    config.model_directory = jstringToString(env, modelDirectory);
    config.thread_count    = static_cast<int>(threadCount);
    config.memory_limit    = static_cast<size_t>(memoryLimitBytes);
    config.log_level       = LogLevel::Info;

    auto result = SDKManager::initialize(config);
    if (result.isError()) {
        throwSDKError(env, result.error());
    }
}

JNIEXPORT void JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeShutdown(JNIEnv* /*env*/, jobject /*obj*/)
{
    LOGI("nativeShutdown");
    SDKManager::shutdown();
}

JNIEXPORT void JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeSetThreadCount(
    JNIEnv* env, jobject /*obj*/, jint count)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return; }
    mgr->setThreadCount(static_cast<int>(count));
}

JNIEXPORT void JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeSetLogLevel(
    JNIEnv* env, jobject /*obj*/, jint level)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return; }
    mgr->setLogLevel(static_cast<LogLevel>(level));
}

// ---------------------------------------------------------------------------
// LLM Engine
// ---------------------------------------------------------------------------

JNIEXPORT jlong JNICALL
Java_com_ondeviceai_LLMEngine_nativeLoadModel(
    JNIEnv* env, jobject /*obj*/, jstring modelPath)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return -1; }

    std::string path = jstringToString(env, modelPath);
    LOGI("LLM loadModel: %s", path.c_str());

    auto result = mgr->getLLMEngine()->loadModel(path);
    if (result.isError()) {
        throwSDKError(env, result.error());
        return -1;
    }
    return static_cast<jlong>(result.value());
}

JNIEXPORT void JNICALL
Java_com_ondeviceai_LLMEngine_nativeUnloadModel(
    JNIEnv* env, jobject /*obj*/, jlong handle)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return; }

    auto result = mgr->getLLMEngine()->unloadModel(static_cast<ModelHandle>(handle));
    if (result.isError()) {
        throwSDKError(env, result.error());
    }
}

JNIEXPORT jstring JNICALL
Java_com_ondeviceai_LLMEngine_nativeGenerate(
    JNIEnv* env, jobject /*obj*/, jlong handle, jstring prompt,
    jdouble temperature, jdouble topP, jint maxTokens)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return nullptr; }

    std::string promptStr = jstringToString(env, prompt);

    GenerationConfig config;
    config.temperature = static_cast<float>(temperature);
    config.top_p       = static_cast<float>(topP);
    config.max_tokens  = static_cast<int>(maxTokens);

    auto result = mgr->getLLMEngine()->generate(
        static_cast<ModelHandle>(handle), promptStr, config);
    if (result.isError()) {
        throwSDKError(env, result.error());
        return nullptr;
    }
    return stringToJstring(env, result.value());
}

JNIEXPORT void JNICALL
Java_com_ondeviceai_LLMEngine_nativeGenerateStreaming(
    JNIEnv* env, jobject obj, jlong handle, jstring prompt,
    jdouble temperature, jdouble topP, jint maxTokens)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return; }

    std::string promptStr = jstringToString(env, prompt);

    GenerationConfig config;
    config.temperature = static_cast<float>(temperature);
    config.top_p       = static_cast<float>(topP);
    config.max_tokens  = static_cast<int>(maxTokens);

    // Get the Kotlin callback method
    jclass cls = env->GetObjectClass(obj);
    jmethodID onTokenMethod = env->GetMethodID(cls, "onNativeToken", "(Ljava/lang/String;)V");
    if (!onTokenMethod) {
        throwRuntime(env, "onNativeToken method not found on LLMEngine");
        return;
    }

    // Create a weak global ref so the callback can survive across JNI calls
    jobject globalObj = env->NewGlobalRef(obj);

    auto result = mgr->getLLMEngine()->generateStreaming(
        static_cast<ModelHandle>(handle), promptStr, config,
        [globalObj, onTokenMethod](const std::string& token) -> bool {
            JNIEnv* cbEnv = getEnv();
            if (!cbEnv) return false;
            jstring jToken = cbEnv->NewStringUTF(token.c_str());
            cbEnv->CallVoidMethod(globalObj, onTokenMethod, jToken);
            cbEnv->DeleteLocalRef(jToken);
            bool hasException = cbEnv->ExceptionCheck();
            if (hasException) {
                cbEnv->ExceptionClear();
                return false; // stop generation
            }
            return true; // continue
        });

    env->DeleteGlobalRef(globalObj);

    if (result.isError()) {
        throwSDKError(env, result.error());
    }
}

// ---------------------------------------------------------------------------
// STT Engine
// ---------------------------------------------------------------------------

JNIEXPORT jlong JNICALL
Java_com_ondeviceai_STTEngine_nativeLoadModel(
    JNIEnv* env, jobject /*obj*/, jstring modelPath)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return -1; }

    std::string path = jstringToString(env, modelPath);
    LOGI("STT loadModel: %s", path.c_str());

    auto result = mgr->getSTTEngine()->loadModel(path);
    if (result.isError()) {
        throwSDKError(env, result.error());
        return -1;
    }
    return static_cast<jlong>(result.value());
}

JNIEXPORT void JNICALL
Java_com_ondeviceai_STTEngine_nativeUnloadModel(
    JNIEnv* env, jobject /*obj*/, jlong handle)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return; }

    auto result = mgr->getSTTEngine()->unloadModel(static_cast<ModelHandle>(handle));
    if (result.isError()) {
        throwSDKError(env, result.error());
    }
}

JNIEXPORT jstring JNICALL
Java_com_ondeviceai_STTEngine_nativeTranscribe(
    JNIEnv* env, jobject /*obj*/, jlong handle, jfloatArray audioSamples,
    jint sampleRate)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return nullptr; }

    // Convert Java float array to C++ vector
    jsize len = env->GetArrayLength(audioSamples);
    std::vector<float> samples(len);
    env->GetFloatArrayRegion(audioSamples, 0, len, samples.data());

    AudioData audio;
    audio.samples     = std::move(samples);
    audio.sample_rate = static_cast<int>(sampleRate);
    audio.channels    = 1;

    TranscriptionConfig tConfig;
    auto result = mgr->getSTTEngine()->transcribe(
        static_cast<ModelHandle>(handle), audio, tConfig);
    if (result.isError()) {
        throwSDKError(env, result.error());
        return nullptr;
    }
    return stringToJstring(env, result.value().text);
}

// ---------------------------------------------------------------------------
// TTS Engine
// ---------------------------------------------------------------------------

JNIEXPORT jlong JNICALL
Java_com_ondeviceai_TTSEngine_nativeLoadModel(
    JNIEnv* env, jobject /*obj*/, jstring modelPath)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return -1; }

    std::string path = jstringToString(env, modelPath);
    LOGI("TTS loadModel: %s", path.c_str());

    auto result = mgr->getTTSEngine()->loadModel(path);
    if (result.isError()) {
        throwSDKError(env, result.error());
        return -1;
    }
    return static_cast<jlong>(result.value());
}

JNIEXPORT void JNICALL
Java_com_ondeviceai_TTSEngine_nativeUnloadModel(
    JNIEnv* env, jobject /*obj*/, jlong handle)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return; }

    auto result = mgr->getTTSEngine()->unloadModel(static_cast<ModelHandle>(handle));
    if (result.isError()) {
        throwSDKError(env, result.error());
    }
}

JNIEXPORT jfloatArray JNICALL
Java_com_ondeviceai_TTSEngine_nativeSynthesize(
    JNIEnv* env, jobject /*obj*/, jlong handle, jstring text,
    jstring voiceId, jfloat speed, jfloat pitch)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return nullptr; }

    std::string textStr = jstringToString(env, text);

    SynthesisConfig sConfig;
    sConfig.voice_id = jstringToString(env, voiceId);
    sConfig.speed    = speed;
    sConfig.pitch    = pitch;

    auto result = mgr->getTTSEngine()->synthesize(
        static_cast<ModelHandle>(handle), textStr, sConfig);
    if (result.isError()) {
        throwSDKError(env, result.error());
        return nullptr;
    }

    const auto& audioData = result.value();
    jsize len = static_cast<jsize>(audioData.samples.size());
    jfloatArray jsamples = env->NewFloatArray(len);
    env->SetFloatArrayRegion(jsamples, 0, len, audioData.samples.data());
    return jsamples;
}

// ---------------------------------------------------------------------------
// Voice Pipeline
// ---------------------------------------------------------------------------

JNIEXPORT void JNICALL
Java_com_ondeviceai_VoicePipeline_nativeConfigure(
    JNIEnv* env, jobject /*obj*/,
    jstring sttModelPath, jstring llmModelPath, jstring ttsModelPath)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return; }

    VoicePipelineConfig vpConfig;
    vpConfig.stt_model_path = jstringToString(env, sttModelPath);
    vpConfig.llm_model_path = jstringToString(env, llmModelPath);
    vpConfig.tts_model_path = jstringToString(env, ttsModelPath);

    auto result = mgr->getVoicePipeline()->configure(vpConfig);
    if (result.isError()) {
        throwSDKError(env, result.error());
    }
}

JNIEXPORT void JNICALL
Java_com_ondeviceai_VoicePipeline_nativeStopConversation(
    JNIEnv* env, jobject /*obj*/)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) { throwRuntime(env, "SDK not initialized"); return; }

    mgr->getVoicePipeline()->stopConversation();
}

// ---------------------------------------------------------------------------
// Memory / Lifecycle
// ---------------------------------------------------------------------------

JNIEXPORT jlong JNICALL
Java_com_ondeviceai_LifecycleManager_nativeGetMemoryUsage(
    JNIEnv* env, jobject /*obj*/)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) return 0;
    return static_cast<jlong>(mgr->getMemoryManager()->getCurrentUsage());
}

JNIEXPORT jlong JNICALL
Java_com_ondeviceai_LifecycleManager_nativeGetMemoryLimit(
    JNIEnv* env, jobject /*obj*/)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) return 0;
    return static_cast<jlong>(mgr->getMemoryManager()->getMemoryLimit());
}

JNIEXPORT jboolean JNICALL
Java_com_ondeviceai_LifecycleManager_nativeIsMemoryPressure(
    JNIEnv* env, jobject /*obj*/)
{
    auto* mgr = SDKManager::getInstance();
    if (!mgr) return JNI_FALSE;
    return mgr->getMemoryManager()->isMemoryPressure() ? JNI_TRUE : JNI_FALSE;
}
