//! [code]
#include "ServerMain.hpp"
#include <QApplication>
#include <QDebug>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv, false);

    qDebug() << "Startar Arn Chat Server ...";
    new ServerMain;

    return a.exec();
}
//! [code]
