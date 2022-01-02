// Copyright (C) 2010-2022 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. Usage of these other libraries is subject to their respective
// license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE_LGPL.txt included in the packaging of this
// file. In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General Public
// License version 3.0 as published by the Free Software Foundation and appearing
// in the file LICENSE_GPL.txt included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and conditions
// contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//

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
