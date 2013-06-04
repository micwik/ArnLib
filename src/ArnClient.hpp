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

#ifndef ARNCLIENT_HPP
#define ARNCLIENT_HPP

#include "Arn.hpp"
#include "ArnLib_global.hpp"
#include "XStringMap.hpp"
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
    explicit ArnClient(QObject *parent = 0);

    //! Clear the Arn connection list
    /*! Typically used to start making a new Arn connection list.
     */
    void  clearArnList();

    //! Add an _Arn Server_ to the Arn connection list
    /*! \param[in] arnHost is host name or ip address, e.g. "192.168.1.1".
     *  \param[in] port is the port number (default 2022).
     */
    void  addToArnList( const QString& arnHost, quint16 port = 0);

    //! Connect to an _Arn Server_ in the Arn connection list
    /*! Will scan the connection list once until a successful connection is made.
     *  If the end of the list is reached without connection, the tcpError() signal
     */
    void  connectToArnList();

    //! Connect to an _Arn Server_
    /*! \param[in] arnHost is host name or ip address, e.g. "192.168.1.1".
     *  \param[in] port is the port number (default 2022).
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

    //! Set automatic reconnect
    /*! \param[in] isAuto true if using auto reconnect
     *  \param[in] retryTime is the time between reconnection attempts in seconds
     */
    void  setAutoConnect( bool isAuto, int retryTime = 2);

    //! \cond ADV
    void  commandGet( const QString& path);
    void  commandSet( const QString& path, const QString& data);
    void  commandLs( const QString& path);
    void  commandVersion();
    void  commandExit();
    ArnItemNet*  newNetItem( QString path,
                             ArnItem::SyncMode syncMode = ArnItem::SyncMode::Normal, bool* isNewPtr = 0);
    void  setId( QString id)  {_id = id;}
    QString  id()  const {return _id;}
    //! \endcond

signals:
    //! Signal emitted when a connection tcp error occur.
    /*! \param[in] errorText is the human readable description of the error.
     *  \param[in] socketError is the error from tcp socket, see Qt doc.
     */
    void  tcpError( QString errorText, QAbstractSocket::SocketError socketError);

    //! Signal emitted when the tcp connection is successfull.
    void  tcpConnected();

    //! Signal emitted when the tcp connection is broken (has been successfull).
    void  tcpDisConnected();

    //! \cond ADV
    void  replyRecord( XStringMap& replyMap);
    void  replyGet( QString data, QString path);
    void  replyLs( QStringList subItems, QString path);
    void  replyVer( QString version);
    //! \endcond

private slots:
    void  newNetItemProxy( ArnThreadCom* threadCom,
                           const QString& path, int syncMode = 0, void* isNewPtr = 0);
    void  tcpError(QAbstractSocket::SocketError socketError);
    void  createNewItem( QString path);
    void  doReplyRecord( XStringMap& replyMap);
    void  reConnectArn();

private:
    void doConnectArnLogic();

    struct HostSlot {
        QString  arnHost;
        quint16  port;
    };
    QList<HostSlot>  _hostTab;
    int  _nextHost;

    QStringList  makeItemList( XStringMap& xsMap);
    QTcpSocket*  _socket;
    ArnSync*  _arnNetSync;

    QString  _arnHost;
    quint16  _port;
    bool  _isAutoConnect;
    int  _retryTime;
    QTimer*  _connectTimer;
    ArnItem*  _arnMountPoint;
    XStringMap  _commandMap;
    QString  _id;
};

#endif // ARNCLIENT_HPP
