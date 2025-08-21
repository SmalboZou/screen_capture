#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QTimer>
#include <QCheckBox>
#include <memory>
#include "SimpleCapture.h"

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

private:
    void setupUI();
    void startRecordingInternal(const QString& outputPath);
    QString formatDuration(qint64 ms);

    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *browseButton;
    QLineEdit *outputPathEdit;
    QComboBox *fpsCombo;
    QComboBox *screenCombo; // 新增：选择录制屏幕
    QCheckBox *autoMinimizeCheckBox; // 新增：自动最小化选项
    QLabel *statusLabel;
    QLabel *timeLabel;
    QTimer *updateTimer;
    
    std::unique_ptr<SimpleCapture> videoCapture;
    bool isRecording;
    qint64 recordStartTime;
};

#endif // MAINWINDOW_H