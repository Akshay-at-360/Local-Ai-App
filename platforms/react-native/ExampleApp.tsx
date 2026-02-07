/**
 * App.tsx
 * OnDevice AI React Native Example Application
 * 
 * Requirements: 20.5, 20.7
 */

import React, { useState, useCallback, useEffect } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  ScrollView,
  StyleSheet,
  ActivityIndicator,
  Alert,
} from 'react-native';
import OnDeviceAI, { SDKError } from './OnDeviceAI';
import {
  NativeOnDeviceAI,
  OnDeviceAIEventEmitter,
  OnDeviceAIEvents,
} from './NativeModuleBridge';

interface LogEntry {
  timestamp: string;
  message: string;
  level: 'debug' | 'info' | 'warning' | 'error';
}

export default function App() {
  const [isInitialized, setIsInitialized] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [logs, setLogs] = useState<LogEntry[]>([]);
  const [prompt, setPrompt] = useState('');
  const [response, setResponse] = useState('');
  const [memoryInfo, setMemoryInfo] = useState('');

  // Logging utility
  const addLog = useCallback((message: string, level: 'debug' | 'info' | 'warning' | 'error' = 'info') => {
    const timestamp = new Date().toLocaleTimeString();
    setLogs(prev => [...prev, { timestamp, message, level }].slice(-20));
  }, []);

  // Initialize SDK
  const handleInitialize = useCallback(async () => {
    setIsLoading(true);
    try {
      addLog('Initializing OnDevice AI SDK...');
      await OnDeviceAI.initialize({
        threadCount: 2,
        memoryLimitMB: 500,
      });
      addLog('SDK initialized successfully', 'info');
      setIsInitialized(true);
    } catch (error) {
      const msg = error instanceof SDKError ? error.message : String(error);
      addLog(`Initialization failed: ${msg}`, 'error');
      Alert.alert('Error', msg);
    } finally {
      setIsLoading(false);
    }
  }, [addLog]);

  // Generate inference
  const handleGenerate = useCallback(async () => {
    if (!isInitialized) {
      Alert.alert('Error', 'SDK not initialized');
      return;
    }

    if (!prompt.trim()) {
      Alert.alert('Error', 'Please enter a prompt');
      return;
    }

    setIsLoading(true);
    try {
      addLog(`Generating response for: "${prompt}"`);
      const result = await NativeOnDeviceAI.llmGenerate(0, prompt);
      setResponse(result);
      addLog('Generation complete', 'info');
    } catch (error) {
      const msg = error instanceof Error ? error.message : String(error);
      addLog(`Generation failed: ${msg}`, 'error');
      Alert.alert('Error', msg);
    } finally {
      setIsLoading(false);
    }
  }, [isInitialized, prompt, addLog]);

  // Get memory info
  const handleMemoryInfo = useCallback(async () => {
    try {
      const usage = await NativeOnDeviceAI.getMemoryUsage();
      const limit = await NativeOnDeviceAI.getMemoryLimit();
      
      const usageMB = (usage / 1024 / 1024).toFixed(1);
      const limitMB = (limit / 1024 / 1024).toFixed(1);
      const percentage = limit > 0 ? ((usage / limit) * 100).toFixed(1) : 'N/A';
      
      const info = `Memory: ${usageMB} MB / ${limitMB} MB (${percentage}%)`;
      setMemoryInfo(info);
      addLog(info);
    } catch (error) {
      const msg = error instanceof Error ? error.message : String(error);
      addLog(`Memory query failed: ${msg}`, 'error');
    }
  }, [addLog]);

  // Shutdown SDK
  const handleShutdown = useCallback(async () => {
    setIsLoading(true);
    try {
      addLog('Shutting down SDK...');
      await NativeOnDeviceAI.shutdown();
      addLog('SDK shutdown complete', 'info');
      setIsInitialized(false);
      setResponse('');
      setPrompt('');
    } catch (error) {
      const msg = error instanceof Error ? error.message : String(error);
      addLog(`Shutdown failed: ${msg}`, 'error');
    } finally {
      setIsLoading(false);
    }
  }, [addLog]);

  // Setup event listeners
  useEffect(() => {
    const subscriptions = [
      OnDeviceAIEventEmitter.addListener(OnDeviceAIEvents.ERROR, (event) => {
        addLog(`Native error: ${event.message}`, 'error');
      }),
      OnDeviceAIEventEmitter.addListener(OnDeviceAIEvents.MEMORY_WARNING, (event) => {
        addLog(`Memory warning: ${event.usedBytes} / ${event.totalBytes}`, 'warning');
      }),
    ];

    return () => {
      subscriptions.forEach(sub => sub.remove());
    };
  }, [addLog]);

  return (
    <View style={styles.container}>
      <Text style={styles.header}>OnDevice AI</Text>

      <View style={styles.controlSection}>
        <TouchableOpacity
          style={[styles.button, isInitialized && styles.buttonDisabled]}
          onPress={handleInitialize}
          disabled={isInitialized || isLoading}
        >
          {isLoading && <ActivityIndicator color="#fff" />}
          <Text style={styles.buttonText}>Initialize SDK</Text>
        </TouchableOpacity>

        {isInitialized && (
          <TouchableOpacity
            style={[styles.button, styles.buttonDanger]}
            onPress={handleShutdown}
            disabled={isLoading}
          >
            <Text style={styles.buttonText}>Shutdown SDK</Text>
          </TouchableOpacity>
        )}
      </View>

      {isInitialized && (
        <>
          <View style={styles.inferenceSection}>
            <Text style={styles.sectionTitle}>Inference</Text>
            
            <TextInput
              style={styles.input}
              placeholder="Enter your prompt..."
              value={prompt}
              onChangeText={setPrompt}
              editable={!isLoading}
              placeholderTextColor="#999"
            />

            <TouchableOpacity
              style={[styles.button, isLoading && styles.buttonDisabled]}
              onPress={handleGenerate}
              disabled={isLoading}
            >
              {isLoading && <ActivityIndicator color="#fff" />}
              <Text style={styles.buttonText}>Generate</Text>
            </TouchableOpacity>

            {response && (
              <View style={styles.responseBox}>
                <Text style={styles.responseLabel}>Response:</Text>
                <Text style={styles.responseText}>{response}</Text>
              </View>
            )}
          </View>

          <View style={styles.memorySection}>
            <TouchableOpacity
              style={styles.button}
              onPress={handleMemoryInfo}
              disabled={isLoading}
            >
              <Text style={styles.buttonText}>Get Memory Info</Text>
            </TouchableOpacity>

            {memoryInfo && (
              <Text style={styles.memoryText}>{memoryInfo}</Text>
            )}
          </View>
        </>
      )}

      <View style={styles.logsSection}>
        <Text style={styles.sectionTitle}>Logs</Text>
        <ScrollView style={styles.logsContainer}>
          {logs.map((log, idx) => (
            <Text
              key={idx}
              style={[
                styles.logEntry,
                log.level === 'error' && styles.logError,
                log.level === 'warning' && styles.logWarning,
              ]}
            >
              [{log.timestamp}] {log.message}
            </Text>
          ))}
        </ScrollView>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    padding: 16,
  },
  header: {
    fontSize: 28,
    fontWeight: 'bold',
    marginBottom: 20,
    marginTop: 40,
  },
  controlSection: {
    flexDirection: 'row',
    gap: 10,
    marginBottom: 20,
  },
  inferenceSection: {
    backgroundColor: '#fff',
    padding: 16,
    borderRadius: 8,
    marginBottom: 16,
  },
  memorySection: {
    backgroundColor: '#fff',
    padding: 16,
    borderRadius: 8,
    marginBottom: 16,
  },
  logsSection: {
    flex: 1,
    backgroundColor: '#fff',
    borderRadius: 8,
    padding: 12,
  },
  button: {
    backgroundColor: '#007AFF',
    paddingVertical: 12,
    paddingHorizontal: 20,
    borderRadius: 8,
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    gap: 8,
  },
  buttonDisabled: {
    opacity: 0.5,
  },
  buttonDanger: {
    backgroundColor: '#FF3B30',
  },
  buttonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '600',
    marginBottom: 12,
  },
  input: {
    borderWidth: 1,
    borderColor: '#ddd',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
    fontSize: 14,
  },
  responseBox: {
    backgroundColor: '#f0f0f0',
    padding: 12,
    borderRadius: 8,
    marginTop: 12,
  },
  responseLabel: {
    fontSize: 12,
    fontWeight: '600',
    color: '#666',
    marginBottom: 8,
  },
  responseText: {
    fontSize: 14,
    lineHeight: 20,
  },
  memoryText: {
    fontSize: 13,
    color: '#666',
    marginTop: 12,
  },
  logsContainer: {
    flex: 1,
  },
  logEntry: {
    fontSize: 11,
    color: '#333',
    marginBottom: 4,
    fontFamily: 'Courier New',
    lineHeight: 14,
  },
  logError: {
    color: '#FF3B30',
  },
  logWarning: {
    color: '#FF9500',
  },
});
