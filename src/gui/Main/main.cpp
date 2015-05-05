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
    MainWindow w(QString("Cleaver ") + QString(CLEAVER_VERSION_MAJOR) + QString(".") + QString(CLEAVER_VERSION_MINOR));
    w.resize(900,400);
#ifdef WIN32
	w.setWindowIcon(QIcon(WIN32_ICON_FILE));
#endif
    w.show();
    return a.exec();
}


