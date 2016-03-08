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

#ifndef ARNCLIENT_P_HPP
#define ARNCLIENT_P_HPP

#include "ArnInc/ArnClient.hpp"
#include <QMutex>

class ArnSync;
class QTcpSocket;
class QTimer;


class ArnClientPrivate
{
    friend class ArnClient;
public:
    ArnClientPrivate();
    virtual  ~ArnClientPrivate();

private:
    void  resetConnectionFlags();
    void  clearArnList( int prioFilter);
    ArnClient::HostList  arnList( int prioFilter)  const;
    void  addToArnList( const QString& arnHost, quint16 port, int prio);

    ArnClient::HostList  _hostTab;
    QList<int>  _hostPrioTab;
    int  _nextHost;
    int  _curPrio;

    QTcpSocket*  _socket;
    ArnSync*  _arnNetSync;
    QMutex  _mutex;
    QString  _arnHost;
    quint16  _port;
    bool  _isAutoConnect;
    int  _recTimeoutCount;
    int  _receiveTimeout;
    int  _retryTime;
    QTimer*  _connectTimer;
    QTimer*  _recTimer;
    ArnItem*  _arnMountPoint;
    QList<ArnClient::MountPointSlot>  _mountPoints;
    Arn::XStringMap  _commandMap;
    QString  _id;
    ArnClient::HostAddrPort  _curConnectAP;
    ArnClient::ConnectStat  _connectStat;
    bool  _isValidCredent;
    bool  _isReContact;
    bool  _isReConnect;
    bool  _wasContact;
    bool  _wasConnect;
};

#endif // ARNCLIENT_P_HPP
