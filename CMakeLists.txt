cmake_minimum_required(VERSION 3.16)

project(AIcp VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt6包
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Network)

# 平台特定：仅在 macOS 查找系统框架
if(APPLE)
    find_library(AVFOUNDATION_LIBRARY AVFoundation)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    find_library(COREMEDIA_LIBRARY CoreMedia)
endif()

# 设置Qt元对象系统
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 基础源文件（跨平台）
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/AISummaryConfigDialog.cpp
    src/AISummaryConfigDialog.h
    src/VideoFrameExtractor.cpp
    src/VideoFrameExtractor.h
    src/AIVisionAnalyzer.cpp
    src/AIVisionAnalyzer.h
    src/VideoSummaryManager.cpp
    src/VideoSummaryManager.h
    include/SimpleCapture.h
    resources/resources.qrc
)

# 平台特定源文件
if(APPLE)
    list(APPEND SOURCES
        src/SimpleCapture.mm
        src/SimpleCapture.h
    )
elseif(WIN32)
    list(APPEND SOURCES
        src/SimpleCapture_win.cpp
    )
endif()

# 创建可执行文件（在 Windows 使用 WIN32 子系统隐藏控制台）
if(WIN32)
    add_executable(AIcp WIN32 ${SOURCES})
else()
    add_executable(AIcp ${SOURCES})
endif()

# 链接Qt6库和平台库
target_link_libraries(AIcp 
    PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Network
)

if(APPLE)
    target_link_libraries(AIcp PRIVATE
        ${AVFOUNDATION_LIBRARY}
        ${COCOA_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${COREMEDIA_LIBRARY}
    )
endif()

# 包含头文件目录
target_include_directories(AIcp PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# 设置Objective-C++编译标志（仅 macOS 生效）
if(APPLE)
    set_source_files_properties(src/SimpleCapture.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
endif()

# 平台特定定义
if(APPLE)
    target_compile_definitions(AIcp PRIVATE PLATFORM_MACOS)
elseif(WIN32)
    target_compile_definitions(AIcp PRIVATE PLATFORM_WINDOWS)
endif()

# macOS应用程序包配置
if(APPLE)
    set_target_properties(AIcp PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "AIcp"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.aipcp.screenrecorder"
        MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
    )
    # 设置应用程序图标
    set(ICON_FILE "${CMAKE_SOURCE_DIR}/assets/AppIcon.icns")
    if(EXISTS ${ICON_FILE})
        target_sources(AIcp PRIVATE ${ICON_FILE})
        set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    endif()
endif()

# Windows 资源：应用图标
if(WIN32)
    set(APP_ICON_RC ${CMAKE_CURRENT_SOURCE_DIR}/resources/app_icon.rc)
    set(APP_ICON_ICO ${CMAKE_CURRENT_SOURCE_DIR}/resources/app.ico)
    if(EXISTS ${APP_ICON_ICO})
        # 生成 .rc 内容（如果缺失）
        file(WRITE ${APP_ICON_RC} "IDI_ICON1 ICON \"${APP_ICON_ICO}\"\n")
        list(APPEND SOURCES ${APP_ICON_RC})
    endif()
endif()
