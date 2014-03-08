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

#ifndef ARNMONITOR_HPP
#define ARNMONITOR_HPP


#include "ArnLib_global.hpp"
#include <QStringList>
#include <QObject>

class ArnClient;
class ArnItemNet;


//! A client remote monitor to detect changes at server.
/*!
The monitor must be set at a [shared](\ref gen_shareArnobj) path.

When the monitor is started, all the _arnChildFound_ signals are emmited for present
childs. Later the signals are emmited for newly created childs.

<b>Example usage</b> \n \code
    // In class declare
    ArnMonitor*  _arnMon;
    ArnClient*  _client;

    // In class code
    _arnMon = new ArnMonitor( this);
    _arnMon->setMonitorPath("//Pipes/", _client);
    connect( _arnMon, SIGNAL(arnChildFound(QString)), this, SLOT(netChildFound(QString)));
\endcode
*/
class ARNLIBSHARED_EXPORT ArnMonitor : public QObject
{
Q_OBJECT

public:
    explicit  ArnMonitor( QObject* parent = 0);

    //! Set the _client_ to be used
    /*! \param[in] client
     *  \param[in] id is an optional name to assign to the client.
     */
    void  setClient( ArnClient* client, QString id = QString());

    //! Get the id name of the used _client_
    /*! \return The _client_ id name
     *  \see setClient()
     */
    QString  clientId()  const;

    //! Set the _path_ to be monitored
    /*! The monitor must be set at a [shared](\ref gen_shareArnobj) _path_.
     *  This function also starts the monitoring.
     *  \param[in] path
     *  \param[in] client to be used. If 0, keep previous set client.
     */
    void  setMonitorPath( QString path, ArnClient* client = 0);

    //! Get the monitored _path_
    /*! \return The _path_
     *  \see setMonitorPath()
     */
    QString  monitorPath()  const {return _monitorPath;}

    //! The monitor is restarted
    /*! This makes the monitor forget the signals sent for present children and
     *  the _arnChildFound_ signals are emmited again for present childs.
     */
    void  reStart();

    //! Set an associated external reference
    /*! This is typically used when having many _ArnMonitors_ signal connected to a
     *  common slot. The slot can then discover the signalling ArnMonitor:s associated
     *  structure for further processing.
     *  \param[in] reference Any external structure or id.
     *  \see reference()
     */
    void  setReference( void* reference)  {_reference = reference;}

    //! Get the stored external reference
    /*! \return The associated external reference
     *  \see setReference()
     */
    void*  reference()  const {return _reference;}

signals:
    //! Signal emitted when an _Arn Data Object_ is created in the tree below.
    /*! The ArnMonitor monitors a folder. Created objects in this folder or its
     *  children below will give this signal.
     *  Only created non folder objects will give this signal.
     *  \param[in] path to the created _Arn Data Object_
     */
    void  arnItemCreated( QString path);

    //! Signal emitted for present and newly created childs in the monitor folder
    /*! The ArnMonitor monitors a folder. Present and newly created objects in this
     *  folder will give this signal.
     *  For newly created childs, the origin comes from the arnItemCreated() signal,
     *  so only non folder objects will then give this signal.
     *
     *  Example: monitorPath = "//Sensors/", created object = "//Sensors/Temp1/value"
     *  ==> path to child = "//Sensors/Temp1/"
     *  \param[in] path to the child
     *  \see arnItemCreated()
     */
    void  arnChildFound( QString path);

    //! Signal emitted for present and newly created folder childs in the monitor folder
    /*! The ArnMonitor monitors a folder. Present and newly created folder objects in this
     *  folder will give this signal.
     *  For newly created childs, the origin comes from the arnItemCreated() signal,
     *  so only non folder objects will then give this signal.
     *
     *  Example: monitorPath = "//Sensors/", created object = "//Sensors/Temp1/value"
     *  ==> path to child = "//Sensors/Temp1/"
     *  \param[in] path to the child
     *  \see arnItemCreated()
     *  \see arnChildFound()
     */
    void  arnChildFoundFolder( QString path);

    //! Signal emitted for present and newly created leaf childs in the monitor folder
    /*! The ArnMonitor monitors a folder. Present and newly created leaf objects in this
     *  folder will give this signal.
     *
     *  Example: monitorPath = "//Sensors/", created object = "//Sensors/count"
     *  ==> path to child = "//Sensors/count"
     *  \param[in] path to the child
     *  \see arnChildFound()
     */
    void  arnChildFoundLeaf( QString path);

public slots:
    //! Help telling the monitor about deletion of a previous found child
    /*! The monitor remembers every child it has signalled. If a deleted child
     *  reappears later it will not give a signal unless this function is used.
     *  \param[in] path to the deleted child
     */
    void  foundChildDeleted( QString path);

protected:
    ArnClient*  _arnClient;
    QString  _monitorPath;

private:
    QStringList  _foundChilds;
    ArnItemNet*  _itemNet;
    void*  _reference;

private slots:
    void  dispatchArnEvent( QByteArray type, QByteArray data, bool isLocal);
    void  emitArnEvent( QByteArray type, QByteArray data = QByteArray());
};


#endif // ARNMONITOR_HPP
