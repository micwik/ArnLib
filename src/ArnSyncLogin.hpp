#ifndef ARNSYNCLOGIN_HPP
#define ARNSYNCLOGIN_HPP

#include "ArnInc/Arn.hpp"
#include <QMap>


//! \cond ADV
class ArnSyncLogin
{
public:
    struct AccessSlot {
        QString  userName;
        QString  pwHash;  // Hashed Password
        Arn::Allow  allow;
    };

    ArnSyncLogin();

    void  addAccess( const QString& userName, const QString& password, Arn::Allow allow);
    const AccessSlot*  findAccess( const QString& userName);

    static QByteArray  pwHashXchg( uint saltA, uint saltB, const QString& password);
    static QString  passwordHash( const QString& password);
    static bool  isPwHash( const QString& password);

private:
    QMap<QString,AccessSlot>  _accessTab;
};
//! \endcond

#endif // ARNSYNCLOGIN_HPP
