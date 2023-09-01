// Copyright (C) 2010-2022 Michael Wiklund.
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

#include "ArnInc/ArnClient.hpp"
#include "private/ArnClient_p.hpp"
#include "ArnInc/Arn.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnSync.hpp"
#include "ArnSyncLogin.hpp"
#include <QSslSocket>
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

    return _clientTab.value( id, arnNullptr);
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



ArnClientPrivate::ArnClientPrivate()
{
    _arnMountPoint   = arnNullptr;
    _isAutoConnect   = false;
    _isValidCredent  = false;
    _isClosed        = true;
    _receiveTimeout  = 10;
    _recTimeoutCount = 0;
    _retryTime       = 2;
    _port            = 0;
    _nextHost        = -1;
    _curPrio         = -1;
    _syncMode        = ArnClient::SyncMode::StdAutoMaster;
    resetConnectionFlags();

    _socket       = new QSslSocket;
    _arnNetSync   = new ArnSync( _socket, true,  arnNullptr);
    _arnNetSync->setClientSyncMode( _syncMode);
    _arnNetSync->start();
    _connectTimer = new QTimer;
    _recTimer     = new QTimer;
}


ArnClientPrivate::~ArnClientPrivate()
{
    delete _socket;
    delete _arnNetSync;
    delete _connectTimer;
    delete _recTimer;
}


void  ArnClientPrivate::resetConnectionFlags()
{
    _isReContact = false;
    _isReConnect = false;
    _wasContact  = false;
    _wasConnect  = false;
}


void  ArnClientPrivate::clearArnList( int prioFilter)
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


ArnClient::HostList  ArnClientPrivate::arnList( int prioFilter)  const
{
    if ((prioFilter < 0) || _hostTab.isEmpty())
        return _hostTab;

    ArnClient::HostList  retVal;
    int  index = -1;
    forever {
        index = _hostPrioTab.indexOf( prioFilter, index + 1);
        if (index < 0)  break;
        retVal += _hostTab.at( index);
    }
    return retVal;
}


void  ArnClientPrivate::addToArnList( const QString& arnHost, quint16 port, int prio)
{
    if (arnHost.isEmpty())  return;  // Invalid

    ArnClient::HostAddrPort  slot;
    slot.addr = arnHost;
    slot.port = port ? port : Arn::defaultTcpPort;

    int index;
    for (index = 0; index < _hostPrioTab.size(); ++index) {
        if (prio < _hostPrioTab.at( index))  break;  // Found place
    }
    _hostTab.insert( index, slot);
    _hostPrioTab.insert( index, prio);
}


void  ArnClient::init()
{
    Q_D(ArnClient);

    QString  stdId = "std";
    if (ArnClientReg::instance().store( this, stdId))  // Only use std-id once
        d->_id = stdId;

    ArnSync*    arnSync = d->_arnNetSync;
    QSslSocket*  socket = d->_socket;
    arnSync->setSessionHandler( this);
    arnSync->setToRemotePathCB( &toRemotePathCB);
    connect( socket, SIGNAL(connected()), this, SLOT(doTcpConnected()));
    connect( socket, SIGNAL(disconnected()), this, SLOT(doTcpDisconnected()));
    connect( arnSync, SIGNAL(loginRequired(int)), this, SLOT(doLoginRequired(int)));
    connect( arnSync, SIGNAL(stateChanged(int)), this, SLOT(doSyncStateChanged(int)));
    connect( arnSync, SIGNAL(replyRecord(Arn::XStringMap&)), this, SLOT(doReplyRecord(Arn::XStringMap&)));
    connect( arnSync, SIGNAL(replyRecord(Arn::XStringMap&)), this, SIGNAL(replyRecord(Arn::XStringMap&)));
    connect( arnSync, SIGNAL(xcomDelete(QString)), this, SLOT(onCommandDelete(QString)));
    connect( arnSync, SIGNAL(messageReceived(int,QByteArray)),
             this, SLOT(onMessageReceived(int,QByteArray)));
    connect( d->_recTimer, SIGNAL(timeout()), this, SLOT(doRecTimeout()));
    connect( socket, SIGNAL(readyRead()), this, SLOT(doRecNotified()));
#if (QT_VERSION >= QT_VERSION_CHECK( 5, 15, 0))
    connect( socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
             this, SLOT(doTcpError(QAbstractSocket::SocketError)));
#else
    connect( socket, SIGNAL(error(QAbstractSocket::SocketError)),
             this, SLOT(doTcpError(QAbstractSocket::SocketError)));
#endif
    connect( d->_connectTimer, SIGNAL(timeout()), this, SLOT(onConnectWaitDone()));
}


ArnClient::ArnClient( QObject* parent)
    : QObject( parent)
    , d_ptr( new ArnClientPrivate)
{
    init();
}


ArnClient::ArnClient( ArnClientPrivate& dd, QObject* parent)
    : QObject( parent)
    , d_ptr( &dd)
{
    init();
}


ArnClient::~ArnClient()
{
    ArnClientReg::instance().remove( this);  // If registered, remove it

    delete d_ptr;
}


void  ArnClient::clearArnList( int prioFilter)
{
    Q_D(ArnClient);

    d->clearArnList( prioFilter);
}


ArnClient::HostList  ArnClient::arnList( int prioFilter)  const
{
    Q_D(const ArnClient);

    return d->arnList( prioFilter);
}


void  ArnClient::addToArnList( const QString &arnHost, quint16 port, int prio)
{
    Q_D(ArnClient);

    d->addToArnList( arnHost, port, prio);
}


void  ArnClient::connectToArnList()
{
    Q_D(ArnClient);

    if (d->_hostTab.isEmpty())  return;

    d->_isValidCredent = false;
    d->_nextHost       = 0;
    d->resetConnectionFlags();
    startConnectArn();
}


void  ArnClient::connectToArn( const QString& arnHost, quint16 port)
{
    Q_D(ArnClient);

    d->_isValidCredent = false;
    d->_nextHost       = -1;
    d->_arnHost        = arnHost;
    d->_port           = port ? port : Arn::defaultTcpPort;
    d->resetConnectionFlags();
    startConnectArn();
}


void  ArnClient::disconnectFromArn()
{
    Q_D(ArnClient);

    d->_arnNetSync->sendExit();
    setAutoConnect( false);
    d->_socket->disconnectFromHost();
    d->_isClosed = true;
}


void  ArnClient::loginToArn( const QString& userName, const QString& password, Arn::Allow allow)
{
    QString  pwHash = passwordHash( password);
    loginToArnHashed( userName, pwHash, allow);
}


void  ArnClient::loginToArnHashed( const QString& userName, const QString& passwordHashed, Arn::Allow allow)
{
    Q_D(ArnClient);

    d->_isValidCredent = !userName.isEmpty();
    d->_arnNetSync->loginToArn( userName, passwordHashed, allow);
}


void ArnClient::close()
{
    Q_D(ArnClient);

    setAutoConnect( false);
    d->_arnNetSync->close();
    d->_isClosed = true;
}


ArnClient::ConnectStat  ArnClient::connectStatus()  const
{
    Q_D(const ArnClient);

    return d->_connectStat;
}


bool  ArnClient::setMountPoint( const QString& path)
{
    Q_D(ArnClient);

    QMutexLocker  locker( &d->_mutex);

    if (d->_mountPoints.size() == 1)
        removeMountPointNL( d->_mountPoints.at(0).localPath);

    return addMountPointNL( path, QString());
}


bool  ArnClient::addMountPoint( const QString& localPath, const QString& remotePath)
{
    Q_D(ArnClient);

    d->_mutex.lock();
    bool  retVal = addMountPointNL( localPath, remotePath);
    d->_mutex.unlock();

    return retVal;
}


bool  ArnClient::addMountPointNL( const QString& localPath, const QString& remotePath)
{
    Q_D(ArnClient);

    if (Arn::debugShareObj)  qDebug() << "Adding mount point: localPath=" << localPath
                                      << " remotePath=" << remotePath;
    if (localPath.isEmpty()) {
        return false;
    }
    QString  localPath_  = Arn::fullPath( localPath);

    //// Make sure new path isn't within other mount points path and vice verse
    foreach (const MountPointSlot& mpSlot, d->_mountPoints) {
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
        connect( mpSlot.arnMountPoint, SIGNAL(arnTreeCreated(QString)),
                 this, SLOT(doCreateArnTree(QString)));
        connect( mpSlot.arnMountPoint, SIGNAL(arnTreeDestroyed(QString,bool)),
                 this, SLOT(doDestroyArnTree(QString,bool)));
        d->_mountPoints += mpSlot;
        mpSlot.arnMountPoint->setReference( &d->_mountPoints.last());  // Give a ref to this added slot
    }
    else
        delete mpSlot.arnMountPoint;

    return isOk;
}


bool  ArnClient::removeMountPoint( const QString& localPath)
{
    Q_D(ArnClient);

    d->_mutex.lock();
    bool  retVal = removeMountPointNL( localPath);
    d->_mutex.unlock();

    return retVal;
}


bool  ArnClient::removeMountPointNL( const QString& localPath)
{
    Q_D(ArnClient);

    QString  localPath_ = Arn::fullPath( localPath);

    int mpSize = d->_mountPoints.size();
    for (int i = 0; i < mpSize; ++ i) {
        const MountPointSlot&  mountPoint = d->_mountPoints.at(i);
        if (mountPoint.localPath == localPath_) {
            if (mountPoint.arnMountPoint)
                delete mountPoint.arnMountPoint;
            d->_mountPoints.removeAt(i);
            return true;
        }
    }
    return false;
}


void  ArnClient::setAutoConnect( bool isAuto, int retryTime)
{
    Q_D(ArnClient);

    d->_isAutoConnect = isAuto;
    d->_retryTime = retryTime > 1 ? retryTime : 1;
}


void  ArnClient::registerClient( const QString& id)
{
    Q_D(ArnClient);

    d->_id = id;
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
    Q_D(const ArnClient);

    return d->_id;
}


int  ArnClient::receiveTimeout()  const
{
    Q_D(const ArnClient);

    return d->_receiveTimeout;
}


void  ArnClient::setReceiveTimeout( int receiveTimeout)
{
    Q_D(ArnClient);

    d->_receiveTimeout = receiveTimeout;
}


bool  ArnClient::isDemandLogin()  const
{
    Q_D(const ArnClient);

    return d->_arnNetSync->isDemandLogin();
}


void  ArnClient::setDemandLogin( bool isDemandLogin)
{
    Q_D(ArnClient);

    d->_arnNetSync->setDemandLogin( isDemandLogin);
}


ArnClient::SyncMode  ArnClient::syncMode()  const
{
    Q_D(const ArnClient);

    return d->_syncMode;
}


void  ArnClient::setSyncMode( ArnClient::SyncMode syncMode)
{
    Q_D(ArnClient);

    if (syncMode != SyncMode::Invalid) {
        d->_syncMode = syncMode;
        d->_arnNetSync->setClientSyncMode( syncMode);
    }
}


Arn::EncryptPolicy  ArnClient::encryptPolicy()  const
{
    Q_D(const ArnClient);

    return d->_arnNetSync->encryptPolicy();
}


void  ArnClient::setEncryptPolicy( const Arn::EncryptPolicy& pol)
{
    Q_D(ArnClient);
    if (d->_isClosed) {  // Must not have started connection ...
        d->_arnNetSync->setEncryptPolicy( pol);
    }
}


QString  ArnClient::passwordHash( const QString& password)
{
    return ArnSyncLogin::passwordHash( password);
}


QStringList  ArnClient::freePaths()  const
{
    Q_D(const ArnClient);

    return d->_arnNetSync->freePaths();
}


void  ArnClient::setWhoIAm( const Arn::XStringMap& whoIAmXsm)
{
    Q_D(ArnClient);

    d->_arnNetSync->setWhoIAm( whoIAmXsm.toXString());
}


Arn::XStringMap  ArnClient::remoteWhoIAm()  const
{
    Q_D(const ArnClient);

    return XStringMap( d->_arnNetSync->remoteWhoIAm());
}


bool  ArnClient::isReContact()  const
{
    Q_D(const ArnClient);

    return d->_isReContact;
}


bool  ArnClient::isReConnect()  const
{
    Q_D(const ArnClient);

    return d->_isReConnect;
}


bool  ArnClient::isEncrypted()  const
{
    Q_D(const ArnClient);

    return d->_socket->isEncrypted();
}


int ArnClient::curPrio()  const
{
    Q_D(const ArnClient);

    return d->_curPrio;
}


void  ArnClient::chatSend( const QString& text, int prioType)
{
    Q_D(ArnClient);

    d->_arnNetSync->sendMessage( prioType == 1 ? ArnSync::MessageType::ChatPrio
                                               : ArnSync::MessageType::ChatNormal,
                                 text.toUtf8());
}


void ArnClient::abortKillRequest()
{
    Q_D(ArnClient);

    d->_arnNetSync->sendMessage( ArnSync::MessageType::AbortKillRequest);
}


bool  ArnClient::getTraffic( quint64& in, quint64& out)  const
{
    Q_D(const ArnClient);

    d->_arnNetSync->getTraffic( in, out);
    return true;
}


void  ArnClient::commandGet( const QString& path)
{
    Q_D(ArnClient);

    d->_commandMap.clear();
    d->_commandMap.add(ARNRECNAME, "get").add("path", path);

    d->_arnNetSync->sendXSMap( d->_commandMap);
}


void  ArnClient::commandSet( const QString& path, const QString& data)
{
    Q_D(ArnClient);

    d->_commandMap.clear();
    d->_commandMap.add(ARNRECNAME, "set").add("path", path).add("data", data);

    d->_arnNetSync->sendXSMap( d->_commandMap);
}


void  ArnClient::commandLs( const QString& path)
{
    Q_D(ArnClient);

    d->_commandMap.clear();
    d->_commandMap.add(ARNRECNAME, "ls").add("path", path);

    // qDebug() << "client-ls: path=" << path;
    d->_arnNetSync->sendXSMap( d->_commandMap);
}


void  ArnClient::commandInfo( int type, const QByteArray& data)
{
    Q_D(ArnClient);

    if (type < Arn::InfoType::N)
        d->_arnNetSync->sendInfo( type, data);
}


void  ArnClient::commandVersion()
{
    Q_D(ArnClient);

    d->_commandMap.clear();
    d->_commandMap.add(ARNRECNAME, "ver");

    d->_arnNetSync->sendXSMap( d->_commandMap);
}


void  ArnClient::newNetItemProxy( ArnThreadCom *threadCom,
                                       const QString &path, int syncMode, void* isNewPtr)
{
    ArnThreadComProxyLock  proxyLock( threadCom);

    if (Arn::debugThreading)  qDebug() << "newNetItemProxy: path=" << path;
    threadCom->_retObj = newNetItem( path, Arn::ObjectSyncMode::fromInt( syncMode), (bool*) isNewPtr);
    if (Arn::debugThreading)  qDebug() << "newNetItemProxy: waking thread";
}


bool  ArnClient::getLocalRemotePath( const QString& path,
                                     QString& localMountPath, QString& remoteMountPath)
{
    Q_D(ArnClient);

    QMutexLocker  locker( &d->_mutex);

    bool  retVal = false;
    QString  path_ = Arn::fullPath( path);
    MountPointSlot  mpSlot;
    foreach (const MountPointSlot& mountPoint, d->_mountPoints) {
        if (path_.startsWith( mountPoint.localPath)) {
            mpSlot = mountPoint;
            retVal = true;
            break;
        }
    }
    localMountPath  = mpSlot.localPath;
    remoteMountPath = mpSlot.remotePath;

    return retVal;
}


QString  ArnClient::toRemotePathCB( void* context, const QString& path)
{
    ArnClient*  that = static_cast<ArnClient*>( context);
    Q_ASSERT(that);

    QString  localMountPath;
    QString  remoteMountPath;
    that->getLocalRemotePath( path, localMountPath, remoteMountPath);

    return Arn::changeBasePath( localMountPath,remoteMountPath, path);
}


ArnItemNet*  ArnClient::newNetItem( const QString& path, Arn::ObjectSyncMode syncMode, bool* isNewPtr)
{
    Q_D(ArnClient);

    if (ArnM::isMainThread()) {
        QString  path_ = Arn::fullPath( path);
        return d->_arnNetSync->newNetItem( path_, syncMode, isNewPtr);
    }
    else {  // Threaded - must be threadsafe
        ArnThreadComCaller  threadCom;

        threadCom.p()->_retObj = arnNullptr;  // Just in case ...
        if (Arn::debugThreading)  qDebug() << "newNetItem-thread: start path=" << path;
        QMetaObject::invokeMethod( this,
                                   "newNetItemProxy",
                                   Qt::QueuedConnection,
                                   Q_ARG( ArnThreadCom*, threadCom.p()),
                                   Q_ARG( QString, path),
                                   Q_ARG( int, syncMode.toInt()),
                                   Q_ARG( void*, isNewPtr));
        threadCom.waitCommandEnd();  // Wait main-thread gives retObj
        ArnItemNet*  retItemNet = static_cast<ArnItemNet*>( threadCom.p()->_retObj);
        if (retItemNet)  if (Arn::debugThreading)  qDebug() << "newNetItem-thread: end path=" << retItemNet->path();

        return retItemNet;
    }
}


void  ArnClient::createNewItem( const QString& path)
{
    Q_D(ArnClient);

    // qDebug() << "ArnClient,ArnItem-created: path=" << path;

    d->_arnNetSync->newNetItem( path);
}


void  ArnClient::doCreateArnTree( const QString& path)
{
    Q_D(ArnClient);

    // qDebug() << "ArnClient,CreateArnTree: path=" << path;
    ArnItemNetEar*  item = qobject_cast<ArnItemNetEar*>( sender());
    Q_ASSERT(item);
    MountPointSlot*  mpSlot = static_cast<MountPointSlot*>( item->reference());
    Q_ASSERT(mpSlot);

    QString  remotePath = Arn::changeBasePath( mpSlot->localPath, mpSlot->remotePath, path);
    d->_arnNetSync->sendSetTree( remotePath);
}


void ArnClient::doDestroyArnTree( const QString& path, bool isGlobal)
{
    Q_D(ArnClient);

    // qDebug() << "ArnClient,DestroyArnTree: path=" << path;
    ArnItemNetEar*  item = qobject_cast<ArnItemNetEar*>( sender());
    Q_ASSERT(item);
    MountPointSlot*  mpSlot = static_cast<MountPointSlot*>( item->reference());
    Q_ASSERT(mpSlot);

    QString  remotePath = Arn::changeBasePath( mpSlot->localPath, mpSlot->remotePath, path);
    if (isGlobal)
        d->_arnNetSync->sendDelete( remotePath);
    else
        d->_arnNetSync->sendNoSync( remotePath);
}


void  ArnClient::doTcpError( QAbstractSocket::SocketError socketError)
{
    Q_D(ArnClient);

    d->_recTimer->stop();

    QMetaObject::invokeMethod( this,
                               "doTcpError",
                               Qt::QueuedConnection,
                               Q_ARG( int, socketError));
}


void ArnClient::doTcpError( int socketError)
{
    Q_D(ArnClient);

    // qDebug() << "ArnClient TcpError: hostAddr=" << _curConnectAP.addr;
    QString  errTextSum = QString(tr("TCP Client Msg:")) + d->_socket->errorString();
    ArnM::errorLog( errTextSum, ArnError::ConnectionError);

    if (d->_connectStat != ConnectStat::Disconnected) {
        d->_connectStat = (  (d->_connectStat == ConnectStat::Connected)
                          || (d->_connectStat == ConnectStat::Stopped)
                          || (d->_connectStat == ConnectStat::Negotiating))
                        ? ConnectStat::Disconnected
                        : ConnectStat::Error;
        emit connectionStatusChanged( d->_connectStat, d->_curPrio);

        if (d->_connectStat == ConnectStat::Error)
            emit tcpError( d->_socket->errorString(), QAbstractSocket::SocketError( socketError));
        else if (d->_connectStat == ConnectStat::Disconnected)
            emit tcpDisConnected();

        reConnectArn();
    }
}


void ArnClient::doTcpDisconnected()
{
    Q_D(ArnClient);

    // qDebug() << "ArnClient TcpDisconnected: hostAddr=" << _curConnectAP.addr;
    d->_recTimer->stop();

    if ((d->_connectStat == ConnectStat::Connected)
    ||  (d->_connectStat == ConnectStat::Stopped)
    ||  (d->_connectStat == ConnectStat::Negotiating))
    {
        d->_connectStat = ConnectStat::Disconnected;
        emit connectionStatusChanged( d->_connectStat, d->_curPrio);
        emit tcpDisConnected();

        reConnectArn();
    }
}


void ArnClient::startConnectArn()
{
    Q_D(ArnClient);

    d->_isClosed = false;
    doConnectArnLogic();
}


void  ArnClient::reConnectArn()
{
    Q_D(ArnClient);

    // qDebug() << " reConnectArn";
    bool wantDelayConnect = true;
    if (d->_nextHost >= 0) {  // Using connection list
        doConnectArnLogic();
        wantDelayConnect = d->_nextHost == 0;  // WantDelay when tried all in connection list
    }

    if (wantDelayConnect && d->_isAutoConnect) {
        d->_connectTimer->start( d->_retryTime * 1000);
    }
}


void  ArnClient::onConnectWaitDone()
{
    Q_D(ArnClient);

    d->_connectTimer->stop();
    doConnectArnLogic();
}


void  ArnClient::doTcpConnected()
{
    Q_D(ArnClient);

    if (d->_isClosed) {  // Unexpected tcp connection, not wanted
        d->_socket->abort();
        return;
    }

    d->_isReContact = d->_wasContact;
    d->_wasContact  = true;

    if ((d->_receiveTimeout > 0) && !Arn::offHeartbeat)
        d->_recTimer->start( d->_receiveTimeout * 1000 / 2);
    d->_recTimeoutCount = 0;

    d->_arnNetSync->connected();

    emit tcpConnected( d->_curConnectAP.addr, d->_curConnectAP.port);
    d->_connectStat = ConnectStat::Negotiating;
    emit connectionStatusChanged( d->_connectStat, d->_curPrio);
}


void  ArnClient::doSyncStateChanged( int state)
{
    Q_D(ArnClient);

    // qDebug() << "ArnClient sync state changed: state=" << state;
    ArnSync::State  syncState = ArnSync::State::fromInt( state);
    if (syncState == syncState.Normal) {
        // qDebug() << "ArnClient connected: remVer="
        //          << _arnNetSync->remoteVer(0) << _arnNetSync->remoteVer(1);
        d->_isReConnect = d->_wasConnect;
        d->_wasConnect  = true;
        d->_connectStat = ConnectStat::Connected;
        emit connectionStatusChanged( d->_connectStat, d->_curPrio);
    }
}


void ArnClient::doRecNotified()
{
    Q_D(ArnClient);

    if (d->_recTimer->isActive())
        d->_recTimer->start();
    d->_recTimeoutCount = 0;

    if (d->_connectStat == ConnectStat::Stopped) {
        d->_connectStat = ConnectStat::Connected;
        emit connectionStatusChanged( d->_connectStat, d->_curPrio);
    }
}


void ArnClient::doRecTimeout()
{
    Q_D(ArnClient);

    ++d->_recTimeoutCount;

    if (d->_recTimeoutCount == 1) {
        commandVersion();  // Use version command as a flow requester
    }
    else if (d->_recTimeoutCount == 6) {
        d->_socket->abort();
    }

    if (d->_recTimeoutCount > 1) {
        if (d->_connectStat == ConnectStat::Connected) {
            d->_connectStat = ConnectStat::Stopped;
            emit connectionStatusChanged( d->_connectStat, d->_curPrio);
        }
    }
}


void  ArnClient::onCommandDelete( const QString& remotePath)
{
    Q_D(ArnClient);

    QMutexLocker  locker( &d->_mutex);

    // qDebug() << "ArnClient-delete: remotePath=" << remotePath;
    QString  localPath;
    foreach (const MountPointSlot& mountPoint, d->_mountPoints) {
        if (remotePath.startsWith( mountPoint.remotePath)) {
            localPath = Arn::changeBasePath( mountPoint.remotePath, mountPoint.localPath, remotePath);
            break;
        }
    }
    if (!localPath.isEmpty())
        ArnM::destroyLink( localPath);
}


void  ArnClient::onMessageReceived( int type, const QByteArray& data)
{
    // XStringMap xmIn( data);

    switch (type) {
    //// Internal message types
    case ArnSync::MessageType::KillRequest:
        emit killRequested();
        close();
        break;
    case ArnSync::MessageType::AbortKillRequest:
        // Not valid for client
        break;
    case ArnSync::MessageType::ChatPrio:
        emit chatReceived( QString::fromUtf8( data.constData(), data.size()), 1);
        break;
    case ArnSync::MessageType::ChatNormal:
        emit chatReceived( QString::fromUtf8( data.constData(), data.size()), 2);
        break;
    default:;
        // Not supported message-type.
    }
}


void  ArnClient::doConnectArnLogic()
{
    Q_D(ArnClient);

    QString  arnHost;
    quint16  port = 0;
    int  curPrio  = -1;
    d->_curPrio = curPrio;

    if (d->_nextHost < 0) {  // Normal single host connect
        arnHost  = d->_arnHost;
        port     = d->_port;
    }
    else if (!d->_hostTab.isEmpty()) {  // Arn connection list
        if (d->_nextHost >= d->_hostTab.size()) {  // Past end of list, restart
            d->_nextHost = 0;
            emit connectionStatusChanged( ConnectStat::TriedAll, -1);
            return;
        }

        const HostAddrPort&  slot = d->_hostTab.at( d->_nextHost);
        arnHost = slot.addr;
        port    = slot.port;
        curPrio = d->_hostPrioTab.at( d->_nextHost);
        // qDebug() << "ArnClient connectlogic: hostTabSize=" << _hostTab.size() << " index=" << _nextHost
        //          << " prio=" << _hostPrioTab.at( _nextHost);
        ++d->_nextHost;
    }

    if (arnHost.isEmpty())  return;

    if (port == 0)
        port = Arn::defaultTcpPort;

    d->_socket->abort();
    d->_socket->connectToHost( Arn::hostFromHostWithInfo( arnHost), port);
    d->_curConnectAP.addr = arnHost;
    d->_curConnectAP.port = port;
    d->_curPrio           = curPrio;
    d->_connectStat       = ConnectStat::Connecting;

    d->_arnNetSync->connectStarted();
    emit connectionStatusChanged( d->_connectStat, d->_curPrio);
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
    else if (reply == "Rinfo") {
        int         type = replyMap.value("type", "-1").toInt();
        QByteArray  data = replyMap.value("data");
        emit replyInfo( type, data);
    }
    else if (reply == "Rver") {
        QString  ver  = replyMap.valueString("ver",  "1.0");  // ver key only after version 1.0
        QString  type = replyMap.valueString("type", "ArnNetSync");
        emit replyVer( type + " ver " + ver);
    }
}


void  ArnClient::doLoginRequired( int contextCode)
{
    Q_D(ArnClient);

    if (d->_isValidCredent && (contextCode == 0))  // First login for this TCP session
        d->_arnNetSync->loginToArn();  // Try to use last credentials
    else {
        d->_isValidCredent = false;
        emit loginRequired( contextCode);
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
