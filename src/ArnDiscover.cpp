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
#include "ArnServer.hpp"
#include <QTimer>


ArnDiscoverAdvertise::ArnDiscoverAdvertise( QObject *parent) :
    QObject( parent)
{
    _arnZCReg = new ArnZeroConfRegister( this);
    _servTimer = new QTimer( this);
}


void  ArnDiscoverAdvertise::setArnServer( ArnServer* arnServer)
{
    QString  hostAddr = arnServer->address().toString();
    int      hostPort = arnServer->port();
    ArnM::setValue("/Sys/Discover/This/Host/value", hostAddr);
    ArnM::setValue("/Sys/Discover/This/Host/Port/value", hostPort);

    _arnZCReg->setPort( hostPort);
    connect( _arnZCReg, SIGNAL(registered(QString)), this, SLOT(serviceRegistered(QString)));

    QTimer::singleShot(0, this, SLOT(postSetup()));  // Ĺet persistance service etc init before ...
}


void  ArnDiscoverAdvertise::postSetup()
{
    _servTimer->start( 5000);
    connect( _servTimer, SIGNAL(timeout()), this, SLOT(serviceTimeout()));

    connect( &_arnService,   SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    connect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    _arnServicePv.open("/Sys/Discover/This/Service/value!");
    _arnService.addMode( ArnItem::Mode::Save);
    _arnService.open("/Sys/Discover/This/Service/value");
}


void  ArnDiscoverAdvertise::serviceTimeout()
{
    qDebug() << "First service setup timeout, using default.";

    firstServiceSetup("Arn Default Service");
}


void  ArnDiscoverAdvertise::firstServiceSetup( QString serviceName)
{
    qDebug() << "firstServiceSetup: serviceName=" << serviceName;

    _servTimer->stop();
    disconnect( &_arnService,   SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    disconnect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    connect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(serviceChanged(QString)));

    serviceChanged( serviceName);
}


void  ArnDiscoverAdvertise::serviceChanged( QString val)
{
    qDebug() << "Service changed: servname=" << val;

    if (_arnZCReg->state() != ArnZeroConfRegister::State::None)
        _arnZCReg->releaseService();
    _arnZCReg->setServiceName( val);
    _arnZCReg->registerService();
}


void  ArnDiscoverAdvertise::serviceRegistered( QString serviceName)
{
    qDebug() << "Service registered: serviceName=" << serviceName;

    _arnServicePv = serviceName;
}
