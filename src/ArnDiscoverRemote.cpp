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

#include "ArnInc/ArnDiscoverRemote.hpp"
#include "ArnInc/ArnZeroConf.hpp"
#include "ArnInc/ArnServer.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QTimer>
#include <QMetaObject>
#include <QHostInfo>
#include <QNetworkInterface>


///////// ArnDiscoverRemote

ArnDiscoverRemote::ArnDiscoverRemote( QObject *parent) :
    ArnDiscoverAdvertise( parent)
{
    _servTimer             = new QTimer( this);
    _arnInternalServer     = 0;
    _arnDResolver          = 0;
    _defaultService        = "Arn Default Service";
    _initialServiceTimeout = 0;
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

    //// Publish static list of network interfaces in Arn
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

            QString  path = (Arn::pathDiscoverThis + "Interfaces/Interf-%1/").arg(i);
            QString  addr = entry.ip().toString();
            QString  name = interface.humanReadableName();
            ArnM::setValue( path + "addr", addr);
            ArnM::setValue( path + "name", name);
            ArnM::setValue( path + "value", addr + "  [" + name + "]");
            ++i;
        }
    }
    
    // Setup advertise, but don't start yet, can be waiting for service name
    ArnDiscoverAdvertise::advertiseService( discoverType, service(), hostPort);
}


void  ArnDiscoverRemote::startUseNewServer( ArnDiscover::Type discoverType, int port)
{
    if (!_arnInternalServer)
        delete _arnInternalServer;
    _arnInternalServer = new ArnServer( ArnServer::Type::NetSync, this);
    _arnInternalServer->start( port);

    startUseServer( _arnInternalServer, discoverType);
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
    _servTimer->start( _initialServiceTimeout * 1000);  // If no service name set, give time for external setting ...
    connect( _servTimer, SIGNAL(timeout()), this, SLOT(serviceTimeout()));

    connect( &_arnService,   SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    connect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    QString  servicePath = Arn::pathDiscoverThis + "Service/value";
    _arnServicePv.open( Arn::twinPath( servicePath));
    _arnService.addMode( ArnItem::Mode::Save);
    _arnService.open( servicePath);
    // Any loaded persistent service name has now been sent to firstServiceSetup()

    ArnDiscoverAdvertise::postSetupThis();
}


void  ArnDiscoverRemote::serviceTimeout()
{
    if (Arn::debugDiscover)  qDebug() << "First service setup timeout, using default.";

    firstServiceSetup( _defaultService, true);
}


void  ArnDiscoverRemote::firstServiceSetup( QString serviceName, bool forceSetup)
{
    QString  useService = service();  // Priority for any previous set service name
    if (useService.isEmpty())
        useService = serviceName;
    if (Arn::debugDiscover)  qDebug() << "firstServiceSetup: service=" << serviceName
                                      << " useService=" << useService << " forceSetup=" << forceSetup;

    if (!forceSetup && useService.isEmpty())  return;  // Try later (at timeout)

    _servTimer->stop();
    disconnect( &_arnService,   SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    disconnect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    connect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(doServiceChanged(QString)));

    setService( useService);
}


void  ArnDiscoverRemote::doServiceChanged( QString val)
{
    if (Arn::debugDiscover)  qDebug() << "DiscoverRemote Service changed: servname=" << val;
    _arnServicePv = val;
    ArnDiscoverAdvertise::setService( val.isEmpty() ? _defaultService : val);
}


void  ArnDiscoverRemote::serviceRegistered( QString serviceName)
{
    if (Arn::debugDiscover)  qDebug() << "DiscoverRemote Service registered: serviceName=" << serviceName;

    ArnM::setValue( Arn::pathDiscoverThis + "UsingService/value", serviceName);

    ArnDiscoverAdvertise::serviceRegistered( serviceName);
}


void  ArnDiscoverRemote::setService( QString service)
{
    if (Arn::debugDiscover)  qDebug() << "DiscoverRemote setService: serviceName=" << service;

    if (hasSetupAdvertise()) {
        bool  isAdvertise = state().isAny( State::Advertise);
        _arnService.setValue( service, isAdvertise ? Arn::SameValue::Ignore
                                                   : Arn::SameValue::Accept);
        // If advertising not yet has started, any value will be tried as service name
    }
    else
        ArnDiscoverAdvertise::setService( service);
}


QString  ArnDiscoverRemote::defaultService()  const
{
    return _defaultService;
}


void  ArnDiscoverRemote::setDefaultService( const QString& defaultService)
{
    _defaultService = defaultService;
}


int  ArnDiscoverRemote::initialServiceTimeout()  const
{
    return _initialServiceTimeout;
}


void  ArnDiscoverRemote::setInitialServiceTimeout( int initialServiceTimeout)
{
    _initialServiceTimeout = initialServiceTimeout;
}
