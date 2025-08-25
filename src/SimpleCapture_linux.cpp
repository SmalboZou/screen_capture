// SimpleCapture_linux.cpp
// Linux 屏幕录制实现：通过 FFmpeg (x11grab) 进行屏幕捕获与 H.264 编码
// 注意：Wayland 下 x11grab 可能失效，可后续扩展 pipewire 支持。

#include "SimpleCapture.h"
#include <iostream>
#include <memory>

#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QStringList>
#include <QProcessEnvironment>

class LinuxSimpleCapture : public SimpleCapture {
public:
    LinuxSimpleCapture() = default;
    ~LinuxSimpleCapture() override {
        if (ffmpeg) {
            stopCapture();
            delete ffmpeg;
            ffmpeg = nullptr;
        }
    }

    bool init() override {
        ffmpegPath = QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg");
        if (!QFileInfo::exists(ffmpegPath)) {
            ffmpegPath = "ffmpeg";
        }
        int code = QProcess::execute(ffmpegPath, {"-version"});
        if (code != 0) {
            std::cerr << "无法找到可用的 ffmpeg，请安装 (sudo apt install ffmpeg) 或放置于程序目录" << std::endl;
            return false;
        }
        if (!qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
            std::cerr << "警告：Wayland 会话中当前实现使用 x11grab，可能无法工作。" << std::endl;
        }
        return true;
    }

    bool startCapture(const std::string& outputPath) override {
        if (isCapturing()) {
            std::cerr << "已在录制中" << std::endl;
            return false;
        }
        QString displayEnv = qEnvironmentVariableIsEmpty("DISPLAY") ? QString(":0.0") : qgetenv("DISPLAY");
        QStringList args;
        args << "-y" << "-f" << "x11grab" << "-framerate" << QString::number(frameRate > 0 ? frameRate : 30);
        if (captureRegionSet) {
            args << "-video_size" << QString::number(regionW) + "x" + QString::number(regionH);
            args << "-i" << QString("%1+%2,%3").arg(displayEnv).arg(regionX).arg(regionY);
            std::cout << "FFmpeg 参数: 区域=" << regionX << "," << regionY << " 尺寸=" << regionW << "x" << regionH << std::endl;
        } else {
            args << "-i" << displayEnv;
            std::cout << "使用默认全屏捕获 display=" << displayEnv.toStdString() << std::endl;
        }
        args << "-pix_fmt" << "yuv420p" << "-c:v" << "libx264" << "-preset" << "veryfast" << "-crf" << "23";
        args << QString::fromStdString(outputPath);
        if (!ffmpeg) ffmpeg = new QProcess();
        ffmpeg->setProcessChannelMode(QProcess::MergedChannels);
        ffmpeg->setProgram(ffmpegPath);
        ffmpeg->setArguments(args);
        ffmpeg->setWorkingDirectory(QCoreApplication::applicationDirPath());
        ffmpeg->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        ffmpeg->start();
        ffmpeg->waitForStarted(3000);
        if (ffmpeg->state() != QProcess::Running) {
            std::cerr << "启动 ffmpeg 失败" << std::endl;
            return false;
        }
        capturing = true;
        return true;
    }

    bool stopCapture() override {
        if (!isCapturing()) return false;
        bool stopped = false;
        if (ffmpeg && ffmpeg->state() == QProcess::Running) {
            if (ffmpeg->isWritable()) {
                QByteArray quitCmd("q\n");
                ffmpeg->write(quitCmd);
                ffmpeg->waitForBytesWritten(500);
            }
            stopped = ffmpeg->waitForFinished(3000);
            if (!stopped) {
                ffmpeg->terminate();
                stopped = ffmpeg->waitForFinished(2000);
            }
            if (!stopped) {
                ffmpeg->kill();
                ffmpeg->waitForFinished(1000);
            }
        }
        capturing = false;
        return true;
    }

    bool isCapturing() const override { return capturing; }
    void setFrameRate(int fps) override { frameRate = fps; }
    void setCaptureRegion(int x, int y, int width, int height) override { regionX = x; regionY = y; regionW = width; regionH = height; captureRegionSet = true; }

private:
    QProcess* ffmpeg = nullptr;
    QString ffmpegPath;
    bool capturing = false;
    int frameRate = 30;
    int regionX = 0, regionY = 0, regionW = 0, regionH = 0;
    bool captureRegionSet = false;
};

std::unique_ptr<SimpleCapture> createSimpleCapture() {
#if defined(__linux__) && !defined(_WIN32) && !defined(__APPLE__)
    return std::make_unique<LinuxSimpleCapture>();
#else
    static_assert(false, "Wrong platform for LinuxSimpleCapture");
    return nullptr;
#endif
}
