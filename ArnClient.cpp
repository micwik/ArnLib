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

#include "ArnClient.hpp"
#include "ArnSync.hpp"
#include <QTcpSocket>
#include <QStringList>
#include <QTimer>
#include <QDebug>


ArnClient::ArnClient(QObject *parent) :
    QObject(parent)
{
    _arnMountPoint = 0;
    _isAutoConnect = false;
    _retryTime = 2;

    _socket = new QTcpSocket( this);
    _arnNetSync = new ArnSync( _socket, true, this);
    _connectTimer = new QTimer( this);

    connect( _socket, SIGNAL(connected()), this, SIGNAL(tcpConnected()));
    connect( _socket, SIGNAL(disconnected()), this, SIGNAL(tcpDisConnected()));
    connect( _arnNetSync, SIGNAL(replyRecord(XStringMap&)), this, SLOT(doReplyRecord(XStringMap&)));
    connect( _arnNetSync, SIGNAL(replyRecord(XStringMap&)), this, SIGNAL(replyRecord(XStringMap&)));

    connect( _socket, SIGNAL(error(QAbstractSocket::SocketError)),
             this, SLOT(tcpError(QAbstractSocket::SocketError)));
    connect( _connectTimer, SIGNAL(timeout()), this, SLOT(reConnectArn()));

    // Special mount point for sys
    ArnItem* sysMountPoint = new ArnItem("/.sys/", this);
    connect( sysMountPoint, SIGNAL(arnItemCreated(QString)), this, SLOT(createNewItem(QString)));
}


void  ArnClient::connectToArn( const QString& arnHost, quint16 port)
{
    _arnHost = arnHost;
    _port = port;
    _socket->abort();
    _socket->connectToHost( arnHost, port);
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

    if (gDebugThreading)  qDebug() << "newNetItemProxy: path=" << path;
    threadCom->_retObj = newNetItem( path, ArnItem::SyncMode::F( syncMode), (bool*) isNewPtr);
    if (gDebugThreading)  qDebug() << "newNetItemProxy: waking thread";
}


ArnItemNet*  ArnClient::newNetItem( QString path, ArnItem::SyncMode syncMode, bool* isNewPtr)
{
    if (Arn::isMainThread()) {
        return _arnNetSync->newNetItem( path, syncMode, isNewPtr);
    }
    else {  // Threaded - must be threadsafe
        ArnThreadComCaller  threadCom;

        threadCom.p()->_retObj = 0;  // Just in case ...
        if (gDebugThreading)  qDebug() << "newNetItem-thread: start path=" << path;
        QMetaObject::invokeMethod( this,
                                   "newNetItemProxy",
                                   Qt::QueuedConnection,
                                   Q_ARG( ArnThreadCom*, threadCom.p()),
                                   Q_ARG( QString, path),
                                   Q_ARG( int, syncMode.f),
                                   Q_ARG( void*, isNewPtr));
        threadCom.waitCommandEnd();  // Wait main-thread gives retObj
        ArnItemNet*  retItemNet = qobject_cast<ArnItemNet*>( threadCom.p()->_retObj);
        if (retItemNet)  if (gDebugThreading)  qDebug() << "newNetItem-thread: end path=" << retItemNet->path();

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
    Arn::errorLog( errTextSum, ArnError::ConnectionError);
    emit tcpError( _socket->errorString(), socketError);

    if (_isAutoConnect) {
        _connectTimer->start( _retryTime * 1000);
    }
}


void  ArnClient::reConnectArn()
{
    _connectTimer->stop();
    _socket->abort();
    _socket->connectToHost( _arnHost, _port);
}


void  ArnClient::doReplyRecord( XStringMap& replyMap)
{
    QByteArray reply = replyMap.value(0);

    if (reply == "Rget") {
        emit replyGet( replyMap.valueString("data"), replyMap.valueString("path"));
    }
    else if (reply == "Rset") {
    }
    else if (reply == "Rsync") {
    }
    else if (reply == "Rls") {
        emit replyLs( makeItemList( replyMap), replyMap.valueString("path"));
    }
    else if (reply == "Rver") {
        emit replyVer( replyMap.valueString("data"));
    }
    else {
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
