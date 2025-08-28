#ifndef REALTIMEFRAMEEXTRACTOR_H
#define REALTIMEFRAMEEXTRACTOR_H

#include <QObject>
#include <QTimer>
#include <QTemporaryDir>
#include <QProcess>
#include <QDateTime>
#include <QString>

/**
 * 实时帧提取器 - 在录制过程中定期提取当前屏幕帧
 * 支持智能间隔调整：录制时长>=10s时使用10s间隔，否则使用0.5s间隔
 */
class RealTimeFrameExtractor : public QObject {
    Q_OBJECT
    
public:
    explicit RealTimeFrameExtractor(QObject *parent = nullptr);
    ~RealTimeFrameExtractor();
    
    // 开始实时帧提取
    void startExtraction(const QString &outputDir);
    
    // 停止实时帧提取
    void stopExtraction();
    
    // 是否正在提取
    bool isExtracting() const;
    
    // 设置录制开始时间（用于计算智能间隔）
    void setRecordingStartTime(qint64 startTime);

signals:
    // 新帧已提取 - 发送帧文件路径和时间戳
    void frameExtracted(const QString &framePath, double timestamp);
    
    // 提取错误
    void extractionError(const QString &error);

private slots:
    // 定时提取当前帧
    void extractCurrentFrame();

private:
    void setupTempDirectory();
    QString findFFmpegPath() const;
    void updateExtractionInterval();
    
    QTimer *extractionTimer;
    QTemporaryDir *tempDir;
    QString outputDirectory;
    bool extracting;
    int frameCounter;
    qint64 recordingStartTime;
    
    // 智能间隔参数
    static const int LONG_INTERVAL_SECONDS = 10;  // 长间隔：10秒
    static const double SHORT_INTERVAL_SECONDS;   // 短间隔：0.5秒
    static const int INTERVAL_SWITCH_THRESHOLD = 10; // 10秒阈值
};

#endif // REALTIMEFRAMEEXTRACTOR_H
