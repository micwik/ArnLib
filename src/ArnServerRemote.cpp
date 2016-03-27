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
#include "ArnSync.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/Arn.hpp"
#include <QTcpSocket>
#include <QHostInfo>
#include <QTimer>


ArnServerRemoteSession::ArnServerRemoteSession( ArnServerSession* arnServerSession,
                                                ArnServerRemote* arnServerRemote)
    : QObject( arnServerRemote)
{
    _arnServerSession = arnServerSession;
    _arnServerRemote  = arnServerRemote;
    _killCountdown    = 0;

    QTcpSocket*  socket = _arnServerSession->socket();
    QHostAddress  remAddr = socket->peerAddress();
    quint32  remAddrV4 = remAddr.toIPv4Address();
    if (remAddrV4)  // MW: TODO Check if this is ok for IPV6
        remAddr = QHostAddress( remAddrV4);

    QString  remIp = remAddr.toString();
    _sessionPath = Arn::pathServerSessions +
                   remIp + ":" + QString::number( socket->peerPort()) + "/";

    ArnM::setValue( _sessionPath + "HostIp/value", remIp);

    connect( _arnServerSession, SIGNAL(infoReceived(int)), this, SLOT(onInfoReceived(int)));
    connect( _arnServerSession, SIGNAL(loginCompleted()), this, SLOT(onLoginCompleted()));
    connect( _arnServerSession, SIGNAL(destroyed(QObject*)), SLOT(shutdown()));

    int  lookupId = QHostInfo::lookupHost( remIp, this, SLOT(onIpLookup(QHostInfo)));
    Q_UNUSED(lookupId);

    _timerKill = new QTimer( this);
    _timerKill->setInterval( 1000);
    connect( _timerKill, SIGNAL(timeout()), this, SLOT(doKillCountdown()));

    _arnKill.open( _sessionPath + "Kill/value");
    _arnKill = KillMode::Off;
    ArnM::setValue( _sessionPath + "Kill/set", KillMode::txt().getEnumSet());
    connect( &_arnKill, SIGNAL(changed()), this, SLOT(doKillChanged()));

    _arnChatPv.setPipeMode();
    _arnChatPv.open(_sessionPath + "Chat!");
    connect( &_arnChatPv, SIGNAL(changed(QString)), this, SLOT(doChatAdd(QString)));
    connect( _arnServerSession, SIGNAL(messageReceived(int,QByteArray)),
             this, SLOT(onMessageReceived(int,QByteArray)));
}


void  ArnServerRemoteSession::updateSessionValue()
{
    QString val = _clientUserName.isEmpty() ? _clientAgent : _clientUserName;
    if (!_clientHostName.isEmpty()) {
        if (!val.isEmpty())
            val += " ";
        val += "@ " + _clientHostName;
    }
    ArnM::setValue( _sessionPath + "value", val);
}


void  ArnServerRemoteSession::onInfoReceived( int type)
{
    switch (type) {
    case ArnSync::InfoType::WhoIAm:
    {
        Arn::XStringMap  wimXsm = _arnServerSession->remoteWhoIAm();
        _clientAgent    = wimXsm.valueString("Agent");
        _clientUserName = wimXsm.valueString("UserName");
        updateSessionValue();
        for (int i = 0; i < wimXsm.size(); ++i) {
            ArnM::setValue( _sessionPath + wimXsm.key(i) + "/value", wimXsm.value(i));
        }
        break;
    }
    default:;
    }
}


void  ArnServerRemoteSession::onLoginCompleted()
{
    ArnM::setValue( _sessionPath + "LoginName/value", _arnServerSession->loginUserName());
}


void  ArnServerRemoteSession::onIpLookup( const QHostInfo& host)
{
    if (host.error() == QHostInfo::NoError) {
        _clientHostName = host.hostName();
        updateSessionValue();
        ArnM::setValue( _sessionPath + "HostName/value", _clientHostName);
    }
    else {
        qDebug() << "ServerRemoteSession Lookup failed:" << host.errorString();
    }
}


void  ArnServerRemoteSession::doKillChanged()
{
    KillMode  kMode = KillMode::fromInt( _arnKill.toInt());

    switch (kMode) {
    case KillMode::Off:
        _killCountdown = 0;
        _timerKill->stop();
        break;
    case KillMode::Delay10Sec:
        _killCountdown = 10;
        _timerKill->start();
        break;
    case KillMode::Delay60Sec:
        _killCountdown = 60;
        _timerKill->start();
        break;
    default:
        break;
    }

    QString txt;
    if (_killCountdown) {
        txt = QString("Arn Connection kill countdown started (%1 sec)").arg( _killCountdown);
    }
    else {
        txt = "Arn Connection kill Aborted at server";
    }
    _arnChatPv = txt;
    _arnServerSession->sendMessage( ArnSync::MessageType::ChatPrio, txt.toUtf8());
}



void  ArnServerRemoteSession::doKillCountdown()
{
    if (_killCountdown == 0) {
        _timerKill->stop();
        return;
    }

    --_killCountdown;
    QString txt;
    if ((_killCountdown % 5 == 0) || (_killCountdown < 10)) {
        txt = QString("Arn Connection kill in %1 sec.").arg( _killCountdown);
        _arnChatPv = txt;
        _arnServerSession->sendMessage( ArnSync::MessageType::ChatPrio, txt.toUtf8());
    }
    if (_killCountdown)  return;

    _timerKill->stop();
    txt = "Arn Connection kill Request from server";
    _arnChatPv = txt;
    _arnServerSession->sendMessage( ArnSync::MessageType::ChatPrio, txt.toUtf8());
    _arnServerSession->sendMessage( ArnSync::MessageType::KillRequest);
}


void  ArnServerRemoteSession::doChatAdd( const QString& txt)
{
    _arnServerSession->sendMessage( ArnSync::MessageType::ChatNormal, txt.toUtf8());
}


void  ArnServerRemoteSession::onMessageReceived( int type, const QByteArray& data)
{
    // XStringMap xmIn( data);
    Arn::Allow  allow = _arnServerSession->getAllow();

    switch (type) {
    //// Internal message types
    case ArnSync::MessageType::KillRequest:
        // Not valid for server
        break;
    case ArnSync::MessageType::AbortKillRequest:
        if (allow.isAny( allow.ReadWrite) && (_arnKill.toInt() != KillMode::Off)) {
            _arnChatPv = "Arn Connection kill Abort request from client";
            _arnKill = KillMode::Off;
        }
        break;
    case ArnSync::MessageType::ChatPrio:
        // Fall throu
    case ArnSync::MessageType::ChatNormal:
        if (allow.isAny( allow.ReadWrite)) {
            _arnChatPv = "Client ==>" + QString::fromUtf8( data.constData(), data.size());
        }
        break;
    default:;
        // Not supported message-type.
    }
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
