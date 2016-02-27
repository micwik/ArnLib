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

#ifndef ARNITEMNET_HPP
#define ARNITEMNET_HPP


#include "ArnInc/ArnItemB.hpp"
#include "ArnInc/ArnItem.hpp"  // MW: tobe removed?
#include <QStringList>


//! \cond ADV
// Must only be used in main thread
class ArnItemNet : public ArnItemB
{
Q_OBJECT
public:
    explicit ArnItemNet( void* sessionHandler, QObject *parent);

    void  setNetId( uint netId);
    uint  netId()  const;

    void  addSyncModeString( const QByteArray& smode, bool linkShare);
    QByteArray  getSyncModeString()  const;
    void  setModeString( const QByteArray& mode);
    QByteArray  getModeString()  const;
    void  emitNewItemEvent( const QString& path, bool isOld = 0);
    void  emitArnMonEvent( int type, const QByteArray& data = QByteArray(),
                           bool isLocal = true);
    void  setBlockEcho( bool blockEcho);
    void  setDisable( bool disable = true);
    bool  isDisable()  const;
    bool  isMonitor()  const;
    void  setMonitor( bool isMonitor);
    void  setQueueNum( int num);
    int  queueNum()  const;

    void  resetDirtyValue();
    void  resetDirtyMode();
    bool  isDirtyMode()  const;
    bool  isLeadValueUpdate();
    bool  isLeadModeUpdate();
    bool  isBlock( quint32 sendId);

    virtual void  arnEvent( QEvent* ev, bool isAlienThread);

    using ArnItemB::addSyncMode;
    using ArnItemB::syncMode;
    using ArnItemB::getMode;
    using ArnItemB::isPipeMode;
    using ArnItemB::isFolder;
    using ArnItemB::setBlockEcho;
    using ArnItemB::addIsOnlyEcho;
    using ArnItemB::isOnlyEcho;
    using ArnItemB::retireType;
    using ArnItemB::type;
    using ArnItemB::arnExport;
    using ArnItemB::arnImport;
    using ArnItemB::childItemsMain;
    using ArnItemB::openFolder;
    using ArnItemB::openWithFlags;

protected:
    virtual void  customEvent( QEvent* ev);

private:
    void  init();

    void*  _sessionHandler;  // E.g ArnClient

    uint  _netId;          // id used during sync over net
    int  _queueNum;        // number used in itemQueue
    bool  _dirty : 1;      // item has been updated but not yet sent
    bool  _dirtyMode : 1;  // item Mode has been updated but not yet sent
    bool  _disable : 1;    // item is defunct and should not send (destroy command)
    bool  _isMonitor : 1;  // item is used as a Monitor
    bool  _blockEcho : 1;
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
