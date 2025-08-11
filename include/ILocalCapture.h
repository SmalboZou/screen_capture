#ifndef ILOCAL_CAPTURE_H
#define ILOCAL_CAPTURE_H

#include <cstdint>
#include <memory>

// 前向声明
struct FrameData;

/**
 * @brief 本地屏幕捕获接口
 */
class ILocalCapture {
public:
    virtual ~ILocalCapture() = default;
    
    /**
     * @brief 初始化捕获器
     * @return true 成功, false 失败
     */
    virtual bool init() = 0;
    
    /**
     * @brief 捕获一帧屏幕数据
     * @return FrameData 帧数据对象
     */
    virtual FrameData captureFrame() = 0;
    
    /**
     * @brief 释放资源
     */
    virtual void release() = 0;
};

#endif // ILOCAL_CAPTURE_H