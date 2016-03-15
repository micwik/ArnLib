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

#include "ArnInc/ArnServerRemote.hpp"
#include "private/ArnServerRemote_p.hpp"
#include "ArnInc/ArnServer.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/Arn.hpp"
#include <QTcpSocket>


ArnServerRemoteSession::ArnServerRemoteSession( ArnServerSession* arnServerSession,
                                                ArnServerRemote* arnServerRemote)
    : QObject( arnServerRemote)
{
    _arnServerSession = arnServerSession;
    _arnServerRemote  = arnServerRemote;

    QTcpSocket*  socket = _arnServerSession->socket();
    QHostAddress  remAddr = socket->peerAddress();
    bool  isOk;
    quint32  remAddrV4 = remAddr.toIPv4Address( &isOk);
    if (isOk)
        remAddr = QHostAddress( remAddrV4);

    _sessionPath = Arn::pathServerSessions +
                   remAddr.toString() + ":" + QString::number( socket->peerPort()) + "/";

    ArnM::setValue( _sessionPath + "Name/value", socket->peerName());

    connect( _arnServerSession, SIGNAL(destroyed(QObject*)), SLOT(shutdown()));
}


void  ArnServerRemoteSession::shutdown()
{
    ArnM::destroyLink( _sessionPath);
    deleteLater();
}



ArnServerRemotePrivate::ArnServerRemotePrivate()
{
    _arnServer = 0;
}


ArnServerRemotePrivate::~ArnServerRemotePrivate()
{
}



void  ArnServerRemote::init()
{
}


ArnServerRemote::ArnServerRemote( QObject* parent)
    : QObject( parent)
    , d_ptr( new ArnServerRemotePrivate)
{
    init();
}


ArnServerRemote::ArnServerRemote( ArnServerRemotePrivate& dd, QObject* parent)
    : QObject( parent)
    , d_ptr( &dd)
{
    init();
}


ArnServerRemote::~ArnServerRemote()
{
    delete d_ptr;
}


void  ArnServerRemote::startUseServer( ArnServer* arnServer)
{
    Q_D(ArnServerRemote);

    if (!arnServer)  return;

    d->_arnServer = arnServer;
    connect( arnServer, SIGNAL(newSession()), this, SLOT(onNewSession()));
}


void  ArnServerRemote::onNewSession()
{
    Q_D(ArnServerRemote);

    Q_ASSERT(d->_arnServer);
    ArnServerSession*  serverSession = d->_arnServer->getSession();
    new ArnServerRemoteSession( serverSession, this);
}
