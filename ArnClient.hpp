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

class ArnSync;
class ArnItemNet;
class QTcpSocket;
class QTimer;


class ARNLIBSHARED_EXPORT ArnClient : public QObject
{
Q_OBJECT
public:
    explicit ArnClient(QObject *parent = 0);
    void  connectToArn( const QString& arnHost, quint16 port = 2022);
    bool  setMountPoint( const QString& path);
    void  setAutoConnect( bool isAuto, int retryTime = 2);
    void  commandGet( const QString& path);
    void  commandSet( const QString& path, const QString& data);
    void  commandLs( const QString& path);
    void  commandVersion();
    void  commandExit();
    ArnItemNet*  newNetItem( QString path,
                             ArnItem::SyncMode syncMode = ArnItem::SyncMode::Normal, bool* isNewPtr = 0);

    void  setId( QString id)  {_id = id;}
    QString  id()  const {return _id;}

signals:
    void  replyRecord( XStringMap& replyMap);
    void  replyGet( QString data, QString path);
    void  replyLs( QStringList subItems, QString path);
    void  replyVer( QString version);
    void  tcpError( QString errorText, QAbstractSocket::SocketError socketError);
    void  tcpConnected();
    void  tcpDisConnected();

private slots:
    void  newNetItemProxy( ArnThreadCom* threadCom,
                           const QString& path, int syncMode = 0, void* isNewPtr = 0);
    void  tcpError(QAbstractSocket::SocketError socketError);
    void  createNewItem( QString path);
    void  doReplyRecord( XStringMap& replyMap);
    void  reConnectArn();

private:
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
