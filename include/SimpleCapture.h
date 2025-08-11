#pragma once

#include <string>
#include <memory>

// 简化的屏幕捕获接口
class SimpleCapture {
public:
    virtual ~SimpleCapture() = default;
    
    virtual bool init() = 0;
    virtual bool startCapture(const std::string& outputPath) = 0;
    virtual bool stopCapture() = 0;
    virtual bool isCapturing() const = 0;
    virtual void setFrameRate(int fps) = 0;
    // 使用 setCaptureRegion 传入所选 QScreen 的 geometry(x,y,w,h)，即可实现捕获指定屏幕
    virtual void setCaptureRegion(int x, int y, int width, int height) = 0;
};

// 创建工厂函数
std::unique_ptr<SimpleCapture> createSimpleCapture();