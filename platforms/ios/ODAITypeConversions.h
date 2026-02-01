//
//  ODAITypeConversions.h
//  OnDeviceAI iOS SDK
//
//  Type conversion utilities between C++ and Objective-C
//  Requirements: 7.1, 7.8
//

#import <Foundation/Foundation.h>
#import "OnDeviceAI-Bridging.h"

#ifdef __cplusplus
#include "ondeviceai/types.hpp"
#include <string>
#include <vector>
#include <map>

namespace ODAIBridge {

// MARK: - C++ to Objective-C Conversions

// Convert C++ string to NSString
NSString* toNSString(const std::string& str);

// Convert C++ vector to NSArray
template<typename T>
NSArray* toNSArray(const std::vector<T>& vec);

// Convert C++ map to NSDictionary
NSDictionary<NSString*, NSString*>* toNSDictionary(const std::map<std::string, std::string>& map);

// Convert C++ Error to NSError
NSError* toNSError(const ondeviceai::Error& error);

// Convert C++ ModelInfo to ODAIModelInfo
ODAIModelInfo* toODAIModelInfo(const ondeviceai::ModelInfo& info);

// Convert C++ StorageInfo to ODAIStorageInfo
ODAIStorageInfo* toODAIStorageInfo(const ondeviceai::StorageInfo& info);

// Convert C++ AudioData to ODAIAudioData
ODAIAudioData* toODAIAudioData(const ondeviceai::AudioData& audio);

// Convert C++ Transcription to ODAITranscription
ODAITranscription* toODAITranscription(const ondeviceai::Transcription& transcription);

// Convert C++ VoiceInfo to ODAIVoiceInfo
ODAIVoiceInfo* toODAIVoiceInfo(const ondeviceai::VoiceInfo& voice);

// Convert C++ AudioSegment to ODAIAudioSegment
ODAIAudioSegment* toODAIAudioSegment(const ondeviceai::AudioSegment& segment);

// Convert C++ ConversationTurn to ODAIConversationTurn
ODAIConversationTurn* toODAIConversationTurn(const ondeviceai::ConversationTurn& turn);

// Convert C++ DeviceCapabilities to ODAIDeviceCapabilities
ODAIDeviceCapabilities* toODAIDeviceCapabilities(const ondeviceai::DeviceCapabilities& caps);

// MARK: - Objective-C to C++ Conversions

// Convert NSString to C++ string
std::string toCppString(NSString* str);

// Convert NSArray to C++ vector
template<typename T>
std::vector<T> toCppVector(NSArray* array);

// Convert NSDictionary to C++ map
std::map<std::string, std::string> toCppMap(NSDictionary<NSString*, NSString*>* dict);

// Convert ODAISDKConfig to C++ SDKConfig
ondeviceai::SDKConfig toCppSDKConfig(ODAISDKConfig* config);

// Convert ODAIGenerationConfig to C++ GenerationConfig
ondeviceai::GenerationConfig toCppGenerationConfig(ODAIGenerationConfig* config);

// Convert ODAITranscriptionConfig to C++ TranscriptionConfig
ondeviceai::TranscriptionConfig toCppTranscriptionConfig(ODAITranscriptionConfig* config);

// Convert ODAISynthesisConfig to C++ SynthesisConfig
ondeviceai::SynthesisConfig toCppSynthesisConfig(ODAISynthesisConfig* config);

// Convert ODAIPipelineConfig to C++ PipelineConfig
ondeviceai::PipelineConfig toCppPipelineConfig(ODAIPipelineConfig* config);

// Convert ODAIAudioData to C++ AudioData
ondeviceai::AudioData toCppAudioData(ODAIAudioData* audio);

// Convert ODAIDeviceCapabilities to C++ DeviceCapabilities
ondeviceai::DeviceCapabilities toCppDeviceCapabilities(ODAIDeviceCapabilities* caps);

// Convert ODAIModelType to C++ ModelType
ondeviceai::ModelType toCppModelType(ODAIModelType type);

// Convert ODAILogLevel to C++ LogLevel
ondeviceai::LogLevel toCppLogLevel(ODAILogLevel level);

// MARK: - Callback Wrappers

// Wrap Objective-C block as C++ callback
ondeviceai::ProgressCallback wrapProgressCallback(ODAIProgressCallback block);
ondeviceai::TokenCallback wrapTokenCallback(ODAITokenCallback block);
ondeviceai::AudioStreamCallback wrapAudioStreamCallback(ODAIAudioStreamCallback block);
ondeviceai::AudioChunkCallback wrapAudioChunkCallback(ODAIAudioChunkCallback block);
ondeviceai::TranscriptionCallback wrapTranscriptionCallback(ODAITranscriptionCallback block);
ondeviceai::TextCallback wrapTextCallback(ODAITextCallback block);

} // namespace ODAIBridge

#endif // __cplusplus
