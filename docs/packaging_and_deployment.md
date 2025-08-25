# AIcp 打包与部署指南

本档汇总在常见平台（Windows / macOS / Linux）上构建、打包、安装与分发 `AIcp` 的步骤与常用命令。包含针对麒麟（Kylin）ARM64 的注意事项与交叉编译建议。

## 快速前提
- 源码位于项目根目录（包含 `CMakeLists.txt`）。
- 项目已使用 CMake 管理构建（已支持 Qt6 ↔ Qt5 回退，以及 CPack DEB 配置）。
- 运行时依赖：Qt（Qt6 或 Qt5）、ffmpeg。打包时需确保目标机安装相应库或把依赖打包/静态化。

---

## 通用构建步骤（适用于本仓库）
1. 创建干净的构建目录并配置：

```bash
# 在 Linux / macOS
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

```powershell
# 在 Windows PowerShell
Remove-Item -Recurse -Force .\build -ErrorAction SilentlyContinue
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

2. 编译：

```bash
cmake --build build -j$(nproc)
```

在 Windows 上使用 Visual Studio 生成器：

```powershell
cmake --build build --config Release
```

3. 运行单元 / 简单 smoke 测试：直接运行目标二进制 `build/AIcp` 或 `build/Release/AIcp.exe`。

---

## Linux（Deb / Kylin ARM64）
本仓库已在 `CMakeLists.txt` 中集成了 CPack（DEB）配置，默认会安装：可执行文件、desktop 文件、图标和 LICENSE。

### 在目标（麒麟 ARM64）机器上本地构建（推荐）
1. 安装依赖（若系统提供 Qt5 而非 Qt6，可使用 Qt5）：

```bash
sudo apt update
# 对于 Qt5
sudo apt install -y build-essential cmake qtbase5-dev qtbase5-dev-tools qt5-qmake ffmpeg libgl1-mesa-dev
# 或者 若使用 Qt6(存在时):
# sudo apt install -y qt6-base-dev ffmpeg
```

2. 构建并打包：

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
cd build
cpack    # 生成 .deb
```

3. 安装 deb：

```bash
sudo dpkg -i aicp_1.0.0_arm64.deb
# 若有依赖问题：
sudo apt -f install
```

4. 运行：

```bash
AIcp   # or /usr/bin/AIcp
```

### 在 x86_64 主机交叉编译 arm64（可选）
- 要点：需要 aarch64 的交叉编译器与对应目标的 Qt（或交叉编译的 Qt）。推荐在真正的 arm64 机器或在 QEMU 中构建以避免复杂依赖问题。

简单交叉工具链示例（Ubuntu）：

```bash
sudo dpkg --add-architecture arm64
sudo apt update
sudo apt install -y g++-aarch64-linux-gnu cmake
# 需要对应的 Qt lib/headers for aarch64（较难），通常建议使用 aarch64 chroot 或 qemu 用户模式
```

更实际的做法：使用 Docker + QEMU 或在 arm64 CI 机器上直接构建。

### 生成 AppImage（替代 deb，减少目标依赖）
1. 安装 `linuxdeployqt` 或 `appimagetool` 工具链。
2. 构建后运行 `linuxdeployqt` 将 Qt 库打包到 AppDir，再转为 AppImage。

概览命令：

```bash
# 假设 build/AIcp 已生成
linuxdeployqt ./build/AIcp -appimage
```

说明：不同发行版可能需要额外设置 `QMAKE` 或 `LD_LIBRARY_PATH`。

---

## Windows（安装程序 NSIS）
仓库中已有 `installer_en.nsi`、`build_installer.bat` 等脚本用于生成 Windows 安装包（使用 NSIS）。

### 在 Windows 上构建与打包
1. 安装依赖：Visual Studio (MSVC) 或 MSVC 工具链、Qt for Windows、CMake、NSIS
2. 生成 Visual Studio 解决方案并编译：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

3. 构建安装程序（示例仓库脚本）：

```powershell
.
\build_installer.bat
```

这会生成 `AIcp_Setup_v1.0.0.exe`（基于仓库内脚本与 NSIS 配置）。

---

## macOS（App bundle / DMG / notarize）
### 本机构建
1. 安装 Qt for macOS（或使用 Homebrew 的 Qt）和 CMake
2. 构建：

```bash
mkdir build && cd build
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

3. 生成 .app bundle（CMake config 中已设置 `MACOSX_BUNDLE`）

4. 可使用 `cpack` 或 `create_installer` 脚本生成 DMG/installer。

### 签名与 notarize
- 若希望在新版 macOS 上无警告，需要对 `.app` 签名并通过 Apple notarization；这需要 Apple Developer 账号与 `altool` / `notarytool`。

---

## WSL2 / Docker 构建（若本机缺少目标环境）
- 在 Windows 下，可使用 WSL2 的 Ubuntu 环境来构建 Linux 包。推荐在 WSL2 里执行 Linux 那节的命令。
- 使用 Docker + QEMU：创建 arm64 容器进行构建，或使用 `multiarch/qemu-user-static` 启用 QEMU，然后运行 arm64 chroot。

示例（在主机上使用 Docker 构建 x86_64）：

```bash
docker run --rm -v $(pwd):/src -w /src ubuntu:22.04 bash -lc "apt update && apt install -y build-essential cmake qtbase5-dev ffmpeg && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build . -j$(nproc)"
```

注意：Qt6/Qt5 的包名和可用性依赖容器发行版与仓库源。

---

## 典型问题与排查
- rcc/moc 生成失败（路径含有 `#`, 空格或括号）
  - 解决：项目路径应避免 `# ( )` 等特殊字符；若已出现，移到不含特殊字符的路径并重新构建。

- 打包后桌面图标/emoji 显示为方框或叉
  - 原因：目标系统缺少彩色 emoji 字体或 Qt 版本较旧（如 Qt 5.12）。
  - 解决：
    - 在程序内使用位图图标（已在 `resources/` 中添加可用图标占位），或
    - 在目标系统安装 emoji 字体：`sudo apt install fonts-noto-color-emoji` 并运行 `fc-cache -fv`，或
    - 将界面文本替换为纯文字（已在代码中做兼容处理）。

- deb 安装提示依赖不满足
  - 检查 `dpkg -I aicp_*.deb` 的依赖字段或 `dpkg -I`，确认包中 `Depends`，必要时调整 `CMakeLists.txt` 中 `CPACK_DEBIAN_PACKAGE_DEPENDS`。

- 运行时报错找不到 Qt 库
  - 若使用系统 Qt 包（apt 安装），系统应提供库；若使用自带 Qt，请确保运行端能找到 Qt 动态库（`LD_LIBRARY_PATH` 或把库打包到 AppDir / installer 中）。

---

## 常用命令汇总
- 生成构建文件：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

- 编译：

```bash
cmake --build build -j$(nproc)
```

- 使用 CPack 生成 deb：

```bash
cd build && cpack
# 或 cmake --build build --target package
```

- 安装 deb：

```bash
sudo dpkg -i aicp_1.0.0_*.deb
sudo apt -f install
```

- 在 Windows 上使用 Visual Studio 构建与 NSIS 打包：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build_installer.bat
```

- 在 macOS 构建 .app：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
# 生成的 bundle 在 build/ 下
```

---

## 附：为麒麟 ARM64 做特别说明
- 默认仓库可能只带 Qt5.12（例：`5.12.12+dfsg`），因此：
  - 我们在 `CMakeLists.txt` 中已经做了 Qt6→Qt5 回退，并把 DEB 的 Qt5 依赖降到 `>=5.12`。如果目标系统更旧，请调整或移除版本号。
  - 若希望使用 Qt6，推荐通过 `aqtinstall` 下载官方 Qt 或在目标机上用官方 SDK。

示例：使用 `aqtinstall` 获取 Qt6 并用于 CMake：

```bash
pip install aqtinstall
mkdir -p $HOME/Qt
aqt install-qt linux desktop 6.6.3 gcc_64 -O $HOME/Qt
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$HOME/Qt/6.6.3/gcc_64
```

---

## 我可以为你做的后续工作（选项）
- 在 `docs/` 下生成具体的 `build_deb.sh`、`build_win.ps1`、`build_mac.sh` 脚本并测试（在可用环境中）。
- 把 `resources/icons/*.png` 换成你提供的真实图标并自动在 UI 中显示。 
- 生成 AppImage 打包流程脚本并调试。

如果你想让我直接创建构建脚本或把图标嵌入 UI，请告诉我你希望生成哪个脚本或把图标文件上传到仓库路径，我会继续完成并测试。
