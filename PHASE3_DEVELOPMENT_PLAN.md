# 第三阶段开发计划：核心功能实现

## 架构设计

### 视频分析流程
```
录制完成 → 视频帧提取 → AI视觉分析 → 内容整合 → 最终总结
    ↓           ↓           ↓           ↓           ↓
启动分析    临时图片     批量API调用   文本收集    生成总结
```

### 核心组件设计

#### 1. VideoFrameExtractor 类
```cpp
class VideoFrameExtractor {
public:
    struct ExtractParams {
        QString videoPath;
        QString tempDir;
        int frameRate;          // 视频帧率
        int extractInterval;    // 提取间隔（帧数）
        QString imageFormat;    // 图片格式 (jpg/png)
        int imageQuality;       // 图片质量 (1-100)
    };
    
    struct FrameInfo {
        QString imagePath;
        double timestamp;       // 时间戳（秒）
        int frameNumber;        // 帧编号
    };
    
    // 提取视频帧
    QList<FrameInfo> extractFrames(const ExtractParams& params);
    
    // 计算提取间隔
    static int calculateInterval(int frameRate);
    
    // 清理临时文件
    void cleanupTempFiles(const QString& tempDir);
};
```

#### 2. AIVisionAnalyzer 类
```cpp
class AIVisionAnalyzer : public QObject {
    Q_OBJECT
    
public:
    struct AnalysisResult {
        bool success;
        QString description;
        QString errorMessage;
        double timestamp;
    };
    
    // 分析单张图片
    void analyzeImage(const QString& imagePath, double timestamp);
    
    // 批量分析图片
    void analyzeImages(const QList<VideoFrameExtractor::FrameInfo>& frames);
    
    // 设置AI配置
    void setConfig(const AISummaryConfig& config);

signals:
    void imageAnalyzed(const AnalysisResult& result);
    void batchAnalysisFinished(const QList<AnalysisResult>& results);
    void progressUpdated(int current, int total);
    void errorOccurred(const QString& error);

private:
    // 创建API请求
    QNetworkRequest createVisionRequest(const QString& imagePath);
    
    // 处理API响应
    AnalysisResult parseVisionResponse(QNetworkReply* reply, double timestamp);
};
```

#### 3. ContentSummarizer 类
```cpp
class ContentSummarizer : public QObject {
    Q_OBJECT
    
public:
    struct SummaryRequest {
        QList<QString> descriptions;
        QString videoPath;
        int videoDuration;
        QString summaryPrompt;
    };
    
    // 生成最终总结
    void generateSummary(const SummaryRequest& request);

signals:
    void summaryGenerated(const QString& summary);
    void errorOccurred(const QString& error);

private:
    // 构建总结提示词
    QString buildSummaryPrompt(const SummaryRequest& request);
    
    // 创建文本API请求
    QNetworkRequest createTextRequest(const QString& content);
};
```

#### 4. VideoSummaryManager 类（主控制器）
```cpp
class VideoSummaryManager : public QObject {
    Q_OBJECT
    
public:
    enum ProcessState {
        Idle,
        ExtractingFrames,
        AnalyzingImages,
        GeneratingSummary,
        Completed,
        Error
    };
    
    // 开始分析视频
    void startAnalysis(const QString& videoPath, const AISummaryConfig& config);
    
    // 取消分析
    void cancelAnalysis();
    
    // 获取当前状态
    ProcessState currentState() const;

signals:
    void stateChanged(ProcessState state);
    void progressUpdated(int percentage, const QString& message);
    void analysisCompleted(const QString& summary);
    void errorOccurred(const QString& error);

private slots:
    void onFramesExtracted(const QList<VideoFrameExtractor::FrameInfo>& frames);
    void onImageAnalyzed(const AIVisionAnalyzer::AnalysisResult& result);
    void onBatchAnalysisFinished(const QList<AIVisionAnalyzer::AnalysisResult>& results);
    void onSummaryGenerated(const QString& summary);

private:
    VideoFrameExtractor* frameExtractor;
    AIVisionAnalyzer* visionAnalyzer;
    ContentSummarizer* contentSummarizer;
    ProcessState currentState_;
    QString currentVideoPath;
    QString tempDirectory;
};
```

## 实现细节

### 1. 视频帧提取

#### 使用 FFmpeg 进行帧提取
```cpp
// 计算提取间隔
int VideoFrameExtractor::calculateInterval(int frameRate) {
    // 一秒提取2帧的规则
    return frameRate / 2;  // 30fps -> 15帧间隔, 60fps -> 30帧间隔
}

// 生成FFmpeg命令
QString VideoFrameExtractor::buildFFmpegCommand(const ExtractParams& params) {
    QString interval = QString::number(params.extractInterval);
    
    return QString("ffmpeg -i \"%1\" -vf \"select='not(mod(n,%2))'\" "
                  "-vsync vfr -q:v %3 \"%4/frame_%06d.jpg\"")
           .arg(params.videoPath)
           .arg(interval)
           .arg(params.imageQuality)
           .arg(params.tempDir);
}
```

#### 使用 Qt 内置功能（备选方案）
```cpp
// 使用QMediaPlayer和QVideoWidget提取帧
void VideoFrameExtractor::extractWithQt(const ExtractParams& params) {
    QMediaPlayer player;
    QVideoSink videoSink;
    
    player.setVideoOutput(&videoSink);
    player.setSource(QUrl::fromLocalFile(params.videoPath));
    
    // 连接帧捕获信号
    connect(&videoSink, &QVideoSink::videoFrameChanged,
            this, &VideoFrameExtractor::onFrameChanged);
}
```

### 2. AI视觉API调用

#### OpenAI GPT-4V API
```cpp
QJsonObject AIVisionAnalyzer::createOpenAIPayload(const QString& imagePath) {
    // 读取并编码图片
    QByteArray imageData;
    QFile file(imagePath);
    if (file.open(QIODevice::ReadOnly)) {
        imageData = file.readAll().toBase64();
    }
    
    QJsonObject message;
    message["role"] = "user";
    
    QJsonArray content;
    
    // 文本部分
    QJsonObject textPart;
    textPart["type"] = "text";
    textPart["text"] = "请详细描述这张图片中发生的事情，包括界面元素、用户操作、显示内容等。";
    content.append(textPart);
    
    // 图片部分
    QJsonObject imagePart;
    imagePart["type"] = "image_url";
    QJsonObject imageUrl;
    imageUrl["url"] = QString("data:image/jpeg;base64,%1").arg(QString(imageData));
    imagePart["image_url"] = imageUrl;
    content.append(imagePart);
    
    message["content"] = content;
    
    QJsonObject payload;
    payload["model"] = aiConfig.modelName;
    payload["messages"] = QJsonArray{message};
    payload["max_tokens"] = 300;
    
    return payload;
}
```

#### 硅基流动 API
```cpp
QJsonObject AIVisionAnalyzer::createSiliconFlowPayload(const QString& imagePath) {
    // 类似OpenAI格式，但可能有特定的参数调整
    QJsonObject payload = createOpenAIPayload(imagePath);
    
    // 硅基流动特定配置
    payload["temperature"] = 0.7;
    payload["top_p"] = 0.9;
    
    return payload;
}
```

### 3. 内容整合和总结

#### 总结提示词模板
```cpp
QString ContentSummarizer::buildSummaryPrompt(const SummaryRequest& request) {
    QString prompt = QString(
        "以下是一个时长%1秒的屏幕录制视频的帧分析结果。"
        "请根据这些帧的描述，生成一个清晰、连贯的视频内容总结。\n\n"
        "分析结果：\n"
    ).arg(request.videoDuration);
    
    for (int i = 0; i < request.descriptions.size(); ++i) {
        prompt += QString("时间点 %1: %2\n")
                 .arg(i * 0.5, 0, 'f', 1)  // 每0.5秒一个描述
                 .arg(request.descriptions[i]);
    }
    
    prompt += "\n请生成一个结构化的总结，包括：\n"
             "1. 视频主要内容概述\n"
             "2. 关键操作步骤\n" 
             "3. 重要界面变化\n"
             "4. 最终结果或目标\n\n"
             "总结应该简洁明了，重点突出，便于用户快速了解视频内容。";
    
    return prompt;
}
```

## 集成到主窗口

### 1. 在 MainWindow 中添加视频分析管理器

```cpp
// MainWindow.h 添加成员
private:
    std::unique_ptr<VideoSummaryManager> videoSummaryManager;

// MainWindow.cpp 修改录制完成处理
void MainWindow::onStopRecording() {
    // ... 现有的停止录制逻辑 ...
    
    // 如果启用了视频总结功能
    if (videoSummaryEnabledCheckBox->isChecked() && aiSummaryConfig.isValid()) {
        startVideoAnalysis();
    }
}

void MainWindow::startVideoAnalysis() {
    if (!videoSummaryManager) {
        videoSummaryManager = std::make_unique<VideoSummaryManager>(this);
        
        // 连接信号
        connect(videoSummaryManager.get(), &VideoSummaryManager::progressUpdated,
                this, &MainWindow::onAnalysisProgressUpdated);
        connect(videoSummaryManager.get(), &VideoSummaryManager::analysisCompleted,
                this, &MainWindow::onAnalysisCompleted);
        connect(videoSummaryManager.get(), &VideoSummaryManager::errorOccurred,
                this, &MainWindow::onAnalysisError);
    }
    
    QString videoPath = outputPathEdit->text();
    videoSummaryManager->startAnalysis(videoPath, aiSummaryConfig);
    
    // 更新UI状态
    videoSummaryTextEdit->setText("正在分析视频内容，请稍候...");
}
```

### 2. 进度反馈和状态更新

```cpp
void MainWindow::onAnalysisProgressUpdated(int percentage, const QString& message) {
    videoSummaryTextEdit->setText(QString("分析进度: %1%\n%2").arg(percentage).arg(message));
}

void MainWindow::onAnalysisCompleted(const QString& summary) {
    videoSummaryTextEdit->setText(summary);
    
    // 显示完成通知
    QMessageBox::information(this, "分析完成", 
                           "视频内容分析已完成！\n总结已显示在右侧面板中。");
}

void MainWindow::onAnalysisError(const QString& error) {
    videoSummaryTextEdit->setText(QString("分析失败: %1").arg(error));
    
    QMessageBox::warning(this, "分析失败", 
                        QString("视频内容分析失败：\n%1").arg(error));
}
```

## 依赖和构建配置

### CMakeLists.txt 更新
```cmake
# 添加新的源文件
set(SOURCES
    # ... 现有文件 ...
    src/VideoFrameExtractor.cpp
    src/VideoFrameExtractor.h
    src/AIVisionAnalyzer.cpp  
    src/AIVisionAnalyzer.h
    src/ContentSummarizer.cpp
    src/ContentSummarizer.h
    src/VideoSummaryManager.cpp
    src/VideoSummaryManager.h
)

# 可能需要的额外依赖
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Network Multimedia)

target_link_libraries(AIcp 
    PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Network
    Qt6::Multimedia  # 用于视频处理
)
```

### 外部依赖（可选）
```bash
# FFmpeg (推荐用于视频帧提取)
# Windows: 下载预编译版本
# macOS: brew install ffmpeg
# Linux: apt-get install ffmpeg
```

## 测试计划

### 1. 单元测试
- VideoFrameExtractor 帧提取功能
- AIVisionAnalyzer API调用功能
- ContentSummarizer 文本生成功能

### 2. 集成测试  
- 完整的视频分析流程
- 错误处理和恢复
- 不同视频格式兼容性

### 3. 性能测试
- 长视频处理能力
- 内存使用优化
- 并发处理性能

## 预期开发时间

- VideoFrameExtractor: 2-3天
- AIVisionAnalyzer: 3-4天
- ContentSummarizer: 1-2天
- VideoSummaryManager: 2-3天
- 集成和测试: 2-3天

**总计：10-15天**

## 风险和挑战

1. **视频格式兼容性**：不同录制格式的处理
2. **API调用限制**：速率限制和配额管理
3. **网络稳定性**：断网重连和错误恢复
4. **内存管理**：大视频文件的内存优化
5. **跨平台兼容**：Windows/macOS/Linux差异

这个技术规划为第三阶段的开发提供了详细的指导，确保功能的完整性和稳定性。
