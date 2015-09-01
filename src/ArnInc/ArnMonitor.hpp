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

#ifndef ARNMONITOR_HPP
#define ARNMONITOR_HPP


#include "ArnLib_global.hpp"
#include <QStringList>
#include <QObject>
#include <QPointer>

class ArnClient;
class ArnItemNet;


//! A client remote monitor to detect changes at server.
/*!
The monitor must normally be set at a [shared](\ref gen_shareArnobj) path. A none shared
path can be used when client is set to 0, i.e. local monitoring.

When the monitor is started, all the _arnChildFound_ signals are emmited for present
childs. Later the signals are emmited for newly created childs.

<b>Example usage</b> \n \code
    // In class declare
    ArnMonitor*  _arnMon;
    ArnClient*  _client;

    // In class code
    _arnMon = new ArnMonitor( this);
    _arnMon->start("//Pipes/", _client);
    connect( _arnMon, SIGNAL(arnChildFound(QString)), this, SLOT(netChildFound(QString)));
\endcode
*/
class ARNLIBSHARED_EXPORT ArnMonitor : public QObject
{
Q_OBJECT

public:
    explicit  ArnMonitor( QObject* parent = 0);

    //! Set the _client_ to be used
    /*! \param[in] client to be used. If 0, local monitoring is done.
     */
    void  setClient( ArnClient* client);

    //! Set the _client_ to be used by its id
    /*! \param[in] id to identify client. If "", local monitoring is done.
     */
    void  setClient( const QString& id);

    //! Get the id name of the used _client_
    /*! \return The _client_ id name
     *  \see setClient()
     */
    QString  clientId()  const;

    //! Get the used _client_
    /*! \return The _client_
     *  \see setClient()
     */
    ArnClient*  client()  const;

    //! Set the _path_ to be monitored
    /*! The monitor must be set at a [shared](\ref gen_shareArnobj) _path_ that is shared
     *  using client::addMountPoint().
     *  This function also starts the monitoring using start().
     *  \param[in] path
     *  \param[in] client to be used. If 0, keep previous set client.
     *  \see start()
     *  \deprecated Use start() instead, _client_ parameter is changed.
     */
    void  setMonitorPath( const QString& path, ArnClient* client = 0);

    //! Starts the monitoring
    /*! The monitor must normally be set at a [shared](\ref gen_shareArnobj) _path_ that is
     *  shared using client::addMountPoint(). A none shared path can be used when client is
     *  set to 0, i.e. local monitoring.
     *  \param[in] path
     *  \param[in] client to be used. If 0, local monitoring is done.
     */
    bool  start( const QString& path, ArnClient* client);

    //! Starts the monitoring
    /*! The monitor must normally be set at a [shared](\ref gen_shareArnobj) _path_ that is
     *  shared using client::addMountPoint(). A none shared path can be used when client is
     *  set to 0, i.e. local monitoring.
     *  \param[in] path
     */
    bool  start( const QString& path)
    { return start( path, client());}

    //! Get the monitored _path_
    /*! \return The _path_
     *  \see start()
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
     *  Both created folder and leaf objects will give this signal.
     *  \param[in] path to the created _Arn Data Object_
     */
    void  arnItemCreated( const QString& path);

    //! Signal emitted for present and newly created childs in the monitor folder
    /*! The ArnMonitor monitors a folder. Present and newly created objects in this
     *  folder will give this signal.
     *  For newly created objects, the origin comes from the arnItemCreated() signal.
     *
     *  Example 1: monitorPath = "//Sensors/", created object = "//Sensors/Temp1/value"
     *  ==> path to child = "//Sensors/Temp1/"
     *
     *  Example 2: monitorPath = "//Sensors/", created object = "//Sensors/Temp2/folder/"
     *  ==> path to child = "//Sensors/Temp2/"
     *  \param[in] path to the child
     *  \see arnItemCreated()
     */
    void  arnChildFound( const QString& path);

    //! Signal emitted for present and newly created folder childs in the monitor folder
    /*! The ArnMonitor monitors a folder. Present and newly created folder objects in this
     *  folder will give this signal.
     *  For newly created childs, the origin comes from the arnItemCreated() signal.
     *
     *  Example: monitorPath = "//Sensors/", created object = "//Sensors/Temp1/value"
     *  ==> path to child = "//Sensors/Temp1/"
     *  \param[in] path to the child
     *  \see arnItemCreated()
     *  \see arnChildFound()
     */
    void  arnChildFoundFolder( const QString& path);

    //! Signal emitted for present and newly created leaf childs in the monitor folder
    /*! The ArnMonitor monitors a folder. Present and newly created leaf objects in this
     *  folder will give this signal.
     *  For newly created childs, the origin comes from the arnItemCreated() signal.
     *
     *  Example: monitorPath = "//Sensors/", created object = "//Sensors/count"
     *  ==> path to child = "//Sensors/count"
     *  \param[in] path to the child
     *  \see arnChildFound()
     */
    void  arnChildFoundLeaf( const QString& path);

    //! Signal emitted when an _Arn Data Object_ is deleted in the tree below.
    /*! The ArnMonitor monitors a folder. Deleted objects in this folder or its
     *  children below will give this signal.
     *  Both deleted folder and leaf objects will give this signal.
     *  \param[in] path to the deleted _Arn Data Object_
     */
    void  arnItemDeleted( const QString& path);

    //! Signal emitted for deleted childs in the monitor folder
    /*! The ArnMonitor monitors a folder. Deleted objects in this folder will give
     *  this signal.
     *
     *  Example 1: monitorPath = "//Sensors/Temp1/", deleted object = "//Sensors/Temp1/value"
     *  ==> path to child = "//Sensors/Temp1/value"
     *
     *  Example 2: monitorPath = "//Sensors/Temp2/", deleted object = "//Sensors/Temp2/folder/"
     *  ==> path to child = "//Sensors/Temp2/folder/"
     *  \param[in] path to the child
     *  \see arnItemDeleted()
     */
    void  arnChildDeleted( const QString& path);

public slots:
    //! Help telling the monitor about deletion of a previous found child
    /*! The monitor remembers every child it has signalled. If a deleted child
     *  reappears later it will not give a signal unless this function is used.
     *
     *  Since ArnLib 3.0 this function is called automatically when a child is deleted.
     *  This function is still available to manually handle any problems.
     *  \param[in] path to the deleted child
     */
    void  foundChildDeleted( const QString& path);

protected:
    virtual QString  outPathConvert( const QString& path);
    virtual QString  inPathConvert( const QString& path);

    QPointer<ArnClient>  _arnClient;
    QString  _monitorPath;

private slots:
    void  dispatchArnMonEvent( int type, const QByteArray& data, bool isLocal);
    void  emitArnMonEvent( int type, const QByteArray& data = QByteArray());
    void  setupLocalMonitorItem();

private:
    void  doEventItemFoundCreated( ArnItemNet* itemNet, int type, const QByteArray& data, bool isLocal);
    void  doEventItemDeleted( ArnItemNet* itemNet, const QByteArray& data, bool isLocal);

    QStringList  _foundChilds;
    ArnItemNet*  _itemNet;
    void*  _reference;
};


#endif // ARNMONITOR_HPP
