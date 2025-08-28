#ifndef REALTIMEVIDEOSUMMARYMANAGER_H
#define REALTIMEVIDEOSUMMARYMANAGER_H

#include <QObject>
#include <memory>
#include "RealTimeFrameExtractor.h"
#include "RealTimeAIVisionAnalyzer.h"
#include "AISummaryConfigDialog.h"

/**
 * 实时视频总结管理器 - 管理录制期间的实时帧提取和AI分析
 * 协调RealTimeFrameExtractor和RealTimeAIVisionAnalyzer的工作
 */
class RealTimeVideoSummaryManager : public QObject {
    Q_OBJECT
    
public:
    explicit RealTimeVideoSummaryManager(QObject *parent = nullptr);
    ~RealTimeVideoSummaryManager();
    
    // 设置AI配置
    void setConfig(const AISummaryConfig &config);
    
    // 设置捕获区域（与录制保持一致）
    void setCaptureRegion(int x, int y, int width, int height);
    
    // 开始录制时调用 - 启动实时分析
    void startRecording(const QString &videoPath);
    
    // 停止录制时调用 - 停止实时分析并生成最终总结
    void stopRecording();
    
    // 是否正在进行实时分析
    bool isRealTimeAnalyzing() const;
    
    // 取消所有分析
    void cancelAnalysis();

signals:
    // 实时帧分析完成
    void realTimeFrameAnalyzed(const QString &analysis, double timestamp);
    
    // 总结进度更新
    void summaryProgress(const QString &status, int percentage);
    
    // 最终总结完成
    void summaryCompleted(bool success, const QString &summary, const QString &message);

private slots:
    // 帧提取完成
    void onFrameExtracted(const QString &framePath, double timestamp);
    
    // 帧提取错误
    void onFrameExtractionError(const QString &error);
    
    // 实时帧分析完成
    void onRealTimeFrameAnalyzed(const QString &framePath, const QString &analysis, double timestamp);
    
    // 录制后处理进度
    void onPostRecordingProgress(int current, int total);
    
    // 最终总结生成完成
    void onFinalSummaryGenerated(bool success, const QString &summary, const QString &message);

private:
    void updateProgress(const QString &status, int percentage);
    
    std::unique_ptr<RealTimeFrameExtractor> frameExtractor;
    std::unique_ptr<RealTimeAIVisionAnalyzer> visionAnalyzer;
    
    AISummaryConfig config;
    QString currentVideoDirectory; // 当前录制视频的目录
    bool realTimeAnalyzing;
    int realTimeFrameCount; // 实时分析的帧数计数
};

#endif // REALTIMEVIDEOSUMMARYMANAGER_H
