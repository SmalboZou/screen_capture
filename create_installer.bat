@echo off
echo ===================================
echo AIcp Windows 安装包生成脚本
echo ===================================
echo.

REM 设置变量
set PROJECT_ROOT=%~dp0
set BUILD_DIR=%PROJECT_ROOT%build-release
set RELEASE_DIR=%BUILD_DIR%\Release
set DEPLOY_DIR=%PROJECT_ROOT%deploy
set QT_DIR=C:\Qt\6.5.3\msvc2019_64

echo 项目根目录: %PROJECT_ROOT%
echo 构建目录: %BUILD_DIR%
echo 部署目录: %DEPLOY_DIR%
echo.

REM 检查是否存在可执行文件
if not exist "%RELEASE_DIR%\AIcp.exe" (
    echo 错误: 找不到 AIcp.exe 文件
    echo 请先运行 cmake --build . --config Release 编译项目
    pause
    exit /b 1
)

REM 创建部署目录
if exist "%DEPLOY_DIR%" (
    echo 清理旧的部署目录...
    rmdir /s /q "%DEPLOY_DIR%"
)
mkdir "%DEPLOY_DIR%"

REM 复制主程序
echo 复制主程序...
copy "%RELEASE_DIR%\AIcp.exe" "%DEPLOY_DIR%\"

REM 使用 windeployqt 部署 Qt 依赖项
echo 部署 Qt 运行时依赖项...
if exist "%QT_DIR%\bin\windeployqt.exe" (
    "%QT_DIR%\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-opengl-sw "%DEPLOY_DIR%\AIcp.exe"
) else (
    echo 警告: 找不到 windeployqt.exe
    echo 请确保 Qt 安装路径正确: %QT_DIR%
    echo 尝试从 PATH 中查找...
    windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw "%DEPLOY_DIR%\AIcp.exe"
    if errorlevel 1 (
        echo 错误: windeployqt 执行失败
        echo 请确保 Qt 的 bin 目录在 PATH 环境变量中
        pause
        exit /b 1
    )
)

REM 复制资源文件
echo 复制资源文件...
if exist "%PROJECT_ROOT%resources" (
    xcopy /s /y "%PROJECT_ROOT%resources" "%DEPLOY_DIR%\resources\"
)

REM 复制文档文件
echo 复制文档文件...
if exist "%PROJECT_ROOT%README.md" copy "%PROJECT_ROOT%README.md" "%DEPLOY_DIR%\"
if exist "%PROJECT_ROOT%LICENSE" copy "%PROJECT_ROOT%LICENSE" "%DEPLOY_DIR%\"

REM 检查是否安装了 NSIS
echo 检查 NSIS 安装...
where makensis >nul 2>&1
if errorlevel 1 (
    echo 错误: 找不到 NSIS (makensis.exe)
    echo 请下载并安装 NSIS: https://nsis.sourceforge.io/Download
    echo 确保 NSIS 的安装目录已添加到 PATH 环境变量中
    pause
    exit /b 1
)

REM 临时修改 NSIS 脚本中的路径
echo 准备 NSIS 脚本...
powershell -Command "(Get-Content '%PROJECT_ROOT%installer.nsi') -replace 'build-release\\Release\\AIcp.exe', 'deploy\\AIcp.exe' | Set-Content '%PROJECT_ROOT%installer_temp.nsi'"

REM 生成安装包
echo 生成安装包...
makensis "%PROJECT_ROOT%installer_temp.nsi"

REM 清理临时文件
del "%PROJECT_ROOT%installer_temp.nsi"

if exist "%PROJECT_ROOT%AIcp_Setup_v1.0.0.exe" (
    echo.
    echo ===================================
    echo 安装包生成成功!
    echo 文件位置: %PROJECT_ROOT%AIcp_Setup_v1.0.0.exe
    echo ===================================
    echo.
    echo 部署文件位置: %DEPLOY_DIR%
    echo 你可以直接运行部署目录中的程序进行测试
    echo.
) else (
    echo 错误: 安装包生成失败
    pause
    exit /b 1
)

pause
