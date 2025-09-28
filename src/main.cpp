#include "mainwindow.h"

#include <QApplication>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // Set DPI awareness for accurate coordinate handling on high-DPI displays
    SetProcessDPIAware();
#endif

    // Note: High DPI scaling is automatically enabled in Qt 6.x
    // No need to set AA_EnableHighDpiScaling or AA_UseHighDpiPixmaps

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
