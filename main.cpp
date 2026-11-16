#include "mainwindow.h"
#include "version.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("QSlackpkg");
    a.setApplicationVersion(APP_VERSION);
    a.setOrganizationName("Slackware Linux Community Tools");

    MainWindow w;
    w.show();
    return a.exec();
}
