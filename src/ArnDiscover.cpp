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

#include "ArnInc/ArnDiscover.hpp"
#include "private/ArnDiscover_p.hpp"
#include "ArnInc/ArnZeroConf.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QHostInfo>
#include <QNetworkInterface>
#include <QTimer>

using Arn::XStringMap;


///////// ArnDiscoverInfo

ArnDiscoverInfoPrivate::ArnDiscoverInfoPrivate()
{
    _id         = -1;  // Unknown
    _resolvCode = ArnZeroConf::Error::Ok;
    _hostPort   = 0;
    _stopState  = ArnDiscoverInfo::State::HostIp;
}


ArnDiscoverInfoPrivate::~ArnDiscoverInfoPrivate()
{
}


ArnDiscoverInfo::ArnDiscoverInfo()
    : d_ptr( new ArnDiscoverInfoPrivate)
{
}


ArnDiscoverInfo::ArnDiscoverInfo( ArnDiscoverInfoPrivate& dd)
    : d_ptr( &dd)
{
}


ArnDiscoverInfo::ArnDiscoverInfo( const ArnDiscoverInfo& other)
    : d_ptr( new ArnDiscoverInfoPrivate( *other.d_ptr))
{
}


ArnDiscoverInfo&  ArnDiscoverInfo::operator=( const ArnDiscoverInfo& other)
{
    *d_ptr = *other.d_ptr;

    return *this;
}


ArnDiscoverInfo::~ArnDiscoverInfo()
{

    delete d_ptr;
}


bool  ArnDiscoverInfo::inProgress()  const
{
    Q_D(const ArnDiscoverInfo);

    return (d->_state < d->_stopState) && !isError();
}


bool  ArnDiscoverInfo::isError()  const
{
    Q_D(const ArnDiscoverInfo);

    return (d->_state == State::HostInfoErr) || (d->_state == State::HostIpErr);
}


ArnDiscoverInfo::State  ArnDiscoverInfo::state()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_state;
}


ArnDiscoverInfo::State  ArnDiscoverInfo::stopState()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_stopState;
}


ArnDiscover::Type  ArnDiscoverInfo::type()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_type;
}


QStringList  ArnDiscoverInfo::groups()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_properties.values("group");
}


QString  ArnDiscoverInfo::serviceName()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_serviceName;
}


QString  ArnDiscoverInfo::domain()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_domain;
}


QString  ArnDiscoverInfo::hostName()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_hostName;
}


quint16  ArnDiscoverInfo::hostPort()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_hostPort;
}


QHostAddress  ArnDiscoverInfo::hostIp()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_hostIp;
}


XStringMap  ArnDiscoverInfo::properties()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_properties;
}


QString  ArnDiscoverInfo::typeString()  const
{
    Q_D(const ArnDiscoverInfo);

    if (d->_state < State::HostInfo)  return QString();

    switch (d->_type) {
    case ArnDiscover::Type::Server:  return "Server";
    case ArnDiscover::Type::Client:  return "Client";
    case ArnDiscover::Type::None:
        // Fall throu
    default:                         return "Unknown";
    }
}


QString  ArnDiscoverInfo::hostPortString()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_state < State::HostInfo ? QString() : QString::number( d->_hostPort);
}


QString  ArnDiscoverInfo::hostIpString()  const
{
    Q_D(const ArnDiscoverInfo);

    return d->_state < State::HostIp ? QString() : d->_hostIp.toString();
}


QString ArnDiscoverInfo::hostWithInfo() const
{
    return Arn::makeHostWithInfo( hostIpString(), hostName());
}


int  ArnDiscoverInfo::resolvCode() const
{
    Q_D(const ArnDiscoverInfo);

    return d->_resolvCode;
}


///////// ArnDiscoverBrowser

ArnDiscoverBrowser::ArnDiscoverBrowser( QObject* parent) :
    ArnDiscoverBrowserB( parent)
{
}


///////// ArnDiscoverResolver

ArnDiscoverResolverPrivate::ArnDiscoverResolverPrivate()
{
    _defaultService = "Arn Default Service";
}


ArnDiscoverResolverPrivate::~ArnDiscoverResolverPrivate()
{
}


ArnDiscoverResolver::ArnDiscoverResolver( QObject* parent)
    : ArnDiscoverBrowserB( *new ArnDiscoverResolverPrivate, parent)
{
}


ArnDiscoverResolver::ArnDiscoverResolver( ArnDiscoverResolverPrivate& dd, QObject* parent)
    : ArnDiscoverBrowserB( dd, parent)
{
}


int  ArnDiscoverResolver::resolve( const QString& serviceName, bool forceUpdate)
{
    Q_D(ArnDiscoverResolver);

    return ArnDiscoverBrowserB::resolve( serviceName.isEmpty() ? d->_defaultService : serviceName, forceUpdate);
}


QString  ArnDiscoverResolver::defaultService()  const
{
    Q_D(const ArnDiscoverResolver);

    return d->_defaultService;
}


void  ArnDiscoverResolver::setDefaultService( const QString& defaultService)
{
    Q_D(ArnDiscoverResolver);

    if (!defaultService.isEmpty())
        d->_defaultService = defaultService;
}


///////// ArnDiscoverBrowserB

ArnDiscoverBrowserBPrivate::ArnDiscoverBrowserBPrivate()
{
    _defaultStopState = ArnDiscoverInfo::State::HostIp;
    _serviceBrowser   = new ArnZeroConfBrowser;
}


ArnDiscoverBrowserBPrivate::~ArnDiscoverBrowserBPrivate()
{
    delete _serviceBrowser;
}


void ArnDiscoverBrowserB::init()
{
    Q_D(ArnDiscoverBrowserB);

    connect( d->_serviceBrowser, SIGNAL(browseError(int)),
             this, SLOT(onBrowseError(int)));
    connect( d->_serviceBrowser, SIGNAL(serviceAdded(int,QString,QString)),
             this, SLOT(onServiceAdded(int,QString,QString)));
    connect( d->_serviceBrowser, SIGNAL(serviceRemoved(int,QString,QString)),
             this, SLOT(onServiceRemoved(int,QString,QString)));

    //// Make a list of subNets for network interfaces on this computer
    foreach (QNetworkInterface  interface, QNetworkInterface::allInterfaces()) {
        QNetworkInterface::InterfaceFlags  flags = interface.flags();
        if (!flags.testFlag( QNetworkInterface::IsUp)
        || flags.testFlag( QNetworkInterface::IsPointToPoint)
        || flags.testFlag( QNetworkInterface::IsLoopBack))
            continue;

        foreach (QNetworkAddressEntry  entry, interface.addressEntries()) {
            QAbstractSocket::NetworkLayerProtocol  prot = entry.ip().protocol();
            if ((prot != QAbstractSocket::IPv4Protocol) && (prot != QAbstractSocket::IPv6Protocol))
                continue;

            int  prefixLen = entry.prefixLength();
            if (prefixLen < 0) {
                // This is a bug in some Qt for android, windows ...  (Not linux)
                prefixLen = 24;
                if (Arn::warningMDNS)  qWarning() << "Bad netmask: nif=" << interface.humanReadableName()
                                                  << ", asume prefixLen=" << prefixLen;
            }
            ArnDiscoverBrowserBPrivate::SubNet  subNet( entry.ip(), prefixLen);
            d->_localHostNetList += subNet;
        }
    }
}


ArnDiscoverBrowserB::ArnDiscoverBrowserB( QObject* parent) :
    QObject( parent)
  , d_ptr( new ArnDiscoverBrowserBPrivate)
{
    init();
}


ArnDiscoverBrowserB::~ArnDiscoverBrowserB()
{
    delete d_ptr;
}


ArnDiscoverBrowserB::ArnDiscoverBrowserB( ArnDiscoverBrowserBPrivate& dd, QObject* parent)
    : QObject( parent)
    , d_ptr( &dd)
{
    init();
}


int  ArnDiscoverBrowserB::serviceCount()  const
{
    Q_D(const ArnDiscoverBrowserB);

    return d->_activeServInfos.size();
}


const ArnDiscoverInfo&  ArnDiscoverBrowserB::infoByIndex( int index)
{
    Q_D(ArnDiscoverBrowserB);

    static ArnDiscoverInfo  nullInfo;

    if ((index < 0) || (index >= d->_activeServInfos.size()))  return nullInfo;

    return d->_activeServInfos.at( index);
}


const ArnDiscoverInfo&  ArnDiscoverBrowserB::infoById( int id)
{
    Q_D(ArnDiscoverBrowserB);

    int  index = d->_activeServIds.indexOf( id);
    return infoByIndex( index);
}


const ArnDiscoverInfo&  ArnDiscoverBrowserB::infoByName( const QString& serviceName)
{
    return infoById( serviceNameToId( serviceName));
}


int  ArnDiscoverBrowserB::indexToId( int index)
{
    Q_D(ArnDiscoverBrowserB);

    if ((index < 0) || (index >= d->_activeServIds.size()))  return -1;

    return d->_activeServIds.at( index);
}


int  ArnDiscoverBrowserB::IdToIndex( int id)
{
    Q_D(ArnDiscoverBrowserB);

    return d->_activeServIds.indexOf( id);
}


int  ArnDiscoverBrowserB::serviceNameToId( const QString& name)
{
    Q_D(ArnDiscoverBrowserB);

    foreach (const ArnDiscoverInfo& info, d->_activeServInfos) {
        if (info.d_ptr->_serviceName == name)
            return info.d_ptr->_id;
    }
    return -1;  // Not found
}


bool  ArnDiscoverBrowserB::isBrowsing()  const
{
    Q_D(const ArnDiscoverBrowserB);

    return d->_serviceBrowser->isBrowsing();
}


void  ArnDiscoverBrowserB::setFilter( ArnDiscover::Type typeFilter)
{
    Q_D(ArnDiscoverBrowserB);

    switch (typeFilter) {
    case ArnDiscover::Type::Server:  d->_filter = "server";  break;
    case ArnDiscover::Type::Client:  d->_filter = "client";  break;
    case ArnDiscover::Type::None:
        // Fall throu
    default:                         d->_filter = "";
    }
}


void ArnDiscoverBrowserB::setFilter( const QString& group)
{
    Q_D(ArnDiscoverBrowserB);

    d->_filter = group;
}


ArnDiscoverInfo::State  ArnDiscoverBrowserB::defaultStopState()  const
{
    Q_D(const ArnDiscoverBrowserB);

    return d->_defaultStopState;
}


void  ArnDiscoverBrowserB::setDefaultStopState( ArnDiscoverInfo::State defaultStopState)
{
    Q_D(ArnDiscoverBrowserB);

    d->_defaultStopState = defaultStopState;
}


bool  ArnDiscoverBrowserB::goTowardState( int index, ArnDiscoverInfo::State state)
{
    Q_D(ArnDiscoverBrowserB);

    if ((index < 0) || (index >= d->_activeServInfos.size()))  return false;  // Out of index

    ArnDiscoverInfo&  info = d->_activeServInfos[ index];
    if (state <= info.state())  return false;  // Can only go forward

    if (info.inProgress()) {  // Next state is in progress
        info.d_ptr->_stopState = state;  // Just update final state
    }
    else if (info.state() <= info.stopState()) {  // Nothing in progress
        info.d_ptr->_stopState = state;  // Update for new final state
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
    Q_D(ArnDiscoverBrowserB);

    if (!enable)  return stopBrowse();
    if (isBrowsing())  return;  // Already browsing

    d->_activeServIds.clear();
    d->_activeServInfos.clear();

    d->_serviceBrowser->setSubType( d->_filter);
    d->_serviceBrowser->browse( enable);
}


void  ArnDiscoverBrowserB::stopBrowse()
{
    Q_D(ArnDiscoverBrowserB);

    d->_serviceBrowser->stopBrowse();
}


int  ArnDiscoverBrowserB::resolve( const QString& serviceName, bool forceUpdate)
{
    Q_D(ArnDiscoverBrowserB);

    if (Arn::debugDiscover)  qDebug() << "Man resolve Service: name=" << serviceName;

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

        info = &d->_activeServInfos[ index];
        doNextState( *info);
    }
    else {  // Already resolving / resolved
        index = IdToIndex( id);
        info = &d->_activeServInfos[ index];
        if (!info->inProgress())  // Not in progress, possibly do retry error
            doNextState( *info);
    }
    Q_ASSERT(info && (index >= 0));

    if (info->inProgress()) {
        info->d_ptr->_resolvCode = ArnZeroConf::Error::Running;
    }
    emit infoUpdated( index, info->state());

    return index;
}


void  ArnDiscoverBrowserB::onBrowseError( int code)
{
    if (Arn::debugDiscover)  qDebug() << "Browse Error code=" << code;
}


void  ArnDiscoverBrowserB::onServiceAdded( int id, const QString& name, const QString& domain)
{
    Q_D(ArnDiscoverBrowserB);

    if (Arn::debugDiscover)  qDebug() << "Browse Service added: name=" << name << " domain=" << domain
                                      << " escFullDomain=" << d->_serviceBrowser->escapedFullDomain();

    int  index = newServiceInfo( id, name, domain);
    Q_ASSERT(index >= 0);

    emit serviceAdded( index, name);
    doNextState( d->_activeServInfos[ index]);
}


void  ArnDiscoverBrowserB::onServiceRemoved( int id, const QString& name, const QString& domain)
{
    Q_D(ArnDiscoverBrowserB);

    if (Arn::debugDiscover)  qDebug() << "Browse Service removed: name=" << name << " domain=" << domain;
    int  index = d->_activeServIds.indexOf( id);
    removeServiceInfo( index);

    emit serviceRemoved( index);
}


void  ArnDiscoverBrowserB::onResolveError( int id, int code)
{
    Q_D(ArnDiscoverBrowserB);

    ArnZeroConfResolve*  ds = qobject_cast<ArnZeroConfResolve*>( sender());
    Q_ASSERT(ds);

    if (Arn::debugDiscover)  qDebug() << "Resolve Error code=" << code;

    int  index = d->_activeServIds.indexOf( id);
    if (index >= 0) {  // Service still exist
        ArnDiscoverInfo&  info = d->_activeServInfos[ index];
        info.d_ptr->_state      = ArnDiscoverInfo::State::HostInfoErr;
        info.d_ptr->_resolvCode = code;

        emit infoUpdated( index, info.state());
        // Will not go to next state during error
    }

    ds->releaseResolve();
    ds->deleteLater();
}


void  ArnDiscoverBrowserB::onResolved( int id, const QByteArray& escFullDomain)
{
    Q_UNUSED(escFullDomain)
    Q_D(ArnDiscoverBrowserB);

    ArnZeroConfResolve*  ds = qobject_cast<ArnZeroConfResolve*>( sender());
    Q_ASSERT(ds);

    QString  name = ds->serviceName();
    if (Arn::debugDiscover)  qDebug() << "Resolved Service: name=" << name
                                      << " escFullDomain=" << ds->escapedFullDomain();
    int  index = d->_activeServIds.indexOf( id);
    if (index >= 0) {  // Service still exist
        ArnDiscoverInfo&  info = d->_activeServInfos[ index];
        XStringMap  xsmTxt;
        ds->getTxtRecordMap( xsmTxt);
        if (xsmTxt.key( xsmTxt.size() - 1) == "protovers")  // Mitigate reversed order from ZeroConfig manipulation
            xsmTxt.reverseOrder();
        QByteArray  servProp = xsmTxt.value("server");
        info.d_ptr->_type = servProp.isNull() ? ArnDiscover::Type::None
                                              : (servProp.toInt() ? ArnDiscover::Type::Server
                                                           : ArnDiscover::Type::Client);
        info.d_ptr->_state      = ArnDiscoverInfo::State::HostInfo;
        info.d_ptr->_resolvCode = ArnZeroConf::Error::Ok;
        info.d_ptr->_hostName   = ds->host();
        info.d_ptr->_hostPort   = ds->port();
        info.d_ptr->_properties = xsmTxt;

        emit infoUpdated( index, info.state());
        doNextState( info);
    }

    ds->releaseResolve();
    ds->deleteLater();
}


void  ArnDiscoverBrowserB::onLookupError( int id, int code)
{
    Q_D(ArnDiscoverBrowserB);

    ArnZeroConfLookup*  ds = qobject_cast<ArnZeroConfLookup*>( sender());
    Q_ASSERT(ds);

    if (Arn::debugDiscover)  qDebug() << "Lookup Error code=" << code;

    int  index = d->_activeServIds.indexOf( id);
    if (index >= 0) {  // Service still exist
        ArnDiscoverInfo&  info = d->_activeServInfos[ index];
        info.d_ptr->_state = ArnDiscoverInfo::State::HostIpErr;
        info.d_ptr->_resolvCode = code;  // MW: Ok?

        emit infoUpdated( index, info.state());
        // Will not go to next state during error
    }

    ds->releaseLookup();
    ds->deleteLater();
}


void  ArnDiscoverBrowserB::onLookuped( int id)
{
    Q_D(ArnDiscoverBrowserB);

    ArnZeroConfLookup*  ds = qobject_cast<ArnZeroConfLookup*>( sender());
    Q_ASSERT(ds);

    QString  hostName = ds->host();
    if (Arn::debugDiscover)  qDebug() << "Lookuped host: name=" << hostName;
    int  index = d->_activeServIds.indexOf( id);
    if (index >= 0) {  // Service still exist
        ArnDiscoverInfo&  info    = d->_activeServInfos[ index];
        info.d_ptr->_state        = ArnDiscoverInfo::State::HostIp;
        info.d_ptr->_hostIpLookup = ds->hostAddr();
        info.d_ptr->_hostIp       = info.d_ptr->_hostIpLookup;  // Prelimary
        info.d_ptr->_resolvCode   = ArnZeroConf::Error::Ok;  // MW: Ok?
        doHostIpLogic( info);

        emit infoUpdated( index, info.state());
    }

    ds->releaseLookup();
    ds->deleteLater();
}


void  ArnDiscoverBrowserB::doHostIpLogic( ArnDiscoverInfo& info)
{
    Q_D(ArnDiscoverBrowserB);

    QList<QHostAddress>  remHostIpList;
    remHostIpList += info.d_ptr->_hostIpLookup;  // Lookup Ip has first prio

    XStringMap&  xsm = info.d_ptr->_properties;
    for (uint i = 0; true; ++i) {
        QString  hostIpTxt = xsm.valueString( "hostIp", i);
        if (hostIpTxt.isNull())  break;
        QHostAddress  hostIp( hostIpTxt);
        if (hostIp.isNull())  continue;  // Not valid address

        remHostIpList += hostIp;
    }

    foreach (const QHostAddress& remHostIp, remHostIpList) {
        foreach ( const ArnDiscoverBrowserBPrivate::SubNet& subNet, d->_localHostNetList) {
            if (remHostIp.isInSubnet( subNet)) {  // This remote host can be reached in a local net
                info.d_ptr->_hostIp = remHostIp;  // Modify to use this address (can be same)
            }
        }
    }
}


int  ArnDiscoverBrowserB::newServiceInfo( int id, const QString& name, const QString& domain)
{
    Q_D(ArnDiscoverBrowserB);

    ArnDiscoverInfo  info;
    info.d_ptr->_id          = id;
    info.d_ptr->_state       = ArnDiscoverInfo::State::ServiceName;
    info.d_ptr->_serviceName = name;
    info.d_ptr->_domain      = domain;
    info.d_ptr->_stopState   = d->_defaultStopState;

    int  index;
    for (index = 0; index < d->_activeServInfos.size(); ++index) {
        const QString&  indexName = d->_activeServInfos.at( index).serviceName();
        if (name == indexName)  return -1;  // Error already exist
        if (name < indexName)  break;  // Sorting place found
    }
    d->_activeServIds.insert( index, id);
    d->_activeServInfos.insert( index, info);

    return index;
}


void  ArnDiscoverBrowserB::removeServiceInfo( int index)
{
    Q_D(ArnDiscoverBrowserB);

    if ((index < 0) || (index >= d->_activeServIds.size()))  return;

    d->_activeServIds.removeAt( index);
    d->_activeServInfos.removeAt( index);
}


void  ArnDiscoverBrowserB::doNextState( ArnDiscoverInfo& info)
{
    if (info.state() >= info.stopState())  return;  // At stop state, do nothing more now

    switch (info.state()) {
    case ArnDiscoverInfo::State::HostInfoErr:  // Will retry resolv
        // Fall throu
    case ArnDiscoverInfo::State::ServiceName:
    {
        info.d_ptr->_state = ArnDiscoverInfo::State::ServiceName;

        ArnZeroConfResolve*  ds = new ArnZeroConfResolve( info.serviceName(), this);
        ds->setId( info.d_ptr->_id);
        connect( ds, SIGNAL(resolveError(int,int)), this, SLOT(onResolveError(int,int)));
        connect( ds, SIGNAL(resolved(int,QByteArray)), this, SLOT(onResolved(int,QByteArray)));
        ds->resolve();
        break;
    }
    case ArnDiscoverInfo::State::HostIpErr:  // Will retry ip lookup
        // Fall throu
    case ArnDiscoverInfo::State::HostInfo:
    {
        info.d_ptr->_state = ArnDiscoverInfo::State::HostInfo;

        ArnZeroConfLookup*  ds = new ArnZeroConfLookup( info.hostName(), this);
        ds->setId( info.d_ptr->_id);
        connect( ds, SIGNAL(lookupError(int,int)), this, SLOT(onLookupError(int,int)));
        connect( ds, SIGNAL(lookuped(int)), this, SLOT(onLookuped(int)));
        if (Arn::debugDiscover)  qDebug() << "LookingUp host=" << info.hostName() << " Id=" << info.d_ptr->_id;
        ds->lookup();
        break;
    }
    default:
        break;
    }
}


///////// ArnDiscoverAdvertise

ArnDiscoverAdvertisePrivate::ArnDiscoverAdvertisePrivate()
{
    _hasSetupAdvertise = false;
    _arnZCReg          = new ArnZeroConfRegister;
}


ArnDiscoverAdvertisePrivate::~ArnDiscoverAdvertisePrivate()
{
    delete _arnZCReg;
}


void ArnDiscoverAdvertise::init()
{
}


ArnDiscoverAdvertise::ArnDiscoverAdvertise( QObject *parent) :
    QObject( parent)
  , d_ptr( new ArnDiscoverAdvertisePrivate)
{
    init();
}


ArnDiscoverAdvertise::ArnDiscoverAdvertise( ArnDiscoverAdvertisePrivate& dd, QObject* parent)
    : QObject( parent)
    , d_ptr( &dd)
{
    init();
}


ArnDiscoverAdvertise::~ArnDiscoverAdvertise()
{

    delete d_ptr;
}


void  ArnDiscoverAdvertise::advertiseService( ArnDiscover::Type discoverType, const QString& serviceName,
                                              int port, const QString& hostName)
{
    Q_D(ArnDiscoverAdvertise);

    if (Arn::debugDiscover)  qDebug() << "Discover advertise setup: serviceName=" << serviceName
                                      << " port=" << port << " hostName=" << hostName;
    d->_discoverType = discoverType;
    d->_service      = serviceName;

    XStringMap  xsm;
    xsm.add("protovers", "1.1");  // Should be first
    xsm.add("arnlibVers", XStringMap( ArnM::info()).value("Ver"));
    xsm.add("server", QByteArray::number( d->_discoverType == ArnDiscover::Type::Server));
    d->_arnZCReg->setSubTypes( d->_groups);
    d->_arnZCReg->addSubType( d->_discoverType == ArnDiscover::Type::Server ? "server" : "client");
    for (int i = 0; i < d->_groups.size(); ++i) {
        xsm.add("group", uint(i), d->_groups.at(i));
    }
    for (int i = 0; i < d->_hostIpList.size(); ++i) {
        xsm.add("hostIp", uint(i), d->_hostIpList.at(i));
    }
    xsm += d->_customProperties;

    d->_arnZCReg->setTxtRecordMap( xsm);
    d->_arnZCReg->setHost( hostName);
    d->_arnZCReg->setPort( port >= 0 ? port : Arn::defaultTcpPort);

    connect( d->_arnZCReg, SIGNAL(registered(QString)), this, SLOT(serviceRegistered(QString)));
    connect( d->_arnZCReg, SIGNAL(registrationError(int)), this, SLOT(serviceRegistrationError(int)));

    QTimer::singleShot(0, this, SLOT(postSetupThis()));  // Ĺet persistance service etc init before ...
}


void  ArnDiscoverAdvertise::postSetupThis()
{
    Q_D(ArnDiscoverAdvertise);

    d->_hasSetupAdvertise = true;
    if (Arn::debugDiscover)  qDebug() << "DiscoverAdvertise setup has finnished";
    if (!d->_service.isEmpty())
        setService( d->_service);
}


void  ArnDiscoverAdvertise::serviceRegistered( const QString& serviceName)
{
    if (Arn::debugDiscover)  qDebug() << "DiscoverAdvertice Service registered: serviceName=" << serviceName;

    emit serviceChanged( serviceName);
}


void  ArnDiscoverAdvertise::serviceRegistrationError(int code)
{
    if (Arn::debugDiscover)  qDebug() << "Service registration error: code=" << code;

    emit serviceChangeError( code);
}


XStringMap  ArnDiscoverAdvertise::customProperties()  const
{
    Q_D(const ArnDiscoverAdvertise);

    return d->_customProperties;
}


void  ArnDiscoverAdvertise::setCustomProperties( const XStringMap& customProperties)
{
    Q_D(ArnDiscoverAdvertise);

    d->_customProperties = customProperties;
}


void ArnDiscoverAdvertise::addCustomProperty(const QString& key, const QString& val)
{
    Q_D(ArnDiscoverAdvertise);

    d->_customProperties.add( key, val);
}


bool  ArnDiscoverAdvertise::hasSetupAdvertise()  const
{
    Q_D(const ArnDiscoverAdvertise);

    return d->_hasSetupAdvertise;
}


void  ArnDiscoverAdvertise::setHostIpList( const QStringList& hostIpList)
{
    Q_D(ArnDiscoverAdvertise);

    d->_hostIpList = hostIpList;
}


QString  ArnDiscoverAdvertise::service()  const
{
    Q_D(const ArnDiscoverAdvertise);

    return d->_service;
}


QString  ArnDiscoverAdvertise::currentService()  const
{
    Q_D(const ArnDiscoverAdvertise);

    return d->_hasSetupAdvertise ? d->_arnZCReg->currentServiceName() : d->_service;
}


ArnDiscoverAdvertise::State  ArnDiscoverAdvertise::state()  const
{
    Q_D(const ArnDiscoverAdvertise);

    return State::fromInt( d->_arnZCReg->state());
}


void  ArnDiscoverAdvertise::setService( const QString& service)
{
    Q_D(ArnDiscoverAdvertise);

    if (Arn::debugDiscover)  qDebug() << "DiscoverAdvertise setService: serviceName=" << service;

    d->_service = service;
    if (!d->_hasSetupAdvertise)  return;
    if (service.isEmpty())  return;

    if (Arn::debugDiscover)  qDebug() << "DiscoverAdvertise Service will change: serviceName=" << d->_service;
    if (d->_arnZCReg->state() != ArnZeroConf::State::None)
        d->_arnZCReg->releaseService();
    d->_arnZCReg->setServiceName( d->_service);
    d->_arnZCReg->registerService();
}


QStringList  ArnDiscoverAdvertise::groups()  const
{
    Q_D(const ArnDiscoverAdvertise);

    return d->_groups;
}


void  ArnDiscoverAdvertise::setGroups( const QStringList& groups)
{
    Q_D(ArnDiscoverAdvertise);

    d->_groups = groups;
}


void  ArnDiscoverAdvertise::addGroup( const QString& group)
{
    Q_D(ArnDiscoverAdvertise);

    d->_groups += group;
}
