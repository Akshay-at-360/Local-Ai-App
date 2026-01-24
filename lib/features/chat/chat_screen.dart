import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../models/models.dart';
import '../../core/llm/llm_service.dart';
import '../../core/llm/model_manager.dart';
import '../../core/storage/chat_repository.dart';
import '../../theme/app_theme.dart';
import 'widgets/message_bubble.dart';
import 'widgets/chat_input.dart';
import '../settings/settings_screen.dart';
import '../onboarding/model_download_screen.dart';

/// Main chat screen
class ChatScreen extends StatefulWidget {
  const ChatScreen({super.key});

  @override
  State<ChatScreen> createState() => _ChatScreenState();
}

class _ChatScreenState extends State<ChatScreen> {
  final _scrollController = ScrollController();
  final _repository = ChatRepository();
  
  Conversation? _conversation;
  List<Message> _messages = [];
  String _streamingContent = '';
  bool _isInitialized = false;

  @override
  void initState() {
    super.initState();
    _initialize();
  }

  Future<void> _initialize() async {
    await _repository.initialize();
    await ModelManager().initialize();
    
    // Check if we have any downloaded models
    final downloadedModels = await ModelManager().getDownloadedModels();
    if (downloadedModels.isEmpty) {
      if (mounted) {
        // Navigate to model download screen
        await Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => const ModelDownloadScreen()),
        );
      }
      return;
    }

    // Load the selected model
    final selectedModelId = _repository.selectedModelId ?? 
        downloadedModels.first.id;
    final modelInfo = ModelCatalog.getModelById(selectedModelId) ?? 
        downloadedModels.first;
    
    final llmService = LlmService();
    if (!llmService.isReady) {
      final modelPath = ModelManager().getModelPath(modelInfo);
      await llmService.loadModel(modelPath, modelInfo);
    }

    // Create or load conversation
    final conversations = _repository.getAllConversations();
    if (conversations.isEmpty) {
      _conversation = await _repository.createConversation(
        modelId: selectedModelId,
      );
    } else {
      _conversation = conversations.first;
      _messages = await _repository.getMessages(_conversation!.id);
    }

    setState(() {
      _isInitialized = true;
    });
  }

  Future<void> _sendMessage(String content) async {
    if (_conversation == null) return;

    final llmService = context.read<LlmService>();
    if (!llmService.isReady) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Model not loaded')),
      );
      return;
    }

    // Add user message
    final userMessage = await _repository.addMessage(
      conversationId: _conversation!.id,
      content: content,
      role: MessageRole.user,
    );
    
    setState(() {
      _messages.add(userMessage);
      _streamingContent = '';
    });
    _scrollToBottom();

    // Generate response
    try {
      final systemPrompt = _repository.systemPrompt ?? 
          'You are a helpful, friendly AI assistant. Be concise and helpful.';
      
      await for (final token in llmService.generateStream(
        messages: _messages,
        systemPrompt: systemPrompt,
        temperature: _repository.temperature,
        maxTokens: _repository.maxTokens,
      )) {
        setState(() {
          _streamingContent += token;
        });
        _scrollToBottom();
      }

      // Save assistant message
      final assistantMessage = await _repository.addMessage(
        conversationId: _conversation!.id,
        content: _streamingContent,
        role: MessageRole.assistant,
      );

      setState(() {
        _messages.add(assistantMessage);
        _streamingContent = '';
      });
    } catch (e) {
      setState(() {
        _streamingContent = '';
      });
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error: $e')),
        );
      }
    }
  }

  void _scrollToBottom() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_scrollController.hasClients) {
        _scrollController.animateTo(
          _scrollController.position.maxScrollExtent,
          duration: const Duration(milliseconds: 200),
          curve: Curves.easeOut,
        );
      }
    });
  }

  void _stopGeneration() {
    context.read<LlmService>().stopGeneration();
  }

  Future<void> _startNewChat() async {
    final llmService = context.read<LlmService>();
    _conversation = await _repository.createConversation(
      modelId: llmService.currentModel?.id ?? 'default',
    );
    setState(() {
      _messages = [];
      _streamingContent = '';
    });
  }

  @override
  Widget build(BuildContext context) {
    if (!_isInitialized) {
      return Scaffold(
        body: Container(
          decoration: const BoxDecoration(
            gradient: AppColors.backgroundGradient,
          ),
          child: const Center(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                CircularProgressIndicator(color: AppColors.primary),
                SizedBox(height: 24),
                Text(
                  'Loading...',
                  style: AppTextStyles.body,
                ),
              ],
            ),
          ),
        ),
      );
    }

    return Consumer<LlmService>(
      builder: (context, llmService, child) {
        return Scaffold(
          body: Container(
            decoration: const BoxDecoration(
              gradient: AppColors.backgroundGradient,
            ),
            child: SafeArea(
              child: Column(
                children: [
                  _buildAppBar(llmService),
                  Expanded(
                    child: _buildMessageList(llmService),
                  ),
                  ChatInput(
                    onSend: _sendMessage,
                    onStop: _stopGeneration,
                    isGenerating: llmService.isGenerating,
                    isEnabled: llmService.isReady,
                  ),
                ],
              ),
            ),
          ),
        );
      },
    );
  }

  Widget _buildAppBar(LlmService llmService) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      child: Row(
        children: [
          IconButton(
            icon: const Icon(Icons.add_rounded, color: AppColors.textPrimary),
            onPressed: _startNewChat,
            tooltip: 'New Chat',
          ),
          Expanded(
            child: Column(
              children: [
                Text(
                  'Local AI Chat',
                  style: AppTextStyles.h3,
                ),
                if (llmService.currentModel != null)
                  Text(
                    llmService.currentModel!.name,
                    style: AppTextStyles.caption,
                  ),
              ],
            ),
          ),
          IconButton(
            icon: const Icon(Icons.settings_rounded, color: AppColors.textPrimary),
            onPressed: () {
              Navigator.of(context).push(
                MaterialPageRoute(builder: (_) => const SettingsScreen()),
              );
            },
            tooltip: 'Settings',
          ),
        ],
      ),
    );
  }

  Widget _buildMessageList(LlmService llmService) {
    if (_messages.isEmpty && _streamingContent.isEmpty) {
      return _buildEmptyState();
    }

    return ListView.builder(
      controller: _scrollController,
      padding: const EdgeInsets.symmetric(vertical: 16),
      itemCount: _messages.length + (llmService.isGenerating ? 1 : 0),
      itemBuilder: (context, index) {
        if (index < _messages.length) {
          return MessageBubble(message: _messages[index]);
        } else {
          // Show streaming message
          return MessageBubble(
            message: Message(
              id: 'streaming',
              content: _streamingContent,
              roleIndex: MessageRole.assistant.index,
              timestamp: DateTime.now(),
              isComplete: false,
            ),
            isStreaming: true,
          );
        }
      },
    );
  }

  Widget _buildEmptyState() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Container(
            width: 80,
            height: 80,
            decoration: BoxDecoration(
              gradient: AppColors.primaryGradient,
              borderRadius: BorderRadius.circular(24),
              boxShadow: [
                BoxShadow(
                  color: AppColors.primary.withOpacity(0.3),
                  blurRadius: 20,
                  offset: const Offset(0, 10),
                ),
              ],
            ),
            child: const Icon(
              Icons.auto_awesome,
              color: Colors.white,
              size: 40,
            ),
          ),
          const SizedBox(height: 24),
          Text(
            'Start a conversation',
            style: AppTextStyles.h3,
          ),
          const SizedBox(height: 8),
          Text(
            'Your AI assistant is ready to help',
            style: AppTextStyles.bodySmall,
          ),
        ],
      ),
    );
  }

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }
}
