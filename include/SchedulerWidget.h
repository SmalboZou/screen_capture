#ifndef SCHEDULER_WIDGET_H
#define SCHEDULER_WIDGET_H

#include <QWidget>
#include <memory>

// 前向声明
class QCalendarWidget;
class QTableWidget;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class LocalScheduler;
class ScheduleTask;

/**
 * @brief 任务调度器组件
 */
class SchedulerWidget : public QWidget {
    Q_OBJECT

public:
    SchedulerWidget(QWidget* parent = nullptr);
    ~SchedulerWidget();
    
    void refreshTaskList();

private:
    void setupUI();
    void setupConnections();
    
    void addTask();
    void removeTask();
    void editTask();
    void toggleTask();
    
    // UI控件
    QCalendarWidget* calendar;
    QTableWidget* taskList;
    QPushButton* addButton;
    QPushButton* removeButton;
    QPushButton* editButton;
    QPushButton* toggleButton;
    
    // 布局
    QVBoxLayout* mainLayout;
    QHBoxLayout* buttonLayout;
    
    // 数据模型
    std::unique_ptr<LocalScheduler> scheduler;
};

#endif // SCHEDULER_WIDGET_H