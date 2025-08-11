#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <memory>

// 前向声明
class RecordingControlPanel;
class SettingsDialog;
class SchedulerWidget;
class FileBrowserWidget;
class RecordingService;
class SettingsManager;
class LocalScheduler;
class FileManager;

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QToolBar;
class QStatusBar;
QT_END_NAMESPACE

/**
 * @brief 主窗口类
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onStartRecording();
    void onPauseRecording();
    void onStopRecording();
    void onSettings();
    void onScheduledTasks();
    void onFileBrowser();
    void onAbout();
    void onHelp();

private:
    void setupUI();
    void setupActions();
    void setupMenus();
    void setupToolbars();
    void setupStatusBar();
    void setupConnections();
    
    void loadSettings();
    void saveSettings();
    
    // 界面组件
    std::unique_ptr<RecordingControlPanel> recordingPanel;
    std::unique_ptr<SettingsDialog> settingsDialog;
    std::unique_ptr<SchedulerWidget> schedulerWidget;
    std::unique_ptr<FileBrowserWidget> fileBrowser;
    
    // 应用服务
    std::unique_ptr<RecordingService> recordingService;
    std::unique_ptr<SettingsManager> settingsManager;
    std::unique_ptr<LocalScheduler> scheduler;
    std::unique_ptr<FileManager> fileManager;
    
    // UI元素
    QMenu* fileMenu;
    QMenu* editMenu;
    QMenu* viewMenu;
    QMenu* toolsMenu;
    QMenu* helpMenu;
    
    QToolBar* mainToolBar;
    QStatusBar* mainStatusBar;
    
    QAction* startAction;
    QAction* pauseAction;
    QAction* stopAction;
    QAction* settingsAction;
    QAction* schedulerAction;
    QAction* fileBrowserAction;
    QAction* exitAction;
    QAction* aboutAction;
    QAction* helpAction;
};

#endif // MAIN_WINDOW_H