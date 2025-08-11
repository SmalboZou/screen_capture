#ifndef RESOURCE_AWARE_ENCODER_H
#define RESOURCE_AWARE_ENCODER_H

#include "DataTypes.h"
#include <cstdint>

// 系统资源信息结构
struct SystemResources {
    double cpuUsage;      // CPU使用率 (%)
    double memoryUsage;   // 内存使用率 (%)
    double diskUsage;     // 磁盘使用率 (%)
    uint64_t freeMemory;  // 可用内存 (bytes)
    uint64_t freeDiskSpace; // 可用磁盘空间 (bytes)
};

// 编码质量等级
enum class EncodingQuality {
    LOW,      // 低质量，高性能
    MEDIUM,   // 中等质量，平衡性能
    HIGH,     // 高质量，较低性能
    ADAPTIVE  // 自适应质量
};

/**
 * @brief 资源感知编码器
 */
class ResourceAwareEncoder {
public:
    ResourceAwareEncoder();
    ~ResourceAwareEncoder();
    
    /**
     * @brief 根据系统资源调整编码参数
     * @param config 原始编码配置
     * @return 调整后的编码配置
     */
    EncoderConfig adjustEncodingBasedOnResources(const EncoderConfig& config);
    
    /**
     * @brief 设置编码质量策略
     * @param quality 质量等级
     */
    void setQualityStrategy(EncodingQuality quality);
    
    /**
     * @brief 获取当前编码质量等级
     * @return 编码质量等级
     */
    EncodingQuality getCurrentQuality() const;
    
private:
    /**
     * @brief 获取系统资源状态
     * @return SystemResources 系统资源信息
     */
    SystemResources getSystemResources();
    
    /**
     * @brief 根据CPU使用率调整帧率
     * @param currentFps 当前帧率
     * @param cpuUsage CPU使用率
     * @return 调整后的帧率
     */
    int adjustFpsBasedOnCpu(int currentFps, double cpuUsage);
    
    /**
     * @brief 根据内存使用率调整比特率
     * @param currentBitrate 当前比特率
     * @param memoryUsage 内存使用率
     * @return 调整后的比特率
     */
    int adjustBitrateBasedOnMemory(int currentBitrate, double memoryUsage);
    
    /**
     * @brief 根据磁盘空间调整文件分割大小
     * @param freeSpace 可用磁盘空间
     * @return 文件分割大小
     */
    uint64_t adjustSplitSizeBasedOnDisk(uint64_t freeSpace);
    
    EncodingQuality qualityStrategy;
    SystemResources lastResources;
};

#endif // RESOURCE_AWARE_ENCODER_H