# AIcp 视频内容总结功能使用指南

## 功能概述

AIcp 现在支持自动生成录制视频的内容总结功能。系统会：

1. **提取视频帧**：从录制的视频中每0.5秒提取一张关键帧
2. **AI分析**：使用配置的视觉模型分析每帧内容
3. **生成总结**：整合所有帧的描述，生成完整的视频内容总结

## 使用步骤

### 1. 安装依赖

视频帧提取功能需要 FFmpeg：

```bash
# 运行安装脚本
./install_ffmpeg.bat

# 或者手动安装 FFmpeg 并添加到系统 PATH
```

### 2. 配置AI模型

1. 勾选 "启用视频内容总结"
2. 点击 "配置AI模型" 按钮
3. 选择模型提供商并填写配置：

#### OpenAI 配置示例：
- **提供商**: OpenAI
- **Base URL**: https://api.openai.com/v1
- **API Key**: 您的OpenAI API密钥
- **模型**: gpt-4-vision-preview 或 gpt-4o

#### 硅基流动配置示例：
- **提供商**: 硅基流动 (SiliconFlow)
- **Base URL**: https://api.siliconflow.cn/v1
- **API Key**: 您的硅基流动API密钥
- **模型**: OpenGVLab/InternVL2-26B

#### 智谱AI配置示例：
- **提供商**: 智谱AI (GLM)
- **Base URL**: https://open.bigmodel.cn/api/paas/v4
- **API Key**: 您的智谱AI API密钥
- **模型**: glm-4v-plus

### 3. 测试连接

配置完成后，点击 "测试连接" 确保配置正确。

### 4. 开始录制

1. 设置录制参数（路径、文件名、帧率等）
2. 勾选 "启用视频内容总结"
3. 开始录制
4. 录制完成后，系统会自动开始视频内容分析

## 功能特点

- **智能帧提取**：根据视频帧率自动调整提取间隔
- **多模型支持**：支持OpenAI、硅基流动、智谱AI等多种视觉模型
- **实时进度**：显示帧提取和AI分析的实时进度
- **自动保存**：总结内容会自动保存为txt文件
- **错误处理**：完善的错误处理和重试机制

## 处理流程

1. **录制完成** → 触发视频总结流程
2. **提取视频帧** → 每0.5秒提取一张图片
3. **AI分析帧内容** → 逐个分析图片内容
4. **生成最终总结** → 整合所有描述生成总结
5. **显示和保存** → 在界面显示并保存到文件

## 注意事项

- 确保网络连接稳定，AI分析需要访问在线API
- 视频时长较长时，分析过程可能需要几分钟
- API调用会产生费用，请根据需要使用
- 支持的视频格式：.mov, .mp4 等常见格式

## 故障排除

### FFmpeg未找到
- 确保已安装FFmpeg并添加到系统PATH
- 或将ffmpeg.exe放到程序的tools目录下

### API调用失败
- 检查网络连接
- 验证API密钥是否正确
- 确认API账户有足够余额

### 分析失败
- 检查视频文件是否完整
- 确认模型支持视觉分析功能
- 查看错误日志了解具体原因

## 支持的模型

### OpenAI
- gpt-4-vision-preview
- gpt-4o
- gpt-4o-mini

### 硅基流动
- OpenGVLab/InternVL2-26B
- OpenGVLab/InternVL2-Llama3-76B
- deepseek-ai/deepseek-vl-7b-chat

### 智谱AI
- glm-4v-plus
- glm-4v
- glm-4v-flash

### 月之暗面
- moonshot-v1-8k (文本模型，用于总结)
- moonshot-v1-32k
- moonshot-v1-128k
