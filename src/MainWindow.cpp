// MainWindow.cpp
#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
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
#include <QSpinBox>
#include <QDir>
#include <QScreen>
#include <QGuiApplication>
#include <QSplitter>
#include <QCoreApplication>
#include <QApplication>
#include <QIcon>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isRecording(false)
    , recordStartTime(0)
    , recordEndTime(0)
    , recordingDurationMs(0)
{
    setWindowTitle("AICP");
    setMinimumSize(650, 450);
    resize(700, 500);
    
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
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // 使用分割器来创建更好的布局
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    
    // 左侧：录制控制区域
    QWidget *leftWidget = new QWidget();
    leftWidget->setMinimumWidth(400);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    
    // 录制控制组
    QGroupBox *controlGroup = new QGroupBox("录制控制");
    controlGroup->setStyleSheet("QGroupBox { font-weight: bold; padding-top: 15px; }");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    
    // 录制按钮 - 使用网格布局使按钮更大更醒目
    QGridLayout *buttonGrid = new QGridLayout();
    startButton = new QPushButton();
    stopButton = new QPushButton();
    
    // 设置按钮图标和文本
    startButton->setText("开始录制");
    // 使用资源文件中的图标，如果加载失败则回退到标准图标
    QIcon startIcon(":/icons/control.png");
    if (startIcon.isNull()) {
        startIcon = style()->standardIcon(QStyle::SP_MediaPlay);
    }
    startButton->setIcon(startIcon);
    
    stopButton->setText("停止录制");
    QIcon stopIcon(":/icons/control.png");
    if (stopIcon.isNull()) {
        stopIcon = style()->standardIcon(QStyle::SP_MediaStop);
    }
    stopButton->setIcon(stopIcon);
    
    startButton->setMinimumHeight(50);
    stopButton->setMinimumHeight(50);
    startButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; font-weight: bold; "
        "font-size: 14px; border-radius: 8px; padding: 10px; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:disabled { background-color: #cccccc; }"
    );
    stopButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; font-weight: bold; "
        "font-size: 14px; border-radius: 8px; padding: 10px; }"
        "QPushButton:hover { background-color: #da190b; }"
        "QPushButton:disabled { background-color: #cccccc; }"
    );
    stopButton->setEnabled(false);
    
    buttonGrid->addWidget(startButton, 0, 0);
    buttonGrid->addWidget(stopButton, 0, 1);
    
    controlLayout->addLayout(buttonGrid);
    
    // 录制设置组
    QGroupBox *settingsGroup = new QGroupBox("录制设置");
    settingsGroup->setStyleSheet("QGroupBox { font-weight: bold; padding-top: 15px; }");
    QGridLayout *settingsLayout = new QGridLayout(settingsGroup);
    
    // 输出路径
    settingsLayout->addWidget(new QLabel("输出路径:"), 0, 0);
    outputPathEdit = new QLineEdit();
    outputPathEdit->setPlaceholderText("请选择文件保存路径");
    browseButton = new QPushButton("浏览...");
    QIcon browseIcon(":/icons/path.png");
    if (browseIcon.isNull()) {
        browseIcon = style()->standardIcon(QStyle::SP_DirOpenIcon);
    }
    browseButton->setIcon(browseIcon);
    browseButton->setMaximumWidth(80);
    settingsLayout->addWidget(outputPathEdit, 0, 1);
    settingsLayout->addWidget(browseButton, 0, 2);
    
    // 输出文件名
    settingsLayout->addWidget(new QLabel("文件名:"), 1, 0);
    outputNameEdit = new QLineEdit();
    outputNameEdit->setPlaceholderText("请输入文件名称");
    settingsLayout->addWidget(outputNameEdit, 1, 1, 1, 2);

    // 帧率设置
    settingsLayout->addWidget(new QLabel("帧率:"), 2, 0);
    fpsCombo = new QComboBox();
    fpsCombo->addItems({"30 FPS", "60 FPS", "24 FPS"});
    settingsLayout->addWidget(fpsCombo, 2, 1, 1, 2);

    // 屏幕选择
    settingsLayout->addWidget(new QLabel("屏幕:"), 3, 0);
    screenCombo = new QComboBox();
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        const auto s = screens[i];
        QString name = s->name().isEmpty() ? QString("屏幕 %1").arg(i + 1) : s->name();
        
        // 获取物理分辨率而不是逻辑分辨率
        QRect g = s->geometry();
        qreal devicePixelRatio = s->devicePixelRatio();
        int physicalWidth = g.width() * devicePixelRatio;
        int physicalHeight = g.height() * devicePixelRatio;
        
        screenCombo->addItem(QString("%1 (%2×%3, 缩放: %4x)").arg(name).arg(physicalWidth).arg(physicalHeight).arg(devicePixelRatio), i);
    }
    settingsLayout->addWidget(screenCombo, 3, 1, 1, 2);

    // 定时录制组
    QGroupBox *timerGroup = new QGroupBox("定时录制");
    timerGroup->setStyleSheet("QGroupBox { font-weight: bold; padding-top: 15px; }");
    QVBoxLayout *timerLayout = new QVBoxLayout(timerGroup);
    
    // 定时开关
    timerEnabledCheckBox = new QCheckBox("启用定时录制");
    QIcon timerIcon(":/icons/timer.png");
    if (timerIcon.isNull()) {
        timerIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    timerEnabledCheckBox->setIcon(timerIcon);
    timerLayout->addWidget(timerEnabledCheckBox);
    
    // 时间设置
    QHBoxLayout *timeLayout = new QHBoxLayout();
    timeLayout->addWidget(new QLabel("录制时长:"));
    
    hoursSpinBox = new QSpinBox();
    hoursSpinBox->setRange(0, 23);
    hoursSpinBox->setValue(0);
    hoursSpinBox->setSuffix(" 小时");
    hoursSpinBox->setEnabled(false);
    hoursSpinBox->setMinimumWidth(80);
    
    minutesSpinBox = new QSpinBox();
    minutesSpinBox->setRange(0, 59);
    minutesSpinBox->setValue(5);
    minutesSpinBox->setSuffix(" 分钟");
    minutesSpinBox->setEnabled(false);
    minutesSpinBox->setMinimumWidth(80);
    
    secondsSpinBox = new QSpinBox();
    secondsSpinBox->setRange(0, 59);
    secondsSpinBox->setValue(0);
    secondsSpinBox->setSuffix(" 秒");
    secondsSpinBox->setEnabled(false);
    secondsSpinBox->setMinimumWidth(80);
    
    timeLayout->addWidget(hoursSpinBox);
    timeLayout->addWidget(minutesSpinBox);
    timeLayout->addWidget(secondsSpinBox);
    timeLayout->addStretch();
    
    timerLayout->addLayout(timeLayout);
    
    // 延时录制组
    QGroupBox *optionsGroup = new QGroupBox("延时录制");
    optionsGroup->setStyleSheet("QGroupBox { font-weight: bold; padding-top: 15px; }");
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);
    
    autoMinimizeCheckBox = new QCheckBox("录制时自动最小化窗口");
    autoMinimizeCheckBox->setChecked(true);
    QIcon delayIcon(":/icons/delay.png");
    if (delayIcon.isNull()) {
        delayIcon = style()->standardIcon(QStyle::SP_DialogApplyButton);
    }
    autoMinimizeCheckBox->setIcon(delayIcon);
    optionsLayout->addWidget(autoMinimizeCheckBox);
    
    // 延时时间设置
    QHBoxLayout *delayLayout = new QHBoxLayout();
    delayLayout->addWidget(new QLabel("延时时间:"));
    
    delaySecondsSpinBox = new QSpinBox();
    delaySecondsSpinBox->setRange(0, 60);
    delaySecondsSpinBox->setValue(2);
    delaySecondsSpinBox->setSuffix(" 秒");
    delaySecondsSpinBox->setEnabled(true); // 默认启用，因为自动最小化默认选中
    delaySecondsSpinBox->setMinimumWidth(80);
    delayLayout->addWidget(delaySecondsSpinBox);
    delayLayout->addWidget(new QLabel("后开始录制"));
    delayLayout->addStretch();
    
    optionsLayout->addLayout(delayLayout);
    
    // 添加所有组到左侧布局
    leftLayout->addWidget(controlGroup);
    leftLayout->addWidget(settingsGroup);
    leftLayout->addWidget(timerGroup);
    leftLayout->addWidget(optionsGroup);
    leftLayout->addStretch();
    
    // 右侧：状态显示区域
    QWidget *rightWidget = new QWidget();
    rightWidget->setMinimumWidth(250);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    
    // 状态显示组
    QGroupBox *statusGroup = new QGroupBox("录制状态");
    statusGroup->setStyleSheet("QGroupBox { font-weight: bold; padding-top: 15px; }");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    
    // 当前状态
    statusLabel = new QLabel("就绪");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(
        "QLabel { font-size: 16px; padding: 15px; background-color: #e8f5e8; "
        "border: 2px solid #4CAF50; border-radius: 8px; }"
    );
    
    // 录制时间
    QLabel *timeTitle = new QLabel("已录制时间:");
    timeTitle->setAlignment(Qt::AlignCenter);
    timeTitle->setStyleSheet("font-weight: bold; margin-top: 10px;");
    
    timeLabel = new QLabel("00:00:00");
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setStyleSheet(
        "QLabel { font-size: 28px; font-weight: bold; padding: 10px; "
        "background-color: #f0f0f0; border-radius: 8px; color: #333; }"
    );
    
    // 剩余时间（定时录制时显示）
    QLabel *remainingTitle = new QLabel("剩余时间:");
    remainingTitle->setAlignment(Qt::AlignCenter);
    remainingTitle->setStyleSheet("font-weight: bold; margin-top: 10px;");
    
    timerRemainingLabel = new QLabel("--:--:--");
    timerRemainingLabel->setAlignment(Qt::AlignCenter);
    timerRemainingLabel->setStyleSheet(
        "QLabel { font-size: 24px; font-weight: bold; padding: 10px; "
        "background-color: #fff3cd; border-radius: 8px; color: #856404; }"
    );
    timerRemainingLabel->setVisible(false);
    remainingTitle->setVisible(false);
    
    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(timeTitle);
    statusLayout->addWidget(timeLabel);
    statusLayout->addWidget(remainingTitle);
    statusLayout->addWidget(timerRemainingLabel);
    statusLayout->addStretch();
    
    rightLayout->addWidget(statusGroup);
    rightLayout->addStretch();
    
    // 添加到分割器
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
    setCentralWidget(centralWidget);
    
    // 连接信号
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartRecording);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopRecording);
    connect(browseButton, &QPushButton::clicked, this, &MainWindow::onBrowsePath);
    connect(timerEnabledCheckBox, &QCheckBox::toggled, this, &MainWindow::onTimerEnabledChanged);
    connect(autoMinimizeCheckBox, &QCheckBox::toggled, delaySecondsSpinBox, &QSpinBox::setEnabled);
    
    // 定时器
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateRecordingTime);
    
    recordingTimer = new QTimer(this);
    recordingTimer->setSingleShot(true);
    connect(recordingTimer, &QTimer::timeout, this, &MainWindow::onTimedRecordingFinished);
    
    restoreWindowTimer = new QTimer(this);
    restoreWindowTimer->setSingleShot(true);
    connect(restoreWindowTimer, &QTimer::timeout, this, [this]() {
        this->showNormal();
        this->raise();
        this->activateWindow();
        
        // 对于定时录制，使用设定的时长作为实际录制时长
        // 因为定时器保证了录制时间的准确性
        qint64 actualRecordingTime;
        if (timerEnabledCheckBox->isChecked() && recordingDurationMs > 0) {
            // 定时录制：使用设定的时长
            actualRecordingTime = recordingDurationMs;
        } else if (recordEndTime > 0 && recordStartTime > 0) {
            // 手动停止：使用实际时间差
            actualRecordingTime = recordEndTime - recordStartTime;
        } else {
            // 备用方案
            actualRecordingTime = recordingDurationMs > 0 ? recordingDurationMs : 0;
        }
        
        QString msg = QString("定时录制已完成！\n文件: %1\n实际录制时长: %2")
            .arg(outputPathEdit->text())
            .arg(formatDuration(actualRecordingTime));
        
        QMessageBox::information(this, "定时录制完成", msg);
        
        // 重置定时显示和时间记录
        timerRemainingLabel->setText("--:--:--");
        recordEndTime = 0;
    });
}

void MainWindow::onStartRecording() {
    if (isRecording) return;
    
    // 获取输出路径（文件夹）
    QString outputDir = outputPathEdit->text();
    if (outputDir.isEmpty()) {
        outputDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    }
    
    // 获取文件名
    QString fileName = outputNameEdit->text().trimmed();
    if (fileName.isEmpty()) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        fileName = "AIcp_" + timestamp;
    }
    
    // 确保文件名有正确的扩展名
    if (!fileName.endsWith(".mov", Qt::CaseInsensitive) && 
        !fileName.endsWith(".mp4", Qt::CaseInsensitive)) {
        fileName += ".mov";
    }
    
    // 组合完整路径
    QString outputPath = outputDir + "/" + fileName;
    
    // 确保目录存在
    QDir().mkpath(outputDir);
    
    // 设置帧率
    int fps = fpsCombo->currentText().split(" ")[0].toInt();
    videoCapture->setFrameRate(fps);

    // 使用所选屏幕的区域作为捕获区域
    int idx = screenCombo->currentData().toInt();
    const auto screens = QGuiApplication::screens();
    if (idx >= 0 && idx < screens.size()) {
        QRect g = screens[idx]->geometry();
        qreal devicePixelRatio = screens[idx]->devicePixelRatio();
        
        // 计算物理分辨率
        int physicalX = g.x() * devicePixelRatio;
        int physicalY = g.y() * devicePixelRatio;
        int physicalWidth = g.width() * devicePixelRatio;
        int physicalHeight = g.height() * devicePixelRatio;
        
        videoCapture->setCaptureRegion(physicalX, physicalY, physicalWidth, physicalHeight);
        
        std::cout << "设置录制区域: " << physicalWidth << "x" << physicalHeight 
                  << " (逻辑: " << g.width() << "x" << g.height() 
                  << ", 缩放: " << devicePixelRatio << ")" << std::endl;
    }
    
    // 计算录制时长（如果启用定时）
    recordingDurationMs = 0;
    if (timerEnabledCheckBox->isChecked()) {
        int hours = hoursSpinBox->value();
        int minutes = minutesSpinBox->value();
        int seconds = secondsSpinBox->value();
        recordingDurationMs = (hours * 3600 + minutes * 60 + seconds) * 1000;
        
        if (recordingDurationMs <= 0) {
            QMessageBox::warning(this, "警告", "请设置有效的录制时长！");
            return;
        }
        
        // 不在这里启动定时器，而是在实际开始录制时启动
        timerRemainingLabel->setText(formatDuration(recordingDurationMs));
    }
    
    // 禁用开始按钮
    startButton->setEnabled(false);
    
    // 根据用户设置决定是否最小化
    if (autoMinimizeCheckBox->isChecked()) {
        // 获取用户设定的延时时间
        int delaySeconds = delaySecondsSpinBox->value();
        
        if (delaySeconds == 0) {
            // 立即最小化并开始录制
            setStatusText("立即开始录制...", "#d1ecf1", "#bee5eb", "#0c5460");
            
            // 最小化窗口
            this->showMinimized();
            
            // 等待1秒让窗口完全最小化后开始录制
            QTimer::singleShot(1000, this, [this, outputPath, outputDir]() {
                this->startRecordingInternal(outputPath, outputDir);
            });
        } else {
            // 显示准备状态
            setStatusText(QString("准备录制中，窗口将在%1秒后最小化...").arg(delaySeconds), "#fff3cd", "#ffc107", "#856404");
            
            // 使用用户设定的延时时间
            int delayMs = delaySeconds * 1000;
            QTimer::singleShot(delayMs, this, [this, outputPath, outputDir]() {
                // 最小化窗口
                this->showMinimized();
                
                // 再等待1秒让窗口完全最小化
                QTimer::singleShot(1000, this, [this, outputPath, outputDir]() {
                    this->startRecordingInternal(outputPath, outputDir);
                });
            });
        }
    } else {
        // 直接开始录制，不最小化
        setStatusText("开始录制...", "#fff3cd", "#ffc107", "#856404");
        
        // 稍微延时一下以显示状态
        QTimer::singleShot(500, this, [this, outputPath, outputDir]() {
            this->startRecordingInternal(outputPath, outputDir);
        });
    }
}

void MainWindow::onStopRecording() {
    if (!isRecording) return;
    
    // 记录录制结束时间
    recordEndTime = QDateTime::currentMSecsSinceEpoch();
    
    // 停止所有定时器
    if (recordingTimer->isActive()) {
        recordingTimer->stop();
    }
    if (restoreWindowTimer->isActive()) {
        restoreWindowTimer->stop();
    }
    
    videoCapture->stopCapture();
    
    isRecording = false;
    qint64 duration = recordEndTime - recordStartTime;
    
    // 更新最终的录制时间显示
    timeLabel->setText(formatDuration(duration));
    
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    setStatusText("就绪", "#e8f5e8", "#4CAF50", "#000000");
    
    updateTimer->stop();
    
    // 恢复窗口显示
    this->showNormal();
    this->raise();
    this->activateWindow();
    
    // 重置定时显示
    timerRemainingLabel->setText("--:--:--");
    
    QString msg = QString("录制完成！\n文件: %1\n时长: %2")
        .arg(outputPathEdit->text())
        .arg(formatDuration(duration));
    
    QMessageBox::information(this, "录制完成", msg);
    
    // 重置时间记录
    recordEndTime = 0;
}

void MainWindow::startRecordingInternal(const QString& outputPath, const QString& outputDir) {
    // 开始录制
    if (videoCapture->startCapture(outputPath.toStdString())) {
        isRecording = true;
        recordStartTime = QDateTime::currentMSecsSinceEpoch();
        
        // 如果启用了定时录制，现在才启动定时器
        if (timerEnabledCheckBox->isChecked() && recordingDurationMs > 0) {
            recordingTimer->start(recordingDurationMs);
        }
        
        stopButton->setEnabled(true);
        setStatusText("录制中...", "#f8d7da", "#dc3545", "#721c24");
        
        // 只显示目录路径，不显示完整文件路径
        outputPathEdit->setText(outputDir);
        
        updateTimer->start(1000);
    } else {
        // 如果录制失败，恢复窗口和按钮状态
        startButton->setEnabled(true);
        setStatusText("就绪", "#e8f5e8", "#4CAF50", "#000000");
        
        // 停止所有定时器
        if (recordingTimer->isActive()) {
            recordingTimer->stop();
        }
        if (restoreWindowTimer->isActive()) {
            restoreWindowTimer->stop();
        }
        timerRemainingLabel->setText("--:--:--");
        
        if (autoMinimizeCheckBox->isChecked()) {
            this->showNormal();
        }
        QMessageBox::critical(this, "错误", "录制启动失败");
    }
}

void MainWindow::onBrowsePath() {
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        "选择输出文件夹",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
    );
    
    if (!dirPath.isEmpty()) {
        outputPathEdit->setText(dirPath);
    }
}

void MainWindow::onTimerEnabledChanged(bool enabled) {
    hoursSpinBox->setEnabled(enabled);
    minutesSpinBox->setEnabled(enabled);
    secondsSpinBox->setEnabled(enabled);
    
    if (enabled) {
        timerRemainingLabel->setVisible(true);
        timerRemainingLabel->parentWidget()->layout()->itemAt(3)->widget()->setVisible(true); // remainingTitle
    } else {
        timerRemainingLabel->setVisible(false);
        timerRemainingLabel->parentWidget()->layout()->itemAt(3)->widget()->setVisible(false); // remainingTitle
    }
}

void MainWindow::updateRecordingTime() {
    if (isRecording) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 duration = currentTime - recordStartTime;
        timeLabel->setText(formatDuration(duration));
        
        // 如果启用了定时录制，更新剩余时间
        if (timerEnabledCheckBox->isChecked() && recordingDurationMs > 0) {
            // 计算已经过去的完整秒数
            int elapsedSeconds = duration / 1000;
            int totalSeconds = recordingDurationMs / 1000;
            int remainingSeconds = totalSeconds - elapsedSeconds;
            
            // 确保剩余时间不小于0
            if (remainingSeconds > 0) {
                timerRemainingLabel->setText(formatDuration(remainingSeconds * 1000));
            } else {
                timerRemainingLabel->setText("00:00:00");
            }
        }
    }
}

void MainWindow::onTimedRecordingFinished() {
    if (!isRecording) return;
    
    // 记录录制结束时间
    recordEndTime = QDateTime::currentMSecsSinceEpoch();
    
    // 停止录制
    videoCapture->stopCapture();
    isRecording = false;
    
    // 最后更新一次录制时间，确保显示正确的时长
    qint64 actualRecordingTime = recordEndTime - recordStartTime;
    timeLabel->setText(formatDuration(actualRecordingTime));
    
    // 更新UI状态
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    setStatusText("延时录制完成", "#d4edda", "#28a745", "#155724");
    
    updateTimer->stop();
    
    // 录制结束后等待2秒再恢复窗口显示
    restoreWindowTimer->start(2000);
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

void MainWindow::setStatusText(const QString& text, const QString& color, const QString& borderColor, const QString& textColor) {
    statusLabel->setText(text);
    
    // 获取状态标签的实际宽度
    int availableWidth = statusLabel->width();
    if (availableWidth <= 0) {
        availableWidth = 220; // 默认宽度
    }
    
    // 基础字体大小
    int baseFontSize = 16;
    int minFontSize = 10;
    
    // 创建字体度量对象来计算文本宽度
    QFont font = statusLabel->font();
    font.setPixelSize(baseFontSize);
    QFontMetrics metrics(font);
    
    // 计算文本宽度，减去内边距
    int textWidth = metrics.horizontalAdvance(text);
    int padding = 30; // 左右各15px内边距
    
    // 如果文本过长，减小字体
    while (textWidth + padding > availableWidth && baseFontSize > minFontSize) {
        baseFontSize--;
        font.setPixelSize(baseFontSize);
        metrics = QFontMetrics(font);
        textWidth = metrics.horizontalAdvance(text);
    }
    
    // 应用样式
    QString styleSheet = QString(
        "QLabel { font-size: %1px; padding: 15px; background-color: %2; "
        "border: 2px solid %3; border-radius: 8px; color: %4; }"
    ).arg(baseFontSize).arg(color).arg(borderColor).arg(textColor);
    
    statusLabel->setStyleSheet(styleSheet);
}