// RecordingService.h
#pragma once

#include <QObject>
#include <memory>
#include "DataTypes.h"

// 前向声明
class ILocalCapture;
class IAudioCapture;
class LocalFileWriter;

class RecordingService : public QObject {
    Q_OBJECT

public:
    explicit RecordingService(QObject *parent = nullptr);
    ~RecordingService();
    
    bool initialize();
    
    void startRecording(const QString& outputPath = QString());
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    
    void setCaptureRegion(int x, int y, int width, int height);
    void setFrameRate(int fps);
    void setQuality(VideoQuality quality);
    void setAudioSampleRate(int sampleRate);
    void setAudioChannels(int channels);
    
    bool isRecordingActive() const;
    qint64 getRecordingDuration() const;

signals:
    void recordingStarted(const QString& fileName);
    void recordingStopped(qint64 duration);
    void recordingPaused();
    void recordingResumed();
    void errorOccurred(const QString& error);

private:
    std::unique_ptr<ILocalCapture> videoCapture;
    std::unique_ptr<IAudioCapture> audioCapture;
    std::unique_ptr<LocalFileWriter> fileWriter;
    
    bool isRecording;
    qint64 recordStartTime;
};