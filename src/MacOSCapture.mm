// MacOSCapture.mm
// macOS屏幕捕获实现
#include "ILocalCapture.h"
#include "DataTypes.h"
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <iostream>

class MacOSCapture : public ILocalCapture {
private:
    AVCaptureSession *captureSession = nil;
    AVCaptureScreenInput *screenInput = nil;
    AVCaptureMovieFileOutput *fileOutput = nil;
    bool isRecording = false;
    std::string outputPath;
    
public:
    MacOSCapture() {
        // 初始化AVFoundation
        captureSession = [[AVCaptureSession alloc] init];
    }
    
    ~MacOSCapture() {
        if (isRecording) {
            stopCapture();
        }
        captureSession = nil;
    }
    
    bool init() override {
        @autoreleasepool {
            // 获取屏幕输入
            screenInput = [[AVCaptureScreenInput alloc] initWithDisplayID:CGMainDisplayID()];
            if (!screenInput) {
                std::cerr << "无法创建屏幕输入" << std::endl;
                return false;
            }
            
            // 配置输入
            screenInput.capturesMouseClicks = true;
            screenInput.capturesCursor = true;
            
            // 添加输入到会话
            if ([captureSession canAddInput:screenInput]) {
                [captureSession addInput:screenInput];
            } else {
                std::cerr << "无法添加屏幕输入" << std::endl;
                return false;
            }
            
            // 创建文件输出
            fileOutput = [[AVCaptureMovieFileOutput alloc] init];
            if ([captureSession canAddOutput:fileOutput]) {
                [captureSession addOutput:fileOutput];
            } else {
                std::cerr << "无法添加文件输出" << std::endl;
                return false;
            }
            
            return true;
        }
    }
    
    bool startCapture(const std::string& outputPath) override {
        @autoreleasepool {
            if (isRecording) {
                std::cerr << "已经在录制中" << std::endl;
                return false;
            }
            
            this->outputPath = outputPath;
            
            NSURL *outputURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:outputPath.c_str()]];
            
            [captureSession startRunning];
            [fileOutput startRecordingToOutputFileURL:outputURL recordingDelegate:nil];
            
            isRecording = true;
            std::cout << "开始屏幕录制: " << outputPath << std::endl;
            return true;
        }
    }
    
    bool stopCapture() override {
        @autoreleasepool {
            if (!isRecording) {
                std::cerr << "没有在录制" << std::endl;
                return false;
            }
            
            [fileOutput stopRecording];
            [captureSession stopRunning];
            
            isRecording = false;
            std::cout << "停止屏幕录制" << std::endl;
            return true;
        }
    }
    
    bool isCapturing() const override {
        return isRecording;
    }
    
    void setCaptureRegion(int x, int y, int width, int height) override {
        @autoreleasepool {
            if (screenInput) {
                screenInput.cropRect = CGRectMake(x, y, width, height);
            }
        }
    }
    
    void setFrameRate(int fps) override {
        @autoreleasepool {
            if (screenInput) {
                screenInput.minFrameDuration = CMTimeMake(1, fps);
            }
        }
    }
    
    void setQuality(VideoQuality quality) override {
        // 设置质量参数
        switch (quality) {
            case VideoQuality::LOW:
                break;
            case VideoQuality::MEDIUM:
                break;
            case VideoQuality::HIGH:
                break;
            case VideoQuality::ULTRA:
                break;
        }
    }
};

// 创建工厂函数
std::unique_ptr<ILocalCapture> createLocalCapture() {
    return std::make_unique<MacOSCapture>();
}