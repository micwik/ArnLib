#include "ScriptMain.hpp"
#include <QCoreApplication>

int main( int argc, char *argv[])
{
    QCoreApplication a( argc, argv);
    ScriptMain scriptMain;

    return a.exec();
}
