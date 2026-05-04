#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("情绪星图日记");
    QApplication::setOrganizationName("Iriya");

    MainWindow window;
    window.show();

    return app.exec();
}
