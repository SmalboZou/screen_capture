#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QTimer>
#include <QCheckBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QSettings>
#include <memory>
#include "SimpleCapture.h"
#include "AISummaryConfigDialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartRecording();
    void onStopRecording();
    void onBrowsePath();
    void updateRecordingTime();
    void onTimerEnabledChanged(bool enabled);
    void onTimedRecordingFinished();
    void onVideoSummaryEnabledChanged(bool enabled);
    void onSummaryConfigClicked();

private:
    void setupUI();
    void startRecordingInternal(const QString& outputPath, const QString& outputDir);
    QString formatDuration(qint64 ms);
    void setStatusText(const QString& text, const QString& color = "#fff3cd", const QString& borderColor = "#ffc107", const QString& textColor = "#856404");
    void loadAISettings();
    void saveAISettings();

    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *browseButton;
    QLineEdit *outputPathEdit;
    QLineEdit *outputNameEdit; // 输出文件名
    QComboBox *fpsCombo;
    QComboBox *screenCombo; // 选择录制屏幕
    QCheckBox *autoMinimizeCheckBox; // 自动最小化选项
    QSpinBox *delaySecondsSpinBox; // 延时时间（秒）
    QCheckBox *timerEnabledCheckBox; // 定时录制开关
    QSpinBox *hoursSpinBox; // 小时
    QSpinBox *minutesSpinBox; // 分钟
    QSpinBox *secondsSpinBox; // 秒钟
    QLabel *statusLabel;
    QLabel *timeLabel;
    QLabel *timerRemainingLabel; // 剩余时间显示
    QTimer *updateTimer;
    QTimer *recordingTimer; // 定时录制计时器
    QTimer *restoreWindowTimer; // 窗口恢复计时器
    
    // 视频内容总结相关UI组件
    QCheckBox *videoSummaryEnabledCheckBox; // 启用视频内容总结
    QTextEdit *videoSummaryTextEdit; // 显示总结内容的多行文本框
    QPushButton *summaryConfigButton; // AI模型配置按钮
    
    std::unique_ptr<SimpleCapture> videoCapture;
    bool isRecording;
    qint64 recordStartTime;
    qint64 recordEndTime; // 记录录制结束时间
    qint64 recordingDurationMs; // 预设录制时长(毫秒)
    
    // AI视频总结配置
    AISummaryConfig aiSummaryConfig;
    std::unique_ptr<AISummaryConfigDialog> summaryConfigDialog;
};

#endif // MAINWINDOW_H