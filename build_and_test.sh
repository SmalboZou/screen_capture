#!/bin/bash
# 构建和测试脚本

echo "开始构建 AIcp 录屏软件..."

# 进入构建目录
cd "build-test"

# 清理之前的构建
echo "清理构建目录..."
rm -rf CMakeCache.txt CMakeFiles/

# 重新配置
echo "配置项目..."
cmake ..

# 构建项目
echo "编译项目..."
cmake --build . --config Debug

# 检查构建结果
if [ -f "Debug/AIcp.exe" ]; then
    echo "✅ 构建成功！"
    echo "📁 可执行文件位置: Debug/AIcp.exe"
    echo ""
    echo "🎯 新功能说明:"
    echo "  - ⏰ 定时录制功能"
    echo "  - 🎨 优化的用户界面"
    echo "  - 📊 实时状态显示"
    echo "  - 🖥️ 多屏幕支持"
    echo ""
    echo "💡 使用提示:"
    echo "  1. 启用定时录制复选框"
    echo "  2. 设置录制时长"
    echo "  3. 点击开始录制"
    echo "  4. 到时间后会自动停止并恢复窗口"
    echo ""
    echo "🚀 可以开始使用了！"
else
    echo "❌ 构建失败"
    exit 1
fi
