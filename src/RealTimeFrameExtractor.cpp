#include "RealTimeFrameExtractor.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

const double RealTimeFrameExtractor::SHORT_INTERVAL_SECONDS = 2.0;

RealTimeFrameExtractor::RealTimeFrameExtractor(QObject *parent)
    : QObject(parent)
    , extractionTimer(new QTimer(this))
    , tempDir(nullptr)
    , extracting(false)
    , frameCounter(0)
    , recordingStartTime(0)
{
    setupTempDirectory();
    
    // 连接定时器
    connect(extractionTimer, &QTimer::timeout, this, &RealTimeFrameExtractor::extractCurrentFrame);
    
    // 设置初始间隔为短间隔（2.0秒）
    extractionTimer->setInterval(SHORT_INTERVAL_SECONDS * 1000);
}

RealTimeFrameExtractor::~RealTimeFrameExtractor() {
    stopExtraction();
    if (tempDir) {
        delete tempDir;
    }
}

void RealTimeFrameExtractor::setupTempDirectory() {
    if (tempDir) {
        delete tempDir;
    }
    
    tempDir = new QTemporaryDir();
    if (!tempDir->isValid()) {
        qWarning() << "Failed to create temporary directory for real-time frame extraction";
    }
}

void RealTimeFrameExtractor::startExtraction(const QString &outputDir) {
    if (extracting) {
        qWarning() << "Real-time frame extraction already in progress";
        return;
    }
    
    if (!tempDir || !tempDir->isValid()) {
        setupTempDirectory();
        if (!tempDir || !tempDir->isValid()) {
            emit extractionError("无法创建临时目录");
            return;
        }
    }
    
    // 检查FFmpeg是否可用
    QString ffmpegPath = findFFmpegPath();
    if (ffmpegPath.isEmpty()) {
        emit extractionError("未找到FFmpeg，请确保已安装FFmpeg");
        return;
    }
    
    outputDirectory = outputDir;
    frameCounter = 0;
    extracting = true;
    
    qDebug() << "开始实时帧提取，输出目录:" << outputDir;
    
    // 立即提取第一帧
    extractCurrentFrame();
    
    // 启动定时器
    extractionTimer->start();
}

void RealTimeFrameExtractor::stopExtraction() {
    if (!extracting) {
        return;
    }
    
    extractionTimer->stop();
    extracting = false;
    frameCounter = 0;
    
    qDebug() << "停止实时帧提取";
}

bool RealTimeFrameExtractor::isExtracting() const {
    return extracting;
}

void RealTimeFrameExtractor::setRecordingStartTime(qint64 startTime) {
    recordingStartTime = startTime;
    updateExtractionInterval();
}

void RealTimeFrameExtractor::extractCurrentFrame() {
    if (!extracting) {
        return;
    }
    
    // 更新提取间隔（根据录制时长智能调整）
    updateExtractionInterval();
    
    QString ffmpegPath = findFFmpegPath();
    if (ffmpegPath.isEmpty()) {
        emit extractionError("FFmpeg不可用");
        return;
    }
    
    frameCounter++;
    double currentTimestamp = (QDateTime::currentMSecsSinceEpoch() - recordingStartTime) / 1000.0;
    
    // 生成输出文件名
    QString frameName = QString("realtime_frame_%1_%2.jpg")
                       .arg(frameCounter, 4, 10, QChar('0'))
                       .arg(qint64(currentTimestamp * 1000));
    QString outputPath = outputDirectory + "/" + frameName;
    
    // 创建FFmpeg进程进行屏幕截图
    QProcess *ffmpegProcess = new QProcess(this);
    
    // 连接完成信号
    connect(ffmpegProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, ffmpegProcess, outputPath, currentTimestamp](int exitCode, QProcess::ExitStatus exitStatus) {
        ffmpegProcess->deleteLater();
        
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            if (QFileInfo::exists(outputPath)) {
                qDebug() << QString("实时提取帧成功: %1 (时间戳: %2s)").arg(outputPath).arg(currentTimestamp, 0, 'f', 1);
                emit frameExtracted(outputPath, currentTimestamp);
            } else {
                qWarning() << "FFmpeg执行成功但文件未生成:" << outputPath;
            }
        } else {
            QString errorOutput = ffmpegProcess->readAllStandardError();
            qWarning() << "实时帧提取失败:" << errorOutput;
            // 注意：实时提取失败不应停止整个过程，只记录警告
        }
    });
    
    connect(ffmpegProcess, &QProcess::errorOccurred,
            [this, ffmpegProcess](QProcess::ProcessError error) {
        ffmpegProcess->deleteLater();
        qWarning() << "FFmpeg进程错误:" << error;
        // 实时提取的单个失败不应停止整个流程
    });
    
    // 构建FFmpeg命令 - 截取当前屏幕
    QStringList arguments;
    
#ifdef Q_OS_WIN
    // Windows: 使用gdigrab捕获桌面
    arguments << "-f" << "gdigrab"
             << "-i" << "desktop"
             << "-vframes" << "1"  // 只捕获一帧
             << "-q:v" << "2"      // 高质量JPEG
             << "-y"               // 覆盖已存在文件
             << outputPath;
#elif defined(Q_OS_MACOS)
    // macOS: 使用avfoundation捕获屏幕
    arguments << "-f" << "avfoundation"
             << "-i" << "1"        // 屏幕输入
             << "-vframes" << "1"  // 只捕获一帧  
             << "-q:v" << "2"      // 高质量JPEG
             << "-y"               // 覆盖已存在文件
             << outputPath;
#else
    // Linux: 使用x11grab捕获屏幕
    arguments << "-f" << "x11grab"
             << "-i" << ":0.0"     // 显示器
             << "-vframes" << "1"  // 只捕获一帧
             << "-q:v" << "2"      // 高质量JPEG  
             << "-y"               // 覆盖已存在文件
             << outputPath;
#endif
    
    // 启动FFmpeg进程
    ffmpegProcess->start(ffmpegPath, arguments);
}

void RealTimeFrameExtractor::updateExtractionInterval() {
    if (recordingStartTime == 0) {
        return; // 还未设置录制开始时间
    }
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 recordingDuration = currentTime - recordingStartTime;
    double recordingSeconds = recordingDuration / 1000.0;
    
    // 智能间隔调整逻辑
    if (recordingSeconds >= INTERVAL_SWITCH_THRESHOLD) {
        // 录制超过10秒，使用长间隔（10秒）
        int newInterval = LONG_INTERVAL_SECONDS * 1000;
        if (extractionTimer->interval() != newInterval) {
            extractionTimer->setInterval(newInterval);
            qDebug() << QString("切换到长间隔模式: %1秒 (录制时长: %2秒)")
                        .arg(LONG_INTERVAL_SECONDS)
                        .arg(recordingSeconds, 0, 'f', 1);
        }
    } else {
        // 录制不足10秒，使用短间隔（2.0秒）
        int newInterval = SHORT_INTERVAL_SECONDS * 1000;
        if (extractionTimer->interval() != newInterval) {
            extractionTimer->setInterval(newInterval);
            qDebug() << QString("使用短间隔模式: %1秒 (录制时长: %2秒)")
                        .arg(SHORT_INTERVAL_SECONDS)
                        .arg(recordingSeconds, 0, 'f', 1);
        }
    }
}

QString RealTimeFrameExtractor::findFFmpegPath() const {
    // 查找FFmpeg可执行文件路径
    QStringList possiblePaths;
    
#ifdef Q_OS_WIN
    possiblePaths << "ffmpeg.exe"
                  << "C:/ffmpeg/bin/ffmpeg.exe"
                  << QCoreApplication::applicationDirPath() + "/ffmpeg.exe"
                  << QCoreApplication::applicationDirPath() + "/bin/ffmpeg.exe";
#else
    possiblePaths << "ffmpeg"
                  << "/usr/bin/ffmpeg"
                  << "/usr/local/bin/ffmpeg"
                  << "/opt/homebrew/bin/ffmpeg"
                  << QCoreApplication::applicationDirPath() + "/ffmpeg";
#endif
    
    for (const QString &path : possiblePaths) {
        QFileInfo fileInfo(path);
        if (fileInfo.exists() && fileInfo.isExecutable()) {
            return path;
        }
        
        // 尝试在PATH中查找
        QProcess process;
        process.start(path, QStringList() << "-version");
        if (process.waitForFinished(3000) && process.exitCode() == 0) {
            return path;
        }
    }
    
    return QString();
}
