#ifndef AIVISIONANALYZER_H
#define AIVISIONANALYZER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringList>
#include <QQueue>
#include <QTimer>
#include <QMutex>
#include "AISummaryConfigDialog.h"

struct FrameAnalysisResult {
    QString imagePath;
    QString description;
    bool success;
    QString errorMessage;
    int frameIndex;
    
    FrameAnalysisResult() : success(false), frameIndex(-1) {}
};

class AIVisionAnalyzer : public QObject {
    Q_OBJECT
    
public:
    explicit AIVisionAnalyzer(QObject *parent = nullptr);
    ~AIVisionAnalyzer();
    
    // 设置AI配置
    void setConfig(const AISummaryConfig &config);
    
    // 分析图片列表
    void analyzeImages(const QStringList &imagePaths);
    
    // 获取分析结果
    QList<FrameAnalysisResult> getResults() const;
    
    // 取消当前分析
    void cancelAnalysis();
    
    // 获取最终总结
    void generateFinalSummary(const QStringList &descriptions);

signals:
    void imageAnalysisProgress(int current, int total);
    void imageAnalysisFinished(bool success, const QString &message);
    void finalSummaryGenerated(bool success, const QString &summary, const QString &message);
    
private slots:
    void processNextImage();
    void onImageAnalysisReply();
    void onSummaryReply();
    void onBatchSummaryReply();
    void onNetworkTimeout();
    
private:
    QString encodeImageToBase64(const QString &imagePath) const;
    QJsonObject createOpenAIRequest(const QString &base64Image) const;
    QJsonObject createSiliconFlowRequest(const QString &base64Image) const;
    QJsonObject createGLMRequest(const QString &base64Image) const;
    QJsonObject createKimiRequest(const QString &base64Image) const;
    QNetworkRequest createNetworkRequest(const QString &endpoint) const;
    QString parseOpenAIResponse(const QJsonObject &response) const;
    QString parseSiliconFlowResponse(const QJsonObject &response) const;
    QString parseGLMResponse(const QJsonObject &response) const;
    QString parseKimiResponse(const QJsonObject &response) const;
    
    // 分批处理方法
    void processBatch(int batchIndex, int batchSize);
    void generateFinalMergedSummary();
    void generateDirectSummary(const QStringList &descriptions);
    
    QNetworkAccessManager *networkManager;
    AISummaryConfig config;
    
    // 图片分析队列
    QQueue<QString> imageQueue;
    QList<FrameAnalysisResult> analysisResults;
    QNetworkReply *currentReply;
    QTimer *timeoutTimer;
    
    // 最终总结
    QStringList frameDescriptions;
    
    // 分批总结
    QStringList batchSummaries;
    int currentBatchIndex;
    int totalBatches;
    
    // 状态管理
    bool isAnalyzing;
    int currentImageIndex;
    int totalImages;
    QMutex resultsMutex;
    
    // 请求限制
    static const int MAX_CONCURRENT_REQUESTS = 1; // 避免API限制
    static const int REQUEST_TIMEOUT_MS = 180000; // 180秒超时 (thinking模型需要更长时间)
    static const int RETRY_DELAY_MS = 2000; // 重试延迟
};

#endif // AIVISIONANALYZER_H
