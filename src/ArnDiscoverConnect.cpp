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

#include "ArnInc/ArnDiscoverConnect.hpp"
#include "private/ArnDiscoverConnect_p.hpp"
#include "ArnInc/ArnZeroConf.hpp"
#include "ArnInc/ArnClient.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QTimer>
#include <QTime>
#include <QMetaObject>


///////// ArnDiscoverConnector

ArnDiscoverConnectorPrivate::ArnDiscoverConnectorPrivate()
{
    _discoverHostPrio      = 1;
    _directHostPrio        = 2;
    _resolveRefreshTimeout = 30;
    _directHosts           = new QObject;
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


ArnDiscoverConnectorPrivate::~ArnDiscoverConnectorPrivate()
{
    delete _directHosts;
}


ArnDiscoverConnector::ArnDiscoverConnector( ArnClient& client, const QString& id)
    : QObject( &client)
    , d_ptr( new ArnDiscoverConnectorPrivate)
{
    Q_D(ArnDiscoverConnector);

    d->_client = &client;
    d->_id     = id;
}


ArnDiscoverConnector::ArnDiscoverConnector( ArnDiscoverConnectorPrivate& dd, QObject* parent,
                                            ArnClient& client, const QString& id)
    : QObject( parent)
    , d_ptr( &dd)
{
    Q_D(ArnDiscoverConnector);

    d->_client = &client;
    d->_id     = id;
}


ArnDiscoverConnector::~ArnDiscoverConnector()
{
    delete d_ptr;
}


void  ArnDiscoverConnector::clearDirectHosts()
{
    Q_D(ArnDiscoverConnector);

    d->_client->clearArnList( d->_directHostPrio);
}


void  ArnDiscoverConnector::addToDirectHosts( const QString& arnHost, quint16 port)
{
    Q_D(ArnDiscoverConnector);

    d->_client->addToArnList( arnHost, port, d->_directHostPrio);
}


void  ArnDiscoverConnector::setResolver( ArnDiscoverResolver* resolver)
{
    Q_D(ArnDiscoverConnector);

    if (d->_resolver) {
        delete d->_resolver;
        d->_resolver = 0;
    }
    if (!resolver)  return;  // No use of resolver

    if (!d->_resolveRefreshTime)  // first time
        d->_resolveRefreshTime = new QTime;

    d->_resolver = resolver;
    d->_resolver->setParent( this);
    d->_resolver->setDefaultStopState( ArnDiscoverInfo::State::HostIp);  // Need IP ...

    if (d->_hasBeenSetupClient)
        postSetupResolver();
}


QString ArnDiscoverConnector::id() const
{
    Q_D(const ArnDiscoverConnector);

    return d->_id;
}


int  ArnDiscoverConnector::resolveRefreshTimeout()  const
{
    Q_D(const ArnDiscoverConnector);

    return d->_resolveRefreshTimeout;
}


void  ArnDiscoverConnector::setResolveRefreshTimeout( int resolveRefreshTimeout)
{
    Q_D(ArnDiscoverConnector);

    d->_resolveRefreshTimeout = resolveRefreshTimeout;
}


int  ArnDiscoverConnector::discoverHostPrio()  const
{
    Q_D(const ArnDiscoverConnector);

    return d->_discoverHostPrio;
}


void  ArnDiscoverConnector::setDiscoverHostPrio( int discoverHostPrio)
{
    Q_D(ArnDiscoverConnector);

    d->_discoverHostPrio = discoverHostPrio;
}


int  ArnDiscoverConnector::directHostPrio()  const
{
    Q_D(const ArnDiscoverConnector);

    return d->_directHostPrio;
}


void  ArnDiscoverConnector::setDirectHostPrio( int directHostPrio)
{
    Q_D(ArnDiscoverConnector);

    d->_directHostPrio = directHostPrio;
}


bool  ArnDiscoverConnector::externalClientConnect()  const
{
    Q_D(const ArnDiscoverConnector);

    return d->_externalClientConnect;
}


void  ArnDiscoverConnector::setExternalClientConnect( bool externalClientConnect)
{
    Q_D(ArnDiscoverConnector);

    d->_externalClientConnect = externalClientConnect;
}


QString  ArnDiscoverConnector::service()  const
{
    Q_D(const ArnDiscoverConnector);

    return d->_service;
}


void ArnDiscoverConnector::setService( const QString& service)
{
    Q_D(ArnDiscoverConnector);

    d->_service = service;

    if (d->_resolver && d->_arnDisHostService)
        *d->_arnDisHostService = service;  // Request service change
}


void  ArnDiscoverConnector::start()
{
    Q_D(ArnDiscoverConnector);

    connect( d->_client, SIGNAL(tcpConnected(QString,quint16)),
             this, SLOT(doClientConnectChange(QString,quint16)));

    QMetaObject::invokeMethod( this,
                               "postSetupClient",
                               Qt::QueuedConnection);
}


void  ArnDiscoverConnector::doClientConnectChanged( int stat, int curPrio)
{
    Q_D(ArnDiscoverConnector);

    ArnClient::ConnectStat  cs = ArnClient::ConnectStat::fromInt( stat);

    if (Arn::debugDiscover)  qDebug() << "Discover ClientConnectChanged 1: stat=" << stat
                                      << " prio=" << curPrio;
    if (!d->_resolver || (cs == cs.Connecting))  return;
    if ((cs == cs.Connected) || (cs == cs.Stopped) || (cs == cs.Negotiating)) {
        d->_resolveRefreshBlocked = false;
        return;
    }
    if (d->_isResolved || (cs != cs.TriedAll)) {  // Resolv ok or still more to try, consider outdated resolv
        if (curPrio != d->_discoverHostPrio)  return;  // Not for resolved host
        if ((cs != cs.Error) && (cs != cs.Disconnected))  return;  // Skip any non error
        if (Arn::debugDiscover)  qDebug() << "Discover ClientConnectChanged 2:";

        if (d->_resolveRefreshTime->elapsed() >= d->_resolveRefreshTimeout * 1000)
            d->_resolveRefreshBlocked = false;
        if (d->_resolveRefreshBlocked)  return;
    }

    d->_resolveRefreshBlocked = true;  // Block for further refresh within lockout time
    d->_resolveRefreshTime->start();

    if (Arn::debugDiscover)  qDebug() << "Discover ClientConnectChanged 3 resolve: service=" << d->_arnDisHostService->toString();
    bool  forceUpdate = d->_isResolved;
    d->_resolver->resolve( d->_arnDisHostService->toString(), forceUpdate);  // Do a resolve refresh / retry
}


void  ArnDiscoverConnector::postSetupClient()
{
    Q_D(ArnDiscoverConnector);

    if (d->_resolver)
        postSetupResolver();

    QString  path;
    QString  connectIdPath = Arn::pathDiscoverConnect + d->_id + "/";

    path = connectIdPath + "Status/";
    ArnItem*  arnConnectStatus = new ArnItem( path + "value", this);
    *arnConnectStatus = d->_client->connectStatus();
    connect( d->_client, SIGNAL(connectionStatusChanged(int,int)), arnConnectStatus, SLOT(setValue(int)));
    ArnM::setValue( path + "set", ArnClient::ConnectStat::txt().getBitSet( ArnClient::ConnectStat::NsHuman));

    path = connectIdPath + "Request/";
    ArnItem*  arnConnectReqPV = new ArnItem( path + "value!", this);
    *arnConnectReqPV = "0";
    connect( arnConnectReqPV, SIGNAL(changed(int)), this, SLOT(doClientConnectRequest(int)));
    ArnM::setValue( path + "set", QString("0=Idle 1=Start_connect"));

    ArnClient::HostList  arnHosts = d->_client->arnList( d->_directHostPrio);
    int  i = 0;
    foreach (ArnClient::HostAddrPort host, arnHosts) {
        path = connectIdPath + "DirectHosts/Host-" + QString::number(i) + "/";
        ArnItem*  hostAddr = new ArnItem( path + "value", d->_directHosts);
        ArnItem*  hostPort = new ArnItem( path + "Port/value", d->_directHosts);
        *hostAddr = host.addr;  // Default addr
        *hostPort = host.port;  // Default port
        hostAddr->addMode( Arn::ObjectMode::Save);  // Save mode after default set, will not save default value
        hostPort->addMode( Arn::ObjectMode::Save);
        connect( hostAddr, SIGNAL(changed()), this, SLOT(doClientDirHostChanged()));
        connect( hostPort, SIGNAL(changed()), this, SLOT(doClientDirHostChanged()));
        ++i;
    }
    doClientDirHostChanged();  // Any loaded persistent values will be used

    connect( d->_client, SIGNAL(connectionStatusChanged(int,int)), this, SLOT(doClientConnectChanged(int,int)));
    d->_hasBeenSetupClient = true;

    if (!d->_resolver)
        doClientReadyToConnect( d->_client, d->_id);
}


void  ArnDiscoverConnector::doClientConnectChange( const QString& arnHost, quint16 port)
{
    Q_D(ArnDiscoverConnector);

    QString  path = Arn::pathDiscoverConnect + d->_id + "/UsingHost/";
    ArnM::setValue( path + "value", arnHost);
    ArnM::setValue( path + "Port/value", port ? QString::number( port) : QString());
}


void  ArnDiscoverConnector::doClientDirHostChanged()
{
    Q_D(ArnDiscoverConnector);

    QObjectList  dirHostOList = d->_directHosts->children();
    int  dirHostOListSize = dirHostOList.size();
    Q_ASSERT((dirHostOListSize & 1) == 0);

    //// Rebuild ArnList in client
    d->_client->clearArnList( d->_directHostPrio);
    for (int i = 0; i < dirHostOListSize / 2; ++i) {
        ArnItem*  hostAddr = qobject_cast<ArnItem*>( dirHostOList.at( 2 * i + 0));
        Q_ASSERT(hostAddr);
        ArnItem*  hostPort = qobject_cast<ArnItem*>( dirHostOList.at( 2 * i + 1));
        Q_ASSERT(hostPort);
        d->_client->addToArnList( hostAddr->toString(), quint16( hostPort->toInt()), d->_directHostPrio);
    }
}


void ArnDiscoverConnector::doClientConnectRequest(int reqCode)
{
    Q_D(ArnDiscoverConnector);

    if (reqCode) {
        doClientConnectChange("", 0);
        d->_client->connectToArnList();
    }
}


void  ArnDiscoverConnector::postSetupResolver()
{
    Q_D(ArnDiscoverConnector);

    QString  path;
    QString  connectIdPath = Arn::pathDiscoverConnect + d->_id + "/";

    path = connectIdPath + "DiscoverHost/";
    d->_arnDisHostServicePv = new ArnItem( path + "Service/value!", d->_resolver);
    d->_arnDisHostService   = new ArnItem( path + "Service/value",  d->_resolver);
    d->_arnDisHostAddress   = new ArnItem( path + "Host/value", d->_resolver);
    d->_arnDisHostPort      = new ArnItem( path + "Host/Port/value", d->_resolver);
    d->_arnDisHostStatus    = new ArnItem( path + "Status/value", d->_resolver);
    typedef ArnZeroConf::Error  Err;
    ArnM::setValue( path + "Status/set",
                    QString("%1=Resolved %2=Resolving %3=Bad_request_sequence %4=Resolv_timeout "
                            "%5=Unicast_DNS_Fail")
                    .arg(Err::Ok).arg(Err::Running).arg(Err::BadReqSeq).arg(Err::Timeout)
                    .arg(Err::UDnsFail));

    *d->_arnDisHostServicePv = d->_resolver->defaultService(); // Use this default if no active persistent service
    d->_arnDisHostService->addMode( Arn::ObjectMode::Save);    // Save mode after default set, will not save default value
    if (d->_service.isEmpty())                                 // Non empty _service is always used
        d->_service = d->_arnDisHostService->toString();       // Otherwise persistent/deafult value will be used as request
    *d->_arnDisHostService = d->_service;

    connect( d->_arnDisHostServicePv,  SIGNAL(changed()), this, SLOT(doClientServicetChanged()));
    connect( d->_resolver, SIGNAL(infoUpdated(int,ArnDiscoverInfo::State)),
             this, SLOT(doClientResolvChanged(int,ArnDiscoverInfo::State)));

    doClientServicetChanged();  // Perform request
}


void  ArnDiscoverConnector::doClientServicetChanged()
{
    Q_D(ArnDiscoverConnector);

    Q_ASSERT(d->_arnDisHostServicePv);
    QString  serviceName = d->_arnDisHostServicePv->toString();
    if (serviceName.isEmpty())
        serviceName = d->_resolver->defaultService();
    d->_service = serviceName;
    *d->_arnDisHostServicePv = serviceName;  // Current service must be set before resolving

    d->_resolver->resolve( serviceName, true);  // Force new resolve
}


void  ArnDiscoverConnector::doClientResolvChanged( int index, ArnDiscoverInfo::State state)
{
    Q_D(ArnDiscoverConnector);

    Q_ASSERT(d->_arnDisHostService);
    Q_ASSERT(d->_arnDisHostAddress);
    Q_ASSERT(d->_arnDisHostPort);

    const ArnDiscoverInfo&  info = d->_resolver->infoByIndex( index);
    if (info.serviceName() != d->_arnDisHostService->toString())  return;  // Not the current service

    *d->_arnDisHostAddress = info.hostWithInfo();
    *d->_arnDisHostPort    = info.hostPortString();
    *d->_arnDisHostStatus  = info.resolvCode();

    if (state <= state.ServiceName) { // New resolv has started
        if (Arn::debugDiscover)  qDebug() << "ArnDiscoverConnector New resolv started: service=" << info.serviceName();
        d->_isResolved = false;
        d->_client->clearArnList( d->_discoverHostPrio);
    }
    else if (state == state.HostIp) {
        d->_isResolved = true;
        d->_client->clearArnList( d->_discoverHostPrio);
        d->_client->addToArnList( info.hostWithInfo(), info.hostPort(), d->_discoverHostPrio);
        if (d->_client->connectStatus() == ArnClient::ConnectStat::Init) {
            doClientReadyToConnect( d->_client, d->_id);
        }
    }
    else if (info.isError()) {
        d->_client->clearArnList( d->_discoverHostPrio);
        if (d->_client->connectStatus() == ArnClient::ConnectStat::Init) {
            doClientReadyToConnect( d->_client, d->_id);
        }
    }
}


void  ArnDiscoverConnector::doClientReadyToConnect( ArnClient* arnClient, const QString& id)
{
    Q_D(ArnDiscoverConnector);

    if (d->_externalClientConnect)
        emit clientReadyToConnect( arnClient, id);
    else {
        if (Arn::debugDiscover)  qDebug() << "Discover Connecting client: id=" << id;
        arnClient->connectToArnList();
    }
}
