// 测试分辨率检测的小程序
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <iostream>

int main(int argc, char *argv[]) {
    // 设置高DPI支持
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    
    QApplication app(argc, argv);
    
    std::cout << "=== 屏幕分辨率检测测试 ===" << std::endl;
    
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        const auto s = screens[i];
        QString name = s->name().isEmpty() ? QString("屏幕 %1").arg(i + 1) : s->name();
        
        QRect g = s->geometry();
        qreal devicePixelRatio = s->devicePixelRatio();
        int physicalWidth = g.width() * devicePixelRatio;
        int physicalHeight = g.height() * devicePixelRatio;
        
        std::cout << "屏幕 " << i + 1 << ": " << name.toStdString() << std::endl;
        std::cout << "  逻辑分辨率: " << g.width() << " x " << g.height() << std::endl;
        std::cout << "  物理分辨率: " << physicalWidth << " x " << physicalHeight << std::endl;
        std::cout << "  DPI缩放比: " << devicePixelRatio << std::endl;
        std::cout << "  位置: (" << g.x() << ", " << g.y() << ")" << std::endl;
        std::cout << "  可用区域: " << s->availableGeometry().width() << " x " 
                  << s->availableGeometry().height() << std::endl;
        std::cout << std::endl;
    }
    
    return 0;
}
