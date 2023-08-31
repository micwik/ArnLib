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

#ifndef ARNSERVER_HPP
#define ARNSERVER_HPP

#include "ArnLib_global.hpp"
#include "Arn.hpp"
#include "MQFlags.hpp"
#include "XStringMap.hpp"
#include <QObject>
#include <QHostAddress>
#include <QMap>
#include <QStringList>

class ArnServerPrivate;
class ArnSync;
class ArnSyncLogin;
class ArnItemNetEar;
class ArnServer;
class QTcpServer;
class QSslSocket;


class ArnServerSession : public QObject
{
    Q_OBJECT
public:
    ArnServerSession( QSslSocket* socket, ArnServer* arnServer);

    QSslSocket*  socket()  const;
    Arn::XStringMap  remoteWhoIAm()  const;
    QString  loginUserName()  const;
    Arn::Allow  getAllow()  const;
    void  sendMessage( int type, const QByteArray& data = QByteArray());
    bool  getTraffic( quint64& in, quint64& out)  const;

signals:
    void  infoReceived( int type);
    void  loginCompleted();
    void  messageReceived( int type, const QByteArray& data);

private slots:
    void  shutdown();
    void  doDestroyArnTree( const QString& path, bool isGlobal);
    void  onCommandDelete( const QString& path);
    void  doSyncStateChanged( int state);

private:
    QSslSocket*  _socket;
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
    Q_DECLARE_PRIVATE(ArnServer)

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
    ArnServer( Type serverType, QObject *parent = arnNullptr);

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
     *  The password can be in clear text or a Hashed password which can be generated
     *  by ArnClient::passwordHash() (see also ArnBrowser Settings).
     *  \param[in] userName
     *  \param[in] password in clear text or Hashed
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

    //! Set the nets not demanding login
    /*! The net can be "localhost", "localnet", "any" or a subnet using syntax from
     *  QHostAddress::parseSubnet(). The "localnet" matches direct adresses on all of the
     *  available interfaces. The "any" will effectively turn off setDemandLogin().
     *  \param[in] noLoginNets is the list of no login nets,
     *                         e.g ("localhost" "192.168.1.0/255.255.255.0").
     *  \see noLoginNets()
     *  \see isDemandLoginNet()
     *  \see QHostAddress::parseSubnet()
     */
    void  setNoLoginNets( const QStringList& noLoginNets);

    //! Get the nets not demanding login
    /*! \return the nets not demanding login.
     *  \see setNoLoginNets()
     */
    QStringList  noLoginNets()  const;

    //! Return if a host address demands login
    /*! \param[in] remoteAddr is the tested host address.
     *  \retval false if the host address belongs to any net not demanding login
     *  \see setNoLoginNets()
     */
    bool  isDemandLoginNet( const QHostAddress& remoteAddr)  const;

    //! Set the server demand for encryption (policy)
    /*! Sets the policy for use of encryption from  server perspective.
     *  This must be used before connection is started.
     *  Default is Arn::EncryptPolicyPreferNo.
     *  \param[in] pol is the policy for encryption.
     */
    void  setEncryptPolicy( const Arn::EncryptPolicy& pol);

    //! Get the server demand for encryption (policy)
    /*! \return the policy for encryption.
     *  \see setEncryptPolicy()
     */
    Arn::EncryptPolicy  encryptPolicy()  const;

    //! Add a new "freePath"
    /*! A freePath can be used even if not logged in to an ArnServer that demands login.
     *  Also all children below freePath is free to use. Usage is restricted to read
     *  operations and alike from ArnServer to ArnClient.
     *  Setting a freePath at ArnServer gives the actual permision for read usage.
     *  All wanted freePaths must be added before ArnServer is started.
     *  \param[in] path is the freePath, eg "/Local/Sys/Legal/".
     *  \see freePaths()
     */
    void  addFreePath( const QString& path);

    //! Returns current list of freePaths.
    /*! The list of freePaths is used to give permision for read uasge of the paths.
     *  \return the freePath list.
     *  \see addFreePath()
     */
    QStringList  freePaths()  const;

    //! Set servers human readable identification information
    /*! This is used to identify the server.
     *  Standard keys to use are: Contact, Location, Description.
     *
     *  <b>Example usage</b> \n \code
     *  Arn::XStringMap  xsm;
     *  xsm.add("Contact",     "arn@arnas.se");
     *  xsm.add("Location",    "The Longhouse");
     *  xsm.add("Description", "Bring connection and integration to the people");
     *  _arnServer->setWhoIAm( xsm);
     *  \endcode
     *  \param[in] whoIAmXsm contains the information.
     *  \see remoteWhoIAm()
     */
    void  setWhoIAm( const Arn::XStringMap& whoIAmXsm);

    //! \cond ADV
    ArnSyncLogin*  arnLogin()  const;
    ArnServerSession*  getSession()  const;
    QByteArray  whoIAm()  const;

signals:
    void  newSession();

protected:
    ArnServer( ArnServerPrivate& dd, QObject* parent);
    ArnServerPrivate* const  d_ptr;
    //! \endcond

private slots:
    void tcpConnection();

private:
};

#endif // ARNSERVER_HPP
