#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Emotion Star Journal");
    QApplication::setOrganizationName("Iriya");

    MainWindow window;
    window.show();

    return app.exec();
}
