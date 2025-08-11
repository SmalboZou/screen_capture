// MacOSAudioCapture.mm
// macOS音频捕获实现
#include "IAudioCapture.h"
#include "DataTypes.h"
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#include <iostream>

class MacOSAudioCapture : public IAudioCapture {
private:
    AVCaptureSession *audioSession = nil;
    AVCaptureDeviceInput *audioInput = nil;
    AVCaptureAudioDataOutput *audioOutput = nil;
    bool isCapturing = false;
    std::function<void(const AudioFrame&)> frameCallback;
    
    // 音频输出队列
    dispatch_queue_t audioQueue;
    
public:
    MacOSCapture() {
        audioSession = [[AVCaptureSession alloc] init];
        audioQueue = dispatch_queue_create("audioQueue", DISPATCH_QUEUE_SERIAL);
    }
    
    ~MacOSAudioCapture() {
        [audioSession release];
        dispatch_release(audioQueue);
    }
    
    bool initialize() override {
        @autoreleasepool {
            // 获取默认音频输入设备
            AVCaptureDevice *audioDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
            if (!audioDevice) {
                std::cerr << "无法获取音频设备" << std::endl;
                return false;
            }
            
            // 创建音频输入
            NSError *error = nil;
            audioInput = [[AVCaptureDeviceInput alloc] initWithDevice:audioDevice error:&error];
            if (error) {
                std::cerr << "音频输入创建失败: " << [[error localizedDescription] UTF8String] << std::endl;
                return false;
            }
            
            // 添加音频输入
            if ([audioSession canAddInput:audioInput]) {
                [audioSession addInput:audioInput];
            } else {
                std::cerr << "无法添加音频输入" << std::endl;
                return false;
            }
            
            // 创建音频输出
            audioOutput = [[AVCaptureAudioDataOutput alloc] init];
            [audioOutput setSampleBufferDelegate:self queue:audioQueue];
            
            if ([audioSession canAddOutput:audioOutput]) {
                [audioSession addOutput:audioOutput];
            } else {
                std::cerr << "无法添加音频输出" << std::endl;
                return false;
            }
            
            return true;
        }
    }
    
    bool startCapture() override {
        @autoreleasepool {
            if (isCapturing) {
                return false;
            }
            
            [audioSession startRunning];
            isCapturing = true;
            std::cout << "开始音频捕获" << std::endl;
            return true;
        }
    }
    
    bool stopCapture() override {
        @autoreleasepool {
            if (!isCapturing) {
                return false;
            }
            
            [audioSession stopRunning];
            isCapturing = false;
            std::cout << "停止音频捕获" << std::endl;
            return true;
        }
    }
    
    bool isCapturingAudio() const override {
        return isCapturing;
    }
    
    void setAudioCallback(std::function<void(const AudioFrame&)> callback) override {
        frameCallback = callback;
    }
    
    void setSampleRate(int sampleRate) override {
        // 设置采样率
    }
    
    void setChannels(int channels) override {
        // 设置声道数
    }
    
    // 音频数据回调
    - (void)captureOutput:(AVCaptureOutput *)output 
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer 
        fromConnection:(AVCaptureConnection *)connection {
        
        if (!frameCallback) return;
        
        // 处理音频数据
        AudioFrame frame;
        frame.data = nullptr;
        frame.size = 0;
        frame.timestamp = 0;
        frame.sampleRate = 44100;
        frame.channels = 2;
        
        // 调用回调
        frameCallback(frame);
    }
};

// 创建工厂函数
std::unique_ptr<IAudioCapture> createAudioCapture() {
    return std::make_unique<MacOSAudioCapture>();
}