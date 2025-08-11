#ifndef VIDEO_PREPROCESSOR_H
#define VIDEO_PREPROCESSOR_H

#include "DataTypes.h"

/**
 * @brief 视频预处理器
 */
class VideoPreprocessor {
public:
    VideoPreprocessor() = default;
    ~VideoPreprocessor() = default;
    
    /**
     * @brief 色彩空间转换
     * @param frame 输入帧
     * @param targetFormat 目标格式
     * @return 转换后的帧
     */
    FrameData convertColorSpace(const FrameData& frame, PixelFormat targetFormat);
    
    /**
     * @brief 分辨率缩放
     * @param frame 输入帧
     * @param width 目标宽度
     * @param height 目标高度
     * @return 缩放后的帧
     */
    FrameData scaleFrame(const FrameData& frame, int width, int height);
    
    /**
     * @brief 叠加鼠标效果
     * @param frame 输入帧
     * @param mousePos 鼠标位置
     * @return 叠加后的帧
     */
    FrameData overlayMouseEffect(const FrameData& frame, const Point& mousePos);
    
private:
    /**
     * @brief RGB到YUV转换
     * @param rgbData RGB数据
     * @param width 宽度
     * @param height 高度
     * @return YUV数据
     */
    uint8_t* rgbToYuv(const uint8_t* rgbData, int width, int height);
    
    /**
     * @brief YUV到RGB转换
     * @param yuvData YUV数据
     * @param width 宽度
     * @param height 高度
     * @return RGB数据
     */
    uint8_t* yuvToRgb(const uint8_t* yuvData, int width, int height);
    
    /**
     * @brief 双线性插值缩放
     * @param data 输入数据
     * @param srcWidth 源宽度
     * @param srcHeight 源高度
     * @param dstWidth 目标宽度
     * @param dstHeight 目标高度
     * @return 缩放后的数据
     */
    uint8_t* bilinearScale(const uint8_t* data, int srcWidth, int srcHeight, int dstWidth, int dstHeight);
};

#endif // VIDEO_PREPROCESSOR_H