#include <QtGui/QApplication>
#include "demoapplication.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DemoApplication w;
    w.show();
    return a.exec();
}
