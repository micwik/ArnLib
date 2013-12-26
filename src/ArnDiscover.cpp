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

#include "ArnDiscover.hpp"
#include "ArnZeroConf.hpp"
#include "ArnClient.hpp"
#include "ArnServer.hpp"
#include <QTimer>
#include <QMetaObject>

ArnDiscoverAdvertise::ArnDiscoverAdvertise( QObject *parent) :
    QObject( parent)
{
    _arnZCReg  = new ArnZeroConfRegister( this);
    _servTimer = new QTimer( this);
    _arnInternalServer = 0;
    _hasBeenSetup = false;
    _defaultService = "Arn Default Service";
}


void  ArnDiscoverAdvertise::setArnServer( ArnServer* arnServer)
{
    QString  hostAddr = arnServer->address().toString();
    int      hostPort = arnServer->port();
    ArnM::setValue("/Sys/Discover/This/Host/value", hostAddr);
    ArnM::setValue("/Sys/Discover/This/Host/Port/value", hostPort);

    _arnZCReg->setPort( hostPort);
    connect( _arnZCReg, SIGNAL(registered(QString)), this, SLOT(serviceRegistered(QString)));
    connect( _arnZCReg, SIGNAL(registrationError(int)), this, SLOT(serviceRegistrationError(int)));

    QTimer::singleShot(0, this, SLOT(postSetupThis()));  // Ĺet persistance service etc init before ...
}


void  ArnDiscoverAdvertise::startNewArnServer( int port)
{
    if (!_arnInternalServer)
        delete _arnInternalServer;
    _arnInternalServer = new ArnServer( ArnServer::Type::NetSync, this);
    _arnInternalServer->start( port);

    setArnServer( _arnInternalServer);
}


void  ArnDiscoverAdvertise::addArnClient( ArnClient* arnClient, const QString& id)
{
    Q_ASSERT( arnClient);
    if (!arnClient)  return;
    arnClient->setObjectName(id);

    connect( arnClient, SIGNAL(tcpConnected(QString,quint16)), this, SLOT(doClientConnected(QString,quint16)));
    QMetaObject::invokeMethod( this,
                               "postSetupClient",
                               Qt::QueuedConnection,
                               Q_ARG( QObject*, arnClient));
}


void  ArnDiscoverAdvertise::postSetupThis()
{
    _servTimer->start( 5000);
    connect( _servTimer, SIGNAL(timeout()), this, SLOT(serviceTimeout()));

    connect( &_arnService,   SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    connect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    _arnServicePv.open("/Sys/Discover/This/Service/value!");
    _arnService.addMode( ArnItem::Mode::Save);
    _arnService.open("/Sys/Discover/This/Service/value");

    _hasBeenSetup = true;
    if (!_service.isNull())
        setService( _service);
}


void  ArnDiscoverAdvertise::serviceTimeout()
{
    qDebug() << "First service setup timeout, using default.";

    firstServiceSetup( _defaultService);
}


void  ArnDiscoverAdvertise::firstServiceSetup( QString serviceName)
{
    QString  service = serviceName;
    if (service.isEmpty())
        service = _defaultService;
    qDebug() << "firstServiceSetup: serviceName=" << service;

    _servTimer->stop();
    disconnect( &_arnService,   SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    disconnect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    connect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(doServiceChanged(QString)));

    doServiceChanged( service);
}


void  ArnDiscoverAdvertise::doServiceChanged( QString val)
{
    qDebug() << "Service changed: servname=" << val;
    _service = val;

    if (_arnZCReg->state() != ArnZeroConfRegister::State::None)
        _arnZCReg->releaseService();
    _arnZCReg->setServiceName( val);
    _arnZCReg->registerService();
}


void  ArnDiscoverAdvertise::serviceRegistered( QString serviceName)
{
    qDebug() << "Service registered: serviceName=" << serviceName;

    _arnServicePv = serviceName;
    emit serviceChanged( serviceName);
}


void  ArnDiscoverAdvertise::serviceRegistrationError(int code)
{
    qDebug() << "Service registration error: code=" << code;

    emit serviceChangeError( code);
}


void  ArnDiscoverAdvertise::postSetupClient( QObject* arnClientObj)
{
    ArnClient*  arnClient = qobject_cast<ArnClient*>( arnClientObj);
    Q_ASSERT( arnClient);
    if (!arnClient)  return;
    QString  id = arnClient->objectName();

    QObject*  dirHosts = new QObject( arnClient);
    dirHosts->setObjectName("dirHosts");

    QString  path;
    QString  connectIdPath = "/Sys/Discover/Connect/" + id + "/";

    path = connectIdPath + "Status/";
    ArnItem*  arnConnectStatus = new ArnItem( path + "value", arnClient);
    *arnConnectStatus = arnClient->connectStatus();
    connect( arnClient, SIGNAL(connectionStatusChanged(int)), arnConnectStatus, SLOT(setValue(int)));
    typedef ArnClient::ConnectStat CS;
    ArnM::setValue( path + "set",
                    QString("%1=Initialized %2=Connecting %3=Connected %4=Connect_error %5=Disconnected")
                    .arg(CS::Init).arg(CS::Connecting).arg(CS::Connected).arg(CS::Error).arg(CS::Disconnected));

    path = connectIdPath + "Request/";
    ArnItem*  arnConnectReqPV = new ArnItem( path + "value!", arnClient);
    *arnConnectReqPV = "0";
    connect( arnConnectReqPV, SIGNAL(changed(int)), this, SLOT(doClientConnectRequest(int)));
    ArnM::setValue( path + "set", QString("0=Idle 1=Start_connect"));

    ArnClient::HostList  arnHosts = arnClient->ArnList();
    int  i = 0;
    foreach (ArnClient::HostAddrPort  host, arnHosts) {
        ++i;
        path = connectIdPath + "DirectHosts/Host-" + QString::number(i) + "/";
        ArnItem*  hostAddr = new ArnItem( path + "value", dirHosts);
        ArnItem*  hostPort = new ArnItem( path + "Port/value", dirHosts);
        *hostAddr = host.addr;  // Default addr
        *hostPort = host.port;  // Default port
        hostAddr->addMode( ArnItem::Mode::Save);
        hostPort->addMode( ArnItem::Mode::Save);
        connect( hostAddr, SIGNAL(changed()), this, SLOT(doClientDirHostChanged()));
        connect( hostPort, SIGNAL(changed()), this, SLOT(doClientDirHostChanged()));
    }
    doClientDirHostChanged( dirHosts);
    emit clientReadyToConnect( arnClient);
}


void  ArnDiscoverAdvertise::doClientConnected( QString arnHost, quint16 port)
{
    ArnClient*  client = qobject_cast<ArnClient*>( sender());
    Q_ASSERT(client);

    QString  path = "/Sys/Discover/Connect/" + client->objectName() + "/UsingHost/";
    ArnM::setValue( path + "value", arnHost);
    ArnM::setValue( path + "Port/value", port);
}


void  ArnDiscoverAdvertise::doClientDirHostChanged( QObject* dirHostsObj)
{
    QObject*  dirHostsO = dirHostsObj;
    if (!dirHostsO) {
        QObject*  s = sender();
        Q_ASSERT(s);
        dirHostsO = s->parent();
        Q_ASSERT(dirHostsO);
    }
    ArnClient*  client = qobject_cast<ArnClient*>( dirHostsO->parent());
    Q_ASSERT(client);

    QObjectList  dirHostOList = dirHostsO->children();
    int  dirHostOListSize = dirHostOList.size();
    Q_ASSERT((dirHostOListSize & 1) == 0);

    //// Rebuild ArnList in client
    client->clearArnList();
    for (int i = 0; i < dirHostOListSize / 2; ++i) {
        ArnItem*  hostAddr = qobject_cast<ArnItem*>( dirHostOList.at( 2 * i + 0));
        Q_ASSERT(hostAddr);
        ArnItem*  hostPort = qobject_cast<ArnItem*>( dirHostOList.at( 2 * i + 1));
        Q_ASSERT(hostPort);
        client->addToArnList( hostAddr->toString(), quint16( hostPort->toInt()));
    }
}


void  ArnDiscoverAdvertise::doClientConnectRequest( int reqCode)
{
    ArnItem*  arnConnectReqPV = qobject_cast<ArnItem*>( sender());
    Q_ASSERT(arnConnectReqPV);
    ArnClient*  client = qobject_cast<ArnClient*>( arnConnectReqPV->parent());
    Q_ASSERT(client);

    if (reqCode)
        client->connectToArnList();
}


QString  ArnDiscoverAdvertise::service()  const
{
    return _service;
}


void  ArnDiscoverAdvertise::setService( QString service)
{
    if (_hasBeenSetup)
        _arnService = service;
    else
        _service = service;
}


QString  ArnDiscoverAdvertise::defaultService()  const
{
    return _defaultService;
}


void  ArnDiscoverAdvertise::setDefaultService( const QString& defaultService)
{
    _defaultService = defaultService;
}
