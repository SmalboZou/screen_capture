#ifndef AV_SYNCER_H
#define AV_SYNCER_H

#include "DataTypes.h"
#include <queue>
#include <vector>

/**
 * @brief 音视频同步器
 */
class AVSyncer {
public:
    AVSyncer();
    ~AVSyncer();
    
    /**
     * @brief 同步音视频数据
     * @param videoFrames 视频帧队列
     * @param audioFrames 音频帧队列
     * @return 同步后的媒体包列表
     */
    std::vector<MediaPacket> sync(
        const std::queue<FrameData>& videoFrames,
        const std::queue<AudioData>& audioFrames);
    
    /**
     * @brief 设置视频时间基准
     * @param timeBase 时间基准
     */
    void setVideoTimeBase(double timeBase);
    
    /**
     * @brief 设置音频时间基准
     * @param timeBase 时间基准
     */
    void setAudioTimeBase(double timeBase);
    
private:
    /**
     * @brief 计算时间戳差值
     * @param ts1 时间戳1
     * @param ts2 时间戳2
     * @return 时间差值
     */
    double calculateTimeDifference(uint64_t ts1, uint64_t ts2);
    
    /**
     * @brief 将帧数据转换为媒体包
     * @param frame 帧数据
     * @param isKeyFrame 是否关键帧
     * @return 媒体包
     */
    MediaPacket frameToPacket(const FrameData& frame, bool isKeyFrame);
    
    /**
     * @brief 将音频数据转换为媒体包
     * @param audio 音频数据
     * @return 媒体包
     */
    MediaPacket audioToPacket(const AudioData& audio);
    
    double videoTimeBase;
    double audioTimeBase;
};

#endif // AV_SYNCER_H