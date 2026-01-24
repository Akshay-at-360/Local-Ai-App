import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../models/models.dart';
import '../../core/llm/llm_service.dart';
import '../../core/llm/model_manager.dart';
import '../../core/storage/chat_repository.dart';
import '../../theme/app_theme.dart';

/// Settings screen for configuration
class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  final _repository = ChatRepository();
  final _modelManager = ModelManager();
  
  late double _temperature;
  late int _maxTokens;
  String? _systemPrompt;
  List<ModelInfo> _downloadedModels = [];
  String? _selectedModelId;

  @override
  void initState() {
    super.initState();
    _loadSettings();
  }

  Future<void> _loadSettings() async {
    await _modelManager.initialize();
    _downloadedModels = await _modelManager.getDownloadedModels();
    
    setState(() {
      _temperature = _repository.temperature;
      _maxTokens = _repository.maxTokens;
      _systemPrompt = _repository.systemPrompt;
      _selectedModelId = _repository.selectedModelId;
    });
  }

  Future<void> _switchModel(ModelInfo model) async {
    final llmService = context.read<LlmService>();
    
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => Center(
        child: Container(
          padding: const EdgeInsets.all(32),
          decoration: AppDecorations.glassmorphism(),
          child: const Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              CircularProgressIndicator(color: AppColors.primary),
              SizedBox(height: 16),
              Text('Switching model...', style: AppTextStyles.body),
            ],
          ),
        ),
      ),
    );

    try {
      await llmService.unloadModel();
      final modelPath = _modelManager.getModelPath(model);
      final success = await llmService.loadModel(modelPath, model);
      
      if (success) {
        await _repository.setSelectedModelId(model.id);
        setState(() {
          _selectedModelId = model.id;
        });
      }
      
      if (mounted) Navigator.of(context).pop();
    } catch (e) {
      if (mounted) {
        Navigator.of(context).pop();
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error: $e')),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: AppColors.backgroundGradient,
        ),
        child: SafeArea(
          child: Column(
            children: [
              _buildAppBar(),
              Expanded(
                child: ListView(
                  padding: const EdgeInsets.all(16),
                  children: [
                    _buildSection('Current Model', [
                      _buildModelSelector(),
                    ]),
                    const SizedBox(height: 24),
                    _buildSection('Generation Settings', [
                      _buildTemperatureSlider(),
                      const SizedBox(height: 16),
                      _buildMaxTokensSlider(),
                    ]),
                    const SizedBox(height: 24),
                    _buildSection('System Prompt', [
                      _buildSystemPromptField(),
                    ]),
                    const SizedBox(height: 24),
                    _buildSection('Model Management', [
                      _buildDownloadedModelsList(),
                    ]),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildAppBar() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 12),
      child: Row(
        children: [
          IconButton(
            icon: const Icon(Icons.arrow_back_rounded, color: AppColors.textPrimary),
            onPressed: () => Navigator.of(context).pop(),
          ),
          const Expanded(
            child: Text(
              'Settings',
              style: AppTextStyles.h3,
              textAlign: TextAlign.center,
            ),
          ),
          const SizedBox(width: 48),
        ],
      ),
    );
  }

  Widget _buildSection(String title, List<Widget> children) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(title, style: AppTextStyles.h3),
        const SizedBox(height: 12),
        Container(
          decoration: AppDecorations.glassmorphism(opacity: 0.15),
          padding: const EdgeInsets.all(16),
          child: Column(children: children),
        ),
      ],
    );
  }

  Widget _buildModelSelector() {
    final currentModel = _downloadedModels.isEmpty
        ? null
        : _downloadedModels.firstWhere(
            (m) => m.id == _selectedModelId,
            orElse: () => _downloadedModels.first,
          );

    return Row(
      children: [
        if (currentModel != null) ...[
          Container(
            width: 48,
            height: 48,
            decoration: BoxDecoration(
              gradient: AppColors.primaryGradient,
              borderRadius: BorderRadius.circular(12),
            ),
            child: const Icon(Icons.auto_awesome, color: Colors.white),
          ),
          const SizedBox(width: 16),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(currentModel.name, style: AppTextStyles.body),
                Text(
                  '${currentModel.sizeFormatted} • ${currentModel.quantization}',
                  style: AppTextStyles.caption,
                ),
              ],
            ),
          ),
          TextButton(
            onPressed: () => _showModelPicker(),
            child: const Text('Change'),
          ),
        ] else
          const Text('No models downloaded', style: AppTextStyles.body),
      ],
    );
  }

  void _showModelPicker() {
    showModalBottomSheet(
      context: context,
      backgroundColor: AppColors.backgroundMedium,
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(24)),
      ),
      builder: (context) => Container(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text('Select Model', style: AppTextStyles.h3),
            const SizedBox(height: 16),
            ...List.generate(_downloadedModels.length, (index) {
              final model = _downloadedModels[index];
              final isSelected = model.id == _selectedModelId;
              
              return ListTile(
                leading: Container(
                  width: 40,
                  height: 40,
                  decoration: BoxDecoration(
                    color: isSelected ? AppColors.primary : AppColors.surface,
                    borderRadius: BorderRadius.circular(10),
                  ),
                  child: Icon(
                    Icons.auto_awesome,
                    color: isSelected ? Colors.white : AppColors.textSecondary,
                    size: 20,
                  ),
                ),
                title: Text(model.name, style: AppTextStyles.body),
                subtitle: Text(model.sizeFormatted, style: AppTextStyles.caption),
                trailing: isSelected
                    ? const Icon(Icons.check_circle, color: AppColors.success)
                    : null,
                onTap: () {
                  Navigator.of(context).pop();
                  _switchModel(model);
                },
              );
            }),
          ],
        ),
      ),
    );
  }

  Widget _buildTemperatureSlider() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            const Text('Temperature', style: AppTextStyles.body),
            Text(
              _temperature.toStringAsFixed(2),
              style: AppTextStyles.body.copyWith(color: AppColors.primary),
            ),
          ],
        ),
        const SizedBox(height: 8),
        SliderTheme(
          data: SliderThemeData(
            activeTrackColor: AppColors.primary,
            inactiveTrackColor: AppColors.surface,
            thumbColor: AppColors.primary,
            overlayColor: AppColors.primary.withOpacity(0.2),
          ),
          child: Slider(
            value: _temperature,
            min: 0.0,
            max: 2.0,
            onChanged: (value) {
              setState(() => _temperature = value);
              _repository.setTemperature(value);
            },
          ),
        ),
        Text(
          'Higher values = more creative, lower = more focused',
          style: AppTextStyles.caption,
        ),
      ],
    );
  }

  Widget _buildMaxTokensSlider() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            const Text('Max Tokens', style: AppTextStyles.body),
            Text(
              _maxTokens.toString(),
              style: AppTextStyles.body.copyWith(color: AppColors.primary),
            ),
          ],
        ),
        const SizedBox(height: 8),
        SliderTheme(
          data: SliderThemeData(
            activeTrackColor: AppColors.primary,
            inactiveTrackColor: AppColors.surface,
            thumbColor: AppColors.primary,
            overlayColor: AppColors.primary.withOpacity(0.2),
          ),
          child: Slider(
            value: _maxTokens.toDouble(),
            min: 64,
            max: 2048,
            divisions: 31,
            onChanged: (value) {
              setState(() => _maxTokens = value.toInt());
              _repository.setMaxTokens(value.toInt());
            },
          ),
        ),
        Text(
          'Maximum length of AI response',
          style: AppTextStyles.caption,
        ),
      ],
    );
  }

  Widget _buildSystemPromptField() {
    return TextField(
      maxLines: 4,
      decoration: InputDecoration(
        hintText: 'Enter a system prompt to customize AI behavior...',
        hintStyle: AppTextStyles.bodySmall,
        filled: true,
        fillColor: AppColors.backgroundLight,
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: BorderSide.none,
        ),
      ),
      style: AppTextStyles.body,
      controller: TextEditingController(text: _systemPrompt),
      onChanged: (value) {
        _systemPrompt = value;
        _repository.setSystemPrompt(value.isEmpty ? null : value);
      },
    );
  }

  Widget _buildDownloadedModelsList() {
    if (_downloadedModels.isEmpty) {
      return const Text('No models downloaded', style: AppTextStyles.bodySmall);
    }

    return Column(
      children: List.generate(_downloadedModels.length, (index) {
        final model = _downloadedModels[index];
        return ListTile(
          contentPadding: EdgeInsets.zero,
          leading: Container(
            width: 40,
            height: 40,
            decoration: BoxDecoration(
              color: AppColors.surface,
              borderRadius: BorderRadius.circular(10),
            ),
            child: const Icon(Icons.folder, color: AppColors.textSecondary),
          ),
          title: Text(model.name, style: AppTextStyles.body),
          subtitle: Text(model.sizeFormatted, style: AppTextStyles.caption),
          trailing: IconButton(
            icon: const Icon(Icons.delete_outline, color: AppColors.error),
            onPressed: () => _confirmDeleteModel(model),
          ),
        );
      }),
    );
  }

  void _confirmDeleteModel(ModelInfo model) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: AppColors.backgroundMedium,
        title: const Text('Delete Model?', style: AppTextStyles.h3),
        content: Text(
          'Are you sure you want to delete ${model.name}? You will need to download it again to use it.',
          style: AppTextStyles.body,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () async {
              Navigator.of(context).pop();
              await _modelManager.deleteModel(model);
              await _loadSettings();
            },
            child: const Text('Delete', style: TextStyle(color: AppColors.error)),
          ),
        ],
      ),
    );
  }
}
