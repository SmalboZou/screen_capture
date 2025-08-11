#ifndef RECORDING_CONTROL_PANEL_H
#define RECORDING_CONTROL_PANEL_H

#include <QWidget>
#include <memory>

// 前向声明
class QPushButton;
class QLabel;
class QProgressBar;
class QTimer;
enum class RecStatus;

/**
 * @brief 录制控制面板
 */
class RecordingControlPanel : public QWidget {
    Q_OBJECT

public:
    RecordingControlPanel(QWidget* parent = nullptr);
    ~RecordingControlPanel();
    
    void updateRecordingStatus(RecStatus status);
    void updateRecordingTime(double seconds);
    void updateAudioLevel(float level);

signals:
    void startRequested();
    void pauseRequested();
    void stopRequested();

private:
    void setupUI();
    void updateButtonStates(RecStatus status);
    
    // UI控件
    QPushButton* startButton;
    QPushButton* pauseButton;
    QPushButton* stopButton;
    QLabel* timerLabel;
    QProgressBar* audioMeter;
    QLabel* previewLabel;
    
    // 定时器
    std::unique_ptr<QTimer> updateTimer;
    
    // 状态
    RecStatus currentStatus;
    double recordingTime;
};

#endif // RECORDING_CONTROL_PANEL_H