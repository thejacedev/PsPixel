#include <QApplication>
#include <QIcon>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName(PixelPaint::APP_NAME);
    app.setApplicationVersion(PixelPaint::VERSION);
    app.setOrganizationName("Jace Sleeman");

    // Set global application icon
    app.setWindowIcon(QIcon(":/assets/icon.png"));

    PixelPaint::MainWindow window;
    window.show();

    return app.exec();
}