#ifndef VIDEOFRAMEEXTRACTOR_H
#define VIDEOFRAMEEXTRACTOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QTemporaryDir>

class VideoFrameExtractor : public QObject {
    Q_OBJECT
    
public:
    explicit VideoFrameExtractor(QObject *parent = nullptr);
    ~VideoFrameExtractor();
    
    // 提取视频帧
    void extractFrames(const QString &videoPath, int frameRate = 30);
    
    // 提取视频帧（支持自定义间隔）
    void extractFrames(const QString &videoPath, double intervalSeconds, int frameRate = 30);
    
    // 获取提取的帧图片路径列表
    QStringList getExtractedFrames() const;
    
    // 清理临时文件
    void cleanup();
    
    // 获取临时目录路径
    QString getTempDir() const;

signals:
    void frameExtractionFinished(bool success, const QString &message);
    void frameExtractionProgress(int progress, int total);
    
private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    
private:
    QString findFFmpegPath() const;
    void setupTempDirectory();
    
    QProcess *ffmpegProcess;
    QTemporaryDir *tempDir;
    QStringList extractedFrames;
    QString currentVideoPath;
    int targetFrameRate;
    bool isExtracting;
};

#endif // VIDEOFRAMEEXTRACTOR_H
