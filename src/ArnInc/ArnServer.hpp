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

#ifndef ARNSERVER_HPP
#define ARNSERVER_HPP

#include "ArnLib_global.hpp"
#include "Arn.hpp"
#include "MQFlags.hpp"
#include "XStringMap.hpp"
#include <QObject>
#include <QHostAddress>
#include <QMap>

class ArnSync;
class ArnSyncLogin;
class ArnItemNetEar;
class ArnServer;
class QTcpServer;
class QTcpSocket;


class ArnServerNetSync : public QObject
{
    Q_OBJECT
public:
    ArnServerNetSync( QTcpSocket* socket, ArnServer* arnServer);

signals:

private slots:
    void  shutdown();
    void  doDestroyArnTree( const QString& path, bool isGlobal);
    void  onCommandDelete( const QString& path);
    void  doSyncStateChanged( int state);

private:
    ArnServer*  _arnServer;
    ArnSync*  _arnNetSync;
    ArnItemNetEar*  _arnNetEar;
};


//! Class for making an _Arn Server_.
/*!
[About Sharing Arn Data Objects](\ref gen_shareArnobj)

<b>Example usage</b> \n \code
    // In class declare
    ArnServer*  _server;

    // In class code
    _server = new ArnServer( ArnServer::Type::NetSync, this);
    _server->start();
\endcode
*/
class ARNLIBSHARED_EXPORT ArnServer : public QObject
{
    Q_OBJECT
public:
    struct Type {
        enum E {
            NetSync
        };
        MQ_DECLARE_ENUM( Type)
    };

    //! Create an Arn _server_ object
    /*! \param[in] serverType For now only _NetSync_ is available.
     *  \param[in] parent
     */
    ArnServer( Type serverType, QObject *parent = 0);

    ~ArnServer();

    //! Start the Arn _server_
    /*! \param[in] port is the server port,
     *                  -1 gives Arn::defaultTcpPort, 0 gives [dynamic port](\ref gen_dynamicPort)
     *  \param[in] listenAddr is the interface address to listen for connections (default any)
     */
    void  start( int port = -1, QHostAddress listenAddr = QHostAddress::Any);

    //! Port number of the Arn _server_
    /*! \retval is the port number.
     */
    int  port();

    //! Address of the interface used to listening for connections to the Arn _server_
    /*! \retval is the address (which usually is QHostAddress::Any).
     *  \see start()
     */
    QHostAddress  listenAddress();

    //! Add an access entry
    /*! This adds an entry to build an access table for the server. This access table
     *  restricts the operations of connected clients. Each client refer to one entry
     *  by its userName.
     *  Each entry must have a unique userName. Any equal userName is making the entry
     *  being replaced by the last added one.
     *  \param[in] userName
     *  \param[in] password
     *  \param[in] allow have flags defining allowed basic operations (write, delete ...)
     */
    void  addAccess( const QString& userName, const QString& password, Arn::Allow allow);

    //! Get servers demand for login
    /*! If any of server or client demand login, it must be used.
     *  \retval true if server demand login.
     *  \see setDemandLogin()
     */
    bool  isDemandLogin()  const;

    //! Set servers demand for login
    /*! If any of server or client demand login, it must be used.
     *  \param[in] isDemandLogin true if server demand login.
     *  \see isDemandLogin()
     */
    void  setDemandLogin( bool isDemandLogin);

    //! Add a new "freePath"
    /*! A freePath can be used even if not logged in to an ArnServer that demands login.
     *  Also all children below freePath is free to use. Usage is restricted to read
     *  operations and alike from ArnServer to ArnClient.
     *  Setting a freePath at ArnServer gives the actual permision for read usage.
     *  All wanted freePaths must be added before ArnServer is started.
     *  \param[in] path is the freePath, eg "/Local/Sys/Licenses/".
     *  \see freePaths()
     */
    void  addFreePath( const QString& path);

    //! Returns current list of freePaths.
    /*! The list of freePaths is used to give permision for read uasge of the paths.
     *  \return the freePath list.
     *  \see addFreePath()
     */
    QStringList  freePaths()  const;

    //! \cond ADV
    ArnSyncLogin*  arnLogin()  const;
    //! \endcond


private:
    QTcpServer*  _tcpServer;
    ArnSyncLogin*  _arnLogin;
    QStringList  _freePathTab;
    bool  _tcpServerActive;
    Type  _serverType;
    bool  _isDemandLogin;

private slots:
    void tcpConnection();
};

#endif // ARNSERVER_HPP
