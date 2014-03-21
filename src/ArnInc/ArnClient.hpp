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

class ArnSync;
class ArnItemNet;
class QTcpSocket;
class QTimer;


//! Class for connecting to an _Arn Server_.
/*!
[About Sharing Arn Data Objects](\ref gen_shareArnobj)

<b>Example usage</b> \n \code
    // In class declare
    ArnClient  _arnClient;

    // In class code
    _arnClient.connectToArn("localhost");
    _arnClient.setMountPoint("//");
    _arnClient.setAutoConnect( true);
\endcode
*/
class ARNLIBSHARED_EXPORT ArnClient : public QObject
{
Q_OBJECT
public:
    struct ConnectStat {
        enum E {
            //! Initialized, not yet any result of trying to connect ...
            Init = 0,
            //! Trying to connect to an Arn host
            Connecting,
            //! Successfully connected to an Arn host
            Connected,
            //! Unsuccessfull when trying to connect to an Arn host
            Error,
            //! TCP connection is broken (has been successfull)
            Disconnected,
            //! Unsuccessfully tried to connect to all hosts in the Arn connection List
            TriedAll
        };
        MQ_DECLARE_ENUM( ConnectStat)
    };

    struct HostAddrPort {
        QString  addr;
        quint16  port;

        HostAddrPort() {
            port = Arn::defaultTcpPort;
        }
    };
    typedef QList<HostAddrPort>  HostList;

    explicit ArnClient(QObject *parent = 0);

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
     *  \param[in] port is the port number (default 2022).
     *  \param[in] prio gives the sorting (connection) order and can be used for selection filter.
     *  \see clearArnList()
     *  \see arnList()
     *  \see Arn::makeHostWithInfo()
     */
    void  addToArnList( const QString& arnHost, quint16 port = 0, int prio = 0);

    //! Connect to an _Arn Server_ in the Arn connection list
    /*! Will scan the connection list once until a successful connection is made.
     *  If the end of the list is reached without connection, the tcpError() signal
     */
    void  connectToArnList();

    //! Connect to an _Arn Server_
    /*! \param[in] arnHost is host name or ip address, e.g. "192.168.1.1".
     *  \param[in] port is the port number (default 2022).
     *  \see Arn::makeHostWithInfo()
     */
    void  connectToArn( const QString& arnHost, quint16 port = 0);

    //! Set the sharing tree path
    /*! Mountpoint is an association to the similarity of mounting a "remote filesystem".
     *  In Arn the remote "file system" is at the same sub path as the mountpoint,
     *  e.g. a client having mountpoint "/a/b/" and opening an _Arn Data Object_ at
     *  "/a/b/c" will have the object _c_ shared with the server at its path "/a/b/c".
     *  \param[in] path is the sharing tree.
     *  \retval false if error.
     *  \see \ref gen_shareArnobj
     */
    bool  setMountPoint( const QString& path);

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

    //! \cond ADV
    void  setId( QString id);
    QString  id()  const;
    int  curPrio()  const;

    void  commandGet( const QString& path);
    void  commandSet( const QString& path, const QString& data);
    void  commandLs( const QString& path);
    void  commandVersion();
    void  commandExit();
    ArnItemNet*  newNetItem( QString path,
                             Arn::ObjectSyncMode syncMode = Arn::ObjectSyncMode::Normal,
                             bool* isNewPtr = 0);
    //! \endcond


signals:
    //! Signal emitted when a connection tcp error occur.
    /*! \param[in] errorText is the human readable description of the error.
     *  \param[in] socketError is the error from tcp socket, see Qt doc.
     */
    void  tcpError( QString errorText, QAbstractSocket::SocketError socketError);

    //! Signal emitted when the tcp connection is successfull.
    /*! \param[in] arnHost is host name or ip address, e.g. "192.168.1.1".
     *  \param[in] port is the port number (default 2022).
     */
    void  tcpConnected( QString arnHost = QString(), quint16 port = 0);

    //! Signal emitted when the tcp connection is broken (has been successfull).
    void  tcpDisConnected();

    //! Signal emitted when the connection status is changed.
    /*! \param[in] status is the new connection status ArnClient::ConnectStat.
     *  \param[in] curPrio is the current priority of the connection in ArnList
     *  \see curPrio()
     */
    void  connectionStatusChanged( int status, int curPrio);

    //! \cond ADV
    void  replyRecord( Arn::XStringMap& replyMap);
    void  replyGet( QString data, QString path);
    void  replyLs( QStringList subItems, QString path);
    void  replyVer( QString version);
    //! \endcond

private slots:
    void  newNetItemProxy( ArnThreadCom* threadCom,
                           const QString& path, int syncMode = 0, void* isNewPtr = 0);
    void  tcpError(QAbstractSocket::SocketError socketError);
    void  createNewItem( QString path);
    void  doReplyRecord( Arn::XStringMap& replyMap);
    void  reConnectArn();
    void  doTcpConnected();

private:
    void doConnectArnLogic();

    HostList  _hostTab;
    QList<int>  _hostPrioTab;
    int  _nextHost;
    int  _curPrio;

    QStringList  makeItemList( Arn::XStringMap& xsMap);
    QTcpSocket*  _socket;
    ArnSync*  _arnNetSync;

    QString  _arnHost;
    quint16  _port;
    bool  _isAutoConnect;
    int  _retryTime;
    QTimer*  _connectTimer;
    ArnItem*  _arnMountPoint;
    Arn::XStringMap  _commandMap;
    QString  _id;
    HostAddrPort  _curConnectAP;
    ConnectStat  _connectStat;
};

#endif // ARNCLIENT_HPP
