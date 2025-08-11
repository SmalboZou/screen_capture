cmake_minimum_required(VERSION 3.16)

# 查找Qt6包
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

# 查找macOS框架
find_library(AVFOUNDATION_LIBRARY AVFoundation)
find_library(COCOA_LIBRARY Cocoa)
find_library(QUARTZCORE_LIBRARY QuartzCore)
find_library(COREMEDIA_LIBRARY CoreMedia)

# 设置Qt元对象系统
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 设置源文件
set(SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
    SimpleCapture.mm
)

# 创建可执行文件
add_executable(AIcp ${SOURCES})

# 链接Qt6库和macOS框架
target_link_libraries(AIcp 
    PRIVATE
    Qt6::Widgets
    Qt6::Core
    ${AVFOUNDATION_LIBRARY}
    ${COCOA_LIBRARY}
    ${QUARTZCORE_LIBRARY}
    ${COREMEDIA_LIBRARY}
)

# 包含头文件目录
target_include_directories(AIcp PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# 设置Objective-C++编译标志
set_source_files_properties(SimpleCapture.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")

# 设置C++标准
set_target_properties(AIcp PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 平台特定定义
if(APPLE)
    target_compile_definitions(AIcp PRIVATE PLATFORM_MACOS)
endif()
