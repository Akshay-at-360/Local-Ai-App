import 'dart:async';
import 'dart:math';
import 'package:flutter/foundation.dart';
import '../../models/models.dart';

/// Status of the LLM service
enum LlmStatus {
  unloaded,
  loading,
  ready,
  generating,
  error,
}

/// Abstract LLM provider interface - allows for different backends
abstract class LlmProvider {
  Future<bool> loadModel(String modelPath);
  Future<void> unloadModel();
  Stream<String> generateStream({
    required String prompt,
    double temperature,
    int maxTokens,
  });
  void stopGeneration();
  bool get isReady;
}

/// Mock LLM provider for development and testing
/// Simulates AI responses without requiring actual model loading
class MockLlmProvider implements LlmProvider {
  bool _isReady = false;
  bool _isGenerating = false;
  bool _stopRequested = false;

  // Sample responses for demo
  static const _responses = [
    "I'm a local AI assistant running on your device! Since this is a demo mode, I'm simulating responses. When you connect a real LLM model (like Gemma, Qwen, or Phi), you'll get actual AI-generated responses.",
    "That's an interesting question! In a real implementation with an actual GGUF model loaded, I would process your input through the neural network to generate a thoughtful response. For now, I'm demonstrating the chat interface.",
    "Great to chat with you! This app is designed to run AI models completely offline on your device. The UI you're seeing is fully functional - just add a compatible LLM model to unlock real AI conversations.",
    "Hello! I'm running in demo mode right now. This Flutter app supports local LLM inference using GGUF models. Once a real model is loaded, you'll experience genuine AI responses with streaming text generation.",
    "Thanks for trying out the Local AI Chat app! While I'm currently simulating responses, the underlying architecture is ready to handle real LLM inference with models like Gemma 3, Qwen 2.5, or Phi-3.",
  ];

  @override
  bool get isReady => _isReady;

  @override
  Future<bool> loadModel(String modelPath) async {
    // Simulate model loading delay
    await Future.delayed(const Duration(milliseconds: 800));
    _isReady = true;
    return true;
  }

  @override
  Future<void> unloadModel() async {
    await Future.delayed(const Duration(milliseconds: 200));
    _isReady = false;
  }

  @override
  Stream<String> generateStream({
    required String prompt,
    double temperature = 0.7,
    int maxTokens = 512,
  }) async* {
    _isGenerating = true;
    _stopRequested = false;
    
    // Select a random response
    final random = Random();
    final response = _responses[random.nextInt(_responses.length)];
    
    // Simulate streaming by yielding word by word
    final words = response.split(' ');
    for (int i = 0; i < words.length; i++) {
      if (_stopRequested) break;
      
      // Simulate thinking delay (variable for realism)
      await Future.delayed(Duration(milliseconds: 30 + random.nextInt(70)));
      
      yield words[i] + (i < words.length - 1 ? ' ' : '');
    }
    
    _isGenerating = false;
  }

  @override
  void stopGeneration() {
    _stopRequested = true;
    _isGenerating = false;
  }
}

/// LLM Service - Singleton that manages LLM inference
/// Uses platform-agnostic provider interface
class LlmService extends ChangeNotifier {
  static final LlmService _instance = LlmService._internal();
  factory LlmService() => _instance;
  LlmService._internal();

  LlmProvider? _provider;
  LlmStatus _status = LlmStatus.unloaded;
  ModelInfo? _currentModel;
  String? _errorMessage;

  LlmStatus get status => _status;
  bool get isReady => _status == LlmStatus.ready;
  bool get isLoading => _status == LlmStatus.loading;
  bool get isGenerating => _status == LlmStatus.generating;
  ModelInfo? get currentModel => _currentModel;
  String? get errorMessage => _errorMessage;

  /// Load a model from the given path
  Future<bool> loadModel(String modelPath, ModelInfo modelInfo) async {
    try {
      _status = LlmStatus.loading;
      _errorMessage = null;
      notifyListeners();

      // Use mock provider for now
      // In production, this would check if flutter_llama is available
      // and use the real provider, falling back to mock if not
      _provider = MockLlmProvider();
      
      final success = await _provider!.loadModel(modelPath);
      
      if (success) {
        _currentModel = modelInfo;
        _status = LlmStatus.ready;
        debugPrint('Model loaded (demo mode): ${modelInfo.name}');
      } else {
        _status = LlmStatus.error;
        _errorMessage = 'Failed to load model';
      }
      
      notifyListeners();
      return success;
    } catch (e) {
      _status = LlmStatus.error;
      _errorMessage = e.toString();
      notifyListeners();
      return false;
    }
  }

  /// Unload the current model
  Future<void> unloadModel() async {
    if (_provider != null) {
      await _provider!.unloadModel();
    }
    _provider = null;
    _currentModel = null;
    _status = LlmStatus.unloaded;
    notifyListeners();
  }

  /// Generate text from messages using streaming
  Stream<String> generateStream({
    required List<Message> messages,
    String? systemPrompt,
    double temperature = 0.7,
    int maxTokens = 512,
  }) async* {
    if (_provider == null || !_provider!.isReady) {
      throw Exception('Model not loaded');
    }

    _status = LlmStatus.generating;
    notifyListeners();

    try {
      // Build prompt from messages (simplified for mock)
      final prompt = _buildPrompt(messages, systemPrompt);
      
      await for (final token in _provider!.generateStream(
        prompt: prompt,
        temperature: temperature,
        maxTokens: maxTokens,
      )) {
        yield token;
      }
    } finally {
      _status = LlmStatus.ready;
      notifyListeners();
    }
  }

  /// Stop ongoing generation
  void stopGeneration() {
    _provider?.stopGeneration();
    _status = LlmStatus.ready;
    notifyListeners();
  }

  String _buildPrompt(List<Message> messages, String? systemPrompt) {
    final buffer = StringBuffer();
    
    if (systemPrompt != null && systemPrompt.isNotEmpty) {
      buffer.writeln('<|system|>$systemPrompt</s>');
    }
    
    for (final message in messages) {
      final role = message.role == MessageRole.user ? 'user' : 'assistant';
      buffer.writeln('<|$role|>${message.content}</s>');
    }
    
    buffer.write('<|assistant|>');
    return buffer.toString();
  }
}
