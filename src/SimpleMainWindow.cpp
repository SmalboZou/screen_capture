// SimpleMainWindow.cpp
#include <QApplication>
#include <QMainWindow>
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
#include <QDir>
#include "SimpleCapture.h"
#include <memory>
#include <iostream>

class SimpleMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit SimpleMainWindow(QWidget *parent = nullptr);
    ~SimpleMainWindow();

private slots:
    void onStartRecording();
    void onStopRecording();
    void onBrowsePath();
    void updateRecordingTime();

private:
    void setupUI();
    QString formatDuration(qint64 ms);
    
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *browseButton;
    QLineEdit *outputPathEdit;
    QComboBox *fpsCombo;
    QLabel *statusLabel;
    QLabel *timeLabel;
    QTimer *updateTimer;
    
    std::unique_ptr<SimpleCapture> videoCapture;
    bool isRecording;
    qint64 recordStartTime;
};

SimpleMainWindow::SimpleMainWindow(QWidget *parent)
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
    
    // 设置窗口图标
    setWindowIcon(QIcon());
}

SimpleMainWindow::~SimpleMainWindow() = default;

void SimpleMainWindow::setupUI() {
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
    controlLayout->addWidget(statusLabel);
    controlLayout->addWidget(timeLabel);
    
    mainLayout->addWidget(controlGroup);
    
    setCentralWidget(centralWidget);
    
    // 连接信号
    connect(startButton, &QPushButton::clicked, this, &SimpleMainWindow::onStartRecording);
    connect(stopButton, &QPushButton::clicked, this, &SimpleMainWindow::onStopRecording);
    connect(browseButton, &QPushButton::clicked, this, &SimpleMainWindow::onBrowsePath);
    
    // 定时器
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &SimpleMainWindow::updateRecordingTime);
}

void SimpleMainWindow::onStartRecording() {
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
    
    // 开始录制
    if (videoCapture->startCapture(outputPath.toStdString())) {
        isRecording = true;
        recordStartTime = QDateTime::currentMSecsSinceEpoch();
        
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
        statusLabel->setText("录制中...");
        statusLabel->setStyleSheet("font-size: 16px; padding: 10px; color: red;");
        
        outputPathEdit->setText(outputPath);
        
        updateTimer->start(1000);
    } else {
        QMessageBox::critical(this, "错误", "录制启动失败");
    }
}

void SimpleMainWindow::onStopRecording() {
    if (!isRecording) return;
    
    videoCapture->stopCapture();
    
    isRecording = false;
    qint64 duration = QDateTime::currentMSecsSinceEpoch() - recordStartTime;
    
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    statusLabel->setText("就绪");
    statusLabel->setStyleSheet("font-size: 16px; padding: 10px;");
    
    updateTimer->stop();
    
    QString msg = QString("录制完成！\n文件: %1\n时长: %2")
        .arg(outputPathEdit->text())
        .arg(formatDuration(duration));
    
    QMessageBox::information(this, "录制完成", msg);
}

void SimpleMainWindow::onBrowsePath() {
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

void SimpleMainWindow::updateRecordingTime() {
    if (isRecording) {
        qint64 duration = QDateTime::currentMSecsSinceEpoch() - recordStartTime;
        timeLabel->setText(formatDuration(duration));
    }
}

QString SimpleMainWindow::formatDuration(qint64 ms) {
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

#include "SimpleMainWindow.moc"

// 主函数
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    app.setApplicationName("AIcp");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("AIcp Project");
    
    SimpleMainWindow window;
    window.show();
    
    return app.exec();
}