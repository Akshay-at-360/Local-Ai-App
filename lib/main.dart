import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'core/llm/llm_service.dart';
import 'core/llm/model_manager.dart';
import 'core/storage/chat_repository.dart';
import 'theme/app_theme.dart';
import 'features/onboarding/model_download_screen.dart';
import 'features/chat/chat_screen.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  
  // Initialize services
  final chatRepository = ChatRepository();
  await chatRepository.initialize();
  
  final modelManager = ModelManager();
  await modelManager.initialize();
  
  runApp(const LocalAiChatApp());
}

class LocalAiChatApp extends StatelessWidget {
  const LocalAiChatApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider<LlmService>(
          create: (_) => LlmService(),
        ),
        ChangeNotifierProvider<ModelManager>(
          create: (_) => ModelManager(),
        ),
      ],
      child: MaterialApp(
        title: 'Local AI Chat',
        debugShowCheckedModeBanner: false,
        theme: AppTheme.darkTheme,
        home: const AppStartupScreen(),
      ),
    );
  }
}

class AppStartupScreen extends StatefulWidget {
  const AppStartupScreen({super.key});

  @override
  State<AppStartupScreen> createState() => _AppStartupScreenState();
}

class _AppStartupScreenState extends State<AppStartupScreen> {
  @override
  void initState() {
    super.initState();
    _checkModelStatus();
  }

  Future<void> _checkModelStatus() async {
    final modelManager = ModelManager();
    final downloadedModels = await modelManager.getDownloadedModels();
    
    if (mounted) {
      if (downloadedModels.isEmpty) {
        Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => const ModelDownloadScreen()),
        );
      } else {
        Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => const ChatScreen()),
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
        child: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Container(
                width: 100,
                height: 100,
                decoration: BoxDecoration(
                  gradient: AppColors.primaryGradient,
                  borderRadius: BorderRadius.circular(28),
                  boxShadow: [
                    BoxShadow(
                      color: AppColors.primary.withOpacity(0.4),
                      blurRadius: 30,
                      offset: const Offset(0, 15),
                    ),
                  ],
                ),
                child: const Icon(
                  Icons.auto_awesome,
                  color: Colors.white,
                  size: 50,
                ),
              ),
              const SizedBox(height: 32),
              Text(
                'Local AI Chat',
                style: AppTextStyles.h1.copyWith(
                  foreground: Paint()
                    ..shader = AppColors.primaryGradient.createShader(
                      const Rect.fromLTWH(0, 0, 200, 50),
                    ),
                ),
              ),
              const SizedBox(height: 16),
              const Text(
                'Your private AI assistant',
                style: AppTextStyles.bodySmall,
              ),
              const SizedBox(height: 48),
              const SizedBox(
                width: 24,
                height: 24,
                child: CircularProgressIndicator(
                  strokeWidth: 2,
                  color: AppColors.primary,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
