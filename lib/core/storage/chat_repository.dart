import 'package:hive_flutter/hive_flutter.dart';
import 'package:uuid/uuid.dart';
import '../../models/models.dart';

/// Repository for managing chat conversations and messages
class ChatRepository {
  static final ChatRepository _instance = ChatRepository._internal();
  factory ChatRepository() => _instance;
  ChatRepository._internal();

  static const String _conversationsBox = 'conversations';
  static const String _messagesBoxPrefix = 'messages_';
  static const String _settingsBox = 'settings';

  final _uuid = const Uuid();
  bool _initialized = false;

  /// Initialize Hive and open boxes
  Future<void> initialize() async {
    if (_initialized) return;
    await Hive.initFlutter();
    
    // Register adapters if not already registered
    if (!Hive.isAdapterRegistered(0)) {
      Hive.registerAdapter(MessageAdapter());
    }
    if (!Hive.isAdapterRegistered(1)) {
      Hive.registerAdapter(ConversationAdapter());
    }

    await Hive.openBox<Conversation>(_conversationsBox);
    await Hive.openBox(_settingsBox);
    
    _initialized = true;
  }

  Box<Conversation> get _conversations => 
      Hive.box<Conversation>(_conversationsBox);

  Box get _settings => Hive.box(_settingsBox);

  /// Create a new conversation
  Future<Conversation> createConversation({
    String? title,
    required String modelId,
    String? systemPrompt,
  }) async {
    final now = DateTime.now();
    final conversation = Conversation(
      id: _uuid.v4(),
      title: title ?? 'New Chat',
      createdAt: now,
      updatedAt: now,
      modelId: modelId,
      systemPrompt: systemPrompt,
    );

    await _conversations.put(conversation.id, conversation);
    await Hive.openBox<Message>('$_messagesBoxPrefix${conversation.id}');
    return conversation;
  }

  /// Get all conversations
  List<Conversation> getAllConversations() {
    final conversations = _conversations.values.toList();
    conversations.sort((a, b) => b.updatedAt.compareTo(a.updatedAt));
    return conversations;
  }

  /// Get a conversation by ID
  Conversation? getConversation(String id) {
    return _conversations.get(id);
  }

  /// Update a conversation
  Future<void> updateConversation(Conversation conversation) async {
    conversation.updatedAt = DateTime.now();
    await _conversations.put(conversation.id, conversation);
  }

  /// Delete a conversation
  Future<void> deleteConversation(String id) async {
    await _conversations.delete(id);
    final messagesBox = await Hive.openBox<Message>('$_messagesBoxPrefix$id');
    await messagesBox.deleteFromDisk();
  }

  /// Get messages for a conversation
  Future<List<Message>> getMessages(String conversationId) async {
    final box = await Hive.openBox<Message>('$_messagesBoxPrefix$conversationId');
    final messages = box.values.toList();
    messages.sort((a, b) => a.timestamp.compareTo(b.timestamp));
    return messages;
  }

  /// Add a message to a conversation
  Future<Message> addMessage({
    required String conversationId,
    required String content,
    required MessageRole role,
    bool isComplete = true,
  }) async {
    final message = Message(
      id: _uuid.v4(),
      content: content,
      roleIndex: role.index,
      timestamp: DateTime.now(),
      isComplete: isComplete,
    );

    final box = await Hive.openBox<Message>('$_messagesBoxPrefix$conversationId');
    await box.put(message.id, message);

    // Update conversation title if it's the first user message
    final conversation = _conversations.get(conversationId);
    if (conversation != null) {
      if (conversation.title == 'New Chat' && role == MessageRole.user) {
        conversation.title = content.length > 50 
            ? '${content.substring(0, 50)}...' 
            : content;
      }
      await updateConversation(conversation);
    }

    return message;
  }

  /// Update a message
  Future<void> updateMessage(String conversationId, Message message) async {
    final box = await Hive.openBox<Message>('$_messagesBoxPrefix$conversationId');
    await box.put(message.id, message);
  }

  /// Get/set selected model ID
  String? get selectedModelId => _settings.get('selectedModelId');
  Future<void> setSelectedModelId(String modelId) => 
      _settings.put('selectedModelId', modelId);

  /// Get/set temperature
  double get temperature => _settings.get('temperature', defaultValue: 0.7);
  Future<void> setTemperature(double value) => 
      _settings.put('temperature', value);

  /// Get/set max tokens
  int get maxTokens => _settings.get('maxTokens', defaultValue: 512);
  Future<void> setMaxTokens(int value) => 
      _settings.put('maxTokens', value);

  /// Get/set system prompt
  String? get systemPrompt => _settings.get('systemPrompt');
  Future<void> setSystemPrompt(String? value) => 
      _settings.put('systemPrompt', value);
}
