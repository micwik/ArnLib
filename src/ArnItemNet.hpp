// Copyright (C) 2010-2016 Michael Wiklund.
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

#ifndef ARNITEMNET_HPP
#define ARNITEMNET_HPP


#include "ArnInc/ArnBasicItem.hpp"
#include "ArnInc/ArnItem.hpp"
#include <QStringList>


//! \cond ADV
class ArnItemNet : public ArnBasicItem
{
public:
    explicit ArnItemNet( void* sessionHandler);
    virtual  ~ArnItemNet();
    bool  openWithFlags( const QString& path, Arn::LinkFlags linkFlags);

    void  setNetId( uint netId);
    uint  netId()  const;
    void*  sessionHandler()  const;

    void  addSyncModeString( const QByteArray& smode, bool linkShare);
    QByteArray  getSyncModeString()  const;
    void  setModeString( const QByteArray& mode);
    QByteArray  getModeString()  const;

    void  sendNewItemMonEvent( const QString& path, bool isOld = 0);
    void  sendMonEvent( int type, const QByteArray& data = QByteArray(), bool isLocal = true);
    void  setBlockEcho( bool blockEcho);
    void  setDisable( bool disable = true);
    bool  isDisable()  const;
    bool  isMonitor()  const;
    void  setMonitor( bool isMonitor);
    void  setQueueNum( int num);
    int  queueNum()  const;
    void  nextEchoSeq();
    void  resetEchoSeq();
    void  setEchoSeq( qint8 echoSeq);
    qint8  echoSeq()  const;
    bool  isEchoSeqOld( qint8 receivedEchoSeq);

    void  resetDirtyValue();
    void  resetDirtyMode();
    bool  isDirtyValue()  const;
    bool  isDirtyMode()  const;
    bool  isLeadValueUpdate();
    bool  isLeadModeUpdate();
    bool  isBlock( quint32 sendId);
    void  setIniMaster( bool iniMaster);
    void  setIniSlave( bool iniSlave);
    bool  isMasterAtStart()  const;
    void  setSyncFlux( bool isSyncFlux);
    bool  isSyncFlux()  const;
    void  setSaveFlux( bool saveFlux);
    bool  isSaveFlux()  const;
    quint32  localUpdateSinceStop()  const;
    void  onConnectStop();

    virtual void  arnEvent( QEvent* ev, bool isAlienThread);

    static Arn::ObjectMode  stringToObjectMode( const QByteArray& modeString);
    static QByteArray  ObjectModeToString( Arn::ObjectMode mode);

    using ArnBasicItem::addSyncMode;
    using ArnBasicItem::syncMode;
    using ArnBasicItem::getMode;
    using ArnBasicItem::isPipeMode;
    using ArnBasicItem::isFolder;
    using ArnBasicItem::addIsOnlyEcho;
    using ArnBasicItem::isOnlyEcho;
    using ArnBasicItem::retireType;
    using ArnBasicItem::type;
    using ArnBasicItem::arnExport;
    using ArnBasicItem::arnImport;
    using ArnBasicItem::childItemsMain;
    using ArnBasicItem::openWithFlags;

private:
    void  init();

    void*  _sessionHandler;  // E.g ArnClient

    uint  _netId;               // id used during sync over net
    int  _queueNum;             // number used in itemQueue
    quint32  _updateCountStop;  // Local update count at connection lost
    qint8  _curEchoSeq;         // Used to avoid obsolete echo
    bool  _dirty : 1;           // item has been updated but not yet sent
    bool  _dirtyMode : 1;       // item Mode has been updated but not yet sent
    bool  _disable : 1;         // item is defunct and should not send (destroy command)
    bool  _isMonitor : 1;       // item is used as a Monitor
    bool  _blockEcho : 1;
    bool  _iniMaster : 1;       // Temporary master logic for next sync
    bool  _iniSlave  : 1;       // Temporary slave logic for next sync
    bool  _syncFlux  : 1;       // Value to be fluxed is due to sync
    bool  _saveFlux  : 1;       // Value to be fluxed is persist load or initial flux persistent
};


class ArnItemNetEar : public ArnItem
{
    Q_OBJECT
public:
    explicit ArnItemNetEar( QObject* parent = 0);

signals:
    void  arnTreeCreated( const QString& path);
    void  arnTreeDestroyed( const QString& path, bool isGlobal);

protected:
    virtual void  customEvent( QEvent* ev);

private:
};
//! \endcond

#endif // ARNITEMNET_HPP
