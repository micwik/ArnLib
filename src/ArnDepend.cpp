// Copyright (C) 2010-2014 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these
// other libraries is subject to their respective license agreements.
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

#include "ArnInc/ArnDepend.hpp"
#include "private/ArnDepend_p.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QUuid>
#include <QTimer>
#include <QtAlgorithms>
#include <QDebug>

const char*  ArnDependPath = "//.sys/Depend/";


///////////// Depend Offer

ArnDependOfferPrivate::ArnDependOfferPrivate()
{
}


ArnDependOfferPrivate::~ArnDependOfferPrivate()
{
}


ArnDependOffer::ArnDependOffer( QObject* parent)
    : QObject( parent)
    , d_ptr( new ArnDependOfferPrivate)
{
}


ArnDependOffer::ArnDependOffer( ArnDependOfferPrivate& dd, QObject* parent)
    : QObject( parent)
    , d_ptr( &dd)
{
}


ArnDependOffer::~ArnDependOffer()
{
    delete d_ptr;
}


void  ArnDependOffer::advertise( const QString& serviceName)
{
    Q_D(ArnDependOffer);

    d->_serviceName = serviceName;
    QString  basePath = QString( ArnDependPath) + serviceName + "/";

    d->_arnEchoPipeFB.setPipeMode().setMaster();
    d->_arnStateName.setMaster();
    d->_arnStateId.setMaster();
    d->_arnEchoPipeFB.open( basePath + "echoPipe!");
    d->_arnStateName.open(  basePath + "stateName");
    d->_arnStateId.open(    basePath + "stateId");

    d->_arnEchoPipeFB = "{}";
    d->_arnStateName  = "Start";
    d->_arnStateId    = 0;

    connect( &d->_arnEchoPipeFB, SIGNAL(changed(QByteArray)), this, SLOT(requestReceived(QByteArray)));
}


void  ArnDependOffer::setStateName( const QString& name)
{
    Q_D(ArnDependOffer);

    d->_arnStateName = name;
}


QString  ArnDependOffer::stateName()  const
{
    Q_D(const ArnDependOffer);

    return d->_arnStateName.toString();
}


void  ArnDependOffer::setStateId( int id)
{
    Q_D(ArnDependOffer);

    d->_arnStateId = id;
}


int  ArnDependOffer::stateId()  const
{
    Q_D(const ArnDependOffer);

    return d->_arnStateId.toInt();
}


void  ArnDependOffer::requestReceived( QByteArray req)
{
    Q_D(ArnDependOffer);

    if (Arn::debugDepend)  qDebug() << "DepOffer request: service=" << d->_serviceName << " req=" << req
                                    << "itemId=" << d->_arnEchoPipeFB.itemId();
    d->_arnEchoPipeFB = req;
}



///////////// Depend


ArnDependPrivate::ArnDependPrivate()
{
    QUuid  uuid = QUuid::createUuid();
    _uuid = uuid.toString();
    _started = false;
    _timerEchoRefresh = new QTimer;
    _timerEchoRefresh->setInterval(10000);
}


ArnDependPrivate::~ArnDependPrivate()
{
    qDeleteAll( _depTab);
    delete _timerEchoRefresh;
}


void ArnDepend::init()
{
    Q_D(ArnDepend);

    connect( d->_timerEchoRefresh, SIGNAL(timeout()), this, SLOT(echoRefresh()));
}


ArnDepend::ArnDepend( QObject* parent)
    : QObject( parent)
    , d_ptr( new ArnDependPrivate)
{
    init();
}


ArnDepend::ArnDepend( ArnDependPrivate& dd, QObject* parent)
    : QObject( parent)
    , d_ptr( &dd)
{
    init();
}


ArnDepend::~ArnDepend()
{
    delete d_ptr;
}


ArnDepend::DepSlot*  ArnDepend::setupDepSlot( const QString& serviceName)
{
    Q_D(ArnDepend);

    QString  basePath = QString( ArnDependPath) + serviceName + "/";
    DepSlot*  slot = new DepSlot;

    slot->arnEchoPipe.setPipeMode();
    slot->serviceName = serviceName;
    slot->arnEchoPipe.open(   basePath + "echoPipe");
    slot->arnStateName.open(  basePath + "stateName");
    slot->arnStateId.open(    basePath + "stateId");

    slot->arnEchoPipe.setReference( slot);
    slot->arnStateName.setReference( slot);
    slot->arnStateId.setReference( slot);

    d->_depTab += slot;
    return slot;
}


void  ArnDepend::add( const QString& serviceName, const QString& stateName)
{
    DepSlot*  slot = setupDepSlot( serviceName);
    slot->stateName = stateName;
    slot->useStateCheck = true;
}


void  ArnDepend::add( const QString& serviceName, int stateId)
{
    DepSlot*  slot = setupDepSlot( serviceName);
    slot->stateId = stateId;
    slot->useStateCheck = stateId >= 0;
}


void  ArnDepend::setMonitorName( const QString& name)
{
    Q_D(ArnDepend);

    d->_name = name;
}


void  ArnDepend::startMonitor()
{
    Q_D(ArnDepend);

    if (d->_started)  return;  // Already started

    // Connect & check all depency slots
    foreach( DepSlot* slot, d->_depTab) {
        connect( &slot->arnEchoPipe, SIGNAL(changed(QString)), this, SLOT(echoCheck(QString)));
        echoCheck("", slot);
    }
    d->_started = true;
}


void  ArnDepend::echoRefresh()
{
    Q_D(ArnDepend);

    ArnM::errorLog( QString(tr("Lost echo, doing refresh for DependEchoCheck monitor=")) + d->_name,
                        ArnError::Warning);
    foreach( DepSlot* slot, d->_depTab) {
        echoCheck("", slot);
    }
    d->_timerEchoRefresh->stop();
}


void  ArnDepend::echoCheck( const QString& echo, DepSlot* slot)
{
    Q_D(ArnDepend);

    if (Arn::debugDepend)  qDebug() << "echoCheck: monitorName=" << d->_name;
    if (slot == 0) {
        ArnItem* arnItem = qobject_cast<ArnItem*>( sender());
        if (arnItem)  slot = static_cast<DepSlot*>( arnItem->reference());
    }
    if (slot == 0) {
        ArnM::errorLog( QString(tr("Can't get slot for DependEchoCheck monitor=")) + d->_name,
                            ArnError::Undef);
        return;
    }
    if (slot->isEchoOk)  return;  // Echo already ok, test just in case ...

    if (echo.isEmpty()) {
        if (Arn::debugDepend)  qDebug() << "echoCheck: Send request monitorName=" << d->_name << " req=" << d->_uuid;
        slot->arnEchoPipe = d->_uuid;  // Dependency request
    }
    else {
        d->_timerEchoRefresh->start();  // Only when getting echo, to avoid fillup during disconnect
    }
    if (echo == d->_uuid) {  // Dependency echo ok
        slot->isEchoOk = true;
        disconnect( &slot->arnEchoPipe,   SIGNAL(changed(QString)), this, SLOT(echoCheck(QString)));  // Avoid more signals
        if (slot->useStateCheck) {  // Go on with state check
            connect( &slot->arnStateId,   SIGNAL(changed()), this, SLOT(stateCheck()));
            connect( &slot->arnStateName, SIGNAL(changed()), this, SLOT(stateCheck()));
            stateCheck( slot);
        }
        else {  // This dependency is ok
            doDepOk( slot);
        }
    }
}


void  ArnDepend::stateCheck( DepSlot* slot)
{
    Q_D(ArnDepend);

    if (slot == 0) {
        ArnItem* arnItem = qobject_cast<ArnItem*>( sender());
        if (arnItem)  slot = static_cast<DepSlot*>( arnItem->reference());
    }
    if (slot == 0) {
        ArnM::errorLog( QString(tr("Can't get slot for DependStateCheck monitor=")) + d->_name,
                            ArnError::Undef);
        return;
    }
    if (slot->isStateOk)  return;  // State already ok

    if (!slot->stateName.isEmpty() && (slot->arnStateName.toString() == slot->stateName)) {
        slot->isStateOk = true;
        doDepOk( slot);
    }
    else if ((slot->stateId > 0) && (slot->arnStateId.toInt() >= slot->stateId)) {
        slot->isStateOk = true;
        doDepOk( slot);
    }
}


void  ArnDepend::doDepOk( DepSlot* slot)
{
    Q_D(ArnDepend);

    if (Arn::debugDepend)  qDebug() << "depOk monitorName=" << d->_name;
    QMetaObject::invokeMethod( this,
                               "deleteSlot",
                               Qt::QueuedConnection,  // Delete later
                               Q_ARG( void*, slot));
}


void  ArnDepend::deleteSlot( void* slot_)
{
    Q_D(ArnDepend);

    DepSlot*  slot = reinterpret_cast<DepSlot*>( slot_);
    if (Arn::debugDepend)  qDebug() << "deleteSlot monitorName=" << d->_name;
    if (!d->_depTab.removeOne( slot)) {
        ArnM::errorLog( QString(tr("Can't get slot for delete Depend monitor=")) + d->_name,
                            ArnError::Undef);
        return;
    }
    delete slot;

    if (d->_depTab.isEmpty()) {  // All dependencys ok
        d->_timerEchoRefresh->stop();
        emit completed();
        d->_started = false;  // Ok to start again
    }
}
