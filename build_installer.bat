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

REM Step 1: Build the project
echo Step 1: Building the project...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
cd "%PROJECT_ROOT%"

if not exist "%RELEASE_DIR%\AIcp.exe" (
    echo Error: Build failed - AIcp.exe not found
    pause
    exit /b 1
)

REM Step 2: Create deploy directory
echo Step 2: Creating deploy directory...
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

REM Step 3: Copy main executable
echo Step 3: Copying main executable...
copy "%RELEASE_DIR%\AIcp.exe" "%DEPLOY_DIR%\"

REM Step 4: Deploy Qt dependencies
echo Step 4: Deploying Qt dependencies...
windeployqt --release --no-translations "%DEPLOY_DIR%\AIcp.exe"

REM Step 5: Copy additional files
echo Step 5: Copying additional files...
xcopy /s /y "resources" "%DEPLOY_DIR%\resources\"
copy "README.md" "%DEPLOY_DIR%\"
copy "LICENSE" "%DEPLOY_DIR%\"

REM Step 6: Generate installer
echo Step 6: Generating installer...
"C:\Program Files (x86)\NSIS\makensis.exe" installer_en.nsi

if exist "%PROJECT_ROOT%AIcp_Setup_v1.0.1.exe" (
    echo.
    echo ===================================
    echo SUCCESS! Installer generated successfully!
    echo ===================================
    echo Installer: "%PROJECT_ROOT%AIcp_Setup_v1.0.1.exe"
    echo Size:
    dir "%PROJECT_ROOT%AIcp_Setup_v1.0.1.exe" | find "AIcp_Setup"
    echo.
    echo Deploy directory: "%DEPLOY_DIR%"
    echo You can test the program directly from the deploy directory.
    echo.
) else (
    echo Error: Installer generation failed
    pause
    exit /b 1
)

pause
