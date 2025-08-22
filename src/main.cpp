#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // 设置高DPI支持，确保获取真实的屏幕分辨率
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    
    QApplication app(argc, argv);
    
    app.setApplicationName("AIcp");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("AIcp Project");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
