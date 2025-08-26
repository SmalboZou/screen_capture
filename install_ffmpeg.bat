@echo off
echo Installing FFmpeg for AIcp video summary feature...
echo.

REM 创建tools目录
if not exist "tools" mkdir tools
cd tools

REM 检查是否已存在FFmpeg
if exist "ffmpeg.exe" (
    echo FFmpeg is already installed.
    pause
    exit /b 0
)

echo Downloading FFmpeg...
echo Please download FFmpeg from: https://ffmpeg.org/download.html
echo.
echo 1. Download the Windows executable from the official website
echo 2. Extract the files and copy ffmpeg.exe to the tools folder
echo 3. Or download directly from: https://www.gyan.dev/ffmpeg/builds/
echo.
echo Alternative: You can also install FFmpeg system-wide and add it to PATH
echo.

pause
