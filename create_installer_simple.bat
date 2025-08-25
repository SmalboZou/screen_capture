@echo off
chcp 65001 >nul
echo ===================================
echo AIcp Windows Installer Generator
echo ===================================
echo.

set PROJECT_ROOT=%~dp0
set BUILD_DIR=%PROJECT_ROOT%build-release
set RELEASE_DIR=%BUILD_DIR%\Release
set DEPLOY_DIR=%PROJECT_ROOT%deploy

echo Project Root: %PROJECT_ROOT%
echo Build Directory: %BUILD_DIR%
echo Deploy Directory: %DEPLOY_DIR%
echo.

REM Check if executable exists
if not exist "%RELEASE_DIR%\AIcp.exe" (
    echo Error: AIcp.exe not found
    echo Please build the project first with: cmake --build . --config Release
    pause
    exit /b 1
)

REM Create deploy directory
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

REM Copy main executable
echo Copying main executable...
copy "%RELEASE_DIR%\AIcp.exe" "%DEPLOY_DIR%\"

REM Deploy Qt dependencies using windeployqt
echo Deploying Qt runtime dependencies...
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw "%DEPLOY_DIR%\AIcp.exe"
if errorlevel 1 (
    echo Error: windeployqt failed
    pause
    exit /b 1
)

REM Copy resource files
echo Copying resource files...
if exist "%PROJECT_ROOT%resources" (
    xcopy /s /y "%PROJECT_ROOT%resources" "%DEPLOY_DIR%\resources\"
)

REM Copy documentation files
echo Copying documentation files...
if exist "%PROJECT_ROOT%README.md" copy "%PROJECT_ROOT%README.md" "%DEPLOY_DIR%\"
if exist "%PROJECT_ROOT%LICENSE" copy "%PROJECT_ROOT%LICENSE" "%DEPLOY_DIR%\"

REM Create installer using NSIS
echo Generating installer...
set NSIS_PATH="C:\Program Files (x86)\NSIS\makensis.exe"
if exist %NSIS_PATH% (
    REM Create temporary NSIS script with correct paths
    powershell -Command "(Get-Content '%PROJECT_ROOT%installer.nsi') -replace 'build-release\\\\Release\\\\AIcp.exe', 'deploy\\AIcp.exe' | Set-Content '%PROJECT_ROOT%installer_temp.nsi'"
    
    REM Generate installer
    %NSIS_PATH% "%PROJECT_ROOT%installer_temp.nsi"
    
    REM Clean up temporary file
    del "%PROJECT_ROOT%installer_temp.nsi"
    
    if exist "%PROJECT_ROOT%AIcp_Setup_v1.0.0.exe" (
        echo.
        echo ===================================
        echo Installer generated successfully!
        echo Location: %PROJECT_ROOT%AIcp_Setup_v1.0.0.exe
        echo ===================================
        echo.
        echo Deploy files location: %DEPLOY_DIR%
        echo You can test the program directly from the deploy directory
        echo.
    ) else (
        echo Error: Installer generation failed
        pause
        exit /b 1
    )
) else (
    echo Error: NSIS not found at %NSIS_PATH%
    echo Please install NSIS from: https://nsis.sourceforge.io/Download
    pause
    exit /b 1
)

pause
