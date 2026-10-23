#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char **argv)
{
    // Enable Wayland integration
    qputenv("QT_QPA_PLATFORM", "wayland;xcb");

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ManyTerminals"));
    app.setApplicationDisplayName(QStringLiteral("ManyTerminals"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setDesktopFileName(QStringLiteral("manyterminals"));

    QIcon appIcon(QStringLiteral("assets/icon.svg"));
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    MainWindow window;
    window.show();

    return app.exec();
}
