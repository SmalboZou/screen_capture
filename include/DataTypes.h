#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

// 像素格式枚举
enum class PixelFormat {
    RGB24,
    BGR24,
    RGBA32,
    BGRA32,
    YUV420P,
    YUV422P,
    YUV444P
};

// GPU类型枚举
enum class GPUType {
    NONE,
    NVIDIA,
    AMD,
    INTEL
};

// 质量等级枚举
enum class QualityLevel {
    LOW,
    MEDIUM,
    HIGH,
    LOSSLESS
};

// 视频质量枚举（用于接口）
enum class VideoQuality {
    LOW,
    MEDIUM,
    HIGH,
    ULTRA
};

// 文件格式枚举
enum class FileFormat {
    MP4,
    AVI,
    MKV,
    MOV,
    WEBM
};

// 录制状态枚举
enum class RecordingState {
    STOPPED,
    RECORDING,
    PAUSED
};

// 使用前缀避免命名冲突
struct CapturePoint {
    int x;
    int y;
};

struct CaptureRect {
    int x;
    int y;
    int width;
    int height;
};

// 帧数据结构
struct FrameData {
    uint8_t* data = nullptr;     // 帧数据指针
    size_t size = 0;             // 数据大小
    int width = 0;               // 宽度
    int height = 0;              // 高度
    int stride = 0;              // 步长
    PixelFormat format = PixelFormat::RGB24;  // 像素格式
    uint64_t timestamp = 0;      // 时间戳
    
    // 构造函数
    FrameData() = default;
    
    // 析构函数
    ~FrameData() {
        if (data) {
            delete[] data;
            data = nullptr;
        }
    }
    
    // 拷贝构造函数
    FrameData(const FrameData& other) {
        copyFrom(other);
    }
    
    // 拷贝赋值运算符
    FrameData& operator=(const FrameData& other) {
        if (this != &other) {
            // 释放原有资源
            if (data) {
                delete[] data;
            }
            copyFrom(other);
        }
        return *this;
    }
    
private:
    void copyFrom(const FrameData& other) {
        size = other.size;
        width = other.width;
        height = other.height;
        stride = other.stride;
        format = other.format;
        timestamp = other.timestamp;
        
        if (other.data && other.size > 0) {
            data = new uint8_t[other.size];
            memcpy(data, other.data, other.size);
        } else {
            data = nullptr;
        }
    }
};

// 音频数据结构
struct AudioData {
    uint8_t* data = nullptr;     // 音频数据指针
    size_t size = 0;             // 数据大小
    int sampleRate = 0;          // 采样率
    int channels = 0;            // 声道数
    int bitsPerSample = 0;       // 位深度
    uint64_t timestamp = 0;      // 时间戳
    
    // 构造函数
    AudioData() = default;
    
    // 析构函数
    ~AudioData() {
        if (data) {
            delete[] data;
            data = nullptr;
        }
    }
    
    // 拷贝构造函数
    AudioData(const AudioData& other) {
        copyFrom(other);
    }
    
    // 拷贝赋值运算符
    AudioData& operator=(const AudioData& other) {
        if (this != &other) {
            // 释放原有资源
            if (data) {
                delete[] data;
            }
            copyFrom(other);
        }
        return *this;
    }
    
private:
    void copyFrom(const AudioData& other) {
        size = other.size;
        sampleRate = other.sampleRate;
        channels = other.channels;
        bitsPerSample = other.bitsPerSample;
        timestamp = other.timestamp;
        
        if (other.data && other.size > 0) {
            data = new uint8_t[other.size];
            memcpy(data, other.data, other.size);
        } else {
            data = nullptr;
        }
    }
};

// 视频帧结构（简化版）
struct VideoFrame {
    uint8_t* data = nullptr;
    size_t size = 0;
    int width = 0;
    int height = 0;
    int format = 0;
    int64_t timestamp = 0;
};

// 音频帧结构（简化版）
struct AudioFrame {
    uint8_t* data = nullptr;
    size_t size = 0;
    int sampleRate = 0;
    int channels = 0;
    int64_t timestamp = 0;
};

// 录制配置结构
struct RecordingConfig {
    int width = 1920;
    int height = 1080;
    int fps = 30;
    int bitrate = 5000000;
    std::string codec = "h264";
    std::string outputPath;
};

#endif // DATA_TYPES_H