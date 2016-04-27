// Copyright (C) 2010-2016 Michael Wiklund.
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

#include "ArnInc/ArnSapi.hpp"
#include "private/ArnSapi_p.hpp"
#include <QDebug>


ArnSapiPrivate::ArnSapiPrivate()
{
}


ArnSapiPrivate::~ArnSapiPrivate()
{
}


ArnSapi::ArnSapi( QObject* parent)
    : ArnRpc( *new ArnSapiPrivate, parent)
{
}


ArnSapi::ArnSapi( const QString& defaultPath, QObject* parent)
    : ArnRpc( *new ArnSapiPrivate, parent)
{
    setDefaultPath( defaultPath);
}


ArnSapi::ArnSapi(ArnSapiPrivate& dd, QObject* parent)
    : ArnRpc( dd, parent)
{
}


bool  ArnSapi::open( const QString& pipePath, Mode mode,
                     const char *providerPrefix, const char *requesterPrefix)
{
    Q_D(ArnSapi);

    if (mode.is( mode.Provider)) {
        d->_receivePrefix = providerPrefix ? providerPrefix : "pv_";
        d->_sendPrefix    = requesterPrefix ? requesterPrefix : "rq_";
    }
    else {
        d->_receivePrefix = requesterPrefix ? requesterPrefix : "rq_";
        d->_sendPrefix    = providerPrefix ? providerPrefix : "pv_";
    }

    ArnRpc::setMethodPrefix( d->_receivePrefix);
    ArnRpc::addSenderSignals( this, d->_sendPrefix);
    ArnRpc::setReceiver( this, false);
    ArnRpc::setMode( mode);

    return ArnRpc::open( pipePath.isEmpty() ? d->_defaultPath : pipePath);
}


void  ArnSapi::batchConnectTo( const QObject* receiver, const QString& prefix, ArnRpc::Mode mode) {
    Q_D(ArnSapi);

    batchConnect( this,
                  QRegExp("^" + d->_receivePrefix + "(.+)"),
                  receiver,
                  prefix + "\\1",
                  mode);
    if (ArnRpc::mode().is( ArnRpc::Mode::UseDefaultCall)) {
        batchConnect( this,
                      QRegExp("^defaultCall"),
                      receiver,
                      prefix + "Default",
                      mode);
    }
}


void  ArnSapi::batchConnectFrom( const QObject* sender, const QString& prefix, ArnRpc::Mode mode) {
    Q_D(ArnSapi);

    batchConnect( sender,
                  QRegExp("^" + prefix + "(.+)"),
                  this,
                  d->_sendPrefix + "\\1",
                  mode);
}


QString  ArnSapi::defaultPath()  const
{
    Q_D(const ArnSapi);

    return d->_defaultPath;
}


void  ArnSapi::setDefaultPath( const QString& defaultPath)
{
    Q_D(ArnSapi);

    d->_defaultPath = Arn::providerPathIf( defaultPath, false);
}
