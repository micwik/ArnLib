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

#include "ArnInc/ArnServerRemote.hpp"
#include "private/ArnServerRemote_p.hpp"
#include "ArnInc/ArnServer.hpp"
#include "ArnSync.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/Arn.hpp"
#include <QTcpSocket>
#include <QHostInfo>
#include <QTimer>
#include <QDateTime>


ArnServerRemoteSession::ArnServerRemoteSession( ArnServerSession* arnServerSession,
                                                ArnServerRemote* arnServerRemote)
    : QObject( arnServerRemote)
{
    _arnServerSession = arnServerSession;
    _arnServerRemote  = arnServerRemote;
    _killCountdown    = 0;
    _pollCount        = 0;

    QTcpSocket*  socket = _arnServerSession->socket();
    QHostAddress  remAddr = socket->peerAddress();
    quint32  remAddrV4 = remAddr.toIPv4Address();
    if (remAddrV4)  // MW: TODO Check if this is ok for IPV6
        remAddr = QHostAddress( remAddrV4);

    QString  remIp = remAddr.toString();
    QString  remIpPort = remIp + ":" + QString::number( socket->peerPort());
    _sessionPath = Arn::pathServerSessions +
                   remIpPort + "/";

    ArnM::setValue( _sessionPath + "HostIp/value", remIp);

    _clientHostName = remIpPort;  // Preliminary name
    updateSessionValue();

    connect( _arnServerSession, SIGNAL(infoReceived(int)), this, SLOT(onInfoReceived(int)));
    connect( _arnServerSession, SIGNAL(loginCompleted()), this, SLOT(onLoginCompleted()));
    connect( _arnServerSession, SIGNAL(destroyed(QObject*)), SLOT(shutdown()));

    int  lookupId = QHostInfo::lookupHost( remIp, this, SLOT(onIpLookup(QHostInfo)));
    Q_UNUSED(lookupId);

    _timerPoll = new QTimer( this);
    _timerPoll->start( 1000);
    connect( _timerPoll, SIGNAL(timeout()), this, SLOT(doPoll()));

    _arnTraffic.open( _sessionPath + "Traffic/value");
    _arnTrafficIn.open( _sessionPath + "Traffic/In/value");
    _arnTrafficOut.open( _sessionPath + "Traffic/Out/value");

    _arnKill.open( _sessionPath + "Kill/value");
    _arnKill = KillMode::Off;
    ArnM::setValue( _sessionPath + "Kill/set", KillMode::txt().getEnumSet());
    connect( &_arnKill, SIGNAL(changed()), this, SLOT(doKillChanged()));

    _arnChatPv.setPipeMode();
    _arnChatPv.open( _sessionPath + "Chat!");
    connect( &_arnChatPv, SIGNAL(changed(QString)), this, SLOT(doChatAdd(QString)));

    _arnChatAllPv.setPipeMode();
    _arnChatAllPv.open( Arn::pathServer + "ChatSessions!");
    connect( &_arnChatAllPv, SIGNAL(changed(QString)), this, SLOT(doChatAdd(QString)));

    connect( _arnServerSession, SIGNAL(messageReceived(int,QByteArray)),
             this, SLOT(onMessageReceived(int,QByteArray)));

    _startTime = QDateTime::currentDateTimeUtc();
    _arnUpTime.open( _sessionPath + "UpTime/value");
    ArnM::setValue( _sessionPath + "UpTime/property", "prec=2 unit=h");
}


void  ArnServerRemoteSession::updateSessionValue()
{
    if (_sessionPath.isNull())  return;  // Retired

    _sessionValue = _clientUserName.isEmpty() ? _clientAgent : _clientUserName;
    if (!_clientHostName.isEmpty()) {
        if (!_sessionValue.isEmpty())
            _sessionValue += " ";
        _sessionValue += "@ " + _clientHostName;
    }
    ArnM::setValue( _sessionPath + "name", _sessionValue);
}


void  ArnServerRemoteSession::onInfoReceived( int type)
{
    if (_sessionPath.isNull())  return;  // Retired

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
    if (_sessionPath.isNull())  return;  // Retired

    ArnM::setValue( _sessionPath + "LoginName/value", _arnServerSession->loginUserName());
}


void  ArnServerRemoteSession::onIpLookup( const QHostInfo& host)
{
    if (_sessionPath.isNull())  return;  // Retired

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
    if (_sessionPath.isNull())  return;  // Retired

    KillMode  kMode = KillMode::fromInt( _arnKill.toInt());

    switch (kMode) {
    case KillMode::Off:
        _killCountdown = 0;
        break;
    case KillMode::Delay10Sec:
        _killCountdown = 10;
        break;
    case KillMode::Delay60Sec:
        _killCountdown = 60;
        break;
    default:
        break;
    }

    QString txt;
    if (_killCountdown > 0) {
        txt = QString("Arn Connection kill countdown started (%1 sec)").arg( _killCountdown);
    }
    else {
        txt = "Arn Connection kill Aborted at server";
    }
    _arnChatPv    = txt;
    _arnChatAllPv = _sessionValue + ": " + txt;
    _arnServerSession->sendMessage( ArnSync::MessageType::ChatPrio, txt.toUtf8());
}


void  ArnServerRemoteSession::doPoll()
{
    if (_sessionPath.isNull())  return;  // Retired

    if (_killCountdown > 0) {
        //// Connection Kill
        --_killCountdown;
        QString txt;
        if ((_killCountdown % 5 == 0) || (_killCountdown < 10)) {
            txt = QString("Arn Connection kill in %1 sec.").arg( _killCountdown);
            _arnChatPv = txt;
            _arnServerSession->sendMessage( ArnSync::MessageType::ChatPrio, txt.toUtf8());
        }
        if (_killCountdown == 0) {
            txt = "Arn Connection kill Request from server";
            _arnChatPv    = txt;
            _arnChatAllPv = _sessionValue + ": " + txt;
            _arnServerSession->sendMessage( ArnSync::MessageType::ChatPrio, txt.toUtf8());
            _arnServerSession->sendMessage( ArnSync::MessageType::KillRequest);
        }
    }

    if (_pollCount % 5 == 0) {
        //// Uptime
        ARNREAL upTime = ARNREAL( _startTime.secsTo( QDateTime::currentDateTimeUtc())) / 3600.;
        _arnUpTime.setValue( upTime);

        //// Traffic
        quint64  trafficIn;
        quint64  trafficOut;
        bool isOk = _arnServerSession->getTraffic( trafficIn, trafficOut);
        if (isOk) {
            _arnTraffic.setValue( trafficIn + trafficOut);
            _arnTrafficIn.setValue( trafficIn);
            _arnTrafficOut.setValue( trafficOut);
        }
    }

    ++_pollCount;
}


void  ArnServerRemoteSession::doChatAdd( const QString& txt)
{
    if (_sessionPath.isNull())  return;  // Retired

    _arnServerSession->sendMessage( ArnSync::MessageType::ChatNormal, txt.toUtf8());
}


void  ArnServerRemoteSession::onMessageReceived( int type, const QByteArray& data)
{
    if (_sessionPath.isNull())  return;  // Retired

    // XStringMap xmIn( data);
    Arn::Allow  allow = _arnServerSession->getAllow();
    QString  txt;

    switch (type) {
    //// Internal message types
    case ArnSync::MessageType::KillRequest:
        // Not valid for server
        break;
    case ArnSync::MessageType::AbortKillRequest:
        if (allow.isAny( allow.ReadWrite) && (_arnKill.toInt() != KillMode::Off)) {
            txt = "Arn Connection kill Abort request from client";
            _arnChatPv    = txt;
            _arnChatAllPv = _sessionValue + ": " + txt;
            _arnKill = KillMode::Off;
        }
        break;
    case ArnSync::MessageType::ChatPrio:
        // Fall throu
    case ArnSync::MessageType::ChatNormal:
        if (allow.isAny( allow.ReadWrite)) {
            _arnChatPv    = "Client ==>" + QString::fromUtf8( data.constData(), data.size());
            _arnChatAllPv = _sessionValue + " ==>" + QString::fromUtf8( data.constData(), data.size());
        }
        break;
    default:;
        // Not supported message-type.
    }
}


void  ArnServerRemoteSession::shutdown()
{
    ArnM::destroyLink( _sessionPath);
    _sessionPath = QString();  // Mark retired
    deleteLater();
}



ArnServerRemotePrivate::ArnServerRemotePrivate()
{
    _arnServer    = 0;
    _sessionCount = 0;
    _sessionNum   = 0;
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

    d->_timerPoll.start(5000);
    connect( &d->_timerPoll, SIGNAL(timeout()), this, SLOT(doPoll()));

    d->_startTime = QDateTime::currentDateTimeUtc();
    d->_arnUpTime.open(       Arn::pathServer + "ServerUpTime/value");
    d->_arnSessionCount.open( Arn::pathServer + "SessionCount/value");
    d->_arnSessionNum.open(   Arn::pathServer + "SessionNum/value");
    ArnM::setValue( Arn::pathServer + "ServerUpTime/property", "prec=2 unit=h");
}


void  ArnServerRemote::onNewSession()
{
    Q_D(ArnServerRemote);

    ++d->_sessionCount;
    d->_arnSessionCount = d->_sessionCount;
    ++d->_sessionNum;
    d->_arnSessionNum = d->_sessionNum;

    Q_ASSERT(d->_arnServer);
    ArnServerSession*  serverSession = d->_arnServer->getSession();
    new ArnServerRemoteSession( serverSession, this);

    connect( serverSession, SIGNAL(destroyed(QObject*)), SLOT(onDelSession(QObject*)));
}


void  ArnServerRemote::onDelSession( QObject* sessionObj)
{
    Q_UNUSED(sessionObj)

    Q_D(ArnServerRemote);

    --d->_sessionNum;
    d->_arnSessionNum = d->_sessionNum;
}


void  ArnServerRemote::doPoll()
{
    Q_D(ArnServerRemote);

    ARNREAL upTime = ARNREAL( d->_startTime.secsTo( QDateTime::currentDateTimeUtc())) / 3600.;
    d->_arnUpTime.setValue( upTime);
}
