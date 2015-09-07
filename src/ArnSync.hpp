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

#ifndef ARNSYNC_HPP
#define ARNSYNC_HPP

#include "ArnInc/ArnLib_global.hpp"
#include "ArnInc/ArnClient.hpp"
#include "ArnInc/XStringMap.hpp"
#include "ArnItemNet.hpp"
#include "ArnInc/MQFlags.hpp"
#include <QObject>
#include <QByteArray>
#include <QMap>
#include <QQueue>

#define ARNRECNAME  ""

class QTcpSocket;


//! \cond ADV
class ArnSync : public QObject
{
    Q_OBJECT

public:
    struct State {
        enum E {
            //! Initialized, not yet any result of trying to connect ...
            Init = 0,
            //! Trying to connect to an Arn host
            Version,
            //! Normal syncing
            Normal
        };
        MQ_DECLARE_ENUM( State)
    };

    ArnSync( QTcpSocket* socket, bool clientSide = 0, QObject *parent = 0);
    ~ArnSync();
    void  start();
    void  setLegacy( bool isLegacy);
    ArnItemNet*  newNetItem( const QString& path,
                             const QString& localMountPath, const QString& remoteMountPath,
                             Arn::ObjectSyncMode syncMode = Arn::ObjectSyncMode::Normal,
                             bool* isNewPtr = 0);
    void  close();
    void  sendXSMap( const Arn::XStringMap& xsMap);
    void  send( const QByteArray& xString);
    void  sendNoSync( const QString& path);
    void  sendSetTree( const QString& path);
    void  sendDelete( const QString& path);
    void  sendExit();
    uint  remoteVer( uint index);

    static void  setupMonitorItem( ArnItemNet* itemNet);
    static void  doChildsToEvent( ArnItemNet* itemNet);

signals:
    void  replyRecord( Arn::XStringMap& replyMap);
    void  xcomDelete( const QString& path);
    void  stateChanged( int state);

private slots:
    void  connected();
    void  startNormalSync();
    void  disConnected();
    void  socketInput();
    void  addToFluxQue( const ArnLinkHandle& handleData);
    void  addToModeQue();
    void  sendNext();
    void  linkDestroyedHandle();
    void  doArnMonEvent( int type, const QByteArray& data, bool isLocal);

private:
    struct FluxRec {
        QByteArray  xString;
        int  queueNum;
    };

    void  setupItemNet( ArnItemNet* itemNet, uint netId);
    FluxRec*  getFreeFluxRec();
    QByteArray  makeFluxString( const ArnItemNet* itemNet, const ArnLinkHandle& handleData);
    void  sendFluxItem( const ArnItemNet* itemNet);
    void  sendSyncItem( ArnItemNet* itemNet);
    void  sendModeItem( ArnItemNet* itemNet);
    void  eventToFluxQue( uint netId, int type, const QByteArray& data);
    void  destroyToFluxQue( ArnItemNet* itemNet);
    void  removeItemNet( ArnItemNet* itemNet);
    void  closeFinal();
    void  clearQueues();
    void  setRemoteVer( const QByteArray& remVer);
    void  setState( State state);

    void  doCommands();
    void  doSpecialStateCommands( const QByteArray& cmd);
    uint  doCommandSync();
    uint  doCommandMode();
    uint  doCommandNoSync();
    uint  doCommandDestroy();
    uint  doCommandFlux();
    uint  doCommandEvent();
    uint  doCommandSet();
    uint  doCommandGet();
    uint  doCommandLs();
    uint  doCommandDelete();

    QTcpSocket*  _socket;

    QByteArray  _dataReadBuf;
    QByteArray  _dataRemain;
    Arn::XStringMap  _commandMap;
    Arn::XStringMap  _replyMap;
    Arn::XStringMap  _syncMap;

    QMap<uint,ArnItemNet*>  _itemNetMap;

    QList<FluxRec*>  _fluxRecPool;
    QQueue<FluxRec*>  _fluxPipeQueue;
    QQueue<ArnItemNet*>  _fluxItemQueue;

    QQueue<ArnItemNet*>  _syncQueue;
    QQueue<ArnItemNet*>  _modeQueue;

    State  _state;
    int  _queueNumCount;
    int  _queueNumDone;
    bool  _isConnected;
    bool  _isSending;
    bool  _isClosed;
    bool  _wasClosed;
    bool  _isClientSide;      // True if this is the client side of the connection
    bool  _isLegacy;
    uint  _remoteVer[2];
};
//! \endcond

#endif // ARNSYNC_HPP
