// Copyright (C) 2010-2015 Michael Wiklund.
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

#ifndef ARNCLIENT_HPP
#define ARNCLIENT_HPP

#include "ArnLib_global.hpp"
#include "ArnM.hpp"
#include "XStringMap.hpp"
#include "MQFlags.hpp"
#include <QObject>
#include <QAbstractSocket>
#include <QStringList>
#include <QList>

class ArnClientPrivate;
class ArnItemNet;
class ArnItemNetEar;


namespace ArnPrivate {
class ConnectStat {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        //! Initialized, not yet any result of trying to connect ...
        Init = 0,
        //! Trying to connect to an Arn host
        Connecting,
        //! Negotiating terms and compatibility with an Arn host
        Negotiating,
        //! Successfully connected to an Arn host
        Connected,
        //! No data flow within set timeout (still connected)
        Stopped,
        //! Unsuccessfull when trying to connect to an Arn host
        Error,
        //! TCP connection is broken (has been successfull)
        Disconnected,
        //! Unsuccessfully tried to connect to all hosts in the Arn connection List
        TriedAll
    };
    MQ_DECLARE_ENUMTXT( ConnectStat)

    enum NS {NsEnum, NsHuman};
    MQ_DECLARE_ENUM_NSTXT(
        { NsHuman, Init,     "Initialized" },
        { NsHuman, Error,    "Connect error" },
        { NsHuman, TriedAll, "Tried all" },
        { NsHuman, MQ_NSTXT_FILL_MISSING_FROM( NsEnum) }
    )
};
}


//! Class for connecting to an _Arn Server_.
/*!
[About Sharing Arn Data Objects](\ref gen_shareArnobj)

Connection can be made to a specific Host by connectToArn(). It's also possible to define
an _Arn Connection List_. Each host address is added to the list with a priority. The
priority is used to control the order at which the host addresses will be tried for
connection. Lowest priority number is tried first. Connection trials are started with
connectToArnlList(). The priority can also be used for selction in clearArnList() and
arnList().

<b>Example usage</b> \n \code
    // In class declare
    ArnClient  _arnClient;

    // In class code
    _arnClient.connectToArn("localhost");
    _arnClient.addMountPoint("//");
    _arnClient.setAutoConnect( true);
\endcode
*/
class ARNLIBSHARED_EXPORT ArnClient : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnClient)

public:
    typedef ArnPrivate::ConnectStat  ConnectStat;

    struct HostAddrPort {
        QString  addr;
        quint16  port;

        HostAddrPort() {
            port = Arn::defaultTcpPort;
        }
    };
    typedef QList<HostAddrPort>  HostList;

    explicit ArnClient(QObject *parent = 0);
    ~ArnClient();

    //! Clear the Arn connection list
    /*! Typically used to start making a new Arn connection list.
     *  \param[in] prioFilter selects hosts in the list with this pri, to be removed. Default -1 removes all.
     *  \see addToArnList()
     */
    void  clearArnList( int prioFilter = -1);

    //! Return the Arn connection list
    /*! \param[in] prioFilter selects hosts in the list with this pri. Default -1 selects all.
     *  \retval the selected Arn connection list.
     *  \see addToArnList()
     */
    HostList  arnList( int prioFilter = -1)  const;

    //! Add an _Arn Server_ to the Arn connection list
    /*! \param[in] arnHost is host name or ip address, e.g. "192.168.1.1".
     *  \param[in] port is the host port, 0 gives Arn::defaultTcpPort.
     *  \param[in] prio gives the sorting (connection) order and can be used for selection filter.
     *  \see clearArnList()
     *  \see arnList()
     *  \see Arn::makeHostWithInfo()
     */
    void  addToArnList( const QString& arnHost, quint16 port = 0, int prio = 0);

    //! Connect to an _Arn Server_ in the Arn connection list
    /*! Will scan the connection list once until a successful connection is made.
     *  If the end of the list is reached without connection, the tcpError() signal
     *  \see connectToArn()
     */
    void  connectToArnList();

    //! Connect to an _Arn Server_
    /*! \param[in] arnHost is host name or ip address, e.g. "192.168.1.1".
     *  \param[in] port is the host port, 0 gives Arn::defaultTcpPort.
     *  \see Arn::makeHostWithInfo()
     *  \see connectToArnList()
     */
    void  connectToArn( const QString& arnHost, quint16 port = 0);

    //! Disconnect from an _Arn Server_
    /*! Force disconnect from the _Arn server_, similar behaviour to losing connection.
     *  All pending data is written before disconnect. All _Arn objects_ that has been
     *  setup to be synronized is still kept. This implies that it's possible to continue
     *  previous session by just connecting to the _Arn server_ again.
     *
     *  Auto connection is also disabled.
     *  \see close()
     *  \see setAutoConnect()
     *  \see connectToArn()
     */
    void  disconnectFromArn();

    //! Login to an _Arn Server_
    /*! This routine must be called when the signal loginRequired() is emitted.
     *  Otherwise the client will not be fully conected to the server, ie the apropriate
     *  access privileges will not be setup at server and client.
     *  If a reconnect occurs, usually due to tcp breakage, login process is handled
     *  automatically by ArnLib using the last used login credentials. If this automatical
     *  login is failed, signal loginRequired() is emitted.
     *  \param[in] userName
     *  \param[in] password
     *  \param[in] allow is the permissions for the server actions to this client.
     *  \see Arn::Allow
     *  \see loginRequired;
     *  \see loginToArnHashed()
     */
    void  loginToArn( const QString& userName, const QString& password,
                      Arn::Allow allow = Arn::Allow::All);

    //! Login to an _Arn Server_ using hashed password
    /*! This behaves exactly as loginToArn(), exept for password being hashed.
     *  The hashed password which can be generated by ArnClient::passwordHash()
     *  (see also ArnBrowser Settings).
     *  \param[in] userName
     *  \param[in] passwordHashed
     *  \param[in] allow is the permissions for the server actions to this client.
     *  \see loginToArn()
     *  \see Arn::Allow
     *  \see loginRequired;
     */
    void  loginToArnHashed( const QString& userName, const QString& passwordHashed,
                            Arn::Allow allow = Arn::Allow::All);

    //! Close sharing with an _Arn Server_
    /*! Stop sharing _Arn objects_ with the _Arn server_. Similar to disconnectFromArn().
     *  All pending data is written before disconnect. No syncronized _Arn objects_ are
     *  remembered. This implies that it's not possible to continue previous session.
     *  This function is aimed at later starting a new session from scratch.
     *
     *  Auto connection is also disabled.
     *  \see disconnectFromArn()
     *  \see setAutoConnect()
     *  \see connectToArn()
     */
    void  close();

    //! Set the sharing tree path
    /*! For campatibility, this can only set one mount point and with same local as remote
     *  path. If exactly one mount point exist, it will be removed before this new one is
     *  added.
     *  \param[in] path is the sharing tree.
     *  \retval false if error.
     *  \see \ref gen_shareArnobj
     *  \deprecated Use addMountPoint() and removeMountPoint()
     */
    bool  setMountPoint( const QString& path);

    //! Add a sharing tree path
    /*! Mountpoint is an association to the similarity of mounting a "remote filesystem".
     *  In Arn, the remote "file system" can be at different sub path than the local
     *  mountpoint, e.g. a client having mountpoint local="/a/b/" remote="/r/" and opening
     *  an  _Arn Data Object_ at "/a/b/c" will have the object _c_ shared with the server
     *  at its path "/r/c".
     *  However if _remotePath_ is not specified, it will be same as _localPath_. In the
     *  above example, the _c_ object will then be shared with the server at its path
     *  "/a/b/c".
     *  \param[in] localPath is the local sharing tree.
     *  \param[in] remotePath is the remote sharing tree. If empty, same as _localPath_.
     *  \retval false if error.
     *  \see \ref gen_shareArnobj
     */
    bool  addMountPoint( const QString& localPath, const QString& remotePath = QString());

    //! Remove a sharing tree path
    /*! Only the mount point will be removed, i.e any new _Arn Data Objects_ created within
     *  the _localPath_ tree will not be shared with the server. However already existing
     *  objects will not be affected and is still shared with the server.
     *  \param[in] localPath is the sharing tree to be removed. Only affects newly created
     *                       objects.
     *  \retval false if error.
     *  \see \ref gen_shareArnobj
     */
    bool  removeMountPoint( const QString& localPath);

    //! Return the Arn connection status
    /*! \retval the Arn connection status.
     */
    ConnectStat  connectStatus()  const;

    //! Set automatic reconnect
    /*! If connectToArnList() is used, this auto connect funtionality starts every time
     *  after the last host in the Arn connection list has failed. The connection list is
     *  retried after _retryTime_.
     *  When using connectToArn(), there will be a _retryTime_ delay between each reConnect
     *  to the host.
     *  \param[in] isAuto true if using auto reconnect
     *  \param[in] retryTime is the time between attempts in seconds
     */
    void  setAutoConnect( bool isAuto, int retryTime = 2);

    //! Register this client to be avaiable with id
    /*! When instantiating an ArnClient, it's always registered as id = "std", if that's
     *  not taken by another client.
     *
     *  Any previous registration of id for this client will be released when using
     *  registerClient().
     *  \param[in] id must not be "".
     *  \see getClient()
     *  \see id()
     */
    void  registerClient( const QString& id);

    //! Get a client by its id
    /*! \param[in] id if "" will always return 0.
     *  \return the found client, 0 = not found or id == ""
     *  \see registerClient()
     */
    static ArnClient*  getClient( const QString& id);

    //! Get the id of this client
    /*! \return the id, "" = none (local)
     *  \see registerClient()
     */
    QString  id()  const;

    //! Get receive data timeout (base time)
    /*! \return the timeout in seconds
     *  \see setReceiveTimeout()
     */
    int  receiveTimeout()  const;

    //! Set receive data timeout (base time)
    /*! The timeout deals with no received data. This base time T is used as follows:
     *  time passed == T/2, send a dummy request to ArnServer
     *  time passed == T, signal status ConnectStat::Stopped
     *  time passed == 3*T, abort ArnClient tcp socket.
     *
     *  Default base time T is set to 10 seconds.
     *  \param[in] receiveTimeout is the base time T in seconds. 0 = off (no timeout).
     *  \see receiveTimeout()
     *  \Note Must be set before client is connected
     */
    void  setReceiveTimeout( int receiveTimeout);

    //! Get clients demand for login
    /*! If any of server or client demand login, it must be used.
     *  \retval true if client demand login.
     *  \see setDemandLogin()
     */
    bool  isDemandLogin()  const;

    //! Set clients demand for login
    /*! If any of server or client demand login, it must be used.
     *  \param[in] isDemandLogin true if client demand login.
     *  \see isDemandLogin()
     */
    void  setDemandLogin( bool isDemandLogin);

    //! Generate a hashed password from clear text password
    /*! \param[in] password is the clear text password.
     *  \return the hashed password, e.g "{A5ha62Aug}"
     */
    static QString  passwordHash( const QString& password);

    //! Returns current list of freePaths.
    /*! A freePath can be used even if not logged in to an ArnServer that demands login.
     *  Also all children below freePath is free to use. Usage is restricted to read
     *  operations and alike from ArnServer to ArnClient.
     *  The list of freePaths is used to enable the operation requests to be transfered
     *  to ArnServer. ArnServer still decides what's allowed. The list is automatically
     *  transfered from ArnServer to ArnClient during the negotiation phase.
     *  \return the freePath list.
     *  \see ArnServer::addFreePath()
     */
    QStringList  freePaths()  const;

    //! Is last TCP connection a reContact
    /*! ReContact occurs if a TCP connection is successful, then lost and then restored
     *  due to autoConnect.
     *  Successful TCP connection gives a state change to ConnectStat::Negotiating.
     *  \retval true if this is a reContact.
     *  \see isReConnect()
     *  \see setAutoConnect()
     *  \see connectionStatusChanged()
     */
    bool  isReContact()  const;

    //! Is last Arn Connection a reConnect
    /*! ReConnect occurs if an Arn connection is successful, then lost and then restored
     *  due to autoConnect.
     *  Successful Arn connection gives a state change to ConnectStat::Connected.
     *  \retval true if this is a reConnect.
     *  \see isReContact()
     *  \see setAutoConnect()
     *  \see connectionStatusChanged()
     */
    bool  isReConnect()  const;

    //! \cond ADV
    int  curPrio()  const;

    void  commandGet( const QString& path);
    void  commandSet( const QString& path, const QString& data);
    void  commandLs( const QString& path);
    void  commandInfo( int type, const QByteArray& data = QByteArray());
    void  commandVersion();
    bool  getLocalRemotePath( const QString& path,
                              QString& localMountPath, QString& remoteMountPath)  const;
    ArnItemNet*  newNetItem( const QString& path,
                             Arn::ObjectSyncMode syncMode = Arn::ObjectSyncMode::Normal,
                             bool* isNewPtr = 0);
    //! \endcond

signals:
    //! Signal emitted when a connection tcp error occur.
    /*! \param[in] errorText is the human readable description of the error.
     *  \param[in] socketError is the error from tcp socket, see Qt doc.
     */
    void  tcpError( const QString& errorText, QAbstractSocket::SocketError socketError);

    //! Signal emitted when the tcp connection is successfull.
    /*! \param[in] arnHost is host name or ip address, e.g. "192.168.1.1".
     *  \param[in] port is the host port, e.g. 2022.
     */
    void  tcpConnected( const QString& arnHost, quint16 port);

    //! Signal emitted when the tcp connection is broken (has been successfull).
    void  tcpDisConnected();

    //! Signal emitted when the connection status is changed.
    /*! \param[in] status is the new connection status ArnClient::ConnectStat.
     *  \param[in] curPrio is the current priority of the connection in ArnList
     *  \see curPrio()
     */
    void  connectionStatusChanged( int status, int curPrio);

    //! Signal emitted when the remote ArnServer demands a login.
    /*! When this signal is emitted, a call to loginToArn() must be done to complete
     *  the connection process.
     *  \param[in] contextCode is the situation context as:
     *             0 = First login trial
     *             1 = Server deny, login retry
     *             2 = Client deny, server gave bad password (fake server?)
     *             3 = Client deny, server not support login
     *             4 = Client deny, server bad negotiate sequence
     *  \see loginToArn()
     */
    void  loginRequired( int contextCode);

    //! \cond ADV
    void  replyRecord( Arn::XStringMap& replyMap);
    void  replyGet( const QString& data, const QString& path);
    void  replyLs( const QStringList& subItems, const QString& path);
    void  replyInfo( int type, const QByteArray& data);
    void  replyVer( const QString& version);

protected:
    ArnClient( ArnClientPrivate& dd, QObject* parent);
    ArnClientPrivate* const  d_ptr;
    //! \endcond

private slots:
    void  newNetItemProxy( ArnThreadCom* threadCom,
                           const QString& path, int syncMode = 0, void* isNewPtr = 0);
    void  createNewItem( const QString& path);
    void  doCreateArnTree( const QString& path);
    void  doDestroyArnTree( const QString& path, bool isGlobal);
    void  doReplyRecord( Arn::XStringMap& replyMap);
    void  doLoginRequired( int contextCode);
    void  onConnectWaitDone();
    void  doTcpConnected();
    void  doTcpError( QAbstractSocket::SocketError socketError);
    void  doTcpError( int socketError);
    void  doTcpDisconnected();
    void  doSyncStateChanged( int state);
    void  doRecNotified();
    void  doRecTimeout();
    void  onCommandDelete( const QString& remotePath);

private:
    struct MountPointSlot {
        ArnItemNetEar*  arnMountPoint;
        QString  localPath;
        QString  remotePath;

        MountPointSlot()
        {
            arnMountPoint = 0;
        }
    };

    void  init();
    void  reConnectArn();
    void  doConnectArnLogic();
    static QString  toRemotePathCB( void* context, const QString& path);

    QStringList  makeItemList( Arn::XStringMap& xsMap);
};

#endif // ARNCLIENT_HPP
