#ifdef USING_QT5
#include <QApplication>
#else
#include <QtGui/QApplication>
#endif
#include <QtGui>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(QString("Cleaver 2.0"));
    w.resize(900,400);
    w.show();
    return a.exec();
}


