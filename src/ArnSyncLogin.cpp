#include "ArnSyncLogin.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QCryptographicHash>
#include <QString>
#include <QDebug>


ArnSyncLogin::ArnSyncLogin()
{

}


void  ArnSyncLogin::addAccess( const QString& userName, const QString& password, Arn::Allow allow)
{
    AccessSlot  accessSlot;
    accessSlot.userName = userName;
    accessSlot.password = password;
    accessSlot.allow    = allow;

    _accessTab.insert( userName, accessSlot);
}


const ArnSyncLogin::AccessSlot*  ArnSyncLogin::findAccess( const QString& userName)
{
    if (!_accessTab.contains( userName))  return 0;

    return &_accessTab[ userName];
}


QByteArray ArnSyncLogin::pwHash(uint saltA, uint saltB, const QString& password)
{
    QByteArray  pwSalted = password.toUtf8()
                         + "." + QByteArray::number( saltA, 16)
                         + "." + QByteArray::number( saltB, 16);

    QCryptographicHash  hasher( QCryptographicHash::Sha1);
    hasher.addData( pwSalted);
    return hasher.result().toHex();
}

