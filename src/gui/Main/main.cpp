#include <QApplication>
#include <QtGui>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(QString("Cleaver ") + QString(CLEAVER_VERSION_MAJOR) 
      + QString(".") + QString(CLEAVER_VERSION_MINOR));
    w.resize(900,400);
#ifdef WIN32
	w.setWindowIcon(QIcon(WINDOW_ICON));
#endif
    w.show();
    return a.exec();
}


