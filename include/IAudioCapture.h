#ifndef IAUDIO_CAPTURE_H
#define IAUDIO_CAPTURE_H

#include <cstdint>
#include <memory>

// 前向声明
struct AudioData;

/**
 * @brief 音频捕获接口
 */
class IAudioCapture {
public:
    virtual ~IAudioCapture() = default;
    
    /**
     * @brief 初始化音频捕获器
     * @return true 成功, false 失败
     */
    virtual bool init() = 0;
    
    /**
     * @brief 捕获音频数据
     * @return AudioData 音频数据对象
     */
    virtual AudioData captureAudio() = 0;
    
    /**
     * @brief 释放资源
     */
    virtual void release() = 0;
};

#endif // IAUDIO_CAPTURE_H