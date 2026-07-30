#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    MainWindow window;
    window.show();

    return app.exec();
}
