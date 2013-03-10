// Copyright (C) 2010-2013 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt 4 and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these other
// libraries is subject to their respective license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE.LGPL included in the packaging of this file.
// In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General
// Public License version 3.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and
// conditions contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//

#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include "ArnError.hpp"
#include "Arn.hpp"
#include "ArnServer.hpp"
#include "ArnSync.hpp"


ArnServer::ArnServer( Type serverType, QObject *parent)
    : QObject( parent)
{
    _tcpServerActive = false;
    _tcpServer       = new QTcpServer( this);
    _serverType      = serverType;
}


void  ArnServer::start(int port)
{
    if (port == 0) {
        switch (_serverType) {
        case Type::NetSync:
            port = 2022;
            break;
        default:
            ArnM::errorLog( QString(tr("Unknown Arn server Type:")) + QString::number( _serverType),
                                ArnError::Undef);
            return;
        }
    }
    if (_tcpServer->listen(QHostAddress::Any, port)) {
        _tcpServerActive = true;

        connect(_tcpServer, SIGNAL(newConnection()), this,
                SLOT(tcpConnection()));
    }
    else {
        ArnM::errorLog( QString(tr("Failed start Arn Server Port:")) + QString::number( port),
                            ArnError::ConnectionError);
    }
}


void  ArnServer::tcpConnection()
{
    QTcpSocket*  socket = _tcpServer->nextPendingConnection();

    switch (_serverType) {
    case Type::NetSync:
        new ArnSync( socket, false);
        break;
    }
}
