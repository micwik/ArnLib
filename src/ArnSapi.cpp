// Copyright (C) 2010-2014 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these
// other libraries is subject to their respective license agreements.
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
#include <QDebug>


ArnSapi::ArnSapi( QObject* parent) :
    ArnRpc( parent)
{
}


bool  ArnSapi::open( QString pipePath, Mode mode,
                     const char *providerPrefix, const char *requesterPrefix)
{
    if (mode.is( mode.Provider)) {
        _receivePrefix = providerPrefix ? providerPrefix : "pv_";
        _sendPrefix    = requesterPrefix ? requesterPrefix : "rq_";
    }
    else {
        _receivePrefix = requesterPrefix ? requesterPrefix : "rq_";
        _sendPrefix    = providerPrefix ? providerPrefix : "pv_";
    }

    ArnRpc::setMethodPrefix( _receivePrefix);
    ArnRpc::addSenderSignals( this, _sendPrefix);
    ArnRpc::setReceiver( this, false);
    ArnRpc::setMode( mode);

    return ArnRpc::open( pipePath);
}


void  ArnSapi::batchConnectTo( const QObject* receiver, const QString& prefix, ArnRpc::Mode mode) {
    batchConnect( this,
                  QRegExp("^" + _receivePrefix + "(.+)"),
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
    batchConnect( sender,
                  QRegExp("^" + prefix + "(.+)"),
                  this,
                  _sendPrefix + "\\1",
                  mode);
}
