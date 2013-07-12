#include <QCoreApplication>
#include "qirc.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    QIrc irc;

    return a.exec();
}
