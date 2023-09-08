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

#ifndef ARNSYNC_HPP
#define ARNSYNC_HPP

#include "ArnInc/ArnLib_global.hpp"
#include "ArnInc/ArnClient.hpp"
#include "ArnInc/XStringMap.hpp"
#include "ArnItemNet.hpp"
#include "ArnInc/MQFlags.hpp"
#include <QTimer>
#include <QByteArray>
#include <QMap>
#include <QQueue>
#include <QSslError>

#define ARNRECNAME  ""
#define MAX_BIG_INT  ( INT_MAX/2 )  // TODO: check if this can cause an over/underflow

class QSslSocket;
class ArnSyncLogin;


//! \cond ADV
class ArnSync : public QObject
{
    Q_OBJECT

public:
    struct State {
        enum E {
            //! Initialized
            Init = 0,
            //! Getting version of remote side
            Version,
            //! Getting static meta info from remote side
            Info,
            //! Authenticate
            Login,
            //! Normal syncing
            Normal
        };
        MQ_DECLARE_ENUM( State)
    };

    //! Internal Info type for exchange static init (meta) info between ArnClient and ArnServer
    //! Public Info type (in Arn.hpp) takes enums from 0 to max 999
    struct InfoType {
        enum  E {
            Start       = 1000,  // Start marker
            //! Get list of free paths not needing login
            FreePaths   = 1001,
            //! Parameters defining server and client (Agent, User, Location ...)
            WhoIAm      = 1002,
            EncryptAsk  = 1101,
            EncryptReq  = 1102,
            EncryptRdy  = 1103,  // Encrypt ready marker
            End                  // End marker
        };
        MQ_DECLARE_ENUM( InfoType)
    };

    //! Internal MessageType for exchange special data/commands between ArnClient and ArnServer
    struct MessageType {
        enum  E {
            KillRequest      = 1001,
            AbortKillRequest = 1002,
            ChatNormal       = 1008,
            ChatPrio         = 1009,
            End                  // End marker
        };
        MQ_DECLARE_ENUM( MessageType)
    };

    typedef QString (*ConVertPathCB)(void* context, const QString& path);

    ArnSync( QSslSocket* socket, bool clientSide, QObject *parent);
    ~ArnSync();

    void  setArnLogin( ArnSyncLogin* arnLogin);
    void  start();
    bool  isDemandLogin()  const;
    void  setDemandLogin( bool isDemandLogin);
    void  addFreePath( const QString& path);
    QStringList  freePaths()  const;

    ArnItemNet*  newNetItem( const QString& path,
                             Arn::ObjectSyncMode syncMode = Arn::ObjectSyncMode::Normal,
                             bool* isNewPtr = arnNullptr);
    void  connectStarted();
    void  connected();
    void  close();
    void  sendXSMap( const Arn::XStringMap& xsMap);
    void  send( const QByteArray& xString);
    void  sendNoSync( const QString& path);
    void  sendSetTree( const QString& path);
    void  sendDelete( const QString& path);
    void  sendInfo( int type, const QByteArray& data = QByteArray());
    void  sendMessage( int type, const QByteArray& data = QByteArray());
    void  sendExit();
    uint  remoteVer( uint index);
    void  loginToArn( const QString& userName, const QString& passwordHash,
                      Arn::Allow allow = Arn::Allow::All);
    void  loginToArn();

    static void  setupMonitorItem( ArnItemNet* itemNet);
    static void  doChildsToEvent( ArnItemNet* itemNet);

    void  setClientSyncMode( Arn::ClientSyncMode clientSyncMode);
    Arn::EncryptPolicy  encryptPolicy()  const;
    void  setEncryptPolicy( const Arn::EncryptPolicy& pol);
    void  setSessionHandler( void* sessionHandler);
    void  setToRemotePathCB( ConVertPathCB toRemotePathCB);
    static QString  nullConvertPath( void* context, const QString& path);
    void  setWhoIAm( const QByteArray& whoIAm);
    QByteArray  remoteWhoIAm()  const;
    QString  loginUserName()  const;
    Arn::Allow  getAllow()  const;
    void  getTraffic( quint64& in, quint64& out)  const;

signals:
    void  replyRecord( Arn::XStringMap& replyMap);
    void  xcomDelete( const QString& path);
    void  stateChanged( int state);
    void  infoReceived( int type);
    void  messageReceived( int type, const QByteArray& data);
    //! Signal emitted when the remote ArnServer demands a login.
    void  loginRequired( int contextCode);
    void  loginCompleted();

protected:
    virtual void  customEvent( QEvent* ev);

private slots:
    void  onEncrypted();
    void  onSslErrors( const QList<QSslError>& errors);
    void  doStartServerEncryption();
    void  doStartClientEncryption();
    void  disConnected();
    void  socketInput();
    void  doLoginSeq0End();
    void  sendNext();
    void  doArnMonEvent( int type, const QByteArray& data, bool isLocal, ArnItemNet* itemNet);

private:
    struct FluxRec {
        QByteArray  xString;
        int  queueNum;
    };

    void  doInfoInternal( int infoType, const QByteArray& data = QByteArray());
    void  startLogin();
    void  startNormalSync();
    void  setupItemNet( ArnItemNet* itemNet, uint netId);
    void  itemValueUpdater( const ArnLinkHandle& handleData, const QByteArray* valueData,
                            ArnItemNet* itemNet);
    void  itemModeUpdater( ArnItemNet* itemNet);
    FluxRec*  getFreeFluxRec();
    QByteArray  makeFluxString( const ArnItemNet* itemNet, const ArnLinkHandle& handleData,
                                const QByteArray* valueData);
    void  addToFluxQue( const ArnLinkHandle& handleData, const QByteArray* valueData,
                        ArnItemNet* itemNet);
    void  addToModeQue( ArnItemNet* itemNet);
    void  sendFluxItem( const ArnItemNet* itemNet);
    void  sendSyncItem( ArnItemNet* itemNet);
    void  sendModeItem( ArnItemNet* itemNet);
    void  sendLogin( int seq, const Arn::XStringMap& xsMap);
    void  eventToFluxQue( uint netId, int type, const QByteArray& data);
    void  atomicOpToFluxQue( int op, const QVariant& arg1, const QVariant& arg2, const ArnItemNet* itemNet);
    void  destroyToFluxQue( ArnItemNet* itemNet);
    void  removeItemNetRefs( ArnItemNet* itemNet);
    void  closeFinal();
    void  clearNonPipeQueues();
    void  clearAllQueues();
    void  setRemoteVerOnce( const QByteArray& remVer);
    void  setState( State state);
    bool  isFreePath( const QString& path)  const;
    int  checkEncryptPolicy()  const;

    void  doCommands();
    uint  doCommandSync();
    uint  doCommandMode();
    uint  doCommandNoSync();
    uint  doCommandFlux();
    uint  doCommandAtomOp();
    uint  doCommandEvent();
    uint  doCommandSet();
    uint  doCommandGet();
    uint  doCommandLs();
    uint  doCommandDelete();
    uint  doCommandMessage();
    uint  doCommandInfo();
    uint  doCommandRInfo();
    uint  doCommandVer();
    uint  doCommandRVer();
    uint  doCommandLogin();

    QSslSocket*  _socket;
    ArnSyncLogin* _arnLogin;
    void*  _sessionHandler;
    ConVertPathCB  _toRemotePathCB;

    QByteArray  _dataReadBuf;
    QByteArray  _dataRemain;
    Arn::XStringMap  _commandMap;
    Arn::XStringMap  _replyMap;
    Arn::XStringMap  _syncMap;
    Arn::XStringMap  _customMap;
    QStringList  _freePathTab;
    QByteArray  _whoIAm;
    QByteArray  _remoteWhoIAm;

    QMap<uint,ArnItemNet*>  _itemNetMap;

    QList<FluxRec*>  _fluxRecPool;
    QQueue<FluxRec*>  _fluxPipeQueue;
    QQueue<ArnItemNet*>  _fluxItemQueue;

    QQueue<ArnItemNet*>  _syncQueue;
    QQueue<ArnItemNet*>  _modeQueue;

    State  _state;
    InfoType  _curInfoType;
    int  _queueNumCount;
    int  _queueNumDone;
    bool  _isConnectStarted;
    bool  _isConnected;
    bool  _isSending;
    bool  _isClosed;
    bool  _isClientSide;      // True if this is the client side of the connection
    bool  _isDemandLogin;
    bool  _needEncrypted;
    uint  _remoteVer[2];
    int  _loginNextSeq;
    int  _loginReqCode;
    uint  _loginSalt1;
    uint  _loginSalt2;
    quint64  _trafficIn;
    quint64  _trafficOut;
    QString  _loginUserName;
    QString  _loginPwHash;
    QTimer  _loginDelayTimer;
    Arn::Allow  _allow;
    Arn::Allow  _remoteAllow;
    Arn::ClientSyncMode  _clientSyncMode;
    Arn::EncryptPolicy  _encryptPol;
    Arn::EncryptPolicy  _remoteEncryptPol;
};
//! \endcond

#endif // ARNSYNC_HPP
