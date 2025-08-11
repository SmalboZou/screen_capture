// RecordingService.cpp
#include "RecordingService.h"
#include "ILocalCapture.h"
#include "IAudioCapture.h"
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <iostream>

RecordingService::RecordingService(QObject *parent) : QObject(parent) {
    // 初始化各个组件
    videoCapture = nullptr;  // 将在initialize中创建
    audioCapture = nullptr;  // 将在initialize中创建
    
    isRecording = false;
    recordStartTime = 0;
}

RecordingService::~RecordingService() = default;

bool RecordingService::initialize() {
    // 创建并初始化视频捕获
    videoCapture = std::make_unique<MacOSCapture>();
    if (!videoCapture || !videoCapture->init()) {
        emit errorOccurred("视频捕获初始化失败");
        return false;
    }
    
    // 创建并初始化音频捕获
    audioCapture = std::make_unique<MacOSCapture>();  // 暂时用视频捕获代替
    // 在实际实现中应该使用音频捕获类
    
    return true;
}

void RecordingService::startRecording(const QString& customPath) {
    if (isRecording) {
        emit errorOccurred("已经在录制中");
        return;
    }
    
    // 生成文件名
    QString fileName;
    if (customPath.isEmpty()) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        fileName = QString("%1/AIcp_%2.mov").arg(defaultPath).arg(timestamp);
    } else {
        fileName = customPath;
    }
    
    // 确保目录存在
    QDir().mkpath(QFileInfo(fileName).absolutePath());
    
    // 开始录制
    if (!videoCapture->startCapture(fileName.toStdString())) {
        emit errorOccurred("视频录制启动失败");
        return;
    }
    
    isRecording = true;
    recordStartTime = QDateTime::currentMSecsSinceEpoch();
    
    emit recordingStarted(fileName);
    std::cout << "开始录制: " << fileName.toStdString() << std::endl;
}

void RecordingService::stopRecording() {
    if (!isRecording) {
        emit errorOccurred("没有在录制");
        return;
    }
    
    // 停止录制
    videoCapture->stopCapture();
    
    isRecording = false;
    qint64 duration = QDateTime::currentMSecsSinceEpoch() - recordStartTime;
    
    emit recordingStopped(duration);
    std::cout << "录制完成，时长: " << duration << "ms" << std::endl;
}

void RecordingService::pauseRecording() {
    // 暂停逻辑
    emit recordingPaused();
}

void RecordingService::resumeRecording() {
    // 恢复逻辑
    emit recordingResumed();
}

void RecordingService::setCaptureRegion(int x, int y, int width, int height) {
    if (videoCapture) {
        videoCapture->setCaptureRegion(x, y, width, height);
    }
}

void RecordingService::setFrameRate(int fps) {
    if (videoCapture) {
        videoCapture->setFrameRate(fps);
    }
}

void RecordingService::setQuality(VideoQuality quality) {
    if (videoCapture) {
        videoCapture->setQuality(quality);
    }
}

void RecordingService::setAudioSampleRate(int sampleRate) {
    // 音频采样率设置
    Q_UNUSED(sampleRate);
}

void RecordingService::setAudioChannels(int channels) {
    // 音频声道数设置
    Q_UNUSED(channels);
}

bool RecordingService::isRecordingActive() const {
    return isRecording;
}

qint64 RecordingService::getRecordingDuration() const {
    if (isRecording) {
        return QDateTime::currentMSecsSinceEpoch() - recordStartTime;
    }
    return 0;
}