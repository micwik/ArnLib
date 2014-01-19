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

#include "ArnInc/ArnDiscover.hpp"
#include "ArnInc/ArnZeroConf.hpp"
#include "ArnInc/ArnClient.hpp"
#include "ArnInc/ArnServer.hpp"
#include <QHostInfo>
#include <QTimer>
#include <QMetaObject>


///////// ArnDiscoverInfo

ArnDiscoverInfo::ArnDiscoverInfo()
{
    _id        = -1;  // Unknown
    _hostPort  = 0;
    _stopState = State::HostIp;
}


ArnDiscoverInfo::State  ArnDiscoverInfo::state()  const
{
    return _state;
}


ArnDiscoverInfo::State  ArnDiscoverInfo::stopState()  const
{
    return _stopState;
}


ArnDiscover::Type  ArnDiscoverInfo::type()  const
{
    return _type;
}


QString  ArnDiscoverInfo::group()  const
{
    return _properties.valueString("group");
}


QString  ArnDiscoverInfo::serviceName()  const
{
    return _serviceName;
}


QString  ArnDiscoverInfo::domain()  const
{
    return _domain;
}


QString  ArnDiscoverInfo::hostName()  const
{
    return _hostName;
}


quint16  ArnDiscoverInfo::hostPort()  const
{
    return _hostPort;
}


QHostAddress  ArnDiscoverInfo::hostIp()  const
{
    return _hostIp;
}


XStringMap  ArnDiscoverInfo::properties()  const
{
    return _properties;
}


QString  ArnDiscoverInfo::typeString()  const
{
    if (_state < State::HostInfo)  return QString();

    switch (_type) {
    case ArnDiscover::Type::Server:  return "Server";
    case ArnDiscover::Type::Client:  return "Client";
    case ArnDiscover::Type::None:
        // Fall throu
    default:                         return "Unknown";
    }
}


QString  ArnDiscoverInfo::hostPortString()  const
{
    return _state < State::HostInfo ? QString() : QString::number( _hostPort);
}


QString  ArnDiscoverInfo::hostIpString()  const
{
    return _state < State::HostIp ? QString() : _hostIp.toString();
}


///////// ArnDiscoverBrowse

ArnDiscoverBrowser::ArnDiscoverBrowser( QObject* parent) :
    QObject( parent)
{
    _defaultStopState = _defaultStopState.HostIp;

    _serviceBrowser = new ArnZeroConfBrowser( this);
    connect(_serviceBrowser, SIGNAL(browseError(int)),
            this, SLOT(onBrowseError(int)));
    connect(_serviceBrowser, SIGNAL(serviceAdded(int,QString,QString)),
            this, SLOT(onServiceAdded(int,QString,QString)));
    connect(_serviceBrowser, SIGNAL(serviceRemoved(int,QString,QString)),
            this, SLOT(onServiceRemoved(int,QString,QString)));
}


const ArnDiscoverInfo&  ArnDiscoverBrowser::infoByIndex( int index)
{
    static ArnDiscoverInfo  nullInfo;

    if ((index < 0) || (index >= _activeServInfos.size()))  return nullInfo;

    return _activeServInfos.at( index);
}


const ArnDiscoverInfo&  ArnDiscoverBrowser::infoById( int id)
{
    int  index = _activeServIds.indexOf( id);
    return infoByIndex( index);
}


const ArnDiscoverInfo&  ArnDiscoverBrowser::infoByName( QString serviceName)
{
    return infoById( _serviceBrowser->serviceNameToId( serviceName));
}


int  ArnDiscoverBrowser::indexToId( int index)
{
    if ((index < 0) || (index >= _activeServIds.size()))  return -1;

    return _activeServIds.at( index);
}


int  ArnDiscoverBrowser::IdToIndex( int id)
{
    return _activeServIds.indexOf( id);
}


bool  ArnDiscoverBrowser::isBrowsing()  const
{
    return _serviceBrowser->isBrowsing();
}


void  ArnDiscoverBrowser::setFilter( ArnDiscover::Type typeFilter)
{
    switch (typeFilter) {
    case ArnDiscover::Type::Server:  _filter = "server";  break;
    case ArnDiscover::Type::Client:  _filter = "client";  break;
    case ArnDiscover::Type::None:
        // Fall throu
    default:                         _filter = "";
    }
}


void ArnDiscoverBrowser::setFilter( QString group)
{
    _filter = group;
}


ArnDiscoverInfo::State  ArnDiscoverBrowser::defaultStopState()  const
{
    return _defaultStopState;
}


void  ArnDiscoverBrowser::setDefaultStopState( ArnDiscoverInfo::State defaultStopState)
{
    _defaultStopState = defaultStopState;
}


bool  ArnDiscoverBrowser::goTowardState( int index, ArnDiscoverInfo::State state)
{
    ArnDiscoverInfo&  info = _activeServInfos[ index];
    if (state <= info._state)  return false;  // Can only go forward

    if (info._state < info._stopState) {  // Next state is in progress
        info._stopState = state;  // Just update final state
    }
    else if (info._state == info._stopState) {  // Nothing in progress
        info._stopState = state;  // Update for new final state
        doNextState( info);  // Startup state change
    }
    else {
        Q_ASSERT_X(false, "ArnDiscoverBrowser::goTowardState()", "State passed stopState");
        return false;  // Internal error
    }

    return true;
}


void  ArnDiscoverBrowser::browse( bool enable)
{
    if (!enable)  return stopBrowse();
    if (isBrowsing())  return;  // Already browsing

    _activeServIds.clear();
    _activeServInfos.clear();
    _ipLookupIds.clear();

    _serviceBrowser->setSubType( _filter);
    _serviceBrowser->browse( enable);
}


void  ArnDiscoverBrowser::stopBrowse()
{
    _serviceBrowser->stopBrowse();
}


void  ArnDiscoverBrowser::onBrowseError( int code)
{
    qDebug() << "Browse Error code=" << code;
}


void  ArnDiscoverBrowser::onServiceAdded( int id, QString name, QString domain)
{
    qDebug() << "Browse Service added: name=" << name << " domain=" << domain
             << " escFullDomain=" << _serviceBrowser->escapedFullDomain();

    ArnDiscoverInfo  info;
    info._id          = id;
    info._state       = ArnDiscoverInfo::State::ServiceName;
    info._serviceName = name;
    info._domain      =  domain;
    info._stopState   = _defaultStopState;

    int  index;
    for (index = 0; index < _activeServInfos.size(); ++index) {
        const QString&  indexName = _activeServInfos.at( index)._serviceName;
        Q_ASSERT(name != indexName);
        if (name < indexName)  break;  // Sorting place found
    }
    _activeServIds.insert( index, id);
    _activeServInfos.insert( index, info);

    emit serviceAdded( index, name);
    doNextState( _activeServInfos.at( index));
}


void  ArnDiscoverBrowser::onServiceRemoved( int id, QString name, QString domain)
{
    qDebug() << "Browse Service removed: name=" << name << " domain=" << domain;
    int  index = _activeServIds.indexOf( id);
    _activeServIds.removeAt( index);
    _activeServInfos.removeAt( index);

    emit serviceRemoved( index);
}


void  ArnDiscoverBrowser::onResolveError( int code)
{
    ArnZeroConfResolv*  ds = qobject_cast<ArnZeroConfResolv*>( sender());
    Q_ASSERT(ds);

    qDebug() << "Resolve Error code=" << code;

    ds->releaseService();
    ds->deleteLater();
}


void  ArnDiscoverBrowser::onResolved( int id, QByteArray escFullDomain)
{
    ArnZeroConfResolv*  ds = qobject_cast<ArnZeroConfResolv*>( sender());
    Q_ASSERT(ds);

    QString  name = ds->serviceName();
    qDebug() << "Resolved Service: name=" << name << " escFullDomainR=" << escFullDomain
             << " escFullDomain=" << ds->escapedFullDomain();
    int  index = _activeServIds.indexOf( id);
    if (index >= 0) {  // Service still exist
        ArnDiscoverInfo&  info = _activeServInfos[ index];
        XStringMap  xsmTxt;
        ds->getTxtRecordMap( xsmTxt);
        QByteArray  servProp = xsmTxt.value("server");
        info._type = servProp.isNull() ? ArnDiscover::Type::None
                                       : (servProp.toInt() ? ArnDiscover::Type::Server
                                                           : ArnDiscover::Type::Client);
        info._state      = ArnDiscoverInfo::State::HostInfo;
        info._hostName   = ds->host();
        info._hostPort   = ds->port();
        info._properties = xsmTxt;

        emit infoUpdated( index, info._state);
        doNextState( info);
    }

    ds->releaseService();
    ds->deleteLater();
}


void  ArnDiscoverBrowser::onIpLookup( const QHostInfo& host)
{
    int  ipLookupId = host.lookupId();
    int  id = _ipLookupIds.value( ipLookupId, -1);
    qDebug() << "onIpLookup: lookupId=" << ipLookupId;
    if (id < 0)  return;  // Service not valid anymore

    _ipLookupIds.remove( ipLookupId);

    if (host.error() != QHostInfo::NoError) {
         qDebug() << "Lookup failed:" << host.errorString();
         return;
    }

    foreach (const QHostAddress &address, host.addresses())
        qDebug() << "Found address:" << address.toString();

    int  index = _activeServIds.indexOf( id);
    if (index < 0)  return;  // Service not exist anymore

    ArnDiscoverInfo&  info = _activeServInfos[ index];
    info._state = ArnDiscoverInfo::State::HostIp;
    info._hostIp = host.addresses().first();

    emit infoUpdated( index, info._state);
}


void  ArnDiscoverBrowser::doNextState( const ArnDiscoverInfo& info)
{
    if (info._state >= info._stopState)  return;  // At stop state, do nothing more now

    switch (info._state) {
    case ArnDiscoverInfo::State::ServiceName: {
        ArnZeroConfResolv*  ds = new ArnZeroConfResolv( info._serviceName, this);
        ds->setId( info._id);
        connect( ds, SIGNAL(resolveError(int)), this, SLOT(onResolveError(int)));
        connect( ds, SIGNAL(resolved(int,QByteArray)), this, SLOT(onResolved(int,QByteArray)));
        ds->resolve();
        break;
    }
    case ArnDiscoverInfo::State::HostInfo: {
        int  ipLookupId = QHostInfo::lookupHost( info._hostName, this, SLOT(onIpLookup(QHostInfo)));
        _ipLookupIds.insert( ipLookupId, info._id);
        qDebug() << "LookingUp host=" << info._hostName << " lookupId=" << ipLookupId;
        break;
    }
    default:
        break;
    }
}


///////// ArnDiscoverAdvertise

ArnDiscoverAdvertise::ArnDiscoverAdvertise( QObject *parent) :
    QObject( parent)
{
    _arnZCReg  = new ArnZeroConfRegister( this);
    _servTimer = new QTimer( this);
    _arnInternalServer = 0;
    _hasBeenSetup = false;
    _defaultService = "Arn Default Service";
}


void  ArnDiscoverAdvertise::setArnServer( ArnServer* arnServer, ArnDiscover::Type discoverType)
{
    _discoverType = discoverType;
    QString  hostAddr = arnServer->address().toString();
    int      hostPort = arnServer->port();
    ArnM::setValue("/Sys/Discover/This/Host/value", hostAddr);
    ArnM::setValue("/Sys/Discover/This/Host/Port/value", hostPort);

    XStringMap  xsm;
    xsm.add("protovers", "1.0");
    xsm.add("server", QByteArray::number( _discoverType == ArnDiscover::Type::Server));
    _arnZCReg->setSubTypes( _groups);
    _arnZCReg->addSubType( _discoverType == ArnDiscover::Type::Server ? "server" : "client");
    for (int i = 0; i < _groups.size(); ++i) {
        xsm.add("group", i, _groups.at(i));
    }
    _arnZCReg->setTxtRecordMap( xsm);
    _arnZCReg->setPort( hostPort);
    connect( _arnZCReg, SIGNAL(registered(QString)), this, SLOT(serviceRegistered(QString)));
    connect( _arnZCReg, SIGNAL(registrationError(int)), this, SLOT(serviceRegistrationError(int)));

    QTimer::singleShot(0, this, SLOT(postSetupThis()));  // Ĺet persistance service etc init before ...
}


void  ArnDiscoverAdvertise::startNewArnServer( ArnDiscover::Type discoverType, int port)
{
    if (!_arnInternalServer)
        delete _arnInternalServer;
    _arnInternalServer = new ArnServer( ArnServer::Type::NetSync, this);
    _arnInternalServer->start( port);

    setArnServer( _arnInternalServer, discoverType);
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


QStringList  ArnDiscoverAdvertise::groups()  const
{
    return _groups;
}


void  ArnDiscoverAdvertise::setGroups( const QStringList& groups)
{
    _groups = groups;
}


void  ArnDiscoverAdvertise::addGroup( const QString& group)
{
    _groups += group;
}
