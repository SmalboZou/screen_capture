#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    app.setApplicationName("AIcp");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("AIcp Project");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
