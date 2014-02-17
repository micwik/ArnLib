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

#include "ArnInc/ArnClient.hpp"
#include "ArnInc/Arn.hpp"
#include "ArnSync.hpp"
#include <QTcpSocket>
#include <QStringList>
#include <QTimer>
#include <QDebug>

using Arn::XStringMap;


ArnClient::ArnClient( QObject* parent) :
    QObject( parent)
{
    _arnMountPoint = 0;
    _isAutoConnect = false;
    _retryTime     = 2;
    _port          = 0;
    _nextHost      = -1;
    _curPrio       = -1;

    _socket = new QTcpSocket( this);
    _arnNetSync = new ArnSync( _socket, true, this);
    _connectTimer = new QTimer( this);

    connect( _socket, SIGNAL(connected()), this, SLOT(doTcpConnected()));
    connect( _socket, SIGNAL(disconnected()), this, SIGNAL(tcpDisConnected()));
    connect( _arnNetSync, SIGNAL(replyRecord(Arn::XStringMap&)), this, SLOT(doReplyRecord(Arn::XStringMap&)));
    connect( _arnNetSync, SIGNAL(replyRecord(Arn::XStringMap&)), this, SIGNAL(replyRecord(Arn::XStringMap&)));

    connect( _socket, SIGNAL(error(QAbstractSocket::SocketError)),
             this, SLOT(tcpError(QAbstractSocket::SocketError)));
    connect( _connectTimer, SIGNAL(timeout()), this, SLOT(reConnectArn()));

    // Special mount point for sys
    ArnItem* sysMountPoint = new ArnItem("/.sys/", this);
    connect( sysMountPoint, SIGNAL(arnItemCreated(QString)), this, SLOT(createNewItem(QString)));
}


void  ArnClient::clearArnList( int prioFilter)
{
    if (_nextHost > 0)
        _nextHost = 0;

    if ((prioFilter < 0) || _hostTab.isEmpty()) {
        _hostTab.clear();
        _hostPrioTab.clear();
        return;
    }

    int  index = -1;
    forever {
        index = _hostPrioTab.indexOf( prioFilter, index + 1);
        if (index < 0)  break;
        _hostTab.removeAt( index);
        _hostPrioTab.removeAt( index);
    }
}


ArnClient::HostList  ArnClient::arnList( int prioFilter)  const
{
    if ((prioFilter < 0) || _hostTab.isEmpty())
        return _hostTab;

    HostList  retVal;
    int  index = -1;
    forever {
        index = _hostPrioTab.indexOf( prioFilter, index + 1);
        if (index < 0)  break;
        retVal += _hostTab.at( index);
    }
    return retVal;
}


void  ArnClient::addToArnList( const QString &arnHost, quint16 port, int prio)
{
    if (arnHost.isEmpty())  return;  // Invalid

    HostAddrPort  slot;
    slot.addr = arnHost;
    slot.port = port ? port : Arn::defaultTcpPort;

    int index;
    for (index = 0; index < _hostPrioTab.size(); ++index) {
        if (prio < _hostPrioTab.at( index))  break;  // Found place
    }
    _hostTab.insert( index, slot);
    _hostPrioTab.insert( index, prio);
}


void  ArnClient::connectToArnList()
{
    if (_hostTab.isEmpty())  return;

    _nextHost = 0;
    doConnectArnLogic();
}


void  ArnClient::connectToArn( const QString& arnHost, quint16 port)
{
    _nextHost = -1;
    _arnHost  = arnHost;
    _port     = port ? port : Arn::defaultTcpPort;
    doConnectArnLogic();
}


ArnClient::ConnectStat  ArnClient::connectStatus()  const
{
    return _connectStat;
}


bool  ArnClient::setMountPoint( const QString& path)
{
    if (_arnMountPoint)  delete _arnMountPoint;

    _arnMountPoint = new ArnItem( this);
    bool  isOk = _arnMountPoint->openFolder( path);
    if (isOk) {
        connect( _arnMountPoint, SIGNAL(arnItemCreated(QString)), this, SLOT(createNewItem(QString)));
    }

    return isOk;
}


void  ArnClient::setAutoConnect( bool isAuto, int retryTime)
{
    _isAutoConnect = isAuto;
    _retryTime = retryTime > 1 ? retryTime : 1;
}


int ArnClient::curPrio() const
{
    return _curPrio;
}


void  ArnClient::commandGet( const QString& path)
{
    _commandMap.clear();
    _commandMap.add(ARNRECNAME, "get").add("path", path);

    _arnNetSync->sendXSMap( _commandMap);
}


void  ArnClient::commandSet( const QString& path, const QString& data)
{
    _commandMap.clear();
    _commandMap.add(ARNRECNAME, "set").add("path", path).add("data", data);

    _arnNetSync->sendXSMap( _commandMap);
}


void  ArnClient::commandLs( const QString& path)
{
    _commandMap.clear();
    _commandMap.add(ARNRECNAME, "ls").add("path", path);

    qDebug() << "client-ls: path=" << path;
    _arnNetSync->sendXSMap( _commandMap);
}


void  ArnClient::commandVersion()
{
    _commandMap.clear();
    _commandMap.add(ARNRECNAME, "ver");

    _arnNetSync->sendXSMap( _commandMap);
}


void  ArnClient::commandExit()
{
    _commandMap.clear();
    _commandMap.add(ARNRECNAME, "exit");

    _arnNetSync->sendXSMap( _commandMap);
}


void  ArnClient::newNetItemProxy( ArnThreadCom *threadCom,
                                       const QString &path, int syncMode, void* isNewPtr)
{
    ArnThreadComProxyLock  proxyLock( threadCom);

    if (Arn::debugThreading)  qDebug() << "newNetItemProxy: path=" << path;
    threadCom->_retObj = newNetItem( path, ArnItem::SyncMode::F( syncMode), (bool*) isNewPtr);
    if (Arn::debugThreading)  qDebug() << "newNetItemProxy: waking thread";
}


ArnItemNet*  ArnClient::newNetItem( QString path, ArnItem::SyncMode syncMode, bool* isNewPtr)
{
    if (ArnM::isMainThread()) {
        return _arnNetSync->newNetItem( path, syncMode, isNewPtr);
    }
    else {  // Threaded - must be threadsafe
        ArnThreadComCaller  threadCom;

        threadCom.p()->_retObj = 0;  // Just in case ...
        if (Arn::debugThreading)  qDebug() << "newNetItem-thread: start path=" << path;
        QMetaObject::invokeMethod( this,
                                   "newNetItemProxy",
                                   Qt::QueuedConnection,
                                   Q_ARG( ArnThreadCom*, threadCom.p()),
                                   Q_ARG( QString, path),
                                   Q_ARG( int, syncMode.f),
                                   Q_ARG( void*, isNewPtr));
        threadCom.waitCommandEnd();  // Wait main-thread gives retObj
        ArnItemNet*  retItemNet = qobject_cast<ArnItemNet*>( threadCom.p()->_retObj);
        if (retItemNet)  if (Arn::debugThreading)  qDebug() << "newNetItem-thread: end path=" << retItemNet->path();

        return retItemNet;
    }
}


void  ArnClient::createNewItem( QString path)
{
    // qDebug() << "ArnClient,ArnItem-created: path=" << path;
    _arnNetSync->newNetItem( path);
}


void  ArnClient::tcpError(QAbstractSocket::SocketError socketError)
{
    QString  errTextSum = QString(tr("TCP Client Msg:")) + _socket->errorString();
    ArnM::errorLog( errTextSum, ArnError::ConnectionError);
    emit tcpError( _socket->errorString(), socketError);

    _connectStat = (_connectStat == ConnectStat::Connected) ? ConnectStat::Disconnected : ConnectStat::Error;
    emit connectionStatusChanged( _connectStat, _curPrio);

    if ((_nextHost >= 0) && (_nextHost < _hostTab.size())) {
        doConnectArnLogic();
    }
    else if (_isAutoConnect) {
        _connectTimer->start( _retryTime * 1000);
    }
}


void  ArnClient::reConnectArn()
{
    _connectTimer->stop();
    doConnectArnLogic();
}


void  ArnClient::doTcpConnected()
{
    qDebug() << "ArnClient TcpConnected: hostAddr=" << _curConnectAP.addr;
    emit tcpConnected( _curConnectAP.addr, _curConnectAP.port);
    _connectStat = ConnectStat::Connected;
    emit connectionStatusChanged( _connectStat, _curPrio);
}


void  ArnClient::doConnectArnLogic()
{
    QString  arnHost;
    quint16  port = 0;
    int  curPrio  = -1;
    _curPrio = curPrio;

    if (_nextHost < 0) {  // Normal single host connect
        arnHost  = _arnHost;
        port     = _port;
    }
    else if (!_hostTab.isEmpty()) {  // Arn connection list
        if (_nextHost >= _hostTab.size()) {  // Past end of list, restart
            _nextHost = 0;
            emit connectionStatusChanged( ConnectStat::TriedAll, -1);
        }

        const HostAddrPort&  slot = _hostTab.at( _nextHost);
        arnHost = slot.addr;
        port    = slot.port;
        curPrio = _hostPrioTab.at( _nextHost);
        qDebug() << "ArnClient connectlogic: hostTabSize=" << _hostTab.size() << " index=" << _nextHost
                 << " prio=" << _hostPrioTab.at( _nextHost);
        ++_nextHost;
    }

    if (arnHost.isEmpty())  return;

    if (port == 0)
        port = Arn::defaultTcpPort;

    _socket->abort();
    _socket->connectToHost( arnHost, port);
    _curConnectAP.addr = arnHost;
    _curConnectAP.port = port;
    _curPrio           = curPrio;
    _connectStat = ConnectStat::Connecting;

    emit connectionStatusChanged( _connectStat, _curPrio);
}


void  ArnClient::doReplyRecord( XStringMap& replyMap)
{
    QByteArray reply = replyMap.value(0);

    if (reply == "Rget") {
        emit replyGet( replyMap.valueString("data"), replyMap.valueString("path"));
    }
    else if (reply == "Rls") {
        emit replyLs( makeItemList( replyMap), replyMap.valueString("path"));
    }
    else if (reply == "Rver") {
        emit replyVer( replyMap.valueString("data"));
    }
}


QStringList  ArnClient::makeItemList( XStringMap& xsMap)
{
    QStringList  items;
    int  n = xsMap.maxEnumOf("item");

    for (int i = 1; i <= n; ++i) {
        items << xsMap.valueString( "item", uint(i));
    }

    return items;
}
