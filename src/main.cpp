#include <QApplication>
#include <QIcon>
#include <QDir>
#include <QCoreApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // 设置高DPI支持，确保获取真实的屏幕分辨率
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    
    QApplication app(argc, argv);
    
    app.setApplicationName("AIcp");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("AIcp Project");
    
    // 设置应用程序图标 - 使用嵌入的资源
    QIcon appIcon(":/app.ico");
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
