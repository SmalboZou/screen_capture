# AIcp Windows Installer

这个目录包含了为AIcp屏幕录制工具创建Windows安装包的所有必要文件。

## 文件说明

- `installer_en.nsi` - NSIS安装脚本（英文版）
- `build_installer.bat` - 一键构建和打包脚本
- `create_installer_simple.bat` - 简化的打包脚本
- `AIcp_Setup_v1.0.0.exe` - 生成的安装包
- `deploy/` - 部署目录，包含程序和所有依赖项

## 系统要求

在生成安装包之前，请确保系统中已安装：

1. **Visual Studio 2019/2022** - 用于编译C++代码
2. **Qt 6.5.3 或更高版本** - Qt开发框架
3. **CMake 3.16 或更高版本** - 构建系统
4. **NSIS 3.x** - 安装包生成工具

## 快速开始

### 方法1：使用一键脚本（推荐）

```batch
build_installer.bat
```

这个脚本会：
1. 自动构建项目
2. 部署Qt依赖项
3. 复制资源文件
4. 生成安装包

### 方法2：手动步骤

1. **构建项目**
   ```batch
   mkdir build-release
   cd build-release
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
   cd ..
   ```

2. **创建部署目录**
   ```batch
   mkdir deploy
   copy build-release\Release\AIcp.exe deploy\
   ```

3. **部署Qt依赖项**
   ```batch
   windeployqt --release --no-translations deploy\AIcp.exe
   ```

4. **复制资源文件**
   ```batch
   xcopy /s resources deploy\resources\
   copy README.md deploy\
   copy LICENSE deploy\
   ```

5. **生成安装包**
   ```batch
   "C:\Program Files (x86)\NSIS\makensis.exe" installer_en.nsi
   ```

## 安装包特性

生成的安装包包含以下特性：

- ✅ 现代化的安装向导界面
- ✅ 自动检测和安装Qt运行时依赖项
- ✅ 创建开始菜单快捷方式
- ✅ 创建桌面快捷方式
- ✅ 注册表集成
- ✅ 完整的卸载支持
- ✅ 管理员权限请求
- ✅ 数字签名支持（可选）

## 测试

在生成安装包后，可以通过以下方式测试：

1. **测试部署版本**：直接运行 `deploy\AIcp.exe`
2. **测试安装包**：运行生成的 `AIcp_Setup_v1.0.0.exe`

## 故障排除

### 常见问题

1. **windeployqt 命令找不到**
   - 确保Qt的bin目录在PATH环境变量中
   - 或者修改脚本中的Qt安装路径

2. **NSIS 命令找不到**
   - 安装NSIS：`winget install NSIS.NSIS`
   - 确保NSIS安装目录在PATH中

3. **编译失败**
   - 检查Visual Studio安装
   - 确保Qt6已正确安装和配置

4. **程序运行时缺少DLL**
   - 重新运行windeployqt命令
   - 检查Qt安装是否完整

### 手动添加缺失的DLL

如果程序运行时提示缺少某些DLL，可以手动复制：

```batch
REM 从Qt安装目录复制缺失的DLL
copy "C:\Qt\6.5.3\msvc2019_64\bin\Qt6Core.dll" deploy\
copy "C:\Qt\6.5.3\msvc2019_64\bin\Qt6Gui.dll" deploy\
copy "C:\Qt\6.5.3\msvc2019_64\bin\Qt6Widgets.dll" deploy\
```

## 自定义

### 修改安装包信息

编辑 `installer_en.nsi` 文件中的以下变量：

```nsis
!define PRODUCT_NAME "AIcp"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "AIcp Team"
!define PRODUCT_WEB_SITE "https://github.com/SmalboZou/screen_capture"
```

### 添加数字签名

在NSIS脚本中添加：

```nsis
!finalize 'signtool.exe sign /f "certificate.pfx" /p "password" /t "http://timestamp.verisign.com/scripts/timstamp.dll" "%1"'
```

## 输出

成功运行后，你将得到：

- `AIcp_Setup_v1.0.0.exe` - 约15MB的安装包
- `deploy/` 目录 - 包含所有必要文件的便携版本

安装包可以在任何Windows 10/11系统上运行，无需预装Qt或其他依赖项。
