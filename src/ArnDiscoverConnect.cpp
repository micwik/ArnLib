// Copyright (C) 2010-2014 Michael Wiklund.
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

#include "ArnInc/ArnDiscoverConnect.hpp"
#include "ArnInc/ArnZeroConf.hpp"
#include "ArnInc/ArnClient.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QTimer>
#include <QTime>
#include <QMetaObject>


///////// ArnDiscoverConnector

ArnDiscoverConnector::ArnDiscoverConnector( ArnClient& client, const QString& id) :
    QObject( &client)
{
    _client = &client;
    _id     = id;
    _discoverHostPrio      = 1;
    _directHostPrio        = 2;
    _resolveRefreshTimeout = 60;
    _directHosts           = new QObject( this);
    _directHosts->setObjectName("dirHosts");
    _hasBeenSetupClient    = false;
    _resolveRefreshBlocked = false;
    _isResolved            = false;
    _externalClientConnect = false;
    _resolveRefreshTime    = 0;
    _resolver              = 0;
    _arnDisHostService     = 0;
    _arnDisHostServicePv   = 0;
    _arnDisHostAddress     = 0;
    _arnDisHostPort        = 0;
}


void  ArnDiscoverConnector::clearDirectHosts()
{
    _client->clearArnList( _directHostPrio);
}


void  ArnDiscoverConnector::addToDirectHosts( const QString& arnHost, quint16 port)
{
    _client->addToArnList( arnHost, port, _directHostPrio);
}


void  ArnDiscoverConnector::setResolver( ArnDiscoverResolver* resolver)
{
    if (_resolver) {
        delete _resolver;
        _resolver = 0;
    }
    if (!resolver)  return;  // No use of resolver

    if (!_resolveRefreshTime)  // first time
        _resolveRefreshTime = new QTime;

    _resolver = resolver;
    _resolver->setParent( this);
    _resolver->setDefaultStopState( ArnDiscoverInfo::State::HostIp);  // Need IP ...

    if (_hasBeenSetupClient)
        postSetupResolver();
}


QString ArnDiscoverConnector::id() const
{
    return _id;
}


int  ArnDiscoverConnector::resolveRefreshTimeout()  const
{
    return _resolveRefreshTimeout;
}


void  ArnDiscoverConnector::setResolveRefreshTimeout( int resolveRefreshTimeout)
{
    _resolveRefreshTimeout = resolveRefreshTimeout;
}


int  ArnDiscoverConnector::resolvHostPrio()  const
{
    return _discoverHostPrio;
}


void  ArnDiscoverConnector::setResolvHostPrio( int resolvHostPrio)
{
    _discoverHostPrio = resolvHostPrio;
}


int  ArnDiscoverConnector::directHostPrio()  const
{
    return _directHostPrio;
}


void  ArnDiscoverConnector::setDirectHostPrio( int directHostPrio)
{
    _directHostPrio = directHostPrio;
}


bool  ArnDiscoverConnector::externalClientConnect()  const
{
    return _externalClientConnect;
}


void  ArnDiscoverConnector::setExternalClientConnect( bool externalClientConnect)
{
    _externalClientConnect = externalClientConnect;
}


QString  ArnDiscoverConnector::service()  const
{
    return _service;
}


void ArnDiscoverConnector::setService(QString service)
{
    _service = service;

    if (_resolver && _arnDisHostService)
        *_arnDisHostService = service;  // Request service change
}


void  ArnDiscoverConnector::start()
{
    connect( _client, SIGNAL(tcpConnected(QString,quint16)), 
             this, SLOT(doClientConnectChange(QString,quint16)));

    QMetaObject::invokeMethod( this,
                               "postSetupClient",
                               Qt::QueuedConnection);
}


void  ArnDiscoverConnector::doClientConnectChanged( int stat, int curPrio)
{
    ArnClient::ConnectStat  cs = ArnClient::ConnectStat::fromInt( stat);

    if (Arn::debugDiscover)  qDebug() << "Discover ClientConnectChanged 1: stat=" << stat
                                      << " prio=" << curPrio;
    if (!_resolver || (cs == cs.Connecting))  return;
    if (cs == cs.Connected) {
        _resolveRefreshBlocked = false;
        return;
    }
    if (_isResolved || (cs != cs.TriedAll)) {  // Resolv ok or still more to try, consider outdated resolv
        if (curPrio != _discoverHostPrio)  return;  // Not for resolved host
        if ((cs != cs.Error) && (cs != cs.Disconnected))  return;  // Skip any non error
        if (Arn::debugDiscover)  qDebug() << "Discover ClientConnectChanged 2:";

        if (_resolveRefreshTime->elapsed() >= _resolveRefreshTimeout * 1000)
            _resolveRefreshBlocked = false;
        if (_resolveRefreshBlocked)  return;
    }

    _resolveRefreshBlocked = true;  // Block for further refresh within lockout time
    _resolveRefreshTime->start();

    if (Arn::debugDiscover)  qDebug() << "Discover ClientConnectChanged 3 resolve: service=" << _arnDisHostService->toString();
    bool  forceUpdate = _isResolved;
    _resolver->resolve( _arnDisHostService->toString(), forceUpdate);  // Do a resolve refresh / retry
}


void  ArnDiscoverConnector::postSetupClient()
{
    if (_resolver)
        postSetupResolver();

    QString  path;
    QString  connectIdPath = Arn::pathDiscoverConnect + _id + "/";

    path = connectIdPath + "Status/";
    ArnItem*  arnConnectStatus = new ArnItem( path + "value", this);
    *arnConnectStatus = _client->connectStatus();
    connect( _client, SIGNAL(connectionStatusChanged(int,int)), arnConnectStatus, SLOT(setValue(int)));
    typedef ArnClient::ConnectStat CS;
    ArnM::setValue( path + "set",
                    QString("%1=Initialized %2=Connecting %3=Connected %4=Connect_error %5=Disconnected")
                    .arg(CS::Init).arg(CS::Connecting).arg(CS::Connected).arg(CS::Error).arg(CS::Disconnected));

    path = connectIdPath + "Request/";
    ArnItem*  arnConnectReqPV = new ArnItem( path + "value!", this);
    *arnConnectReqPV = "0";
    connect( arnConnectReqPV, SIGNAL(changed(int)), this, SLOT(doClientConnectRequest(int)));
    ArnM::setValue( path + "set", QString("0=Idle 1=Start_connect"));

    ArnClient::HostList  arnHosts = _client->arnList( _directHostPrio);
    int  i = 0;
    foreach (ArnClient::HostAddrPort host, arnHosts) {
        path = connectIdPath + "DirectHosts/Host-" + QString::number(i) + "/";
        ArnItem*  hostAddr = new ArnItem( path + "value", _directHosts);
        ArnItem*  hostPort = new ArnItem( path + "Port/value", _directHosts);
        *hostAddr = host.addr;  // Default addr
        *hostPort = host.port;  // Default port
        hostAddr->addMode( ArnItem::Mode::Save);  // Save mode after default set, will not save default value
        hostPort->addMode( ArnItem::Mode::Save);
        connect( hostAddr, SIGNAL(changed()), this, SLOT(doClientDirHostChanged()));
        connect( hostPort, SIGNAL(changed()), this, SLOT(doClientDirHostChanged()));
        ++i;
    }
    doClientDirHostChanged();  // Any loaded persistent values will be used

    connect( _client, SIGNAL(connectionStatusChanged(int,int)), this, SLOT(doClientConnectChanged(int,int)));
    _hasBeenSetupClient = true;

    if (!_resolver)
        doClientReadyToConnect( _client, _id);
}


void  ArnDiscoverConnector::doClientConnectChange( QString arnHost, quint16 port)
{
    QString  path = Arn::pathDiscoverConnect + _id + "/UsingHost/";
    ArnM::setValue( path + "value", arnHost);
    ArnM::setValue( path + "Port/value", port ? QString::number( port) : QString());
}


void  ArnDiscoverConnector::doClientDirHostChanged()
{
    QObjectList  dirHostOList = _directHosts->children();
    int  dirHostOListSize = dirHostOList.size();
    Q_ASSERT((dirHostOListSize & 1) == 0);

    //// Rebuild ArnList in client
    _client->clearArnList( _directHostPrio);
    for (int i = 0; i < dirHostOListSize / 2; ++i) {
        ArnItem*  hostAddr = qobject_cast<ArnItem*>( dirHostOList.at( 2 * i + 0));
        Q_ASSERT(hostAddr);
        ArnItem*  hostPort = qobject_cast<ArnItem*>( dirHostOList.at( 2 * i + 1));
        Q_ASSERT(hostPort);
        _client->addToArnList( hostAddr->toString(), quint16( hostPort->toInt()), _directHostPrio);
    }
}


void ArnDiscoverConnector::doClientConnectRequest(int reqCode)
{
    if (reqCode) {
        doClientConnectChange("", 0);
        _client->connectToArnList();
    }
}


void  ArnDiscoverConnector::postSetupResolver()
{
    QString  path;
    QString  connectIdPath = Arn::pathDiscoverConnect + _id + "/";

    path = connectIdPath + "DiscoverHost/";
    _arnDisHostServicePv = new ArnItem( path + "Service/value!", _resolver);
    _arnDisHostService   = new ArnItem( path + "Service/value",  _resolver);
    _arnDisHostAddress   = new ArnItem( path + "Host/value", _resolver);
    _arnDisHostPort      = new ArnItem( path + "Host/Port/value", _resolver);
    _arnDisHostStatus    = new ArnItem( path + "Status/value", _resolver);
    typedef ArnZeroConf::Error  Err;
    ArnM::setValue( path + "Status/set",
                    QString("%1=Resolved %2=Resolving %3=Bad_request_sequence %4=Resolv_timeout "
                            "%5=Unicast_DNS_Fail")
                    .arg(Err::Ok).arg(Err::Running).arg(Err::BadReqSeq).arg(Err::Timeout)
                    .arg(Err::UDnsFail));

    *_arnDisHostServicePv = _resolver->defaultService(); // Use this default if no active persistent service
    _arnDisHostService->addMode( ArnItem::Mode::Save);   // Save mode after default set, will not save default value
    if (_service.isEmpty())                              // Non empty _service is always used
        _service = _arnDisHostService->toString();       // Otherwise persistent/deafult value will be used as request
    *_arnDisHostService = _service;

    connect( _arnDisHostServicePv,  SIGNAL(changed()), this, SLOT(doClientServicetChanged()));
    connect( _resolver, SIGNAL(infoUpdated(int,ArnDiscoverInfo::State)),
             this, SLOT(doClientResolvChanged(int,ArnDiscoverInfo::State)));

    doClientServicetChanged();  // Perform request
}


void  ArnDiscoverConnector::doClientServicetChanged()
{
    Q_ASSERT(_arnDisHostServicePv);
    QString  serviceName = _arnDisHostServicePv->toString();
    if (serviceName.isEmpty())
        serviceName = _resolver->defaultService();
    _service = serviceName;
    *_arnDisHostServicePv = serviceName;  // Current service must be set before resolving

    _resolver->resolve( serviceName, true);  // Force new resolve
}


void  ArnDiscoverConnector::doClientResolvChanged( int index, ArnDiscoverInfo::State state)
{
    Q_ASSERT(_arnDisHostService);
    Q_ASSERT(_arnDisHostAddress);
    Q_ASSERT(_arnDisHostPort);

    ArnDiscoverInfo  info = _resolver->infoByIndex( index);
    if (info.serviceName() != _arnDisHostService->toString())  return;  // Not the current service

    *_arnDisHostAddress = info.hostWithInfo();
    *_arnDisHostPort    = info.hostPortString();
    *_arnDisHostStatus  = info.resolvCode();

    if (state <= state.ServiceName) { // New resolv has started
        if (Arn::debugDiscover)  qDebug() << "ArnDiscoverConnector New resolv started: service=" << info.serviceName();
        _isResolved = false;
        _client->clearArnList( _discoverHostPrio);
    }
    else if (state == state.HostIp) {
        _isResolved = true;
        _client->clearArnList( _discoverHostPrio);
        _client->addToArnList( info.hostWithInfo(), info.hostPort(), _discoverHostPrio);
        if (_client->connectStatus() == ArnClient::ConnectStat::Init) {
            doClientReadyToConnect( _client, _id);
        }
    }
    else if (info.isError()) {
        _client->clearArnList( _discoverHostPrio);
        if (_client->connectStatus() == ArnClient::ConnectStat::Init) {
            doClientReadyToConnect( _client, _id);
        }
    }
}


void  ArnDiscoverConnector::doClientReadyToConnect( ArnClient* arnClient, const QString& id)
{
    if (_externalClientConnect)
        emit clientReadyToConnect( arnClient, id);
    else {
        if (Arn::debugDiscover)  qDebug() << "Discover Connecting client: id=" << id;
        arnClient->connectToArnList();
    }
}
