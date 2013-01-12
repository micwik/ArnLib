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

#ifndef ARNDEPEND_HPP
#define ARNDEPEND_HPP

#include "ArnLib_global.hpp"
#include "ArnError.hpp"
#include "ArnItem.hpp"
#include <QList>
#include <QString>
#include <QObject>

class QTimer;
struct ArnDependSlot;


class ARNLIBSHARED_EXPORT ArnDependOffer : public QObject
{
    Q_OBJECT
public:
    explicit ArnDependOffer( QObject* parent = 0);
    void  advertise( QString serviceName);
    void  setStateName( const QString& name);
    QString  stateName() const;
    void  setStateId( int id);
    int  stateId() const;

private slots:
    void  requestReceived( QString req);

private:
    QString  _serviceName;
    ArnItem  _arnEchoPipeFB;
    ArnItem  _arnStateName;
    ArnItem  _arnStateId;
};


class ARNLIBSHARED_EXPORT ArnDepend : public QObject
{
    Q_OBJECT
public:
    typedef ArnDependSlot DepSlot;

    explicit ArnDepend( QObject* parent = 0);
    ~ArnDepend();
    void  add( QString serviceName, int stateId = -1);
    void  add( QString serviceName, QString stateName);
    void  setMonitorName( QString name);
    void  startMonitor();

signals:
    void  completed();

private slots:
    void  echoRefresh();
    void  echoCheck( QString echo, DepSlot* slot = 0);
    void  stateCheck( DepSlot* slot = 0);
    void  deleteSlot( void* slot = 0);

private:
    DepSlot*  setupDepSlot( QString name);
    void  doDepOk( DepSlot* slot);

    QList<DepSlot*>  _depTab;
    QString  _uuid;
    QString  _name;
    bool  _started;
    QTimer*  _timerEchoRefresh;
};

#endif // ARNDEPEND_HPP
