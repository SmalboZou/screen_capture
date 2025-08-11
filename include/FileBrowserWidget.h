#ifndef FILE_BROWSER_WIDGET_H
#define FILE_BROWSER_WIDGET_H

#include <QWidget>
#include <memory>

// 前向声明
class QTreeWidget;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class FileManager;
class RecordingInfo;

/**
 * @brief 文件浏览器组件
 */
class FileBrowserWidget : public QWidget {
    Q_OBJECT

public:
    FileBrowserWidget(QWidget* parent = nullptr);
    ~FileBrowserWidget();
    
    void refreshFileList();

private:
    void setupUI();
    void setupConnections();
    
    void playSelectedFile();
    void deleteSelectedFile();
    void exportSelectedFile();
    void showFileInfo();
    
    // UI控件
    QTreeWidget* fileTree;
    QPushButton* playButton;
    QPushButton* deleteButton;
    QPushButton* exportButton;
    QPushButton* infoButton;
    
    // 布局
    QVBoxLayout* mainLayout;
    QHBoxLayout* buttonLayout;
    
    // 数据模型
    std::unique_ptr<FileManager> fileManager;
};

#endif // FILE_BROWSER_WIDGET_H