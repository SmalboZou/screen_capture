#ifndef RECORDING_SERVICE_H
#define RECORDING_SERVICE_H

#include "DataTypes.h"
#include <memory>
#include <string>

// 前向声明
class ILocalCapture;
class IAudioCapture;
class ILocalEncoder;
class LocalFileWriter;
enum class RecStatus;

// 录制状态枚举
enum class RecStatus {
    STOPPED,   // 已停止
    RECORDING, // 录制中
    PAUSED     // 已暂停
};

// 录制配置结构
struct RecConfig {
    // 视频设置
    Rect captureArea = {0, 0, 1920, 1080};  // 捕获区域
    int width = 1920;                       // 宽度
    int height = 1080;                      // 高度
    int fps = 30;                           // 帧率
    std::string codec = "H264";             // 编码器
    
    // 音频设置
    bool captureAudio = true;               // 是否捕获音频
    bool captureMic = false;                // 是否捕获麦克风
    
    // 输出设置
    std::string outputPath = "./recordings"; // 输出路径
    std::string fileName = "recording";      // 文件名
    FileFormat format = FileFormat::MP4;     // 文件格式
};

/**
 * @brief 录制服务
 */
class RecordingService {
public:
    RecordingService();
    ~RecordingService();
    
    /**
     * @brief 开始录制
     * @param config 录制配置
     * @return true 成功, false 失败
     */
    bool startRecording(const RecConfig& config);
    
    /**
     * @brief 暂停录制
     */
    void pauseRecording();
    
    /**
     * @brief 停止录制
     * @return true 成功, false 失败
     */
    bool stopRecording();
    
    /**
     * @brief 获取录制状态
     * @return RecStatus 当前状态
     */
    RecStatus getStatus() const;
    
    /**
     * @brief 获取录制时长(秒)
     * @return 录制时长
     */
    double getRecordingDuration() const;
    
private:
    /**
     * @brief 初始化录制组件
     * @param config 录制配置
     * @return true 成功, false 失败
     */
    bool initializeComponents(const RecConfig& config);
    
    /**
     * @brief 释放录制组件
     */
    void releaseComponents();
    
    /**
     * @brief 录制循环
     */
    void recordingLoop();
    
    std::unique_ptr<ILocalCapture> videoCapture;
    std::unique_ptr<IAudioCapture> audioCapture;
    std::unique_ptr<ILocalEncoder> encoder;
    std::unique_ptr<LocalFileWriter> fileWriter;
    
    RecStatus status;
    RecConfig currentConfig;
    
    // 录制控制
    bool shouldStop;
    bool isPaused;
    
    // 录制统计
    uint64_t startTime;
    uint64_t pausedTime;
    uint64_t totalPausedTime;
};

#endif // RECORDING_SERVICE_H