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

#include "ArnInc/ArnServer.hpp"
#include "private/ArnServer_p.hpp"
#include "ArnInc/ArnError.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnSync.hpp"
#include "ArnSyncLogin.hpp"
#include "ArnItemNet.hpp"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QDebug>

using Arn::XStringMap;


ArnServerSession::ArnServerSession( QTcpSocket* socket, ArnServer* arnServer)
    : QObject( arnServer)
{
    QHostAddress  remoteAddr = socket->peerAddress();
    // QHostAddress  localAddr  = socket->localAddress();
    // qDebug() << "ArnServerNetSync: remoteAddr=" << remoteAddr.toString()
    //          << " localAddr=" << localAddr.toString();

    _socket    = socket;
    _arnServer = arnServer;
    _arnNetSync = new ArnSync( socket, false, this);
    _arnNetSync->setSessionHandler( this);
    _arnNetSync->setArnLogin( _arnServer->arnLogin());
    _arnNetSync->setDemandLogin( _arnServer->isDemandLogin()
                              && _arnServer->isDemandLoginNet( remoteAddr));
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
    _arnNetSync = 0;
    _arnNetEar->close();
    deleteLater();
}


void  ArnServerSession::doDestroyArnTree( const QString& path, bool isGlobal)
{
    Q_UNUSED(isGlobal)  // Destruction of tree on server will allways be global

    if (!_arnNetSync)  return;

    _arnNetSync->sendDelete( path);
}


void  ArnServerSession::onCommandDelete( const QString& path)
{
    // qDebug() << "ArnServerNetSync-delete: path=" << path;
    ArnM::destroyLink( path);
}


void  ArnServerSession::doSyncStateChanged( int state)
{
    // qDebug() << "ArnServer sync state changed: state=" << state;
    ArnSync::State  syncState = ArnSync::State::fromInt( state);
    if (syncState == syncState.Normal) {
        // qDebug() << "ArnServer connected: remVer="
        //          << _arnNetSync->remoteVer(0) << _arnNetSync->remoteVer(1);
    }
}


QTcpSocket*  ArnServerSession::socket()  const
{
    return _socket;
}


Arn::XStringMap  ArnServerSession::remoteWhoIAm()  const
{
    return XStringMap( _arnNetSync->remoteWhoIAm());
}


QString  ArnServerSession::loginUserName()  const
{
    return _arnNetSync->loginUserName();
}


void  ArnServerSession::sendMessage( int type, const QByteArray& data)
{
    _arnNetSync->sendMessage( type, data);
}



ArnServerPrivate::ArnServerPrivate( ArnServer::Type serverType)
{
    _tcpServerActive = false;
    _isDemandLogin   = false;
    _tcpServer       = new QTcpServer;
    _arnLogin        = new ArnSyncLogin;
    _newSession      = 0;
    _serverType      = serverType;
    _freePathTab    += Arn::fullPath( Arn::pathLocalSys + "Legal/");
}


ArnServerPrivate::~ArnServerPrivate()
{
    delete _tcpServer;
    delete _arnLogin;
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
            ArnM::errorLog( QString(tr("Unknown Arn server Type:")) + QString::number( d->_serverType),
                                ArnError::Undef);
            return;
        }
    }

    if (d->_tcpServer->listen( listenAddr, port)) {
        d->_tcpServerActive = true;

        connect( d->_tcpServer, SIGNAL(newConnection()), this, SLOT(tcpConnection()));
    }
    else {
        ArnM::errorLog( QString(tr("Failed start Arn Server Port:")) + QString::number( port),
                        ArnError::ConnectionError);
    }
}


int  ArnServer::port()
{
    Q_D(ArnServer);

    return d->_tcpServer->serverPort();
}


QHostAddress  ArnServer::listenAddress()
{
    Q_D(ArnServer);

    QHostAddress  addr = d->_tcpServer->serverAddress();
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

    foreach (const QString& noLoginNet, d->_noLoginNets) {
        bool  chkLocalHost = noLoginNet == "localhost";
        if (chkLocalHost || (noLoginNet == "localnet")) {
            if ((remoteAddr == QHostAddress( QHostAddress::LocalHost))
            ||  (remoteAddr == QHostAddress( QHostAddress::LocalHostIPv6)))
                return false;  // Localhost, ok for both localhost & localnet

            foreach (QNetworkInterface  interface, QNetworkInterface::allInterfaces()) {
                QNetworkInterface::InterfaceFlags  flags = interface.flags();
                if (flags.testFlag( QNetworkInterface::IsPointToPoint)
                || flags.testFlag( QNetworkInterface::IsLoopBack))
                    continue;

                foreach (QNetworkAddressEntry  entry, interface.addressEntries()) {
                    QAbstractSocket::NetworkLayerProtocol  prot = entry.ip().protocol();
                    if ((prot != QAbstractSocket::IPv4Protocol) && (prot != QAbstractSocket::IPv6Protocol))
                        continue;

                    if (entry.ip() == remoteAddr)  // Address to this host ip, ok for both localhost & localnet
                        return false;

                    QString  subNetString = entry.ip().toString() + "/" + entry.netmask().toString();
                    if (!chkLocalHost && remoteAddr.isInSubnet( QHostAddress::parseSubnet( subNetString)))
                        return false;
                }
            }
        }
        else if (noLoginNet == "any") {
            return false;
        }
        else {
            if (remoteAddr.isInSubnet( QHostAddress::parseSubnet( noLoginNet)))
                return false;
        }
    }

    return true;
}


void ArnServer::addFreePath(const QString& path)
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

    QTcpSocket*  socket = d->_tcpServer->nextPendingConnection();

    switch (d->_serverType) {
    case Type::NetSync:
        d->_newSession = new ArnServerSession( socket, this);
        emit newSession();
        d->_newSession = 0;
        break;
    }
}
