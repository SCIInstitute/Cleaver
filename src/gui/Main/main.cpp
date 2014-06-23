#include <QtGui/QApplication>
#include <QtGui>
#include "MainWindow.h"
#ifdef USE_STELLAR
  #include <Stellar/Stellar.h>
#endif


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(QString("Cleaver 2.0 Beta"));
    w.resize(900,400);
    w.show();
    return a.exec();
}


