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

#include "ArnItemNet.hpp"
#include <QDebug>


void  ArnItemNet::init()
{
    setForceKeep();
    setIgnoreSameValue( false);
    _netId     = 0;
    _dirty     = false;
    _dirtyMode = false;
    _disable   = false;
    _isMonitor = false;
}


ArnItemNet::ArnItemNet( QObject *parent) :
    ArnItemB( parent)
{
    init();
}


ArnItemNet::ArnItemNet( const QString& path, QObject *parent) :
    ArnItemB( parent)
{
    open( path);
    init();
}


void  ArnItemNet::setNetId( uint netId)
{
    _netId = netId;
}


uint  ArnItemNet::netId()  const
{
    return _netId;
}


QString  ArnItemNet::remoteMountPath()  const
{
    return _remoteMountPath;
}

void  ArnItemNet::setRemoteMountPath( const QString& remoteMountPath)
{
    _remoteMountPath = remoteMountPath;
}


QString  ArnItemNet::localMountPath()  const
{
    return _localMountPath;
}


void  ArnItemNet::setLocalMountPath( const QString& localMountPath)
{
    _localMountPath = localMountPath;
}


QString  ArnItemNet::toRemotePath( const QString& localPath)  const
{
    return Arn::changeBasePath( _localMountPath, _remoteMountPath,
                                localPath.isNull() ? path() : localPath);
}


QString  ArnItemNet::toLocalPath( const QString& remotePath)  const
{
    return Arn::changeBasePath( _remoteMountPath, _localMountPath, remotePath);
}


void  ArnItemNet::addSyncModeString( const QByteArray& smode, bool linkShare)
{
    Arn::ObjectSyncMode  syncMode;

    syncMode.set( syncMode.Master,      smode.contains("master"));
    syncMode.set( syncMode.AutoDestroy, smode.contains("autodestroy"));
    syncMode.set( syncMode.Monitor,     smode.contains("mon"));

    addSyncMode( syncMode, linkShare);
}


QByteArray  ArnItemNet::getSyncModeString()  const
{
    QByteArray  smode;
    Arn::ObjectSyncMode  syncMode = ArnItemB::syncMode();

    if (syncMode.is( syncMode.Master))       smode += "master ";
    if (syncMode.is( syncMode.AutoDestroy))  smode += "autodestroy ";
    if (syncMode.is( syncMode.Monitor))      smode += "mon ";

    return smode.trimmed();
}


void  ArnItemNet::setModeString( const QByteArray& modeString)
{
    Arn::ObjectMode  mode;
    if (modeString.contains('P'))  mode.set( mode.Pipe);
    if (modeString.contains('V'))  mode.set( mode.BiDir);  // Legacy
    if (modeString.contains('B'))  mode.set( mode.BiDir);
    if (modeString.contains('S'))  mode.set( mode.Save);

    addMode( mode);
}


QByteArray  ArnItemNet::getModeString()  const
{
    Arn::ObjectMode  mode = getMode();
    QByteArray  modeString;
    if (mode.is( mode.Pipe))   modeString += "P";
    if (mode.is( mode.BiDir))  modeString += "B";
    if (mode.is( mode.Save))   modeString += "S";

    return modeString;
}


void  ArnItemNet::emitNewItemEvent( const QString& path, bool isOld)
{
    emit arnEvent( isOld ? "itemFound" : "itemCreated", path.toUtf8(), true);
}


void  ArnItemNet::setDisable( bool disable)
{
    _disable = disable;
}


bool  ArnItemNet::isDisable()  const
{
    return _disable;
}


bool  ArnItemNet::isMonitor() const
{
    return _isMonitor;
}


void  ArnItemNet::setMonitor( bool isMonitor)
{
    _isMonitor = isMonitor;
}


void  ArnItemNet::setQueueNum( int num)
{
    _queueNum = num;
}


int  ArnItemNet::queueNum()  const
{
    return _queueNum;
}


void  ArnItemNet::resetDirty()
{
    _dirty = false;
    resetOnlyEcho();
}


void  ArnItemNet::resetDirtyMode()
{
    _dirtyMode = false;
}


bool  ArnItemNet::isDirtyMode()  const
{
    return _dirtyMode;
}


void  ArnItemNet::itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value)
{
    Q_UNUSED(value);

    if (!_dirty) {
        _dirty = true;
        emit goneDirty( handleData);
    }
}


void  ArnItemNet::itemCreatedBelow( const QString& path)
{
    if (_isMonitor)
        emitNewItemEvent( path);
}


void  ArnItemNet::modeUpdate( Arn::ObjectMode mode, bool isSetup)
{
    ArnItemB::modeUpdate( mode, isSetup); // must be called for base-class update
    if (isSetup)  return;

    if (!_dirtyMode) {
        _dirtyMode = true;
        emit goneDirtyMode();
    }
}


void  ArnItemNet::emitArnEvent( const QByteArray& type, const QByteArray& data, bool isLocal)
{
    emit arnEvent( type, data, isLocal);
}
