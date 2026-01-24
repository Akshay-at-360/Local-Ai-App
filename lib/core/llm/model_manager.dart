import 'dart:io';
import 'package:dio/dio.dart';
import 'package:flutter/foundation.dart';
import 'package:path_provider/path_provider.dart';
import 'package:path/path.dart' as p;
import '../../models/models.dart';

/// Download status for a model
enum DownloadStatus {
  notStarted,
  downloading,
  completed,
  failed,
  cancelled,
}

/// Download progress information
class DownloadProgress {
  final String modelId;
  final DownloadStatus status;
  final double progress; // 0.0 to 1.0
  final int downloadedBytes;
  final int totalBytes;
  final String? error;

  DownloadProgress({
    required this.modelId,
    required this.status,
    this.progress = 0.0,
    this.downloadedBytes = 0,
    this.totalBytes = 0,
    this.error,
  });

  String get progressText {
    if (totalBytes == 0) return 'Starting...';
    final downloadedMB = (downloadedBytes / (1024 * 1024)).toStringAsFixed(1);
    final totalMB = (totalBytes / (1024 * 1024)).toStringAsFixed(1);
    return '$downloadedMB / $totalMB MB';
  }
}

/// Manager for downloading and managing local models
class ModelManager extends ChangeNotifier {
  static final ModelManager _instance = ModelManager._internal();
  factory ModelManager() => _instance;
  ModelManager._internal();

  final Dio _dio = Dio();
  final Map<String, DownloadProgress> _downloads = {};
  final Map<String, CancelToken> _cancelTokens = {};
  String? _modelsDir;

  Map<String, DownloadProgress> get downloads => Map.unmodifiable(_downloads);

  /// Initialize the model manager
  Future<void> initialize() async {
    final appDir = await getApplicationDocumentsDirectory();
    _modelsDir = p.join(appDir.path, 'models');
    await Directory(_modelsDir!).create(recursive: true);
  }

  /// Get the directory where models are stored
  String get modelsDirectory => _modelsDir ?? '';

  /// Get the full path for a model file
  String getModelPath(ModelInfo model) {
    return p.join(_modelsDir!, model.filename);
  }

  /// Check if a model is downloaded
  Future<bool> isModelDownloaded(ModelInfo model) async {
    if (_modelsDir == null) await initialize();
    final file = File(getModelPath(model));
    return file.existsSync();
  }

  /// Get list of downloaded models
  Future<List<ModelInfo>> getDownloadedModels() async {
    if (_modelsDir == null) await initialize();
    final downloaded = <ModelInfo>[];
    for (final model in ModelCatalog.availableModels) {
      if (await isModelDownloaded(model)) {
        downloaded.add(model);
      }
    }
    return downloaded;
  }

  /// Download a model
  Future<bool> downloadModel(ModelInfo model) async {
    if (_modelsDir == null) await initialize();

    final cancelToken = CancelToken();
    _cancelTokens[model.id] = cancelToken;

    _downloads[model.id] = DownloadProgress(
      modelId: model.id,
      status: DownloadStatus.downloading,
    );
    notifyListeners();

    try {
      final filePath = getModelPath(model);
      
      await _dio.download(
        model.downloadUrl,
        filePath,
        cancelToken: cancelToken,
        onReceiveProgress: (count, total) {
          _downloads[model.id] = DownloadProgress(
            modelId: model.id,
            status: DownloadStatus.downloading,
            progress: total > 0 ? count / total : 0.0,
            downloadedBytes: count,
            totalBytes: total,
          );
          notifyListeners();
        },
      );

      _downloads[model.id] = DownloadProgress(
        modelId: model.id,
        status: DownloadStatus.completed,
        progress: 1.0,
      );
      notifyListeners();
      return true;
    } on DioException catch (e) {
      if (e.type == DioExceptionType.cancel) {
        _downloads[model.id] = DownloadProgress(
          modelId: model.id,
          status: DownloadStatus.cancelled,
        );
      } else {
        _downloads[model.id] = DownloadProgress(
          modelId: model.id,
          status: DownloadStatus.failed,
          error: e.message,
        );
      }
      notifyListeners();
      return false;
    } catch (e) {
      _downloads[model.id] = DownloadProgress(
        modelId: model.id,
        status: DownloadStatus.failed,
        error: e.toString(),
      );
      notifyListeners();
      return false;
    } finally {
      _cancelTokens.remove(model.id);
    }
  }

  /// Cancel a download in progress
  void cancelDownload(String modelId) {
    _cancelTokens[modelId]?.cancel();
  }

  /// Delete a downloaded model
  Future<bool> deleteModel(ModelInfo model) async {
    try {
      final file = File(getModelPath(model));
      if (await file.exists()) {
        await file.delete();
      }
      _downloads.remove(model.id);
      notifyListeners();
      return true;
    } catch (e) {
      return false;
    }
  }

  /// Get download progress for a model
  DownloadProgress? getDownloadProgress(String modelId) {
    return _downloads[modelId];
  }
}
