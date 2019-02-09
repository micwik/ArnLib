// Copyright (C) 2010-2019 Michael Wiklund.
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

