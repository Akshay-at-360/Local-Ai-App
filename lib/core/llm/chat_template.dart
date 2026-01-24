import '../../models/models.dart';

/// Chat template formatter for different model families
class ChatTemplate {
  static String formatPrompt({
    required List<Message> messages,
    required String chatTemplate,
    String? systemPrompt,
  }) {
    switch (chatTemplate.toLowerCase()) {
      case 'gemma':
        return _formatGemma(messages, systemPrompt);
      case 'phi3':
        return _formatPhi3(messages, systemPrompt);
      case 'llama2':
        return _formatLlama2(messages, systemPrompt);
      case 'chatml':
      default:
        return _formatChatML(messages, systemPrompt);
    }
  }

  static String _formatChatML(List<Message> messages, String? systemPrompt) {
    final buffer = StringBuffer();
    if (systemPrompt != null && systemPrompt.isNotEmpty) {
      buffer.writeln('<|im_start|>system');
      buffer.writeln(systemPrompt);
      buffer.writeln('<|im_end|>');
    }
    for (final message in messages) {
      final role = message.role == MessageRole.user ? 'user' : 'assistant';
      buffer.writeln('<|im_start|>$role');
      buffer.write(message.content);
      if (message.isComplete) buffer.writeln('<|im_end|>');
    }
    if (messages.isEmpty || messages.last.role == MessageRole.user) {
      buffer.write('<|im_start|>assistant\n');
    }
    return buffer.toString();
  }

  static String _formatGemma(List<Message> messages, String? systemPrompt) {
    final buffer = StringBuffer();
    String? pendingSystem = systemPrompt;
    for (final message in messages) {
      if (message.role == MessageRole.user) {
        buffer.write('<start_of_turn>user\n');
        if (pendingSystem != null && pendingSystem.isNotEmpty) {
          buffer.writeln(pendingSystem);
          pendingSystem = null;
        }
        buffer.write(message.content);
        buffer.writeln('<end_of_turn>');
      } else if (message.role == MessageRole.assistant) {
        buffer.write('<start_of_turn>model\n');
        buffer.write(message.content);
        if (message.isComplete) buffer.writeln('<end_of_turn>');
      }
    }
    if (messages.isEmpty || messages.last.role == MessageRole.user) {
      buffer.write('<start_of_turn>model\n');
    }
    return buffer.toString();
  }

  static String _formatPhi3(List<Message> messages, String? systemPrompt) {
    final buffer = StringBuffer();
    if (systemPrompt != null && systemPrompt.isNotEmpty) {
      buffer.write('<|system|>\n$systemPrompt<|end|>\n');
    }
    for (final message in messages) {
      if (message.role == MessageRole.user) {
        buffer.write('<|user|>\n${message.content}<|end|>\n');
      } else if (message.role == MessageRole.assistant) {
        buffer.write('<|assistant|>\n${message.content}');
        if (message.isComplete) buffer.write('<|end|>\n');
      }
    }
    if (messages.isEmpty || messages.last.role == MessageRole.user) {
      buffer.write('<|assistant|>\n');
    }
    return buffer.toString();
  }

  static String _formatLlama2(List<Message> messages, String? systemPrompt) {
    final buffer = StringBuffer();
    if (systemPrompt != null && systemPrompt.isNotEmpty) {
      buffer.write('[INST] <<SYS>>\n$systemPrompt\n<</SYS>>\n\n');
    }
    bool isFirst = true;
    for (final message in messages) {
      if (message.role == MessageRole.user) {
        if (isFirst && (systemPrompt == null || systemPrompt.isEmpty)) {
          buffer.write('[INST] ${message.content} [/INST]');
        } else if (isFirst) {
          buffer.write('${message.content} [/INST]');
        } else {
          buffer.write('[INST] ${message.content} [/INST]');
        }
        isFirst = false;
      } else if (message.role == MessageRole.assistant) {
        buffer.write(' ${message.content} ');
      }
    }
    return buffer.toString();
  }
}
