// SimpleCapture.mm
// 简化的macOS屏幕捕获实现
#include "SimpleCapture.h"
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <iostream>

// 录制委托类
@interface RecordingDelegate : NSObject <AVCaptureFileOutputRecordingDelegate>
@end

@implementation RecordingDelegate
- (void)captureOutput:(AVCaptureFileOutput *)output 
    didStartRecordingToOutputFileAtURL:(NSURL *)fileURL 
    fromConnections:(NSArray<AVCaptureConnection *> *)connections {
    std::cout << "录制开始: " << [[fileURL path] UTF8String] << std::endl;
}

- (void)captureOutput:(AVCaptureFileOutput *)output 
    didFinishRecordingToOutputFileAtURL:(NSURL *)outputFileURL 
    fromConnections:(NSArray<AVCaptureConnection *> *)connections 
    error:(NSError *)error {
    if (error) {
        std::cerr << "录制错误: " << [[error localizedDescription] UTF8String] << std::endl;
    } else {
        std::cout << "录制完成: " << [[outputFileURL path] UTF8String] << std::endl;
    }
}
@end

class MacOSSimpleCapture : public SimpleCapture {
private:
    AVCaptureSession *captureSession = nil;
    AVCaptureScreenInput *screenInput = nil;
    AVCaptureMovieFileOutput *fileOutput = nil;
    bool isRecording = false;
    
public:
    MacOSSimpleCapture() {
        captureSession = [[AVCaptureSession alloc] init];
    }
    
    ~MacOSSimpleCapture() {
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
            
            NSURL *outputURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:outputPath.c_str()]];
            
            // 创建录制委托
            RecordingDelegate *delegate = [[RecordingDelegate alloc] init];
            
            [captureSession startRunning];
            [fileOutput startRecordingToOutputFileURL:outputURL recordingDelegate:delegate];
            
            isRecording = true;
            return true;
        }
    }
    
    bool stopCapture() override {
        @autoreleasepool {
            if (!isRecording) {
                return false;
            }
            
            [fileOutput stopRecording];
            [captureSession stopRunning];
            
            isRecording = false;
            return true;
        }
    }
    
    bool isCapturing() const override {
        return isRecording;
    }
    
    void setFrameRate(int fps) override {
        @autoreleasepool {
            if (screenInput) {
                screenInput.minFrameDuration = CMTimeMake(1, fps);
            }
        }
    }
    
    void setCaptureRegion(int x, int y, int width, int height) override {
        @autoreleasepool {
            if (screenInput) {
                screenInput.cropRect = CGRectMake(x, y, width, height);
            }
        }
    }
};

// 创建工厂函数
std::unique_ptr<SimpleCapture> createSimpleCapture() {
    return std::make_unique<MacOSSimpleCapture>();
}