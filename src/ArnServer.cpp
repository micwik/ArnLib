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

#include "ArnInc/ArnServer.hpp"
#include "private/ArnServer_p.hpp"
#include "ArnInc/ArnError.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnSync.hpp"
#include "ArnSyncLogin.hpp"
#include "ArnItemNet.hpp"
#include <QSslSocket>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QPair>
#include <QDebug>

using Arn::XStringMap;


ArnServerSession::ArnServerSession( QSslSocket* socket, ArnServer* arnServer)
    : QObject( arnServer)
{
    QHostAddress  remoteAddr = socket->peerAddress();
    // QHostAddress  localAddr  = socket->localAddress();
    // qDebug() << "ArnServerNetSync: remoteAddr=" << remoteAddr.toString()
    //          << " localAddr=" << localAddr.toString();

    _socket    = socket;
    _arnServer = arnServer;
    _socket->setParent( this);  // Session takes ownership of socket
    _arnNetSync = new ArnSync( _socket, false, this);
    _arnNetSync->setSessionHandler( this);
    _arnNetSync->setArnLogin( _arnServer->arnLogin());
    _arnNetSync->setDemandLogin( _arnServer->isDemandLogin()
                              && _arnServer->isDemandLoginNet( remoteAddr));
    _arnNetSync->setEncryptPolicy( _arnServer->encryptPolicy());
    // qDebug() << "ArnServerNetSync new session: remoteAddr=" << remoteAddr.toString()
    //          << "isDemandLoginNet=" << _arnServer->isDemandLoginNet( remoteAddr);
    _arnNetSync->start();

    foreach (const QString& path, _arnServer->freePaths()) {
        _arnNetSync->addFreePath( path);
    }
    _arnNetSync->setWhoIAm( _arnServer->whoIAm());

    _arnNetEar  = new ArnItemNetEar( this);
    _arnNetEar->open("/");  // MW: Optimize to only mountPoint:s ?

    connect( _arnNetSync, SIGNAL(stateChanged(int)), this, SLOT(doSyncStateChanged(int)));
    connect( _arnNetSync, SIGNAL(destroyed(QObject*)), this, SLOT(shutdown()));
    connect( _arnNetSync, SIGNAL(xcomDelete(QString)), this, SLOT(onCommandDelete(QString)));
    connect( _arnNetSync, SIGNAL(infoReceived(int)), this, SIGNAL(infoReceived(int)));
    connect( _arnNetSync, SIGNAL(loginCompleted()), this, SIGNAL(loginCompleted()));
    connect( _arnNetSync, SIGNAL(messageReceived(int,QByteArray)),
             this, SIGNAL(messageReceived(int,QByteArray)));
    connect( _arnNetEar, SIGNAL(arnTreeDestroyed(QString,bool)),
             this, SLOT(doDestroyArnTree(QString,bool)));
}


void  ArnServerSession::shutdown()
{
    _arnNetSync = arnNullptr;  // Mark retired
    _arnNetEar->close();
    deleteLater();
}


void  ArnServerSession::doDestroyArnTree( const QString& path, bool isGlobal)
{
    Q_UNUSED(isGlobal)  // Destruction of tree on server will allways be global

    if (!_arnNetSync)  return;  // Retired

    _arnNetSync->sendDelete( path);
}


void  ArnServerSession::onCommandDelete( const QString& path)
{
    if (!_arnNetSync)  return;  // Retired

    // qDebug() << "ArnServerNetSync-delete: path=" << path;
    ArnM::destroyLink( path);
}


void  ArnServerSession::doSyncStateChanged( int state)
{
    if (!_arnNetSync)  return;  // Retired

    // qDebug() << "ArnServer sync state changed: state=" << state;
    ArnSync::State  syncState = ArnSync::State::fromInt( state);
    if (syncState == syncState.Normal) {
        // qDebug() << "ArnServer connected: remVer="
        //          << _arnNetSync->remoteVer(0) << _arnNetSync->remoteVer(1);
    }
}


QSslSocket*  ArnServerSession::socket()  const
{
    return _socket;
}


Arn::XStringMap  ArnServerSession::remoteWhoIAm()  const
{
    if (!_arnNetSync)  return XStringMap();  // Retired

    return XStringMap( _arnNetSync->remoteWhoIAm());
}


QString  ArnServerSession::loginUserName()  const
{
    if (!_arnNetSync)  return QString();  // Retired

    return _arnNetSync->loginUserName();
}


Arn::Allow  ArnServerSession::getAllow()  const
{
    if (!_arnNetSync)  return Arn::Allow();  // Retired

    return _arnNetSync->getAllow();
}


void  ArnServerSession::sendMessage( int type, const QByteArray& data)
{
    if (!_arnNetSync)  return;  // Retired

    _arnNetSync->sendMessage( type, data);
}


bool  ArnServerSession::getTraffic( quint64& in, quint64& out)  const
{
    if (!_arnNetSync)  return false;  // Retired

    _arnNetSync->getTraffic( in, out);
    return true;
}



ArnServerPrivate::ArnServerPrivate( ArnServer::Type serverType)
{
    _tcpServerActive = false;
    _isDemandLogin   = false;
    _sslServer       = new ArnSslServer;
    _arnLogin        = new ArnSyncLogin;
    _newSession      = arnNullptr;
    _serverType      = serverType;
    _freePathTab    += Arn::fullPath( Arn::pathLocalSys + "Legal/");
    _encryptPol      = Arn::EncryptPolicy::PreferNo;
}


ArnServerPrivate::~ArnServerPrivate()
{
    delete _sslServer;
    delete _arnLogin;
}


ArnSslServer::ArnSslServer()
{
}


ArnSslServer::~ArnSslServer()
{
}


void  ArnSslServer::incomingConnection( ARNSOCKD socketDescriptor)
{
    QSslSocket*  socket = new QSslSocket;
    if (socket->setSocketDescriptor( socketDescriptor)) {
        addPendingConnection( socket);
    }
    else {
        delete socket;
    }
}


QSslSocket*  ArnSslServer::nextPendingSslConnection()
{
    QTcpSocket*  socket = QTcpServer::nextPendingConnection();
    if (!socket) {
        ArnM::errorLog( QString(tr("Server socket is null")), ArnError::Undef);
        return arnNullptr;
    }

    QSslSocket*  sslSocket = qobject_cast<QSslSocket*>( socket);
    if (!sslSocket) {
        ArnM::errorLog( QString(tr("Server socket is not ssl")), ArnError::Undef);
    }
    return sslSocket;
}



ArnServer::ArnServer( Type serverType, QObject *parent)
    : QObject( parent)
    , d_ptr( new ArnServerPrivate( serverType))
{
}


ArnServer::ArnServer( ArnServerPrivate& dd, QObject* parent)
    : QObject( parent)
    , d_ptr( &dd)
{
}


ArnServer::~ArnServer()
{
    delete d_ptr;
}


void  ArnServer::start( int port, QHostAddress listenAddr)
{
    Q_D(ArnServer);

    if (port < 0) {
        switch (d->_serverType) {
        case Type::NetSync:
            port = Arn::defaultTcpPort;
            break;
        default:
            ArnM::errorLog( QString(tr("Unknown Arn server Type: ")) + QString::number( d->_serverType),
                                ArnError::Undef);
            return;
        }
    }

    if (d->_sslServer->listen( listenAddr, port)) {
        d->_tcpServerActive = true;

        connect( d->_sslServer, SIGNAL(newConnection()), this, SLOT(tcpConnection()));
    }
    else {
        ArnM::errorLog( QString(tr("Failed start Arn Server Port: ")) + QString::number( port),
                        ArnError::ConnectionError);
    }
}


int  ArnServer::port()
{
    Q_D(ArnServer);

    return d->_sslServer->serverPort();
}


QHostAddress  ArnServer::listenAddress()
{
    Q_D(ArnServer);

    QHostAddress  addr = d->_sslServer->serverAddress();
    return addr;
}


void ArnServer::addAccess(const QString& userName, const QString& password, Arn::Allow allow)
{
    Q_D(ArnServer);

    d->_arnLogin->addAccess( userName, password, allow);
}


bool  ArnServer::isDemandLogin()  const
{
    Q_D(const ArnServer);

    return d->_isDemandLogin;
}


void  ArnServer::setDemandLogin( bool isDemandLogin)
{
    Q_D(ArnServer);

    d->_isDemandLogin = isDemandLogin;
}


void  ArnServer::setNoLoginNets( const QStringList& noLoginNets)
{
    Q_D(ArnServer);

    d->_noLoginNets = noLoginNets;
}


QStringList  ArnServer::noLoginNets()  const
{
    Q_D(const ArnServer);

    return d->_noLoginNets;
}


bool  ArnServer::isDemandLoginNet( const QHostAddress& remoteAddr)  const
{
    Q_D(const ArnServer);

    QHostAddress remoteAddrV6 = QHostAddress( remoteAddr.toIPv6Address());

    foreach (const QString& noLoginNet, d->_noLoginNets) {
        bool  chkLocalHost = noLoginNet == "localhost";
        if (chkLocalHost || (noLoginNet == "localnet")) {
            if (remoteAddrV6 == QHostAddress( QHostAddress::LocalHostIPv6))
                return false;  // Localhost, ok for both localhost & localnet

            foreach (QNetworkInterface  interface, QNetworkInterface::allInterfaces()) {
                QNetworkInterface::InterfaceFlags  flags = interface.flags();
                if (flags.testFlag( QNetworkInterface::IsPointToPoint)
                || flags.testFlag( QNetworkInterface::IsLoopBack))
                    continue;

                foreach (QNetworkAddressEntry  entry, interface.addressEntries()) {
                    QHostAddress entryIp = entry.ip();
                    QAbstractSocket::NetworkLayerProtocol  prot = entryIp.protocol();
                    if ((prot != QAbstractSocket::IPv4Protocol) && (prot != QAbstractSocket::IPv6Protocol))
                        continue;

                    QHostAddress entryIpV6 = QHostAddress( entryIp.toIPv6Address());
                    int prefixOffs = (prot == QAbstractSocket::IPv4Protocol) ? 96 : 0;
                    int prefixV6   = entry.prefixLength() + prefixOffs;

                    if (entry.prefixLength() < 0) {
                        // This is a bug in some Qt for android, windows ...  (Not linux)
                        prefixV6 = 24 + prefixOffs;
                        qWarning() << "Bad netmask: nif=" << interface.humanReadableName()
                                                          << ", asume prefixV6Len(=" << prefixV6;
                    }

                    if (entryIpV6 == remoteAddrV6)  // Address to this host ip, ok for both localhost & localnet
                        return false;

                    if (!chkLocalHost && remoteAddrV6.isInSubnet( entryIpV6, prefixV6))
                        return false;
                }
            }
        }
        else if (noLoginNet == "any") {
            return false;
        }
        else {
            QPair<QHostAddress, int> subnet = QHostAddress::parseSubnet( noLoginNet);
            QHostAddress subnetIpV6 = QHostAddress( subnet.first.toIPv6Address());
            int prefixOffs = (subnet.first.protocol() == QAbstractSocket::IPv4Protocol) ? 96 : 0;
            int prefixV6   = subnet.second + prefixOffs;
            if (remoteAddrV6.isInSubnet( subnetIpV6, prefixV6))
                return false;
        }
    }

    return true;
}


void  ArnServer::setEncryptPolicy( const Arn::EncryptPolicy& pol)
{
    Q_D(ArnServer);

    d->_encryptPol = pol;
}


Arn::EncryptPolicy  ArnServer::encryptPolicy()  const
{
    Q_D(const ArnServer);

    return d->_encryptPol;
}


void  ArnServer::addFreePath( const QString& path)
{
    Q_D(ArnServer);

    if (!d->_freePathTab.contains( path))
        d->_freePathTab += path;
}


QStringList  ArnServer::freePaths()  const
{
    Q_D(const ArnServer);

    return d->_freePathTab;
}


ArnSyncLogin*  ArnServer::arnLogin()  const
{
    Q_D(const ArnServer);

    return d->_arnLogin;
}


ArnServerSession*  ArnServer::getSession()  const
{
    Q_D(const ArnServer);

    return d->_newSession;
}


QByteArray  ArnServer::whoIAm()  const
{
    Q_D(const ArnServer);

    return d->_whoIAm;
}


void  ArnServer::setWhoIAm( const Arn::XStringMap& whoIAmXsm)
{
    Q_D(ArnServer);

    d->_whoIAm = whoIAmXsm.toXString();
}


void  ArnServer::tcpConnection()
{
    Q_D(ArnServer);


    QSslSocket*  socket = d->_sslServer->nextPendingSslConnection();
    if (!socket) {  // Socket not ok
        return;
    }
    if (socket->peerPort() == 0) {  // Socket not connected ok
        socket->deleteLater();
        return;
    }

    switch (d->_serverType) {
    case Type::NetSync:
        d->_newSession = new ArnServerSession( socket, this);        
        emit newSession();
        d->_newSession = arnNullptr;
        break;
    }
}
