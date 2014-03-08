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

//! Class for advertising that a _service_ is available.
/*!
Additionally it's possible to indicate the _state_ of the _service_.
The _state_ can either be indicated by a logic name or by an id number whichever
is prefered.

<b>Example usage</b> \n \code
    // In class declare
    ArnDependOffer* _depOffer;

    // In class code
    _depOffer = new ArnDependOffer( this);
    _depOffer->advertise("MyService");  // Service now available
\endcode
*/
class ARNLIBSHARED_EXPORT ArnDependOffer : public QObject
{
    Q_OBJECT
public:
    explicit ArnDependOffer( QObject* parent = 0);

    //! Advertise an available _service_
    /*! \param[in] serviceName is the name of the _service_.
     */
    void  advertise( QString serviceName);

    //! Set the _state_ of the _service_ by a logic name.
    /*! The _state_ starts of by "Start" as default.
     *  \param[in] name is the _state_ name.
     */
    void  setStateName( const QString& name);

    /*! \return The logic _state_ name, e.g. the default "Start"
     *  \see setStateName()
     */
    QString  stateName() const;

    //! Set the _state_ of the _service_ by an id number.
    /*! The _state_ starts of by 0 as default.
     *  \param[in] id is the _state_ id number.
     */
    void  setStateId( int id);

    /*! \return The _state_ id number.
     *  \see setStateId()
     */
    int  stateId() const;

private slots:
    void  requestReceived( QString req);

private:
    QString  _serviceName;
    ArnItem  _arnEchoPipeFB;
    ArnItem  _arnStateName;
    ArnItem  _arnStateId;
};


//! Class for setting up dependencis to needed services.
/*!
The services can be both system types available by internal Arn,
and custom application types. The system types have a service name starting with "$".

This is typically used when an application needs a service to continue. When using
persistent values, a client will need to know when they have been synced from the
server. Then it's convenient to setup a dependency for the system service "$Persist".

When all dependent services are available, the completed() signal is emitted.

<b>Example usage</b> \n \code
    // In class declare
    ArnDepend*  _arnDepend;

    // In class code
    _arnDepend = new ArnDepend( this);
    _arnDepend->setMonitorName("MyApp_Monitor");  // Optional for debug
    _arnDepend->add("$Persist");
    _arnDepend->add("MyService");
    _arnDepend->startMonitor();
    connect( _arnDepend, SIGNAL(completed()), this, SLOT(arnDependOk()));
\endcode
*/
class ARNLIBSHARED_EXPORT ArnDepend : public QObject
{
    Q_OBJECT
public:
    typedef ArnDependSlot DepSlot;

    explicit ArnDepend( QObject* parent = 0);
    ~ArnDepend();

    //! Add a dependency for a _service_
    /*! \param[in] serviceName is the name of the needed _service_.
     *  \param[in] stateId is the needed _state_ id number. -1 is don't care.
     */
    void  add( QString serviceName, int stateId = -1);

    //! Add a dependency for a _service_
    /*! \param[in] serviceName is the name of the needed _service_.
     *  \param[in] stateName is the needed _state_ name.
     */
    void  add( QString serviceName, QString stateName);

    //! Set an optional monitor name for debugging
    /*! \param[in] name is the monitor name.
     */
    void  setMonitorName( QString name);

    //! Starting the dependency monitor
    void  startMonitor();

signals:
    //! Signal emitted when all dependent services are available.
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
