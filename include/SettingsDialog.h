#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <memory>

// 前向声明
class QTabWidget;
class QDialogButtonBox;
class QVBoxLayout;
class AppSettings;
class VideoSettingsWidget;
class AudioSettingsWidget;
class OutputSettingsWidget;
class HotkeySettingsWidget;

/**
 * @brief 设置对话框
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();
    
    AppSettings getSettings() const;
    void setSettings(const AppSettings& settings);

private:
    void setupUI();
    void setupConnections();
    
    // 设置页面
    std::unique_ptr<VideoSettingsWidget> videoPage;
    std::unique_ptr<AudioSettingsWidget> audioPage;
    std::unique_ptr<OutputSettingsWidget> outputPage;
    std::unique_ptr<HotkeySettingsWidget> hotkeyPage;
    
    QTabWidget* tabWidget;
    QDialogButtonBox* buttonBox;
    QVBoxLayout* mainLayout;
};

#endif // SETTINGS_DIALOG_H