// MainWindow.cpp
#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QStandardPaths>
#include <QDateTime>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDir>
#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isRecording(false)
    , recordStartTime(0)
{
    setWindowTitle("AIcp 屏幕录制");
    setMinimumSize(500, 300);
    
    // 创建视频捕获
    videoCapture = createSimpleCapture();
    if (!videoCapture->init()) {
        QMessageBox::critical(this, "错误", "视频捕获初始化失败");
    }
    
    setupUI();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // 录制控制区域
    QGroupBox *controlGroup = new QGroupBox("录制控制");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    
    // 录制按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    startButton = new QPushButton("开始录制");
    stopButton = new QPushButton("停止");
    
    startButton->setStyleSheet("background-color: green; color: white; font-weight: bold;");
    stopButton->setStyleSheet("background-color: red; color: white; font-weight: bold;");
    stopButton->setEnabled(false);
    
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    
    // 输出路径
    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel("输出路径:"));
    outputPathEdit = new QLineEdit();
    browseButton = new QPushButton("浏览...");
    pathLayout->addWidget(outputPathEdit);
    pathLayout->addWidget(browseButton);
    
    // 帧率设置
    QHBoxLayout *fpsLayout = new QHBoxLayout();
    fpsLayout->addWidget(new QLabel("帧率:"));
    fpsCombo = new QComboBox();
    fpsCombo->addItems({"30 FPS", "60 FPS", "24 FPS"});
    fpsLayout->addWidget(fpsCombo);

    // 屏幕选择
    QHBoxLayout *screenLayout = new QHBoxLayout();
    screenLayout->addWidget(new QLabel("屏幕:"));
    screenCombo = new QComboBox();
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        const auto s = screens[i];
        QString name = s->name().isEmpty() ? QString("屏幕 %1").arg(i) : s->name();
        QRect g = s->geometry();
        screenCombo->addItem(QString("%1 (%2x%3 @%4,%5)").arg(name).arg(g.width()).arg(g.height()).arg(g.x()).arg(g.y()), i);
    }
    screenLayout->addWidget(screenCombo);

    // 自动最小化选项
    autoMinimizeCheckBox = new QCheckBox("录制时自动最小化窗口");
    autoMinimizeCheckBox->setChecked(true); // 默认开启

    // 状态显示
    statusLabel = new QLabel("就绪");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-size: 16px; padding: 10px;");
    
    timeLabel = new QLabel("00:00:00");
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setStyleSheet("font-size: 24px; font-weight: bold;");
    
    controlLayout->addLayout(buttonLayout);
    controlLayout->addLayout(pathLayout);
    controlLayout->addLayout(fpsLayout);
    controlLayout->addLayout(screenLayout); // 加入屏幕选择
    controlLayout->addWidget(autoMinimizeCheckBox); // 加入自动最小化选项
    controlLayout->addWidget(statusLabel);
    controlLayout->addWidget(timeLabel);
    
    mainLayout->addWidget(controlGroup);
    
    setCentralWidget(centralWidget);
    
    // 连接信号
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartRecording);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopRecording);
    connect(browseButton, &QPushButton::clicked, this, &MainWindow::onBrowsePath);
    
    // 定时器
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateRecordingTime);
}

void MainWindow::onStartRecording() {
    if (isRecording) return;
    
    QString outputPath = outputPathEdit->text();
    if (outputPath.isEmpty()) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        outputPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + 
                    "/AIcp_" + timestamp + ".mov";
    }
    
    // 确保目录存在
    QDir().mkpath(QFileInfo(outputPath).absolutePath());
    
    // 设置帧率
    int fps = fpsCombo->currentText().split(" ")[0].toInt();
    videoCapture->setFrameRate(fps);

    // 使用所选屏幕的区域作为捕获区域
    int idx = screenCombo->currentData().toInt();
    const auto screens = QGuiApplication::screens();
    if (idx >= 0 && idx < screens.size()) {
        QRect g = screens[idx]->geometry();
        videoCapture->setCaptureRegion(g.x(), g.y(), g.width(), g.height());
    }
    
    // 禁用开始按钮
    startButton->setEnabled(false);
    
    // 根据用户设置决定是否最小化
    if (autoMinimizeCheckBox->isChecked()) {
        // 显示准备状态
        statusLabel->setText("准备录制中，窗口将在2秒后最小化...");
        statusLabel->setStyleSheet("font-size: 16px; padding: 10px; color: orange;");
        
        // 使用定时器延时最小化窗口并开始录制
        QTimer::singleShot(2000, this, [this, outputPath]() {
            // 最小化窗口
            this->showMinimized();
            
            // 再等待1秒让窗口完全最小化
            QTimer::singleShot(1000, this, [this, outputPath]() {
                this->startRecordingInternal(outputPath);
            });
        });
    } else {
        // 直接开始录制，不最小化
        statusLabel->setText("开始录制...");
        statusLabel->setStyleSheet("font-size: 16px; padding: 10px; color: orange;");
        
        // 稍微延时一下以显示状态
        QTimer::singleShot(500, this, [this, outputPath]() {
            this->startRecordingInternal(outputPath);
        });
    }
}

void MainWindow::onStopRecording() {
    if (!isRecording) return;
    
    videoCapture->stopCapture();
    
    isRecording = false;
    qint64 duration = QDateTime::currentMSecsSinceEpoch() - recordStartTime;
    
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    statusLabel->setText("就绪");
    statusLabel->setStyleSheet("font-size: 16px; padding: 10px;");
    
    updateTimer->stop();
    
    // 恢复窗口显示
    this->showNormal();
    this->raise();
    this->activateWindow();
    
    QString msg = QString("录制完成！\n文件: %1\n时长: %2")
        .arg(outputPathEdit->text())
        .arg(formatDuration(duration));
    
    QMessageBox::information(this, "录制完成", msg);
}

void MainWindow::startRecordingInternal(const QString& outputPath) {
    // 开始录制
    if (videoCapture->startCapture(outputPath.toStdString())) {
        isRecording = true;
        recordStartTime = QDateTime::currentMSecsSinceEpoch();
        
        stopButton->setEnabled(true);
        statusLabel->setText("录制中...");
        statusLabel->setStyleSheet("font-size: 16px; padding: 10px; color: red;");
        
        outputPathEdit->setText(outputPath);
        
        updateTimer->start(1000);
    } else {
        // 如果录制失败，恢复窗口和按钮状态
        startButton->setEnabled(true);
        statusLabel->setText("就绪");
        statusLabel->setStyleSheet("font-size: 16px; padding: 10px;");
        if (autoMinimizeCheckBox->isChecked()) {
            this->showNormal();
        }
        QMessageBox::critical(this, "错误", "录制启动失败");
    }
}

void MainWindow::onBrowsePath() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "选择输出文件",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        "视频文件 (*.mov *.mp4)"
    );
    
    if (!fileName.isEmpty()) {
        outputPathEdit->setText(fileName);
    }
}

void MainWindow::updateRecordingTime() {
    if (isRecording) {
        qint64 duration = QDateTime::currentMSecsSinceEpoch() - recordStartTime;
        timeLabel->setText(formatDuration(duration));
    }
}

QString MainWindow::formatDuration(qint64 ms) {
    int seconds = ms / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}