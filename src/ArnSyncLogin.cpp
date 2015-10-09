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
    accessSlot.pwHash   = isPwHash( password) ? password : passwordHash( password);
    accessSlot.allow    = allow;

    _accessTab.insert( userName, accessSlot);
}


const ArnSyncLogin::AccessSlot*  ArnSyncLogin::findAccess( const QString& userName)
{
    if (!_accessTab.contains( userName))  return 0;

    return &_accessTab[ userName];
}


QByteArray ArnSyncLogin::pwHashXchg(uint saltA, uint saltB, const QString& password)
{
    QByteArray  pwSalted = password.toUtf8()
                         + "." + QByteArray::number( saltA, 16)
                         + "." + QByteArray::number( saltB, 16);

    QCryptographicHash  hasher( QCryptographicHash::Sha1);
    hasher.addData( pwSalted);
    return hasher.result().toHex();
}


QString  ArnSyncLogin::passwordHash( const QString& password)
{
    QCryptographicHash  hasher( QCryptographicHash::Sha1);
    hasher.addData( password.toUtf8());
    QByteArray  pwHashed = hasher.result().toBase64();
    pwHashed.chop(1);  // Remove filler "="
    pwHashed = "{" + pwHashed + "}";
    return QString::fromLatin1( pwHashed.constData(), pwHashed.size());
}


bool  ArnSyncLogin::isPwHash( const QString& password)
{
    return password.startsWith("{") && password.endsWith("}");
}

