#include "RealTimeVideoSummaryManager.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>

RealTimeVideoSummaryManager::RealTimeVideoSummaryManager(QObject *parent)
    : QObject(parent)
    , frameExtractor(std::make_unique<RealTimeFrameExtractor>(this))
    , visionAnalyzer(std::make_unique<RealTimeAIVisionAnalyzer>(this))
    , realTimeAnalyzing(false)
    , realTimeFrameCount(0)
{
    // 连接帧提取器信号
    connect(frameExtractor.get(), &RealTimeFrameExtractor::frameExtracted,
            this, &RealTimeVideoSummaryManager::onFrameExtracted);
    connect(frameExtractor.get(), &RealTimeFrameExtractor::extractionError,
            this, &RealTimeVideoSummaryManager::onFrameExtractionError);
    
    // 连接AI分析器信号
    connect(visionAnalyzer.get(), &RealTimeAIVisionAnalyzer::realTimeFrameAnalyzed,
            this, &RealTimeVideoSummaryManager::onRealTimeFrameAnalyzed);
    connect(visionAnalyzer.get(), &RealTimeAIVisionAnalyzer::postRecordingProgress,
            this, &RealTimeVideoSummaryManager::onPostRecordingProgress);
    connect(visionAnalyzer.get(), &RealTimeAIVisionAnalyzer::finalSummaryGenerated,
            this, &RealTimeVideoSummaryManager::onFinalSummaryGenerated);
}

RealTimeVideoSummaryManager::~RealTimeVideoSummaryManager() {
    cancelAnalysis();
}

void RealTimeVideoSummaryManager::setConfig(const AISummaryConfig &newConfig) {
    config = newConfig;
    visionAnalyzer->setConfig(config);
}

void RealTimeVideoSummaryManager::startRecording(const QString &videoPath) {
    if (realTimeAnalyzing) {
        qWarning() << "实时视频分析已在进行中";
        return;
    }
    
    if (!config.isValid()) {
        qWarning() << "AI配置无效，无法开始实时视频分析";
        updateProgress("AI配置无效", 0);
        return;
    }
    
    // 从视频路径获取目录
    QFileInfo videoInfo(videoPath);
    currentVideoDirectory = videoInfo.absolutePath();
    
    qDebug() << "开始实时视频总结分析:" << videoPath;
    qDebug() << "帧保存目录:" << currentVideoDirectory;
    
    realTimeAnalyzing = true;
    realTimeFrameCount = 0;
    
    updateProgress("启动实时分析...", 5);
    
    // 设置录制开始时间
    frameExtractor->setRecordingStartTime(QDateTime::currentMSecsSinceEpoch());
    
    // 启动实时帧提取
    frameExtractor->startExtraction(currentVideoDirectory);
    
    // 启动实时AI分析
    visionAnalyzer->startRealTimeAnalysis();
    
    updateProgress("实时分析已启动", 10);
    
    qDebug() << "实时视频总结分析已启动";
}

void RealTimeVideoSummaryManager::stopRecording() {
    if (!realTimeAnalyzing) {
        return;
    }
    
    qDebug() << "停止实时视频分析并生成最终总结";
    
    updateProgress("停止实时分析，开始生成总结...", 80);
    
    // 停止帧提取
    frameExtractor->stopExtraction();
    
    // 停止AI分析并生成最终总结
    visionAnalyzer->stopAndGenerateFinalSummary();
    
    realTimeAnalyzing = false;
    
    qDebug() << QString("实时分析结束，共分析了 %1 帧").arg(realTimeFrameCount);
}

bool RealTimeVideoSummaryManager::isRealTimeAnalyzing() const {
    return realTimeAnalyzing;
}

void RealTimeVideoSummaryManager::cancelAnalysis() {
    if (!realTimeAnalyzing) {
        return;
    }
    
    qDebug() << "取消实时视频分析";
    
    realTimeAnalyzing = false;
    realTimeFrameCount = 0;
    
    frameExtractor->stopExtraction();
    visionAnalyzer->cancelAnalysis();
    
    updateProgress("分析已取消", 0);
    emit summaryCompleted(false, "", "用户取消了实时视频分析");
}

void RealTimeVideoSummaryManager::onFrameExtracted(const QString &framePath, double timestamp) {
    if (!realTimeAnalyzing) {
        return;
    }
    
    realTimeFrameCount++;
    
    qDebug() << QString("提取到第 %1 帧: %2 (时间戳: %3s)")
                .arg(realTimeFrameCount)
                .arg(QFileInfo(framePath).fileName())
                .arg(timestamp, 0, 'f', 1);
    
    // 将帧添加到AI分析队列
    visionAnalyzer->addFrameForAnalysis(framePath, timestamp);
    
    // 更新状态 - 实时分析阶段，不显示具体进度
    QString progressText = QString("正在分析屏幕内容...");
    updateProgress(progressText, -1); // -1表示不显示进度百分比
}

void RealTimeVideoSummaryManager::onFrameExtractionError(const QString &error) {
    qWarning() << "实时帧提取错误:" << error;
    
    // 实时提取的错误不应该终止整个流程
    // 只记录警告，继续进行
    updateProgress("帧提取遇到问题，继续分析...", -1); // -1表示不改变进度
}

void RealTimeVideoSummaryManager::onRealTimeFrameAnalyzed(const QString &framePath, const QString &analysis, double timestamp) {
    if (!realTimeAnalyzing) {
        return;
    }
    
    qDebug() << QString("实时分析完成 - 时间戳: %1s, 结果: %2")
                .arg(timestamp, 0, 'f', 1)
                .arg(analysis.left(50) + (analysis.length() > 50 ? "..." : ""));
    
    // 发出实时分析完成信号
    emit realTimeFrameAnalyzed(analysis, timestamp);
}

void RealTimeVideoSummaryManager::onPostRecordingProgress(int current, int total) {
    // 录制后处理进度（最终总结生成）
    QString statusText = "正在生成视频内容总结...";
    updateProgress(statusText, -1); // 不显示具体进度百分比
}

void RealTimeVideoSummaryManager::onFinalSummaryGenerated(bool success, const QString &summary, const QString &message) {
    qDebug() << "最终总结生成完成:" << (success ? "成功" : "失败");
    
    if (success) {
        updateProgress("实时视频分析完成", 100);
        qDebug() << "总结内容长度:" << summary.length() << "字符";
    } else {
        updateProgress("总结生成失败", 100);
        qWarning() << "总结生成失败:" << message;
    }
    
    // 发出最终完成信号
    emit summaryCompleted(success, summary, message);
    
    // 重置状态
    realTimeAnalyzing = false;
    realTimeFrameCount = 0;
}

void RealTimeVideoSummaryManager::updateProgress(const QString &status, int percentage) {
    // 如果percentage为-1，表示不显示进度百分比，只显示状态
    if (percentage >= 0) {
        emit summaryProgress(status, percentage);
    } else {
        // 只发送状态信息，不发送进度百分比
        emit summaryProgress(status, 0); // 传递0作为占位符，但UI会忽略显示
    }
    
    qDebug() << QString("实时分析状态: %1").arg(status);
}
