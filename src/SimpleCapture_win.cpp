// SimpleCapture_win.cpp
// Windows 真实录屏实现：通过 FFmpeg (gdigrab) 进行屏幕捕获与 H.264 编码
#include "SimpleCapture.h"
#include <iostream>
#include <memory>

#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QStringList>

class WindowsSimpleCapture : public SimpleCapture {
public:
    WindowsSimpleCapture() = default;
    ~WindowsSimpleCapture() override {
        if (ffmpeg) {
            stopCapture();
            delete ffmpeg;
            ffmpeg = nullptr;
        }
    }

    bool init() override {
        // 优先从应用目录查找 ffmpeg.exe，其次使用 PATH 中的 ffmpeg
        ffmpegPath = QDir(QCoreApplication::applicationDirPath()).filePath("ffmpeg.exe");
        if (!QFileInfo::exists(ffmpegPath)) {
            ffmpegPath = "ffmpeg"; // 走 PATH
        }
        // 验证 ffmpeg 可用
        int code = QProcess::execute(ffmpegPath, {"-version"});
        if (code != 0) {
            std::cerr << "无法找到可用的 ffmpeg，可执行文件应放在程序目录或加入 PATH" << std::endl;
            return false;
        }
        return true;
    }

    bool startCapture(const std::string& outputPath) override {
        if (isCapturing()) {
            std::cerr << "已在录制中" << std::endl;
            return false;
        }

        // 准备参数
        QStringList args;
        args << "-y";
        args << "-f" << "gdigrab";
        if (captureRegionSet) {
            args << "-offset_x" << QString::number(regionX);
            args << "-offset_y" << QString::number(regionY);
            args << "-video_size" << QString::number(regionW) + "x" + QString::number(regionH);
        }
        // 若未设置区域，gdigrab 默认为主屏整体
        args << "-framerate" << QString::number(frameRate > 0 ? frameRate : 30);
        args << "-i" << "desktop";
        // 编码参数：H.264 + yuv420p 保证广泛兼容
        args << "-pix_fmt" << "yuv420p";
        args << "-c:v" << "libx264";
        args << "-preset" << "veryfast";
        args << "-crf" << "23";
        args << QString::fromStdString(outputPath);

        if (!ffmpeg) ffmpeg = new QProcess();
        ffmpeg->setProcessChannelMode(QProcess::MergedChannels);
        ffmpeg->setProgram(ffmpegPath);
        ffmpeg->setArguments(args);
        ffmpeg->setWorkingDirectory(QCoreApplication::applicationDirPath());
        ffmpeg->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        ffmpeg->start();
        // 打开标准输入以便优雅停止（发送 'q'）
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

    void setCaptureRegion(int x, int y, int width, int height) override {
        regionX = x; regionY = y; regionW = width; regionH = height; captureRegionSet = true;
    }

private:
    QProcess* ffmpeg = nullptr;
    QString ffmpegPath;
    bool capturing = false;
    int frameRate = 30;
    int regionX = 0, regionY = 0, regionW = 0, regionH = 0; 
    bool captureRegionSet = false;
};

std::unique_ptr<SimpleCapture> createSimpleCapture() {
#ifdef _WIN32
    return std::make_unique<WindowsSimpleCapture>();
#else
    static_assert(false, "Wrong platform for WindowsSimpleCapture");
    return nullptr;
#endif
}
