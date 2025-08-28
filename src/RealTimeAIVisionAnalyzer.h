#ifndef REALTIMEAIVISIONANALYZER_H
#define REALTIMEAIVISIONANALYZER_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QMutex>
#include <QStringList>
#include "AISummaryConfigDialog.h"
#include "AIVisionAnalyzer.h"

/**
 * 实时AI视觉分析器 - 处理实时提取的帧图片
 * 支持队列处理，避免阻塞实时帧提取
 */
class RealTimeAIVisionAnalyzer : public QObject {
    Q_OBJECT
    
public:
    explicit RealTimeAIVisionAnalyzer(QObject *parent = nullptr);
    ~RealTimeAIVisionAnalyzer();
    
    // 设置AI配置
    void setConfig(const AISummaryConfig &config);
    
    // 添加新帧进行分析
    void addFrameForAnalysis(const QString &framePath, double timestamp);
    
    // 开始实时分析
    void startRealTimeAnalysis();
    
    // 停止实时分析并生成最终总结
    void stopAndGenerateFinalSummary();
    
    // 是否正在进行实时分析
    bool isRealTimeAnalyzing() const;
    
    // 取消所有分析
    void cancelAnalysis();

signals:
    // 实时帧分析完成
    void realTimeFrameAnalyzed(const QString &framePath, const QString &analysis, double timestamp);
    
    // 录制后处理进度（用于最终总结生成）
    void postRecordingProgress(int current, int total);
    
    // 最终总结生成完成
    void finalSummaryGenerated(bool success, const QString &summary, const QString &message);

private slots:
    // 处理队列中的下一帧
    void processNextFrame();

private:
    QString analyzeImageWithAI(const QString &imagePath);
    void generateFinalSummary();
    bool isConfigValid() const;
    
    AISummaryConfig config;
    
    // 实时分析队列
    struct FrameAnalysisTask {
        QString framePath;
        double timestamp;
        QString analysis; // 分析结果（完成后填写）
        bool processed;
    };
    
    QQueue<FrameAnalysisTask> frameQueue;
    QMutex queueMutex;
    QTimer *processTimer;
    
    bool realTimeAnalyzing;
    bool processingQueue;
    
    // 已完成的分析结果
    QList<FrameAnalysisTask> completedAnalyses;
    
    // 用于最终总结生成的AI分析器
    AIVisionAnalyzer *summaryAnalyzer;
};

#endif // REALTIMEAIVISIONANALYZER_H
