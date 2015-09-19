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
        QString  password;
        Arn::Allow  allow;
    };

    ArnSyncLogin();

    void  addAccess( const QString& userName, const QString& password, Arn::Allow allow);
    const AccessSlot*  findAccess( const QString& userName);

    static QByteArray  pwHash( uint saltA, uint saltB, const QString& password);

private:
    QMap<QString,AccessSlot>  _accessTab;
};
//! \endcond

#endif // ARNSYNCLOGIN_HPP
