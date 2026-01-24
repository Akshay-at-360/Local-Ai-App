import 'package:hive/hive.dart';

part 'message.g.dart';

/// Role of the message sender
enum MessageRole {
  user,
  assistant,
  system,
}

/// A single chat message
@HiveType(typeId: 0)
class Message extends HiveObject {
  @HiveField(0)
  final String id;

  @HiveField(1)
  final String content;

  @HiveField(2)
  final int roleIndex;

  @HiveField(3)
  final DateTime timestamp;

  @HiveField(4)
  final bool isComplete;

  Message({
    required this.id,
    required this.content,
    required this.roleIndex,
    required this.timestamp,
    this.isComplete = true,
  });

  MessageRole get role => MessageRole.values[roleIndex];

  Message copyWith({
    String? id,
    String? content,
    MessageRole? role,
    DateTime? timestamp,
    bool? isComplete,
  }) {
    return Message(
      id: id ?? this.id,
      content: content ?? this.content,
      roleIndex: role?.index ?? this.roleIndex,
      timestamp: timestamp ?? this.timestamp,
      isComplete: isComplete ?? this.isComplete,
    );
  }

  Map<String, dynamic> toJson() => {
        'id': id,
        'content': content,
        'role': role.name,
        'timestamp': timestamp.toIso8601String(),
        'isComplete': isComplete,
      };

  factory Message.fromJson(Map<String, dynamic> json) {
    return Message(
      id: json['id'] as String,
      content: json['content'] as String,
      roleIndex: MessageRole.values
          .firstWhere((r) => r.name == json['role'])
          .index,
      timestamp: DateTime.parse(json['timestamp'] as String),
      isComplete: json['isComplete'] as bool? ?? true,
    );
  }
}
