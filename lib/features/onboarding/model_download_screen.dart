import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../../models/models.dart';
import '../../core/llm/llm_service.dart';
import '../../core/llm/model_manager.dart';
import '../../core/storage/chat_repository.dart';
import '../../theme/app_theme.dart';
import '../chat/chat_screen.dart';

/// Model download and selection screen
class ModelDownloadScreen extends StatefulWidget {
  const ModelDownloadScreen({super.key});

  @override
  State<ModelDownloadScreen> createState() => _ModelDownloadScreenState();
}

class _ModelDownloadScreenState extends State<ModelDownloadScreen> {
  final _modelManager = ModelManager();
  final _repository = ChatRepository();
  Set<String> _downloadedModels = {};

  @override
  void initState() {
    super.initState();
    _loadDownloadedModels();
  }

  Future<void> _loadDownloadedModels() async {
    await _modelManager.initialize();
    final downloaded = await _modelManager.getDownloadedModels();
    setState(() {
      _downloadedModels = downloaded.map((m) => m.id).toSet();
    });
  }

  Future<void> _downloadModel(ModelInfo model) async {
    await _modelManager.downloadModel(model);
    await _loadDownloadedModels();
  }

  Future<void> _selectAndContinue(ModelInfo model) async {
    final llmService = context.read<LlmService>();
    
    // Show loading indicator
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
              Text('Loading model...', style: AppTextStyles.body),
            ],
          ),
        ),
      ),
    );

    try {
      final modelPath = _modelManager.getModelPath(model);
      final success = await llmService.loadModel(modelPath, model);
      
      if (success) {
        await _repository.setSelectedModelId(model.id);
        if (mounted) {
          Navigator.of(context).pop(); // Close loading dialog
          Navigator.of(context).pushReplacement(
            MaterialPageRoute(builder: (_) => const ChatScreen()),
          );
        }
      } else {
        if (mounted) {
          Navigator.of(context).pop();
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Failed to load model')),
          );
        }
      }
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
              _buildHeader(),
              Expanded(
                child: _buildModelList(),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildHeader() {
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
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
              Icons.download_rounded,
              color: Colors.white,
              size: 40,
            ),
          ),
          const SizedBox(height: 24),
          Text(
            'Choose Your AI Model',
            style: AppTextStyles.h2,
            textAlign: TextAlign.center,
          ),
          const SizedBox(height: 8),
          Text(
            'Download a model to start chatting offline',
            style: AppTextStyles.bodySmall,
            textAlign: TextAlign.center,
          ),
        ],
      ),
    );
  }

  Widget _buildModelList() {
    return Consumer<ModelManager>(
      builder: (context, modelManager, child) {
        return ListView.builder(
          padding: const EdgeInsets.symmetric(horizontal: 16),
          itemCount: ModelCatalog.availableModels.length,
          itemBuilder: (context, index) {
            final model = ModelCatalog.availableModels[index];
            final isDownloaded = _downloadedModels.contains(model.id);
            final progress = modelManager.getDownloadProgress(model.id);

            return _buildModelCard(model, isDownloaded, progress);
          },
        );
      },
    );
  }

  Widget _buildModelCard(
    ModelInfo model,
    bool isDownloaded,
    DownloadProgress? progress,
  ) {
    final isDownloading = progress?.status == DownloadStatus.downloading;

    return Container(
      margin: const EdgeInsets.only(bottom: 16),
      decoration: AppDecorations.glassmorphism(opacity: 0.15),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          borderRadius: BorderRadius.circular(16),
          onTap: isDownloaded ? () => _selectAndContinue(model) : null,
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Container(
                      width: 48,
                      height: 48,
                      decoration: BoxDecoration(
                        gradient: model.isDefault
                            ? AppColors.primaryGradient
                            : const LinearGradient(
                                colors: [AppColors.accent, AppColors.accentLight],
                              ),
                        borderRadius: BorderRadius.circular(12),
                      ),
                      child: Icon(
                        _getModelIcon(model.family),
                        color: Colors.white,
                        size: 24,
                      ),
                    ),
                    const SizedBox(width: 16),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Row(
                            children: [
                              Text(model.name, style: AppTextStyles.h3),
                              if (model.isDefault) ...[
                                const SizedBox(width: 8),
                                Container(
                                  padding: const EdgeInsets.symmetric(
                                    horizontal: 8,
                                    vertical: 2,
                                  ),
                                  decoration: BoxDecoration(
                                    gradient: AppColors.primaryGradient,
                                    borderRadius: BorderRadius.circular(8),
                                  ),
                                  child: const Text(
                                    'Recommended',
                                    style: TextStyle(
                                      fontSize: 10,
                                      fontWeight: FontWeight.w600,
                                      color: Colors.white,
                                    ),
                                  ),
                                ),
                              ],
                            ],
                          ),
                          const SizedBox(height: 4),
                          Text(
                            '${model.sizeFormatted} • ${model.quantization}',
                            style: AppTextStyles.caption,
                          ),
                        ],
                      ),
                    ),
                    _buildActionButton(model, isDownloaded, isDownloading),
                  ],
                ),
                const SizedBox(height: 12),
                Text(
                  model.description,
                  style: AppTextStyles.bodySmall,
                ),
                if (isDownloading && progress != null) ...[
                  const SizedBox(height: 12),
                  _buildDownloadProgress(progress),
                ],
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildActionButton(
    ModelInfo model,
    bool isDownloaded,
    bool isDownloading,
  ) {
    if (isDownloaded) {
      return Container(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
        decoration: BoxDecoration(
          color: AppColors.success.withOpacity(0.2),
          borderRadius: BorderRadius.circular(12),
          border: Border.all(color: AppColors.success.withOpacity(0.5)),
        ),
        child: const Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.check_rounded, color: AppColors.success, size: 18),
            SizedBox(width: 4),
            Text(
              'Use',
              style: TextStyle(
                color: AppColors.success,
                fontWeight: FontWeight.w600,
              ),
            ),
          ],
        ),
      );
    }

    if (isDownloading) {
      return IconButton(
        icon: const Icon(Icons.close_rounded, color: AppColors.error),
        onPressed: () => _modelManager.cancelDownload(model.id),
      );
    }

    return GestureDetector(
      onTap: () => _downloadModel(model),
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
        decoration: BoxDecoration(
          gradient: AppColors.primaryGradient,
          borderRadius: BorderRadius.circular(12),
        ),
        child: const Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.download_rounded, color: Colors.white, size: 18),
            SizedBox(width: 4),
            Text(
              'Download',
              style: TextStyle(
                color: Colors.white,
                fontWeight: FontWeight.w600,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDownloadProgress(DownloadProgress progress) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(
              progress.progressText,
              style: AppTextStyles.caption,
            ),
            Text(
              '${(progress.progress * 100).toStringAsFixed(1)}%',
              style: AppTextStyles.caption.copyWith(color: AppColors.primary),
            ),
          ],
        ),
        const SizedBox(height: 8),
        ClipRRect(
          borderRadius: BorderRadius.circular(4),
          child: LinearProgressIndicator(
            value: progress.progress,
            backgroundColor: AppColors.surface,
            valueColor: const AlwaysStoppedAnimation(AppColors.primary),
            minHeight: 6,
          ),
        ),
      ],
    );
  }

  IconData _getModelIcon(String family) {
    switch (family.toLowerCase()) {
      case 'gemma':
        return Icons.auto_awesome;
      case 'qwen':
        return Icons.language;
      case 'phi':
        return Icons.psychology;
      case 'llama':
        return Icons.pets;
      default:
        return Icons.smart_toy;
    }
  }
}
