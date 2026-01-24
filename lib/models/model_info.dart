/// Model information for available LLM models
class ModelInfo {
  final String id;
  final String name;
  final String description;
  final String filename;
  final String downloadUrl;
  final int sizeBytes;
  final String quantization;
  final String family;
  final bool isDefault;
  final int contextLength;
  final String chatTemplate;

  const ModelInfo({
    required this.id,
    required this.name,
    required this.description,
    required this.filename,
    required this.downloadUrl,
    required this.sizeBytes,
    required this.quantization,
    required this.family,
    this.isDefault = false,
    this.contextLength = 2048,
    this.chatTemplate = 'chatml',
  });

  String get sizeFormatted {
    if (sizeBytes >= 1024 * 1024 * 1024) {
      return '${(sizeBytes / (1024 * 1024 * 1024)).toStringAsFixed(1)} GB';
    } else if (sizeBytes >= 1024 * 1024) {
      return '${(sizeBytes / (1024 * 1024)).toStringAsFixed(0)} MB';
    } else {
      return '${(sizeBytes / 1024).toStringAsFixed(0)} KB';
    }
  }

  Map<String, dynamic> toJson() => {
        'id': id,
        'name': name,
        'description': description,
        'filename': filename,
        'downloadUrl': downloadUrl,
        'sizeBytes': sizeBytes,
        'quantization': quantization,
        'family': family,
        'isDefault': isDefault,
        'contextLength': contextLength,
        'chatTemplate': chatTemplate,
      };

  factory ModelInfo.fromJson(Map<String, dynamic> json) {
    return ModelInfo(
      id: json['id'] as String,
      name: json['name'] as String,
      description: json['description'] as String,
      filename: json['filename'] as String,
      downloadUrl: json['downloadUrl'] as String,
      sizeBytes: json['sizeBytes'] as int,
      quantization: json['quantization'] as String,
      family: json['family'] as String,
      isDefault: json['isDefault'] as bool? ?? false,
      contextLength: json['contextLength'] as int? ?? 2048,
      chatTemplate: json['chatTemplate'] as String? ?? 'chatml',
    );
  }
}

/// Catalog of available models for download
class ModelCatalog {
  static const List<ModelInfo> availableModels = [
    // Default model - Gemma 3 270M
    ModelInfo(
      id: 'gemma-3-1b-it',
      name: 'Gemma 3 1B IT',
      description: 'Google\'s lightweight instruction-tuned model. Great balance of speed and quality.',
      filename: 'gemma-3-1b-it-Q4_K_M.gguf',
      downloadUrl: 'https://huggingface.co/bartowski/gemma-3-1b-it-GGUF/resolve/main/gemma-3-1b-it-Q4_K_M.gguf',
      sizeBytes: 750 * 1024 * 1024, // ~750MB
      quantization: 'Q4_K_M',
      family: 'gemma',
      isDefault: true,
      contextLength: 8192,
      chatTemplate: 'gemma',
    ),
    
    // Qwen 2.5 0.5B - Ultra small
    ModelInfo(
      id: 'qwen2.5-0.5b-instruct',
      name: 'Qwen 2.5 0.5B',
      description: 'Ultra-compact model for basic conversations. Very fast on any device.',
      filename: 'qwen2.5-0.5b-instruct-q4_k_m.gguf',
      downloadUrl: 'https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_k_m.gguf',
      sizeBytes: 400 * 1024 * 1024, // ~400MB
      quantization: 'Q4_K_M',
      family: 'qwen',
      contextLength: 32768,
      chatTemplate: 'chatml',
    ),
    
    // Qwen 2.5 1.5B
    ModelInfo(
      id: 'qwen2.5-1.5b-instruct',
      name: 'Qwen 2.5 1.5B',
      description: 'Excellent multilingual support with good reasoning capabilities.',
      filename: 'qwen2.5-1.5b-instruct-q4_k_m.gguf',
      downloadUrl: 'https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/qwen2.5-1.5b-instruct-q4_k_m.gguf',
      sizeBytes: 1024 * 1024 * 1024, // ~1GB
      quantization: 'Q4_K_M',
      family: 'qwen',
      contextLength: 32768,
      chatTemplate: 'chatml',
    ),
    
    // Phi-3 mini
    ModelInfo(
      id: 'phi-3-mini-4k-instruct',
      name: 'Phi-3 Mini 3.8B',
      description: 'Microsoft\'s efficient model. Excellent for reasoning, code, and math.',
      filename: 'Phi-3-mini-4k-instruct-q4.gguf',
      downloadUrl: 'https://huggingface.co/microsoft/Phi-3-mini-4k-instruct-gguf/resolve/main/Phi-3-mini-4k-instruct-q4.gguf',
      sizeBytes: 2300 * 1024 * 1024, // ~2.3GB
      quantization: 'Q4_0',
      family: 'phi',
      contextLength: 4096,
      chatTemplate: 'phi3',
    ),
    
    // TinyLlama
    ModelInfo(
      id: 'tinyllama-1.1b-chat',
      name: 'TinyLlama 1.1B',
      description: 'Compact and efficient. Good for devices with limited RAM.',
      filename: 'tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf',
      downloadUrl: 'https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf',
      sizeBytes: 670 * 1024 * 1024, // ~670MB
      quantization: 'Q4_K_M',
      family: 'llama',
      contextLength: 2048,
      chatTemplate: 'llama2',
    ),
    
    // SmolLM2
    ModelInfo(
      id: 'smollm2-1.7b-instruct',
      name: 'SmolLM2 1.7B',
      description: 'HuggingFace\'s efficient small model. Great for general chat.',
      filename: 'smollm2-1.7b-instruct-q4_k_m.gguf',
      downloadUrl: 'https://huggingface.co/HuggingFaceTB/SmolLM2-1.7B-Instruct-GGUF/resolve/main/smollm2-1.7b-instruct-q4_k_m.gguf',
      sizeBytes: 1100 * 1024 * 1024, // ~1.1GB
      quantization: 'Q4_K_M',
      family: 'smollm',
      contextLength: 8192,
      chatTemplate: 'chatml',
    ),
  ];

  static ModelInfo get defaultModel =>
      availableModels.firstWhere((m) => m.isDefault);

  static ModelInfo? getModelById(String id) {
    try {
      return availableModels.firstWhere((m) => m.id == id);
    } catch (_) {
      return null;
    }
  }
}
