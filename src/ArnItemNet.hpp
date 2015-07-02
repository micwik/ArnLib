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
#include <QStringList>


//! \cond ADV
// Must only be used in main thread
class ArnItemNet : public ArnItemB
{
Q_OBJECT
public:
    explicit ArnItemNet( QObject *parent = 0);
    explicit ArnItemNet( const QString& path, QObject *parent = 0);

    void  setNetId( uint netId);
    uint  netId()  const;
    QString  localMountPath()  const;
    void  setLocalMountPath( const QString& localMountPath);
    QString  remoteMountPath()  const;
    void  setRemoteMountPath( const QString& remoteMountPath);
    QString  toRemotePath( const QString& localPath = QString())  const;
    QString  toLocalPath( const QString& remotePath)  const;

    void  addSyncModeString( const QByteArray& smode, bool linkShare);
    QByteArray  getSyncModeString()  const;
    void  setModeString( const QByteArray& mode);
    QByteArray  getModeString()  const;
    void  emitNewItemEvent( const QString& path, bool isOld = 0);

    void  setDisable( bool disable = true);
    bool  isDisable()  const;
    bool  isMonitor()  const;
    void  setMonitor( bool isMonitor);

    void  setQueueNum( int num);
    int  queueNum()  const;
    void  resetDirty();
    void  resetDirtyMode();
    bool  isDirtyMode()  const;

    virtual void  itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value = 0);
    virtual void  itemCreatedBelow( const QString& path);
    virtual void  modeUpdate( bool isSetup = false);

    using ArnItemB::addSyncMode;
    using ArnItemB::syncMode;
    using ArnItemB::getMode;
    using ArnItemB::isPipeMode;
    using ArnItemB::setBlockEcho;
    using ArnItemB::isOnlyEcho;
    using ArnItemB::isRetiredGlobal;
    using ArnItemB::type;
    using ArnItemB::arnExport;
    using ArnItemB::arnImport;
    using ArnItemB::childItemsMain;
    using ArnItemB::openFolder;

signals:
    void  arnEvent( const QByteArray& type, const QByteArray& data, bool isLocal);
    void  goneDirty( const ArnLinkHandle& handleData);
    void  goneDirtyMode();

public slots:
    void  emitArnEvent( const QByteArray& type, const QByteArray& data = QByteArray(),
                        bool isLocal = true);

private:
    void  init();

    uint  _netId;      // id used during sync over net
    int  _queueNum;    // number used in itemQueue
    bool  _dirty;      // item has been updated but not yet sent
    bool  _dirtyMode;  // item Mode has been updated but not yet sent
    bool  _disable;    // item is defunct and should not send (destroy command)
    bool  _isMonitor;  // item is used as a Monitor
    QString  _localMountPath;
    QString  _remoteMountPath;
};
//! \endcond

#endif // ARNITEMNET_HPP
