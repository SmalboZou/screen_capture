#include "VideoSummaryManager.h"
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>

VideoSummaryManager::VideoSummaryManager(QObject *parent)
    : QObject(parent)
    , frameExtractor(std::make_unique<VideoFrameExtractor>(this))
    , visionAnalyzer(std::make_unique<AIVisionAnalyzer>(this))
    , processing(false)
    , currentState(StateIdle)
    , totalFrames(0)
    , analyzedFrames(0)
{
    // 连接信号
    connect(frameExtractor.get(), &VideoFrameExtractor::frameExtractionFinished,
            this, &VideoSummaryManager::onFrameExtractionFinished);
    
    connect(visionAnalyzer.get(), &AIVisionAnalyzer::imageAnalysisProgress,
            this, &VideoSummaryManager::onImageAnalysisProgress);
    connect(visionAnalyzer.get(), &AIVisionAnalyzer::imageAnalysisFinished,
            this, &VideoSummaryManager::onImageAnalysisFinished);
    connect(visionAnalyzer.get(), &AIVisionAnalyzer::finalSummaryGenerated,
            this, &VideoSummaryManager::onFinalSummaryGenerated);
}

VideoSummaryManager::~VideoSummaryManager() {
    cancelProcessing();
}

void VideoSummaryManager::setConfig(const AISummaryConfig &newConfig) {
    config = newConfig;
    visionAnalyzer->setConfig(config);
}

void VideoSummaryManager::startVideoSummary(const QString &videoPath, int frameRate) {
    if (processing) {
        emit summaryCompleted(false, "", "已有视频分析任务在进行中");
        return;
    }
    
    if (!config.isValid()) {
        emit summaryCompleted(false, "", "AI配置无效，请先配置AI模型");
        return;
    }
    
    if (!QFileInfo::exists(videoPath)) {
        emit summaryCompleted(false, "", "视频文件不存在");
        return;
    }
    
    qDebug() << "开始视频内容总结:" << videoPath;
    
    currentVideoPath = videoPath;
    processing = true;
    currentState = StateExtractingFrames;
    totalFrames = 0;
    analyzedFrames = 0;
    
    updateProgress("正在提取视频帧...", 10);
    
    // 智能选择帧提取间隔
    double extractionInterval = calculateSmartInterval(videoPath);
    qDebug() << QString("选择帧提取间隔: %1秒").arg(extractionInterval);
    
    // 开始提取视频帧（使用智能间隔）
    frameExtractor->extractFrames(videoPath, extractionInterval, frameRate);
}

void VideoSummaryManager::onFrameExtractionFinished(bool success, const QString &message) {
    if (!processing || currentState != StateExtractingFrames) {
        return;
    }
    
    if (!success) {
        finishWithError(QString("视频帧提取失败: %1").arg(message));
        return;
    }
    
    QStringList frames = frameExtractor->getExtractedFrames();
    if (frames.isEmpty()) {
        finishWithError("未能提取到任何视频帧");
        return;
    }
    
    totalFrames = frames.size();
    qDebug() << "成功提取" << totalFrames << "帧，开始AI分析";
    
    currentState = StateAnalyzingImages;
    updateProgress(QString("开始分析 %1 帧图片...").arg(totalFrames), 20);
    
    // 开始分析图片
    visionAnalyzer->analyzeImages(frames);
}

void VideoSummaryManager::onImageAnalysisProgress(int current, int total) {
    if (!processing || currentState != StateAnalyzingImages) {
        return;
    }
    
    analyzedFrames = current;
    
    // 图片分析阶段占总进度的60% (20% - 80%)
    int progressPercentage = 20 + (current * 60 / total);
    updateProgress(QString("正在分析图片 %1/%2").arg(current).arg(total), progressPercentage);
}

void VideoSummaryManager::onImageAnalysisFinished(bool success, const QString &message) {
    if (!processing || currentState != StateAnalyzingImages) {
        return;
    }
    
    if (!success) {
        finishWithError(QString("图片分析失败: %1").arg(message));
        return;
    }
    
    qDebug() << "图片分析完成:" << message;
    
    currentState = StateGeneratingSummary;
    updateProgress("正在生成最终总结...", 85);
    
    // AI分析器会自动生成最终总结，这里不需要额外操作
    // onFinalSummaryGenerated 会被调用
}

void VideoSummaryManager::onFinalSummaryGenerated(bool success, const QString &summary, const QString &message) {
    if (!processing || currentState != StateGeneratingSummary) {
        return;
    }
    
    processing = false;
    currentState = StateIdle;
    
    if (success) {
        updateProgress("视频内容总结完成", 100);
        emit summaryCompleted(true, summary, QString("成功分析了 %1 帧图片并生成总结").arg(totalFrames));
    } else {
        finishWithError(QString("总结生成失败: %1").arg(message));
    }
    
    // 清理临时文件
    frameExtractor->cleanup();
}

void VideoSummaryManager::cancelProcessing() {
    if (!processing) {
        return;
    }
    
    qDebug() << "取消视频内容分析";
    
    processing = false;
    currentState = StateIdle;
    
    // 取消各个组件的操作
    frameExtractor->cleanup();
    visionAnalyzer->cancelAnalysis();
    
    emit summaryCompleted(false, "", "用户取消了视频内容分析");
}

bool VideoSummaryManager::isProcessing() const {
    return processing;
}

void VideoSummaryManager::updateProgress(const QString &status, int percentage) {
    emit summaryProgress(status, percentage);
}

void VideoSummaryManager::finishWithError(const QString &message) {
    qDebug() << "视频内容分析失败:" << message;
    
    processing = false;
    currentState = StateIdle;
    
    // 清理资源
    frameExtractor->cleanup();
    visionAnalyzer->cancelAnalysis();
    
    emit summaryCompleted(false, "", message);
}

double VideoSummaryManager::calculateSmartInterval(const QString &videoPath) {
    // 使用FFmpeg获取视频时长
    QString ffmpegPath = findFFmpegPath();
    if (ffmpegPath.isEmpty()) {
        qWarning() << "无法找到FFmpeg，使用默认间隔";
        return 10.0; // 默认10秒间隔
    }
    
    QProcess ffprobe;
    QStringList arguments;
    arguments << "-v" << "quiet"
              << "-show_entries" << "format=duration"
              << "-of" << "csv=p=0"
              << videoPath;
    
    ffprobe.start(ffmpegPath.replace("ffmpeg", "ffprobe"), arguments);
    
    if (!ffprobe.waitForFinished(5000)) {
        qWarning() << "获取视频时长超时，使用默认间隔";
        return 10.0;
    }
    
    QString output = ffprobe.readAllStandardOutput().trimmed();
    bool ok;
    double duration = output.toDouble(&ok);
    
    if (!ok || duration <= 0) {
        qWarning() << "无法解析视频时长，使用默认间隔";
        return 10.0;
    }
    
    qDebug() << QString("视频时长: %1秒").arg(duration);
    
    // 智能间隔选择逻辑
    if (duration < 10.0) {
        // 视频时长小于10秒，使用2秒间隔
        qDebug() << "视频时长小于10秒，使用2秒间隔";
        return 2.0;
    } else {
        // 视频时长大于等于10秒，使用10秒间隔
        qDebug() << "视频时长大于等于10秒，使用10秒间隔";
        return 10.0;
    }
}

QString VideoSummaryManager::findFFmpegPath() const {
    // 查找FFmpeg可执行文件路径
    QStringList possiblePaths;
    
#ifdef Q_OS_WIN
    possiblePaths << "ffmpeg.exe"
                  << "C:/ffmpeg/bin/ffmpeg.exe"
                  << QCoreApplication::applicationDirPath() + "/ffmpeg.exe"
                  << QCoreApplication::applicationDirPath() + "/bin/ffmpeg.exe";
#else
    possiblePaths << "ffmpeg"
                  << "/usr/bin/ffmpeg"
                  << "/usr/local/bin/ffmpeg"
                  << "/opt/homebrew/bin/ffmpeg"
                  << QCoreApplication::applicationDirPath() + "/ffmpeg";
#endif
    
    for (const QString &path : possiblePaths) {
        QFileInfo fileInfo(path);
        if (fileInfo.exists() && fileInfo.isExecutable()) {
            return path;
        }
        
        // 尝试在PATH中查找
        QProcess process;
        process.start(path, QStringList() << "-version");
        if (process.waitForFinished(3000) && process.exitCode() == 0) {
            return path;
        }
    }
    
    return QString();
}
