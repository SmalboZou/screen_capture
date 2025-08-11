#ifndef ILOCAL_ENCODER_H
#define ILOCAL_ENCODER_H

#include <string>
#include <cstdint>

// 前向声明
struct FrameData;
struct EncoderConfig;
struct EncodedData;

/**
 * @brief 本地编码器接口
 */
class ILocalEncoder {
public:
    virtual ~ILocalEncoder() = default;
    
    /**
     * @brief 设置编码器配置
     * @param config 编码配置
     * @return true 成功, false 失败
     */
    virtual bool setup(const EncoderConfig& config) = 0;
    
    /**
     * @brief 编码一帧数据
     * @param frame 输入帧数据
     * @return EncodedData 编码后的数据
     */
    virtual EncodedData encode(const FrameData& frame) = 0;
    
    /**
     * @brief 完成编码并保存文件
     * @param outputPath 输出文件路径
     * @return true 成功, false 失败
     */
    virtual bool finalize(const std::string& outputPath) = 0;
};

#endif // ILOCAL_ENCODER_H