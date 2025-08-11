#ifndef LOCAL_SCHEDULER_H
#define LOCAL_SCHEDULER_H

#include "DataTypes.h"
#include "RecordingService.h"
#include <string>
#include <map>
#include <ctime>
#include <memory>
#include <thread>

// 前向声明
class SQLiteDB;

/**
 * @brief 本地任务调度器
 */
class LocalScheduler {
public:
    LocalScheduler();
    ~LocalScheduler();
    
    /**
     * @brief 添加定时任务
     * @param task 任务对象
     * @return true 成功, false 失败
     */
    bool addTask(const ScheduleTask& task);
    
    /**
     * @brief 移除任务
     * @param id 任务ID
     * @return true 成功, false 失败
     */
    bool removeTask(const std::string& id);
    
    /**
     * @brief 更新任务
     * @param task 任务对象
     * @return true 成功, false 失败
     */
    bool updateTask(const ScheduleTask& task);
    
    /**
     * @brief 获取任务
     * @param id 任务ID
     * @return 任务对象
     */
    ScheduleTask getTask(const std::string& id) const;
    
    /**
     * @brief 获取所有任务
     * @return 任务映射表
     */
    std::map<std::string, ScheduleTask> getAllTasks() const;
    
    /**
     * @brief 启动调度器
     * @return true 成功, false 失败
     */
    bool startScheduler();
    
    /**
     * @brief 停止调度器
     */
    void stopScheduler();
    
    /**
     * @brief 保存任务到磁盘
     * @return true 成功, false 失败
     */
    bool saveTasksToDisk();
    
    /**
     * @brief 从磁盘加载任务
     * @return true 成功, false 失败
     */
    bool loadTasksFromDisk();
    
private:
    /**
     * @brief 检查是否可以启动任务
     * @param task 任务对象
     * @return true 可以启动, false 不能启动
     */
    bool canStartTask(const ScheduleTask& task);
    
    /**
     * @brief 调度循环
     */
    void schedulerLoop();
    
    /**
     * @brief 执行任务
     * @param task 任务对象
     */
    void executeTask(const ScheduleTask& task);
    
    /**
     * @brief 获取可用磁盘空间
     * @param path 路径
     * @return 可用空间(bytes)
     */
    uint64_t getFreeSpace(const std::string& path) const;
    
    /**
     * @brief 估算任务所需空间
     * @param task 任务对象
     * @return 所需空间(bytes)
     */
    uint64_t estimateRequiredSpace(const ScheduleTask& task) const;
    
    std::map<std::string, ScheduleTask> tasks;
    std::unique_ptr<SQLiteDB> db;
    
    // 调度控制
    bool isRunning;
    std::thread schedulerThread;
};

// 定时任务结构
struct ScheduleTask {
    std::string id;          // 任务ID
    std::string name;        // 任务名称
    std::time_t startTime;   // 开始时间
    std::time_t endTime;     // 结束时间
    std::time_t duration;    // 持续时间(秒)
    RecConfig config;        // 录制配置
    bool repeat = false;     // 是否重复
    int repeatInterval = 0;  // 重复间隔(分钟)
    bool enabled = true;     // 是否启用
};

#endif // LOCAL_SCHEDULER_H