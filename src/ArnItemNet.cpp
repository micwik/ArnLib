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

#include <QDebug>
#include "Arn.hpp"
#include "ArnClient.hpp"
#include "ArnItemNet.hpp"


void  ArnItemNet::init()
{
    setForceKeep();
    setIgnoreSameValue( false);
    _netId = 0;
    _dirty = false;
    _dirtyMode = false;
    _disable = false;
}


ArnItemNet::ArnItemNet( QObject *parent) :
    ArnItem( parent)
{
    init();
}


ArnItemNet::ArnItemNet( const QString& path, QObject *parent) :
        ArnItem( path, parent)
{
    init();
}


void  ArnItemNet::addSyncModeString( const QByteArray& smode, bool linkShare)
{
    ArnItem::SyncMode  syncMode;

    syncMode.set( syncMode.Master,      smode.contains("master"));
    syncMode.set( syncMode.AutoDestroy, smode.contains("autodestroy"));
    syncMode.set( syncMode.Monitor,     smode.contains("mon"));

    addSyncMode( syncMode, linkShare);
}


QByteArray  ArnItemNet::getSyncModeString()  const
{
    QByteArray  smode;
    SyncMode  syncMode = ArnItem::syncMode();

    if (syncMode.is( syncMode.Master))       smode += "master ";
    if (syncMode.is( syncMode.AutoDestroy))  smode += "autodestroy ";
    if (syncMode.is( syncMode.Monitor))      smode += "mon ";

    return smode.trimmed();
}


void  ArnItemNet::setModeString( const QByteArray& modeString)
{
    Mode  mode;
    if (modeString.contains('P'))  mode.f |= mode.Pipe;
    if (modeString.contains('V'))  mode.f |= mode.BiDir;
    if (modeString.contains('S'))  mode.f |= mode.Save;

    addMode( mode);
}


QByteArray  ArnItemNet::getModeString()  const
{
    Mode  mode = getMode();
    QByteArray  modeString;
    if (mode.is( mode.Pipe))       modeString += "P";
    if (mode.is( mode.BiDir))  modeString += "V";
    if (mode.is( mode.Save))       modeString += "S";

    return modeString;
}


void  ArnItemNet::submitted()
{
    _dirty = false;
    resetOnlyEcho();
}


void  ArnItemNet::submittedMode()
{
    _dirtyMode = false;
}


void  ArnItemNet::itemUpdateStart( const ArnLinkHandle& handleData, const QByteArray* value)
{
    Q_UNUSED(value);

    if (!_dirty) {
        _dirty = true;
        emit goneDirty( handleData);
    }
}


void  ArnItemNet::itemUpdateEnd()
{
}


void  ArnItemNet::modeUpdate( bool isSetup)
{
    ArnItem::modeUpdate( isSetup); // must be called for base-class update
    if (isSetup)  return;

    if (!_dirtyMode) {
        _dirtyMode = true;
        emit goneDirtyMode();
    }
}


void  ArnItemNet::emitArnEvent( QByteArray type, QByteArray data, bool isLocal)
{
    emit arnEvent( type, data, isLocal);
}