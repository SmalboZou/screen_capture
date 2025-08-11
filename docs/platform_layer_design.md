# 平台抽象层设计文档

## 概述

平台抽象层(PAL - Platform Abstraction Layer)是整个屏幕录制程序的基础，它屏蔽了不同操作系统之间的差异，为上层提供了统一的接口。这一层主要包括屏幕捕获、音频捕获、硬件编码等功能的抽象接口及其实现。

## 核心接口设计

### 1. 屏幕捕获接口 (ILocalCapture)

```cpp
class ILocalCapture {
public:
    virtual ~ILocalCapture() = default;
    
    /**
     * 初始化捕获器
     * @return true 成功, false 失败
     */
    virtual bool init() = 0;
    
    /**
     * 捕获一帧屏幕数据
     * @return FrameData 帧数据对象
     */
    virtual FrameData captureFrame() = 0;
    
    /**
     * 释放资源
     */
    virtual void release() = 0;
};
```

### 2. 编码器接口 (ILocalEncoder)

```cpp
class ILocalEncoder {
public:
    virtual ~ILocalEncoder() = default;
    
    /**
     * 设置编码器配置
     * @param config 编码配置
     * @return true 成功, false 失败
     */
    virtual bool setup(const EncoderConfig& config) = 0;
    
    /**
     * 编码一帧数据
     * @param frame 输入帧数据
     * @return EncodedData 编码后的数据
     */
    virtual EncodedData encode(const FrameData& frame) = 0;
    
    /**
     * 完成编码并保存文件
     * @param outputPath 输出文件路径
     * @return true 成功, false 失败
     */
    virtual bool finalize(const std::string& outputPath) = 0;
};
```

### 3. 音频捕获接口 (IAudioCapture)

```cpp
class IAudioCapture {
public:
    virtual ~IAudioCapture() = default;
    
    /**
     * 初始化音频捕获器
     * @return true 成功, false 失败
     */
    virtual bool init() = 0;
    
    /**
     * 捕获音频数据
     * @return AudioData 音频数据对象
     */
    virtual AudioData captureAudio() = 0;
    
    /**
     * 释放资源
     */
    virtual void release() = 0;
};
```

## 数据结构定义

### 1. 帧数据结构 (FrameData)

```cpp
struct FrameData {
    uint8_t* data;           // 帧数据指针
    size_t size;             // 数据大小
    int width;               // 宽度
    int height;              // 高度
    int stride;              // 步长
    PixelFormat format;      // 像素格式
    uint64_t timestamp;      // 时间戳
};
```

### 2. 编码配置 (EncoderConfig)

```cpp
struct EncoderConfig {
    std::string codec;       // 编码器类型 (H264, HEVC等)
    int width;               // 视频宽度
    int height;              // 视频高度
    int fps;                 // 帧率
    int bitrate;             // 比特率
    QualityLevel quality;    // 质量等级
};
```

### 3. 硬件信息 (HardwareInfo)

```cpp
struct HardwareInfo {
    GPUType gpuType;         // GPU类型
    std::vector<std::string> supportedCodecs;  // 支持的编解码器
    uint64_t totalRAM;       // 总内存大小
    uint64_t freeDiskSpace;  // 可用磁盘空间
};
```

## 平台特定实现

### Windows 实现

#### 屏幕捕获 (DXGI Desktop Duplication)

Windows平台使用DXGI桌面复制技术进行屏幕捕获，该技术具有高性能和低延迟的特点。

#### 音频捕获 (WASAPI Loopback)

使用WASAPI环回捕获技术获取系统音频输出。

#### 硬件编码 (NVENC/DXVA)

支持NVIDIA NVENC和Intel Quick Sync Video硬件编码。

### macOS 实现

#### 屏幕捕获 (CoreGraphics + IOSurface)

使用CoreGraphics框架结合IOSurface进行高效的屏幕捕获。

#### 音频捕获 (AudioQueue)

通过AudioQueue框架捕获系统音频。

#### 硬件编码 (VideoToolbox)

利用VideoToolbox框架进行硬件加速编码。

### Linux 实现

#### 屏幕捕获 (X11/XShm 或 PipeWire)

支持X11共享内存和PipeWire两种屏幕捕获方式。

#### 音频捕获 (PulseAudio)

通过PulseAudio服务器捕获音频流。

#### 硬件编码 (VAAPI)

使用VAAPI (Video Acceleration API) 进行硬件编码。

## 硬件检测模块

```cpp
/**
 * 检测系统硬件信息
 * @return HardwareInfo 硬件信息结构体
 */
HardwareInfo detectHardware() {
    HardwareInfo info;
    
    // GPU检测
    if (hasNVIDIA()) {
        info.gpuType = GPU_NVIDIA;
        info.supportedCodecs = {"H264", "HEVC"};
    }
    else if (hasIntelQSV()) {
        info.gpuType = GPU_INTEL;
        info.supportedCodecs = {"H264", "HEVC", "VP9"};
    }
    else if (hasAMD()) {
        info.gpuType = GPU_AMD;
        info.supportedCodecs = {"H264", "HEVC"};
    }
    
    // 内存检测
    info.totalRAM = getTotalRAM();
    info.freeDiskSpace = getFreeSpace(defaultSavePath);
    
    return info;
}
```

## 设计原则

1. **接口隔离**: 每个功能模块都有独立的接口，便于单独实现和测试
2. **平台无关**: 上层代码只需依赖抽象接口，不关心具体实现
3. **可扩展性**: 新增平台支持只需实现相应接口即可
4. **性能优先**: 尽可能使用平台原生API以获得最佳性能
5. **资源管理**: 明确的初始化和释放接口，避免资源泄漏

## 依赖关系

平台抽象层尽量减少对外部库的依赖，主要依赖于：
- 各平台原生API
- 基础C++标准库
- 必要时使用FFmpeg的部分组件(需遵循LGPL协议)

## 测试策略

每个平台的具体实现都需要有对应的测试用例，确保：
1. 接口正确性
2. 性能达标
3. 资源管理正确
4. 异常处理完善