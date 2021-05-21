#include "sreader.h"
#include "globals.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Globals::init();
    SReader w;
    w.show();

    return a.exec();
}
