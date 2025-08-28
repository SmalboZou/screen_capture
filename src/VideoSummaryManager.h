#ifndef VIDEOSUMMARYMANAGER_H
#define VIDEOSUMMARYMANAGER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>
#include "VideoFrameExtractor.h"
#include "AIVisionAnalyzer.h"
#include "AISummaryConfigDialog.h"

class VideoSummaryManager : public QObject {
    Q_OBJECT
    
public:
    explicit VideoSummaryManager(QObject *parent = nullptr);
    ~VideoSummaryManager();
    
    // 设置AI配置
    void setConfig(const AISummaryConfig &config);
    
    // 开始视频内容总结
    void startVideoSummary(const QString &videoPath, int frameRate = 30);
    
    // 取消当前处理
    void cancelProcessing();
    
    // 获取处理状态
    bool isProcessing() const;

signals:
    void summaryProgress(const QString &status, int percentage);
    void summaryCompleted(bool success, const QString &summary, const QString &message);
    
private slots:
    void onFrameExtractionFinished(bool success, const QString &message);
    void onImageAnalysisProgress(int current, int total);
    void onImageAnalysisFinished(bool success, const QString &message);
    void onFinalSummaryGenerated(bool success, const QString &summary, const QString &message);
    
private:
    void updateProgress(const QString &status, int percentage);
    void finishWithError(const QString &message);
    double calculateSmartInterval(const QString &videoPath);
    QString findFFmpegPath() const;
    
    std::unique_ptr<VideoFrameExtractor> frameExtractor;
    std::unique_ptr<AIVisionAnalyzer> visionAnalyzer;
    
    AISummaryConfig config;
    QString currentVideoPath;
    bool processing;
    
    // 进度跟踪
    enum ProcessState {
        StateIdle,
        StateExtractingFrames,
        StateAnalyzingImages,
        StateGeneratingSummary
    };
    
    ProcessState currentState;
    int totalFrames;
    int analyzedFrames;
};

#endif // VIDEOSUMMARYMANAGER_H
