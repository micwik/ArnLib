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

#include "ArnInc/ArnDiscoverRemote.hpp"
#include "private/ArnDiscoverRemote_p.hpp"
#include "ArnInc/ArnZeroConf.hpp"
#include "ArnInc/ArnServer.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QTimer>
#include <QMetaObject>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QDir>


///////// ArnDiscoverRemote

ArnDiscoverRemotePrivate::ArnDiscoverRemotePrivate()
{
    _servTimer             = new QTimer;
    _arnInternalServer     = arnNullptr;
    _arnDResolver          = arnNullptr;
    _defaultService        = "Arn Default Service";
    _initialServiceTimeout = 0;
}


ArnDiscoverRemotePrivate::~ArnDiscoverRemotePrivate()
{
    delete _servTimer;
}


ArnDiscoverRemote::ArnDiscoverRemote( QObject *parent)
    : ArnDiscoverAdvertise( *new ArnDiscoverRemotePrivate, parent)
{
}


ArnDiscoverRemote::ArnDiscoverRemote( ArnDiscoverRemotePrivate& dd, QObject* parent)
    : ArnDiscoverAdvertise( dd, parent)
{
}


ArnDiscoverRemote::~ArnDiscoverRemote()
{
}


void  ArnDiscoverRemote::startUseServer( ArnServer* arnServer, ArnDiscover::Type discoverType)
{
    QHostAddress  addr  = arnServer->listenAddress();
    QString  listenAddr = ((addr == QHostAddress::Any) || (addr == QHostAddress::AnyIPv6))
                          ? QString("Any") : addr.toString();
    int  hostPort = arnServer->port();
    ArnM::setValue( Arn::pathDiscoverThis + "Interfaces/Listen/value", listenAddr);
    ArnM::setValue( Arn::pathDiscoverThis + "Host/value", QHostInfo::localHostName());
    ArnM::setValue( Arn::pathDiscoverThis + "Host/Port/value", hostPort);
    ArnM::loadFromDirRoot( Arn::pathDiscover + "help.xhtml", QDir( Arn::resourceArnRoot), Arn::Coding::Text);

    //// Publish static list of network interfaces in Arn
    QStringList  hostIpList;
    int  i = 0;
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

            QString  path    = (Arn::pathDiscoverThis + "Interfaces/Interf-%1/").arg(i);
            QString  addr    = entry.ip().toString();
            QString  mask    = entry.netmask().toString();
            QString  sysName = interface.humanReadableName();
            ArnM::setValue( path + "addr", addr);
            ArnM::setValue( path + "mask", mask);
            ArnM::setValue( path + "sysName", sysName);
            ArnM::setValue( path + "name", addr + "  [" + sysName + "]");

            if (prot == QAbstractSocket::IPv4Protocol) {
                   hostIpList += addr;
            }
            ++i;
        }
    }
    
    // Setup advertise, but don't start yet, can be waiting for service name
    ArnDiscoverAdvertise::setHostIpList( hostIpList);
    ArnDiscoverAdvertise::setEncryptPolicy( arnServer->encryptPolicy());
    ArnDiscoverAdvertise::advertiseService( discoverType, service(), hostPort);
}


void  ArnDiscoverRemote::startUseNewServer( ArnDiscover::Type discoverType, int port)
{
    Q_D(ArnDiscoverRemote);

    if (!d->_arnInternalServer)
        delete d->_arnInternalServer;
    d->_arnInternalServer = new ArnServer( ArnServer::Type::NetSync, this);
    d->_arnInternalServer->start( port);

    startUseServer( d->_arnInternalServer, discoverType);
}


ArnDiscoverConnector*  ArnDiscoverRemote::newConnector( ArnClient& client, const QString& id)
{
    ArnDiscoverConnector*  connector = new ArnDiscoverConnector( client, id);
    connect( connector, SIGNAL(clientReadyToConnect(ArnClient*,QString)),
             this, SIGNAL(clientReadyToConnect(ArnClient*,QString)));

    return connector;
}


void  ArnDiscoverRemote::postSetupThis()
{
    Q_D(ArnDiscoverRemote);

    d->_servTimer->start( d->_initialServiceTimeout * 1000);  // If no service name set, give time for external setting ...
    connect( d->_servTimer, SIGNAL(timeout()), this, SLOT(serviceTimeout()));

    connect( &d->_arnService,   SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    connect( &d->_arnServicePv, SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    QString  servicePath = Arn::pathDiscoverThis + "Service/value";
    d->_arnServicePv.open( Arn::twinPath( servicePath));
    d->_arnService.addMode( Arn::ObjectMode::Save);
    d->_arnService.open( servicePath);
    // Any loaded persistent service name has now been sent to firstServiceSetup()

    ArnDiscoverAdvertise::postSetupThis();
}


void  ArnDiscoverRemote::serviceTimeout()
{
    Q_D(ArnDiscoverRemote);

    if (Arn::debugDiscover)  qDebug() << "First service setup timeout, using default.";

    firstServiceSetup( d->_defaultService, true);
}


void  ArnDiscoverRemote::firstServiceSetup( const QString& serviceName, bool forceSetup)
{
    Q_D(ArnDiscoverRemote);

    QString  useService = service();  // Priority for any previous set service name
    if (useService.isEmpty())
        useService = serviceName;
    if (Arn::debugDiscover)  qDebug() << "firstServiceSetup: service=" << serviceName
                                      << " useService=" << useService << " forceSetup=" << forceSetup;

    if (!forceSetup && useService.isEmpty())  return;  // Try later (at timeout)

    d->_servTimer->stop();
    disconnect( &d->_arnService,   SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    disconnect( &d->_arnServicePv, SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    connect( &d->_arnServicePv, SIGNAL(changed(QString)), this, SLOT(doServiceChanged(QString)));

    setService( useService);
}


void  ArnDiscoverRemote::doServiceChanged( const QString& val)
{
    Q_D(ArnDiscoverRemote);

    if (Arn::debugDiscover)  qDebug() << "DiscoverRemote Service changed: servname=" << val;
    d->_arnServicePv = val;
    ArnDiscoverAdvertise::setService( val.isEmpty() ? d->_defaultService : val);
}


void  ArnDiscoverRemote::serviceRegistered( const QString& serviceName)
{
    if (Arn::debugDiscover)  qDebug() << "DiscoverRemote Service registered: serviceName=" << serviceName;

    ArnM::setValue( Arn::pathDiscoverThis + "UsingService/value", serviceName);

    ArnDiscoverAdvertise::serviceRegistered( serviceName);
}


void  ArnDiscoverRemote::setService( const QString& service)
{
    Q_D(ArnDiscoverRemote);

    if (Arn::debugDiscover)  qDebug() << "DiscoverRemote setService: serviceName=" << service;

    if (hasSetupAdvertise()) {
        bool  isAdvertise = state().isAny( State::Advertise);
        d->_arnService.setValue( service, isAdvertise ? Arn::SameValue::Ignore
                                                      : Arn::SameValue::Accept);
        // If advertising not yet has started, any value must trigger the "service changed logic"
    }
    else
        ArnDiscoverAdvertise::setService( service);
}


QString  ArnDiscoverRemote::defaultService()  const
{
    Q_D(const ArnDiscoverRemote);

    return d->_defaultService;
}


void  ArnDiscoverRemote::setDefaultService( const QString& defaultService)
{
    Q_D(ArnDiscoverRemote);

    if (!defaultService.isEmpty())
        d->_defaultService = defaultService;
}


int  ArnDiscoverRemote::initialServiceTimeout()  const
{
    Q_D(const ArnDiscoverRemote);

    return d->_initialServiceTimeout;
}


void  ArnDiscoverRemote::setInitialServiceTimeout( int initialServiceTimeout)
{
    Q_D(ArnDiscoverRemote);

    d->_initialServiceTimeout = initialServiceTimeout;
}
