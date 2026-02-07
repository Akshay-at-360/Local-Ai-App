/*
 * OnDeviceAI.h
 * OnDevice AI Android SDK - JNI Bridge
 *
 * C to Java/Kotlin bridge for Android
 * Requirements: 7.2, 7.8
 */

#pragma once

#include <jni.h>
#include "ondeviceai/sdk_manager.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize JNI bridge with SDK manager
 * Called from Java/Kotlin when SDK is initialized
 */
JNIEXPORT void JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeInitialize(
    JNIEnv* env,
    jobject obj,
    jstring modelDirectory,
    jint threadCount,
    jlong memoryLimitBytes
);

/**
 * Shutdown SDK and release all resources
 */
JNIEXPORT void JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeShutdown(
    JNIEnv* env,
    jobject obj
);

/**
 * Set thread count
 */
JNIEXPORT void JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeSetThreadCount(
    JNIEnv* env,
    jobject obj,
    jint count
);

/**
 * Set log level
 */
JNIEXPORT void JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeSetLogLevel(
    JNIEnv* env,
    jobject obj,
    jint level
);

/**
 * Get current memory usage
 */
JNIEXPORT jlong JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeGetCurrentMemoryUsage(
    JNIEnv* env,
    jobject obj
);

/**
 * Get memory limit
 */
JNIEXPORT jlong JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeGetMemoryLimit(
    JNIEnv* env,
    jobject obj
);

/**
 * Check if memory is under pressure
 */
JNIEXPORT jboolean JNICALL
Java_com_ondeviceai_OnDeviceAI_nativeIsMemoryPressure(
    JNIEnv* env,
    jobject obj
);

#ifdef __cplusplus
}
#endif
