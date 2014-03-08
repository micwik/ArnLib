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

#include "ArnInc/ArnDepend.hpp"
#include "ArnInc/ArnM.hpp"
#include <QUuid>
#include <QTimer>
#include <QtAlgorithms>
#include <QDebug>

const char*  ArnDependPath = "//.sys/Depend/";


///////////// Depend Offer

ArnDependOffer::ArnDependOffer( QObject* parent) :
    QObject( parent)
{
}


void  ArnDependOffer::advertise( QString serviceName)
{
    _serviceName = serviceName;
    QString  basePath = QString( ArnDependPath) + serviceName + "/";

    _arnEchoPipeFB.setPipeMode().setMaster();
    _arnStateName.setMaster();
    _arnStateId.setMaster();
    _arnEchoPipeFB.open( basePath + "echoPipe!");
    _arnStateName.open(  basePath + "stateName");
    _arnStateId.open(    basePath + "stateId");

    _arnEchoPipeFB = "{}";
    _arnStateName  = "Start";
    _arnStateId    = 0;

    connect( &_arnEchoPipeFB, SIGNAL(changed(QString)), this, SLOT(requestReceived(QString)));
}


void  ArnDependOffer::setStateName( const QString& name)
{
    _arnStateName = name;
}


QString  ArnDependOffer::stateName()  const
{
    return _arnStateName.toString();
}


void  ArnDependOffer::setStateId( int id)
{
    _arnStateId = id;
}


int  ArnDependOffer::stateId()  const
{
    return _arnStateId.toInt();
}


void  ArnDependOffer::requestReceived( QString req)
{
    // qDebug() << "DepOffer request: service=" << _serviceName << " req=" << req;
    _arnEchoPipeFB = req;
}


///////////// Depend

//! \cond ADV
struct ArnDependSlot
{
    QString  serviceName;
    QString  stateName;
    int  stateId;
    bool  useStateCheck;
    bool  isEchoOk;
    bool  isStateOk;
    ArnItem  arnEchoPipe;
    ArnItem  arnStateName;
    ArnItem  arnStateId;
    ArnDependSlot() {
        stateId       = -1;
        useStateCheck = false;
        isEchoOk      = false;
        isStateOk     = false;
    }
};
//! \endcond


ArnDepend::ArnDepend( QObject* parent) :
    QObject( parent)
{
    QUuid  uuid = QUuid::createUuid();
    _uuid = uuid.toString();
    _started = false;
    _timerEchoRefresh = new QTimer( this);
    _timerEchoRefresh->setInterval(10000);
    connect( _timerEchoRefresh, SIGNAL(timeout()), this, SLOT(echoRefresh()));
}


ArnDepend::~ArnDepend()
{
    qDeleteAll( _depTab);
}


ArnDepend::DepSlot*  ArnDepend::setupDepSlot( QString serviceName)
{
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

    _depTab += slot;
    return slot;
}


void  ArnDepend::add( QString serviceName, QString stateName)
{
    DepSlot*  slot = setupDepSlot( serviceName);
    slot->stateName = stateName;
    slot->useStateCheck = true;
}


void  ArnDepend::add( QString serviceName, int stateId)
{
    DepSlot*  slot = setupDepSlot( serviceName);
    slot->stateId = stateId;
    slot->useStateCheck = stateId >= 0;
}


void  ArnDepend::setMonitorName( QString name)
{
    _name = name;
}


void  ArnDepend::startMonitor()
{
    if (_started)  return;  // Already started

    // Connect & check all depency slots
    foreach( DepSlot* slot, _depTab) {
        connect( &slot->arnEchoPipe, SIGNAL(changed(QString)), this, SLOT(echoCheck(QString)));
        echoCheck("", slot);
    }
    _started = true;
}


void  ArnDepend::echoRefresh()
{
    ArnM::errorLog( QString(tr("Lost echo, doing refresh for DependEchoCheck monitor=")) + _name,
                        ArnError::Warning);
    foreach( DepSlot* slot, _depTab) {
        echoCheck("", slot);
    }
    _timerEchoRefresh->stop();
}


void  ArnDepend::echoCheck( QString echo, DepSlot* slot)
{
    // qDebug() << "echoCheck: monitorName=" << _name;
    if (slot == 0) {
        ArnItem* arnItem = qobject_cast<ArnItem*>( sender());
        if (arnItem)  slot = static_cast<DepSlot*>( arnItem->reference());
    }
    if (slot == 0) {
        ArnM::errorLog( QString(tr("Can't get slot for DependEchoCheck monitor=")) + _name,
                            ArnError::Undef);
        return;
    }
    if (slot->isEchoOk)  return;  // Echo already ok, test just in case ...

    if (echo.isEmpty()) {
        // qDebug() << "echoCheck: Send request monitorName=" << _name << " req=" << _uuid;
        slot->arnEchoPipe = _uuid;  // Dependency request
    }
    else {
        _timerEchoRefresh->start();  // Only when getting echo, to avoid fillup during disconnect
    }
    if (echo == _uuid) {  // Dependency echo ok
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
    if (slot == 0) {
        ArnItem* arnItem = qobject_cast<ArnItem*>( sender());
        if (arnItem)  slot = static_cast<DepSlot*>( arnItem->reference());
    }
    if (slot == 0) {
        ArnM::errorLog( QString(tr("Can't get slot for DependStateCheck monitor=")) + _name,
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
    // qDebug() << "depOk monitorName=" << _name;
    QMetaObject::invokeMethod( this,
                               "deleteSlot",
                               Qt::QueuedConnection,  // Delete later
                               Q_ARG( void*, slot));
}


void  ArnDepend::deleteSlot( void* slot_)
{
    DepSlot*  slot = reinterpret_cast<DepSlot*>( slot_);
    // qDebug() << "deleteSlot monitorName=" << _name;
    if (!_depTab.removeOne( slot)) {
        ArnM::errorLog( QString(tr("Can't get slot for delete Depend monitor=")) + _name,
                            ArnError::Undef);
        return;
    }
    delete slot;

    if (_depTab.isEmpty()) {  // All dependencys ok
        _timerEchoRefresh->stop();
        emit completed();
        _started = false;  // Ok to start again
    }
}
