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

#ifndef ARNSYNC_HPP
#define ARNSYNC_HPP

#include "ArnItemNet.hpp"
#include "ArnClient.hpp"
#include "ArnLib_global.hpp"
#include <QObject>
#include <QByteArray>
#include <QMap>
#include <QQueue>
#include <QPointer>
#include "XStringMap.hpp"

#define ARNRECNAME  "rec"

class QTcpSocket;


//! \cond ADV
class ArnSync : public QObject
{
    Q_OBJECT

public:
    ArnSync( QTcpSocket* socket, bool clientSide = 0, QObject *parent = 0);
    ~ArnSync();
    ArnItemNet*  newNetItem( const QString& path,
                             ArnItem::SyncMode syncMode = ArnItem::SyncMode::Normal, bool* isNewPtr = 0);
    void  sendXSMap( const XStringMap& xsMap);
    void  send( const QByteArray& xString);

signals:
    void  replyRecord( XStringMap& replyMap);

private slots:
    void  connected();
    void  disConnected();
    void  socketInput();
    void  addToFluxQue( const ArnLinkHandle& handleData);
    void  addToModeQue();
    void  sendNext();
    void  linkDestroyedHandle();
    void  doArnEvent( QByteArray type, QByteArray data, bool isLocal);

private:
    typedef QPointer<ArnItemNet>  ArnItemNetQPtr;
    struct FluxRec {
        QByteArray  xString;
        int  queueNum;
    };

    void  setupItemNet( ArnItemNet* itemNet, uint netId);
    void  setupMonitorItem( ArnItemNet* itemNet);
    void  doChildsToEvent( ArnItemNet* itemNet);
    FluxRec*  getFreeFluxRec();
    QByteArray  makeFluxString( const ArnItemNet* itemNet, const ArnLinkHandle& handleData);
    void  sendFluxItem( const ArnItemNet* itemNet);
    void  sendSyncItem( ArnItemNet* itemNet);
    void  sendModeItem( ArnItemNet* itemNet);
    void  eventToFluxQue( uint netId, QByteArray type, QByteArray data);
    void  destroyToFluxQue( uint netId);

    void  doCommand();
    uint  doCommandSync();
    uint  doCommandMode();
    uint  doCommandDestroy();
    uint  doCommandFlux();
    uint  doCommandEvent();
    uint  doCommandSet();
    uint  doCommandGet();
    uint  doCommandLs();

    QTcpSocket*  _socket;

    QByteArray  _dataReadBuf;
    QByteArray  _dataRemain;
    XStringMap  _commandMap;
    XStringMap  _replyMap;
    XStringMap  _syncMap;

    QMap<uint,ArnItemNet*>  _itemNetMap;

    QList<FluxRec*>  _fluxRecPool;
    QQueue<FluxRec*>  _fluxPipeQueue;
    QQueue<ArnItemNet*>  _fluxItemQueue;

    QQueue<ArnItemNet*>  _syncQueue;
    QQueue<ArnItemNet*>  _modeQueue;

    int  _queueNumCount;
    int  _queueNumDone;
    int  _fluxQueueSwitch;
    bool  _isConnected;
    bool  _isSending;
    bool  _isClientSide;      // True if this is the client side of the connection
};
//! \endcond

#endif // ARNSYNC_HPP
