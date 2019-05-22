#include <QApplication>
#include <QtGui>
#include "MainWindow.h"
#include <cleaver/Cleaver.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(QString("Cleaver ") + QString(cleaver::VersionNumber.c_str()));
    w.resize(900,400);
#ifdef WIN32
	w.setWindowIcon(QIcon(WINDOW_ICON));
#endif
    w.show();
    return a.exec();
}
