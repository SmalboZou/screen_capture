# screen_capture
本地录屏软件
=======
>>>>>>> 7861f79 (screen_capture_0.0.1)
# AIcp# 屏幕录制程序

AIcp# 是一个纯本地运行的屏幕录制程序，专注于隐私保护和高性能。所有数据处理都在本地完成，无需网络连接。

## 特性

- **完全本地化**: 零网络依赖，所有数据处理在设备上完成
- **增强隐私保护**: 敏感操作本地处理，可选文件加密，权限透明化
- **资源高效**: 精简安装包，智能资源管理，硬件加速最大化
- **跨平台支持**: Windows、macOS、Linux支持
- **高性能**: 硬件加速编码，优化的内存管理

## 技术架构

### 分层设计

1. **用户界面层**: 基于Qt 6的跨平台GUI
2. **应用服务层**: 业务逻辑和服务管理
3. **核心引擎层**: 音视频处理和编码
4. **平台抽象层**: 屏幕捕获、音频捕获和硬件编码的平台特定实现

### 关键技术

- Qt 6.4 (GUI + 基础库)
- FFmpeg 6.0 (LGPL)
- 平台专用API (DXVA/NVENC/VideoToolbox)
- SpeexDSP + RNNoise (音频处理)
- SQLite3 (本地任务存储)

## 目录结构

```
AIcp#/
├── src/                   # 源代码目录
│   ├── platform/          # 平台抽象层实现
│   ├── core/              # 核心引擎层实现
│   ├── service/           # 应用服务层实现
│   ├── ui/                # 用户界面层实现
│   └── main.cpp           # 主程序入口
├── include/               # 公共头文件
├── assets/                # 资源文件（图标、图片等）
├── docs/                  # 项目文档
├── cmake/                 # CMake模块
├── tests/                 # 测试代码
├── CMakeLists.txt         # 构建配置文件
├── README.md              # 项目说明
└── LICENSE                # 许可证文件
```

## 安装

### Windows

下载并运行安装包：

```bash
AIcp_Setup_1.0.0_x64.exe
```

- 程序图标已美化，安装后桌面和开始菜单均有快捷方式。

### macOS

```bash
# 使用APP Bundle
AIcp#.app
```

### Linux

```bash
# 使用AppImage
AIcp#.AppImage
```

## 构建

### 环境要求

- C++17 或更高版本
- Qt 6.4 或更高版本
- CMake 3.16 或更高版本
- FFmpeg 6.0 开发库

### 构建步骤

```bash
# 克隆仓库
git clone https://github.com/yourname/AIcp#.git
cd AIcp#

# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 编译项目
make -j$(nproc)

# 运行程序
./AIcp#
```

## 使用说明

1. 启动AIcp#应用程序
2. 配置录制参数（区域、质量、格式等）
3. 点击"开始录制"按钮
4. 完成录制后点击"停止录制"按钮
5. 在文件浏览器中查看录制的文件

## 许可证

本项目采用 MIT 许可证。有关详细信息，请参阅 [LICENSE](LICENSE) 文件。

## 贡献

欢迎提交Issue和Pull Request来改进项目。

## 支持

如有问题，请联系项目维护者。
