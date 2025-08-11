# 在Mac（Apple Silicon）和macOS 15上构建和运行AIcp#

## 系统要求

- **操作系统**: macOS 12.0 (Monterey) 或更高版本，推荐macOS 15
- **处理器**: Apple Silicon (M1, M2, M3系列)
- **内存**: 至少8GB RAM，推荐16GB或更多
- **存储空间**: 至少2GB可用磁盘空间

## 必需软件和依赖项

### 1. Xcode Command Line Tools
```bash
# 安装Xcode命令行工具
xcode-select --install
```

### 2. Homebrew (包管理器)
```bash
# 如果尚未安装Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 3. Qt 6
```bash
# 使用Homebrew安装Qt 6
brew install qt@6

# 或者从Qt官方网站下载安装程序
# https://www.qt.io/download
```

### 4. CMake
```bash
# 使用Homebrew安装CMake
brew install cmake
```

### 5. FFmpeg
```bash
# 使用Homebrew安装FFmpeg
brew install ffmpeg
```

## 环境变量设置

在构建之前，您可能需要设置一些环境变量。将以下内容添加到您的shell配置文件（~/.zshrc或~/.bash_profile）中：

```bash
# Qt 6路径（如果通过Homebrew安装）
export QT6_DIR="/opt/homebrew/opt/qt@6"
export PATH="$QT6_DIR/bin:$PATH"

# FFmpeg路径（如果通过Homebrew安装）
export FFMPEG_ROOT="/opt/homebrew/opt/ffmpeg"
export PKG_CONFIG_PATH="$FFMPEG_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH"
```

然后重新加载您的shell配置：
```bash
source ~/.zshrc
# 或
source ~/.bash_profile
```

## 构建步骤

### 1. 克隆或获取项目源代码
```bash
# 如果您还没有项目源代码，请克隆仓库
git clone https://github.com/yourname/AIcp#.git
cd AIcp#
```

### 2. 创建构建目录
```bash
mkdir build
cd build
```

### 3. 配置项目
```bash
# 对于Apple Silicon Mac，确保使用正确的架构
cmake .. -DCMAKE_OSX_ARCHITECTURES=arm64
```

如果您手动安装了Qt而不是通过Homebrew：
```bash
cmake .. \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_PREFIX_PATH=/path/to/your/qt6/installation
```

### 4. 编译项目
```bash
# 使用所有可用核心进行编译
make -j$(sysctl -n hw.ncpu)
```

或者使用Ninja（如果已安装）：
```bash
# 首先安装Ninja
brew install ninja

# 配置时指定生成器
cmake .. -GNinja -DCMAKE_OSX_ARCHITECTURES=arm64

# 编译
ninja
```

### 5. 构建应用包（可选）
```bash
# 创建.app bundle
make install

# 或者使用macdeployqt工具打包（如果安装了Qt）
macdeployqt bin/AIcp#.app -dmg
```

## 运行程序

### 方法1：直接运行可执行文件
```bash
# 进入构建目录
cd build

# 运行程序
./bin/AIcp#
```

### 方法2：运行.app bundle（如果已创建）
```bash
# 打开应用程序
open bin/AIcp#.app
```

## 权限设置

首次运行时，您可能需要授予屏幕录制和麦克风访问权限：

1. 打开**系统设置** > **隐私与安全性**
2. 在左侧选择**屏幕录制**，然后添加AIcp#应用
3. 在左侧选择**麦克风**，然后添加AIcp#应用（如果需要录制音频）

## 故障排除

### 1. Qt库未找到
如果遇到Qt库未找到的错误：
```bash
# 确认Qt已正确安装
brew list qt@6

# 检查Qt路径
echo $QT6_DIR

# 重新配置CMake
cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6
```

### 2. FFmpeg库未找到
如果遇到FFmpeg库未找到的错误：
```bash
# 确认FFmpeg已正确安装
brew list ffmpeg

# 检查FFmpeg路径
echo $FFMPEG_ROOT

# 重新配置CMake
cmake .. -DFFMPEG_ROOT=/opt/homebrew/opt/ffmpeg
```

### 3. 架构不匹配
如果遇到架构不匹配的问题：
```bash
# 确保只针对ARM64架构构建
cmake .. -DCMAKE_OSX_ARCHITECTURES=arm64
```

### 4. 运行时错误
如果程序崩溃或出现运行时错误：
```bash
# 检查系统日志
log show --predicate 'subsystem == "org.aicp"' --last 1h

# 或者使用Console.app查看应用日志
```

## 性能优化建议

### 1. 利用Apple Silicon的优势
- 确保所有依赖项都是为ARM64架构编译的
- 启用Metal支持以获得更好的图形性能

### 2. 内存管理
- macOS在Apple Silicon上有统一内存架构，合理利用内存可以提高性能
- 避免不必要的内存分配和释放

### 3. 硬件加速
- 确保启用了VideoToolbox硬件编码
- 利用Neural Engine进行AI增强功能（如果有）

## 开发者注意事项

### 1. Universal Binaries
如果您需要构建支持Intel和Apple Silicon的通用二进制文件：
```bash
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

### 2. 代码签名
对于分发应用，您需要对其进行代码签名：
```bash
codesign --force --deep --sign "Developer ID Application: YOUR_NAME" AIcp#.app
```

### 3. Notarization
为了在macOS Catalina及以上版本上顺利运行，应用需要经过公证：
```bash
xcrun notarytool submit AIcp#.dmg --keychain-profile "YOUR_PROFILE"
```

## 常见问题解答

### Q: 为什么我的应用无法访问屏幕？
A: 您需要在系统偏好设置中授权屏幕录制权限。转到**系统设置** > **隐私与安全性** > **屏幕录制**，然后添加AIcp#应用。

### Q: 音频录制不工作怎么办？
A: 确保已在**系统设置** > **隐私与安全性** > **麦克风**中授权应用访问麦克风。

### Q: 构建过程中出现"找不到Qt库"错误怎么办？
A: 确保已正确安装Qt并通过CMAKE_PREFIX_PATH指定其路径。如果是通过Homebrew安装的Qt，路径通常是/opt/homebrew/opt/qt@6。

### Q: 如何调试应用崩溃问题？
A: 使用Xcode的调试工具或者在终端中运行应用以查看控制台输出。也可以查看系统日志获取更多信息。

## 更新日志

### v1.0.0
- 初始版本支持
- Apple Silicon原生支持
- macOS 15兼容性验证

---

通过遵循本指南，您应该能够在Apple Silicon Mac和macOS 15上成功构建和运行AIcp#屏幕录制程序。如果遇到任何问题，请参考故障排除部分或查看相关依赖项的官方文档。