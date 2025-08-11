#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QTimer>
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
    QString formatDuration(qint64 ms);

    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *browseButton;
    QLineEdit *outputPathEdit;
    QComboBox *fpsCombo;
    QComboBox *screenCombo; // 新增：选择录制屏幕
    QLabel *statusLabel;
    QLabel *timeLabel;
    QTimer *updateTimer;
    
    std::unique_ptr<SimpleCapture> videoCapture;
    bool isRecording;
    qint64 recordStartTime;
};

#endif // MAINWINDOW_H