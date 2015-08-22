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

#include "ArnInc/ArnClient.hpp"
#include "ArnInc/Arn.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnSync.hpp"
#include <QTcpSocket>
#include <QStringList>
#include <QTimer>
#include <QMap>
#include <QMutexLocker>
#include <QDebug>

using Arn::XStringMap;


class ArnClientReg
{
public:
    bool  store( ArnClient* client, const QString& id);
    ArnClient*  get( const QString& id);
    int  remove( const QString& id);
    int  remove( const ArnClient* client);

    static ArnClientReg&  instance();

private:
    ArnClientReg()  {}

    QMap<QString, ArnClient*>  _clientTab;
    QMutex  _mutex;
};


bool  ArnClientReg::store( ArnClient* client, const QString& id)
{
    if (id.isEmpty())  return false;

    QMutexLocker  locker( &_mutex);

    if (_clientTab.contains( id))  return false;

    _clientTab.insert( id, client);
    return true;
}


ArnClient*  ArnClientReg::get( const QString& id)
{
    QMutexLocker  locker( &_mutex);

    return _clientTab.value( id, 0);
}


int  ArnClientReg::remove( const QString& id)
{
    QMutexLocker  locker( &_mutex);

    return _clientTab.remove( id);
}


int  ArnClientReg::remove( const ArnClient* client)
{
    QMutexLocker  locker( &_mutex);

    int  eraseCount = 0;

    QMap<QString, ArnClient*>::iterator  i = _clientTab.begin();
    while (i != _clientTab.end()) {
        if (i.value() == client) {
            i = _clientTab.erase(i);
            ++eraseCount;
        }
        else
            ++i;
    }
    return eraseCount;
}


ArnClientReg&  ArnClientReg::instance()
{
    static ArnClientReg  in;

    return in;
}


ArnClient::ArnClient( QObject* parent) :
    QObject( parent)
{
    _arnMountPoint   = 0;
    _isAutoConnect   = false;
    _receiveTimeout  = 10;
    _recTimeoutCount = 0;
    _retryTime       = 2;
    _port            = 0;
    _nextHost        = -1;
    _curPrio         = -1;

    QString  stdId = "std";
    if (ArnClientReg::instance().store( this, stdId))  // Only use std-id once
        _id = stdId;

    _socket       = new QTcpSocket( this);
    _arnNetSync   = new ArnSync( _socket, true, this);
    _connectTimer = new QTimer( this);
    _recTimer     = new QTimer( this);

    connect( _socket, SIGNAL(connected()), this, SLOT(doTcpConnected()));
    connect( _socket, SIGNAL(disconnected()), this, SLOT(doTcpDisconnected()));
    connect( _socket, SIGNAL(disconnected()), this, SIGNAL(tcpDisConnected()));
    connect( _arnNetSync, SIGNAL(replyRecord(Arn::XStringMap&)), this, SLOT(doReplyRecord(Arn::XStringMap&)));
    connect( _arnNetSync, SIGNAL(replyRecord(Arn::XStringMap&)), this, SIGNAL(replyRecord(Arn::XStringMap&)));
    connect( _arnNetSync, SIGNAL(xcomDelete(QString)), this, SLOT(onCommandDelete(QString)));
    connect( _recTimer, SIGNAL(timeout()), this, SLOT(doRecTimeout()));
    connect( _socket, SIGNAL(readyRead()), this, SLOT(doRecNotified()));

    connect( _socket, SIGNAL(error(QAbstractSocket::SocketError)),
             this, SLOT(doTcpError(QAbstractSocket::SocketError)));
    connect( _connectTimer, SIGNAL(timeout()), this, SLOT(onConnectWaitDone()));
}


ArnClient::~ArnClient()
{
    ArnClientReg::instance().remove( this);  // If registered, remove it
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


void  ArnClient::disconnectFromArn()
{
    _arnNetSync->sendExit();
    setAutoConnect( false);
    _socket->disconnectFromHost();
}


void ArnClient::close()
{
    setAutoConnect( false);
    _arnNetSync->close();
}


ArnClient::ConnectStat  ArnClient::connectStatus()  const
{
    return _connectStat;
}


bool  ArnClient::setMountPoint( const QString& path)
{
    if (_mountPoints.size() == 1)
        removeMountPoint( _mountPoints.at(0).localPath);

    return addMountPoint( path);
}


bool  ArnClient::addMountPoint( const QString& localPath, const QString& remotePath)
{
    if (Arn::debugShareObj)  qDebug() << "Adding mount point: localPath=" << localPath
                                      << " remotePath=" << remotePath;
    if (localPath.isEmpty()) {
        return false;
    }
    QString  localPath_  = Arn::fullPath( localPath);

    //// Make sure new path isn't within other mount points path and vice verse
    foreach (const MountPointSlot& mpSlot, _mountPoints) {
        QString  mplPath = mpSlot.localPath;
        if (localPath_.startsWith( mplPath) || mplPath.startsWith( localPath_)) {
            ArnM::errorLog( QString(tr("Mount points not exclusive: new=")) +
                            localPath + " existing=" + mplPath,
                            ArnError::Undef);
            return false;
        }
    }

    MountPointSlot  mpSlot;
    mpSlot.arnMountPoint = new ArnItemNetEar( this);
    bool  isOk = mpSlot.arnMountPoint->openFolder( localPath_);
    if (isOk) {
        mpSlot.localPath  = localPath_;
        mpSlot.remotePath = remotePath.isEmpty() ? localPath_ : Arn::fullPath( remotePath);
        connect( mpSlot.arnMountPoint, SIGNAL(arnItemCreated(QString)), this, SLOT(createNewItem(QString)));
        connect( mpSlot.arnMountPoint, SIGNAL(ArnTreeCreated(QString)),
                 this, SLOT(doCreateArnTree(QString)));
        connect( mpSlot.arnMountPoint, SIGNAL(ArnTreeDestroyed(QString,bool)),
                 this, SLOT(doDestroyArnTree(QString,bool)));
        _mountPoints += mpSlot;
        mpSlot.arnMountPoint->setReference( &_mountPoints.last());  // Give a ref to this added slot
    }
    else
        delete mpSlot.arnMountPoint;

    return isOk;
}


bool  ArnClient::removeMountPoint( const QString& localPath)
{
    QString  localPath_ = Arn::fullPath( localPath);

    int mpSize = _mountPoints.size();
    for (int i = 0; i < mpSize; ++ i) {
        const MountPointSlot&  mountPoint = _mountPoints.at(i);
        if (mountPoint.localPath == localPath_) {
            if (mountPoint.arnMountPoint)
                delete mountPoint.arnMountPoint;
            _mountPoints.removeAt(i);
            return true;
        }
    }
    return false;
}


void  ArnClient::setAutoConnect( bool isAuto, int retryTime)
{
    _isAutoConnect = isAuto;
    _retryTime = retryTime > 1 ? retryTime : 1;
}


void  ArnClient::registerClient( const QString& id)
{
    _id = id;
    ArnClientReg::instance().remove( this);
    ArnClientReg::instance().remove( id);
    ArnClientReg::instance().store( this, id);
}


ArnClient*  ArnClient::getClient( const QString& id)
{
    return ArnClientReg::instance().get( id);
}


QString  ArnClient::id()  const
{
    return _id;
}


int  ArnClient::receiveTimeout()  const
{
    return _receiveTimeout;
}


void  ArnClient::setReceiveTimeout( int receiveTimeout)
{
    _receiveTimeout = receiveTimeout;
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

    // qDebug() << "client-ls: path=" << path;
    _arnNetSync->sendXSMap( _commandMap);
}


void  ArnClient::commandVersion()
{
    _commandMap.clear();
    _commandMap.add(ARNRECNAME, "ver");

    _arnNetSync->sendXSMap( _commandMap);
}


// MW: Should probably be deprecated (removed)
void  ArnClient::commandExit()
{
    _arnNetSync->sendExit();
}


void  ArnClient::newNetItemProxy( ArnThreadCom *threadCom,
                                       const QString &path, int syncMode, void* isNewPtr)
{
    ArnThreadComProxyLock  proxyLock( threadCom);

    if (Arn::debugThreading)  qDebug() << "newNetItemProxy: path=" << path;
    threadCom->_retObj = newNetItem( path, Arn::ObjectSyncMode::fromInt( syncMode), (bool*) isNewPtr);
    if (Arn::debugThreading)  qDebug() << "newNetItemProxy: waking thread";
}


ArnItemNet*  ArnClient::newNetItem( const QString& path, Arn::ObjectSyncMode syncMode, bool* isNewPtr)
{
    if (ArnM::isMainThread()) {
        QString  path_ = Arn::fullPath( path);
        MountPointSlot  mpSlot;
        foreach (const MountPointSlot& mountPoint, _mountPoints) {
            if (path_.startsWith( mountPoint.localPath)) {
                mpSlot = mountPoint;
                break;
            }
        }
        return _arnNetSync->newNetItem( path_, mpSlot.localPath, mpSlot.remotePath, syncMode, isNewPtr);
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
                                   Q_ARG( int, syncMode.toInt()),
                                   Q_ARG( void*, isNewPtr));
        threadCom.waitCommandEnd();  // Wait main-thread gives retObj
        ArnItemNet*  retItemNet = qobject_cast<ArnItemNet*>( threadCom.p()->_retObj);
        if (retItemNet)  if (Arn::debugThreading)  qDebug() << "newNetItem-thread: end path=" << retItemNet->path();

        return retItemNet;
    }
}


void  ArnClient::createNewItem( const QString& path)
{
    // qDebug() << "ArnClient,ArnItem-created: path=" << path;
    ArnItemNetEar*  item = qobject_cast<ArnItemNetEar*>( sender());
    Q_ASSERT(item);
    MountPointSlot*  mpSlot = static_cast<MountPointSlot*>( item->reference());
    Q_ASSERT(mpSlot);

    _arnNetSync->newNetItem( path, mpSlot->localPath, mpSlot->remotePath);
}


void  ArnClient::doCreateArnTree( const QString& path)
{
    qDebug() << "ArnClient,CreateArnTree: path=" << path;
    ArnItemNetEar*  item = qobject_cast<ArnItemNetEar*>( sender());
    Q_ASSERT(item);
    MountPointSlot*  mpSlot = static_cast<MountPointSlot*>( item->reference());
    Q_ASSERT(mpSlot);

    QString  remotePath = Arn::changeBasePath( mpSlot->localPath, mpSlot->remotePath, path);
    _arnNetSync->sendSetTree( remotePath);
}


void ArnClient::doDestroyArnTree( const QString& path, bool isGlobal)
{
    // qDebug() << "ArnClient,DestroyArnTree: path=" << path;
    ArnItemNetEar*  item = qobject_cast<ArnItemNetEar*>( sender());
    Q_ASSERT(item);
    MountPointSlot*  mpSlot = static_cast<MountPointSlot*>( item->reference());
    Q_ASSERT(mpSlot);

    QString  remotePath = Arn::changeBasePath( mpSlot->localPath, mpSlot->remotePath, path);
    if (isGlobal)
        _arnNetSync->sendDelete( remotePath);
    else
        _arnNetSync->sendNoSync( remotePath);
}


void  ArnClient::doTcpError( QAbstractSocket::SocketError socketError)
{
    _recTimer->stop();

    QMetaObject::invokeMethod( this,
                               "doTcpError",
                               Qt::QueuedConnection,
                               Q_ARG( int, socketError));
}


void ArnClient::doTcpError( int socketError)
{
    // qDebug() << "ArnClient TcpError: hostAddr=" << _curConnectAP.addr;
    QString  errTextSum = QString(tr("TCP Client Msg:")) + _socket->errorString();
    ArnM::errorLog( errTextSum, ArnError::ConnectionError);
    emit tcpError( _socket->errorString(), QAbstractSocket::SocketError( socketError));

    if (_connectStat != ConnectStat::Disconnected) {
        _connectStat = (  (_connectStat == ConnectStat::Connected)
                       || (_connectStat == ConnectStat::Stopped))
                     ? ConnectStat::Disconnected : ConnectStat::Error;
        emit connectionStatusChanged( _connectStat, _curPrio);
        reConnectArn();
    }
}


void ArnClient::doTcpDisconnected()
{
    // qDebug() << "ArnClient TcpDisconnected: hostAddr=" << _curConnectAP.addr;
    _recTimer->stop();

    if ((_connectStat == ConnectStat::Connected) || (_connectStat == ConnectStat::Stopped)) {
        _connectStat = ConnectStat::Disconnected;
        emit connectionStatusChanged( _connectStat, _curPrio);
        reConnectArn();
    }
}


void  ArnClient::reConnectArn()
{
    // qDebug() << " reConnectArn";
    if ((_nextHost >= 0) && (_nextHost < _hostTab.size())) {
        doConnectArnLogic();
    }
    else if (_isAutoConnect) {
        _connectTimer->start( _retryTime * 1000);
    }
}


void  ArnClient::onConnectWaitDone()
{
    _connectTimer->stop();
    doConnectArnLogic();
}


void  ArnClient::doTcpConnected()
{
    // qDebug() << "ArnClient TcpConnected: hostAddr=" << _curConnectAP.addr;
    if (_receiveTimeout > 0)
        _recTimer->start( _receiveTimeout * 1000 / 2);

    emit tcpConnected( _curConnectAP.addr, _curConnectAP.port);
    _connectStat = ConnectStat::Connected;
    emit connectionStatusChanged( _connectStat, _curPrio);
}


void ArnClient::doRecNotified()
{
    if (_recTimer->isActive())
        _recTimer->start();
    _recTimeoutCount = 0;

    if (_connectStat == ConnectStat::Stopped) {
        _connectStat = ConnectStat::Connected;
        emit connectionStatusChanged( _connectStat, _curPrio);
    }
}


void ArnClient::doRecTimeout()
{
    ++_recTimeoutCount;

    if (_recTimeoutCount == 1) {
        commandVersion();  // Use version command as a flow requester
    }
    else if (_recTimeoutCount == 6) {
        _socket->abort();
    }

    if (_recTimeoutCount > 1) {
        if (_connectStat == ConnectStat::Connected) {
            _connectStat = ConnectStat::Stopped;
            emit connectionStatusChanged( _connectStat, _curPrio);
        }
    }
}


void  ArnClient::onCommandDelete( const QString& remotePath)
{
    // qDebug() << "ArnClient-delete: remotePath=" << remotePath;
    QString  localPath;
    foreach (const MountPointSlot& mountPoint, _mountPoints) {
        if (remotePath.startsWith( mountPoint.remotePath)) {
            localPath = Arn::changeBasePath( mountPoint.remotePath, mountPoint.localPath, remotePath);
            break;
        }
    }
    if (!localPath.isEmpty())
        ArnM::destroyLink( localPath);
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
        // qDebug() << "ArnClient connectlogic: hostTabSize=" << _hostTab.size() << " index=" << _nextHost
        //          << " prio=" << _hostPrioTab.at( _nextHost);
        ++_nextHost;
    }

    if (arnHost.isEmpty())  return;

    if (port == 0)
        port = Arn::defaultTcpPort;

    _socket->abort();
    _socket->connectToHost( Arn::hostFromHostWithInfo( arnHost), port);
    _curConnectAP.addr = arnHost;
    _curConnectAP.port = port;
    _curPrio           = curPrio;
    _connectStat       = ConnectStat::Connecting;

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
