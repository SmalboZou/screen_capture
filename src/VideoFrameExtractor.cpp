#include "VideoFrameExtractor.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>

VideoFrameExtractor::VideoFrameExtractor(QObject *parent)
    : QObject(parent)
    , ffmpegProcess(nullptr)
    , tempDir(nullptr)
    , targetFrameRate(30)
    , isExtracting(false)
{
    setupTempDirectory();
}

VideoFrameExtractor::~VideoFrameExtractor() {
    cleanup();
}

void VideoFrameExtractor::setupTempDirectory() {
    if (tempDir) {
        delete tempDir;
    }
    
    tempDir = new QTemporaryDir();
    if (!tempDir->isValid()) {
        qWarning() << "Failed to create temporary directory";
    }
}

void VideoFrameExtractor::extractFrames(const QString &videoPath, int frameRate) {
    if (isExtracting) {
        emit frameExtractionFinished(false, "正在提取其他视频的帧，请等待完成");
        return;
    }
    
    if (!QFileInfo::exists(videoPath)) {
        emit frameExtractionFinished(false, "视频文件不存在");
        return;
    }
    
    QString ffmpegPath = findFFmpegPath();
    if (ffmpegPath.isEmpty()) {
        emit frameExtractionFinished(false, "未找到FFmpeg，请确保已安装FFmpeg");
        return;
    }
    
    if (!tempDir || !tempDir->isValid()) {
        setupTempDirectory();
        if (!tempDir || !tempDir->isValid()) {
            emit frameExtractionFinished(false, "无法创建临时目录");
            return;
        }
    }
    
    // 清理之前的结果
    extractedFrames.clear();
    currentVideoPath = videoPath;
    targetFrameRate = frameRate;
    isExtracting = true;
    
    // 计算帧提取间隔
    // 如果是30fps视频，每15帧提取一张（0.5秒间隔）
    // 如果是60fps视频，每30帧提取一张（0.5秒间隔）
    // 如果是24fps视频，每12帧提取一张（0.5秒间隔）
    double interval = 0.5; // 每0.5秒提取一帧
    
    QString outputPattern = tempDir->path() + "/frame_%04d.jpg";
    
    // 创建FFmpeg进程
    if (ffmpegProcess) {
        ffmpegProcess->deleteLater();
    }
    ffmpegProcess = new QProcess(this);
    
    connect(ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VideoFrameExtractor::onProcessFinished);
    connect(ffmpegProcess, &QProcess::errorOccurred,
            this, &VideoFrameExtractor::onProcessError);
    
    // 构建FFmpeg命令
    QStringList arguments;
    arguments << "-i" << videoPath
             << "-vf" << QString("fps=1/%1").arg(interval) // 每0.5秒提取一帧
             << "-q:v" << "2" // 高质量JPEG
             << "-f" << "image2"
             << outputPattern;
    
    qDebug() << "Starting FFmpeg with command:" << ffmpegPath << arguments.join(" ");
    
    ffmpegProcess->start(ffmpegPath, arguments);
    
    if (!ffmpegProcess->waitForStarted()) {
        isExtracting = false;
        emit frameExtractionFinished(false, "FFmpeg启动失败");
    }
}

void VideoFrameExtractor::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    isExtracting = false;
    
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        QString errorOutput = ffmpegProcess->readAllStandardError();
        emit frameExtractionFinished(false, QString("FFmpeg处理失败: %1").arg(errorOutput));
        return;
    }
    
    // 扫描生成的图片文件
    QDir dir(tempDir->path());
    QStringList filters;
    filters << "frame_*.jpg";
    QStringList frameFiles = dir.entryList(filters, QDir::Files, QDir::Name);
    
    extractedFrames.clear();
    for (const QString &fileName : frameFiles) {
        extractedFrames.append(dir.absoluteFilePath(fileName));
    }
    
    if (extractedFrames.isEmpty()) {
        emit frameExtractionFinished(false, "未能提取到任何视频帧");
    } else {
        emit frameExtractionFinished(true, QString("成功提取 %1 帧图片").arg(extractedFrames.size()));
    }
}

void VideoFrameExtractor::onProcessError(QProcess::ProcessError error) {
    isExtracting = false;
    
    QString errorMessage;
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = "FFmpeg启动失败，请检查是否已正确安装";
        break;
    case QProcess::Crashed:
        errorMessage = "FFmpeg进程崩溃";
        break;
    case QProcess::Timedout:
        errorMessage = "FFmpeg进程超时";
        break;
    case QProcess::WriteError:
        errorMessage = "FFmpeg写入错误";
        break;
    case QProcess::ReadError:
        errorMessage = "FFmpeg读取错误";
        break;
    default:
        errorMessage = "FFmpeg未知错误";
        break;
    }
    
    emit frameExtractionFinished(false, errorMessage);
}

QString VideoFrameExtractor::findFFmpegPath() const {
    // 常见的FFmpeg安装路径
    QStringList possiblePaths = {
        "ffmpeg", // 系统PATH中
        "C:/ffmpeg/bin/ffmpeg.exe",
        "C:/Program Files/ffmpeg/bin/ffmpeg.exe",
        "C:/Program Files (x86)/ffmpeg/bin/ffmpeg.exe",
        QCoreApplication::applicationDirPath() + "/ffmpeg.exe",
        QCoreApplication::applicationDirPath() + "/tools/ffmpeg.exe"
    };
    
    for (const QString &path : possiblePaths) {
        QProcess testProcess;
        testProcess.start(path, QStringList() << "-version");
        if (testProcess.waitForFinished(3000) && testProcess.exitCode() == 0) {
            return path;
        }
    }
    
    return QString(); // 未找到
}

QStringList VideoFrameExtractor::getExtractedFrames() const {
    return extractedFrames;
}

void VideoFrameExtractor::cleanup() {
    if (ffmpegProcess && ffmpegProcess->state() != QProcess::NotRunning) {
        ffmpegProcess->kill();
        ffmpegProcess->waitForFinished(3000);
    }
    
    extractedFrames.clear();
    
    if (tempDir) {
        delete tempDir;
        tempDir = nullptr;
    }
}

QString VideoFrameExtractor::getTempDir() const {
    return tempDir ? tempDir->path() : QString();
}
