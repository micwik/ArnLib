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
#include <QHostInfo>
#include <QTimer>

using Arn::XStringMap;


///////// ArnDiscoverInfo

ArnDiscoverInfo::ArnDiscoverInfo()
{
    _id         = -1;  // Unknown
    _resolvCode = ArnZeroConf::Error::Ok;
    _hostPort   = 0;
    _stopState  = State::HostIp;
}


bool  ArnDiscoverInfo::inProgress()  const
{
    return (_state < _stopState) && (_state != State::HostInfoErr) && (_state != State::HostIpErr);
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


int  ArnDiscoverInfo::resolvCode() const
{
    return _resolvCode;
}


///////// ArnDiscoverBrowser

ArnDiscoverBrowser::ArnDiscoverBrowser( QObject* parent) :
    ArnDiscoverBrowserB( parent)
{
}


///////// ArnDiscoverResolver

ArnDiscoverResolver::ArnDiscoverResolver( QObject* parent) :
    ArnDiscoverBrowserB( parent)
{
}


void  ArnDiscoverResolver::resolve( QString serviceName, bool forceUpdate)
{
    ArnDiscoverBrowserB::resolve( serviceName.isEmpty() ? _defaultService : serviceName, forceUpdate);
}


QString  ArnDiscoverResolver::defaultService()  const
{
    return _defaultService;
}


void  ArnDiscoverResolver::setDefaultService( const QString& defaultService)
{
    _defaultService = defaultService;
}


///////// ArnDiscoverBrowserB

ArnDiscoverBrowserB::ArnDiscoverBrowserB( QObject* parent) :
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


const ArnDiscoverInfo&  ArnDiscoverBrowserB::infoByIndex( int index)
{
    static ArnDiscoverInfo  nullInfo;

    if ((index < 0) || (index >= _activeServInfos.size()))  return nullInfo;

    return _activeServInfos.at( index);
}


const ArnDiscoverInfo&  ArnDiscoverBrowserB::infoById( int id)
{
    int  index = _activeServIds.indexOf( id);
    return infoByIndex( index);
}


const ArnDiscoverInfo&  ArnDiscoverBrowserB::infoByName( QString serviceName)
{
    return infoById( serviceNameToId( serviceName));
}


int  ArnDiscoverBrowserB::indexToId( int index)
{
    if ((index < 0) || (index >= _activeServIds.size()))  return -1;

    return _activeServIds.at( index);
}


int  ArnDiscoverBrowserB::IdToIndex( int id)
{
    return _activeServIds.indexOf( id);
}


int  ArnDiscoverBrowserB::serviceNameToId( const QString& name)
{
    foreach (const ArnDiscoverInfo& info, _activeServInfos) {
        if (info._serviceName == name)
            return info._id;
    }
    return -1;  // Not found
}


bool  ArnDiscoverBrowserB::isBrowsing()  const
{
    return _serviceBrowser->isBrowsing();
}


void  ArnDiscoverBrowserB::setFilter( ArnDiscover::Type typeFilter)
{
    switch (typeFilter) {
    case ArnDiscover::Type::Server:  _filter = "server";  break;
    case ArnDiscover::Type::Client:  _filter = "client";  break;
    case ArnDiscover::Type::None:
        // Fall throu
    default:                         _filter = "";
    }
}


void ArnDiscoverBrowserB::setFilter( QString group)
{
    _filter = group;
}


ArnDiscoverInfo::State  ArnDiscoverBrowserB::defaultStopState()  const
{
    return _defaultStopState;
}


void  ArnDiscoverBrowserB::setDefaultStopState( ArnDiscoverInfo::State defaultStopState)
{
    _defaultStopState = defaultStopState;
}


bool  ArnDiscoverBrowserB::goTowardState( int index, ArnDiscoverInfo::State state)
{
    ArnDiscoverInfo&  info = _activeServInfos[ index];
    if (state <= info._state)  return false;  // Can only go forward

    if (info.inProgress()) {  // Next state is in progress
        info._stopState = state;  // Just update final state
    }
    else if (info._state <= info._stopState) {  // Nothing in progress
        info._stopState = state;  // Update for new final state
        doNextState( info);  // Startup state change
    }
    else {
        Q_ASSERT_X(false, "ArnDiscoverBrowser::goTowardState()", "State passed stopState");
        return false;  // Internal error
    }

    return true;
}


void  ArnDiscoverBrowserB::browse( bool enable)
{
    if (!enable)  return stopBrowse();
    if (isBrowsing())  return;  // Already browsing

    _activeServIds.clear();
    _activeServInfos.clear();
    _ipLookupIds.clear();

    _serviceBrowser->setSubType( _filter);
    _serviceBrowser->browse( enable);
}


void  ArnDiscoverBrowserB::stopBrowse()
{
    _serviceBrowser->stopBrowse();
}


void  ArnDiscoverBrowserB::resolve( QString serviceName, bool forceUpdate)
{
    qDebug() << "Man resolve Service: name=" << serviceName;

    int  id = serviceNameToId( serviceName);
    if ((id >= 0) && forceUpdate) {
        int  index = IdToIndex( id);
        removeServiceInfo( index);
        id = -1;  // Mark not found
    }

    ArnDiscoverInfo*  info = 0;
    int index = -1;
    if (id < 0) {  // Not found, resolve new service
        id = ArnZeroConfBrowser::getNextId();
        index = newServiceInfo( id, serviceName, QString());
        Q_ASSERT(index >= 0);  // Already resolving, internal error ...

        info = &_activeServInfos[ index];
        doNextState( *info);
    }
    else {  // Already resolving / resolved
        index = IdToIndex( id);
        info = &_activeServInfos[ index];
        if (!info->inProgress())  // Not in progress, possibly do retry error
            doNextState( *info);
    }
    Q_ASSERT(info && (index >= 0));
    if (info->inProgress()) {
        info->_resolvCode = ArnZeroConf::Error::Running;
        emit infoUpdated( index, info->_state);
    }
}


void  ArnDiscoverBrowserB::onBrowseError( int code)
{
    qDebug() << "Browse Error code=" << code;
}


void  ArnDiscoverBrowserB::onServiceAdded( int id, QString name, QString domain)
{
    qDebug() << "Browse Service added: name=" << name << " domain=" << domain
             << " escFullDomain=" << _serviceBrowser->escapedFullDomain();

    int  index = newServiceInfo( id, name, domain);
    Q_ASSERT(index >= 0);

    emit serviceAdded( index, name);
    doNextState( _activeServInfos[ index]);
}


void  ArnDiscoverBrowserB::onServiceRemoved( int id, QString name, QString domain)
{
    qDebug() << "Browse Service removed: name=" << name << " domain=" << domain;
    int  index = _activeServIds.indexOf( id);
    removeServiceInfo( index);

    emit serviceRemoved( index);
}


void  ArnDiscoverBrowserB::onResolveError( int id, int code)
{
    ArnZeroConfResolv*  ds = qobject_cast<ArnZeroConfResolv*>( sender());
    Q_ASSERT(ds);

    qDebug() << "Resolve Error code=" << code;

    int  index = _activeServIds.indexOf( id);
    if (index >= 0) {  // Service still exist
        ArnDiscoverInfo&  info = _activeServInfos[ index];
        info._state      = ArnDiscoverInfo::State::HostInfoErr;
        info._resolvCode = code;

        emit infoUpdated( index, info._state);
        // Will not go to next state during error
    }

    ds->releaseService();
    ds->deleteLater();
}


void  ArnDiscoverBrowserB::onResolved( int id, QByteArray escFullDomain)
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
        info._resolvCode = ArnZeroConf::Error::Ok;
        info._hostName   = ds->host();
        info._hostPort   = ds->port();
        info._properties = xsmTxt;

        emit infoUpdated( index, info._state);
        doNextState( info);
    }

    ds->releaseService();
    ds->deleteLater();
}


void  ArnDiscoverBrowserB::onIpLookup( const QHostInfo& host)
{
    int  ipLookupId = host.lookupId();
    int  id = _ipLookupIds.value( ipLookupId, -1);
    qDebug() << "onIpLookup: lookupId=" << ipLookupId;
    if (id < 0)  return;  // Service not valid anymore

    _ipLookupIds.remove( ipLookupId);

    int  index = _activeServIds.indexOf( id);
    if (index < 0)  return;  // Service not exist anymore

    ArnDiscoverInfo&  info = _activeServInfos[ index];

    if (host.error() != QHostInfo::NoError) {
         qDebug() << "Lookup failed:" << host.errorString();
         info._state = ArnDiscoverInfo::State::HostIpErr;

         emit infoUpdated( index, info._state);
         return;
    }

    foreach (const QHostAddress &address, host.addresses())
        qDebug() << "Found address:" << address.toString();

    info._state = ArnDiscoverInfo::State::HostIp;
    info._hostIp = host.addresses().first();

    emit infoUpdated( index, info._state);
}


int  ArnDiscoverBrowserB::newServiceInfo( int id, QString name, QString domain)
{
    ArnDiscoverInfo  info;
    info._id          = id;
    info._state       = ArnDiscoverInfo::State::ServiceName;
    info._serviceName = name;
    info._domain      = domain;
    info._stopState   = _defaultStopState;

    int  index;
    for (index = 0; index < _activeServInfos.size(); ++index) {
        const QString&  indexName = _activeServInfos.at( index)._serviceName;
        if (name == indexName)  return -1;  // Error already exist
        if (name < indexName)  break;  // Sorting place found
    }
    _activeServIds.insert( index, id);
    _activeServInfos.insert( index, info);

    return index;
}


void  ArnDiscoverBrowserB::removeServiceInfo( int index)
{
    if ((index < 0) || (index >= _activeServIds.size()))  return;

    _activeServIds.removeAt( index);
    _activeServInfos.removeAt( index);
}


void  ArnDiscoverBrowserB::doNextState( ArnDiscoverInfo& info)
{
    if (info._state >= info._stopState)  return;  // At stop state, do nothing more now

    switch (info._state) {
    case ArnDiscoverInfo::State::HostInfoErr:  // Will retry resolv
        // Fall throu
    case ArnDiscoverInfo::State::ServiceName:
    {
        info._state = ArnDiscoverInfo::State::ServiceName;

        ArnZeroConfResolv*  ds = new ArnZeroConfResolv( info._serviceName, this);
        ds->setId( info._id);
        connect( ds, SIGNAL(resolveError(int,int)), this, SLOT(onResolveError(int,int)));
        connect( ds, SIGNAL(resolved(int,QByteArray)), this, SLOT(onResolved(int,QByteArray)));
        ds->resolve();
        break;
    }
    case ArnDiscoverInfo::State::HostIpErr:  // Will retry ip lookup
        // Fall throu
    case ArnDiscoverInfo::State::HostInfo:
    {
        info._state = ArnDiscoverInfo::State::HostInfo;

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
    _hasSetupAdvertise = false;
}


void  ArnDiscoverAdvertise::advertiseService(ArnDiscover::Type discoverType, QString serviceName,
                                             int port, const QString& hostName)
{
    _discoverType = discoverType;
    _service      = serviceName;

    XStringMap  xsm;
    xsm.add("protovers", "1.0");
    xsm.add("server", QByteArray::number( _discoverType == ArnDiscover::Type::Server));
    _arnZCReg->setSubTypes( _groups);
    _arnZCReg->addSubType( _discoverType == ArnDiscover::Type::Server ? "server" : "client");
    for (int i = 0; i < _groups.size(); ++i) {
        xsm.add("group", i, _groups.at(i));
    }
    _arnZCReg->setTxtRecordMap( xsm);
    _arnZCReg->setHost( hostName);
    _arnZCReg->setPort( port);
    connect( _arnZCReg, SIGNAL(registered(QString)), this, SLOT(serviceRegistered(QString)));
    connect( _arnZCReg, SIGNAL(registrationError(int)), this, SLOT(serviceRegistrationError(int)));

    QTimer::singleShot(0, this, SLOT(postSetupThis()));  // Ĺet persistance service etc init before ...
}


void  ArnDiscoverAdvertise::postSetupThis()
{
    _hasSetupAdvertise = true;
    if (!_service.isNull())
        setService( _service);
}


void  ArnDiscoverAdvertise::serviceRegistered( QString serviceName)
{
    qDebug() << "DiscoverAdvertice Service registered: serviceName=" << serviceName;

    emit serviceChanged( serviceName);
}


void  ArnDiscoverAdvertise::serviceRegistrationError(int code)
{
    qDebug() << "Service registration error: code=" << code;

    emit serviceChangeError( code);
}


bool  ArnDiscoverAdvertise::hasSetupAdvertise()  const
{
    return _hasSetupAdvertise;
}


QString  ArnDiscoverAdvertise::service()  const
{
    return _service;
}


void  ArnDiscoverAdvertise::setService( QString service)
{
    _service = service;
    if (!_hasSetupAdvertise)  return;

    qDebug() << "Advertise Service changed: servname=" << _service;
    if (_arnZCReg->state() != ArnZeroConf::State::None)
        _arnZCReg->releaseService();
    _arnZCReg->setServiceName( _service);
    _arnZCReg->registerService();
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
