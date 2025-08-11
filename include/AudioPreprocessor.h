#ifndef AUDIO_PREPROCESSOR_H
#define AUDIO_PREPROCESSOR_H

#include "DataTypes.h"
#include <vector>

/**
 * @brief 音频预处理器
 */
class AudioPreprocessor {
public:
    AudioPreprocessor() = default;
    ~AudioPreprocessor() = default;
    
    /**
     * @brief 重采样
     * @param audio 输入音频数据
     * @param targetSampleRate 目标采样率
     * @return 重采样后的音频数据
     */
    AudioData resample(const AudioData& audio, int targetSampleRate);
    
    /**
     * @brief 混音处理
     * @param audios 多路音频输入
     * @return 混音后的音频数据
     */
    AudioData mix(const std::vector<AudioData>& audios);
    
    /**
     * @brief 降噪处理
     * @param audio 输入音频数据
     * @return 降噪后的音频数据
     */
    AudioData denoise(const AudioData& audio);
    
private:
    /**
     * @brief 线性插值重采样
     * @param data 输入数据
     * @param srcRate 源采样率
     * @param dstRate 目标采样率
     * @param numSamples 样本数量
     * @return 重采样后的数据
     */
    uint8_t* linearResample(const uint8_t* data, int srcRate, int dstRate, size_t numSamples);
    
    /**
     * @brief 简单混音算法
     * @param samples1 第一路音频样本
     * @param samples2 第二路音频样本
     * @param numSamples 样本数量
     * @return 混音后的样本
     */
    int16_t* simpleMix(const int16_t* samples1, const int16_t* samples2, size_t numSamples);
    
    /**
     * @brief 简单降噪算法
     * @param samples 音频样本
     * @param numSamples 样本数量
     * @param threshold 阈值
     * @return 降噪后的样本
     */
    int16_t* simpleDenoise(const int16_t* samples, size_t numSamples, int16_t threshold);
};

#endif // AUDIO_PREPROCESSOR_H