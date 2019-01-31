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

#include "ArnItemNet.hpp"
#include "ArnLink.hpp"
#include "ArnInc/ArnMonEvent.hpp"
#include "ArnInc/ArnEvent.hpp"
#include "ArnInc/XStringMap.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QCoreApplication>
#include <QThread>
#include <QDebug>


void  ArnItemNet::init()
{
    _netId      = 0;
    _dirty      = false;
    _dirtyMode  = false;
    _disable    = false;
    _isMonitor  = false;
    _blockEcho  = false;
    _iniMaster  = false;
    _iniSlave   = false;
    _syncFlux   = false;
    _curEchoSeq = -1;
    _updateCountStop = 0;

    setUniDir();
    setIgnoreSameValue( false);
}


ArnItemNet::ArnItemNet( void* sessionHandler)
{
    init();
    _sessionHandler = sessionHandler;
}


ArnItemNet::~ArnItemNet()
{
#if 0
    if (path() == "/") {
        qDebug() << "ArnItemNet destruct root: eventH=" << eventHandler() << " sessionH=" << _sessionHandler
                 << " isMon=" << _isMonitor << " netId=" << _netId
                 << " itemNet=" << this;
    }
#endif
}


bool  ArnItemNet::openWithFlags( const QString& path, Arn::LinkFlags linkFlags)
{
#if 0
    if (path == "/") {
        qDebug() << "ArnItemNet open root: eventH=" << eventHandler() << " sessionH=" << _sessionHandler
                 << " isMon=" << _isMonitor << " netId=" << _netId
                 << " itemNet=" << this;
    }
#endif

    return ArnBasicItem::openWithFlags( path, linkFlags);
}


void  ArnItemNet::setNetId( uint netId)
{
    _netId = netId;
}


uint  ArnItemNet::netId()  const
{
    return _netId;
}


void*  ArnItemNet::sessionHandler()  const
{
    return _sessionHandler;
}


void  ArnItemNet::addSyncModeString( const QByteArray& smode, bool linkShare)
{
    Arn::ObjectSyncMode  syncMode;

    setIniMaster(                       smode.contains("inimast"));
    setIniSlave(                        smode.contains("inislave"));
    bool isMaster                     = smode.contains("master");
    isMaster |= _iniSlave;
    syncMode.set( syncMode.Master,      isMaster);
    syncMode.set( syncMode.AutoDestroy, smode.contains("autodestroy"));
    syncMode.set( syncMode.Monitor,     smode.contains("mon"));

    addSyncMode( syncMode, linkShare);
}


QByteArray  ArnItemNet::getSyncModeString()  const
{
    QByteArray  smode;
    Arn::ObjectSyncMode  syncMode = ArnBasicItem::syncMode();

    if (_iniSlave)                            smode += "inislave ";
    else if (syncMode.is( syncMode.Master))   smode += "master ";
    else if (_iniMaster)                      smode += "inimast ";

    if  (syncMode.is( syncMode.AutoDestroy))  smode += "autodestroy ";
    if  (syncMode.is( syncMode.Monitor))      smode += "mon ";

    return smode.trimmed();
}


Arn::ObjectMode  ArnItemNet::stringToObjectMode( const QByteArray& modeString)
{
    Arn::ObjectMode  mode;
    if (modeString.contains('P'))  mode.set( mode.Pipe);
    if (modeString.contains('V'))  mode.set( mode.BiDir);  // Legacy
    if (modeString.contains('B'))  mode.set( mode.BiDir);
    if (modeString.contains('S'))  mode.set( mode.Save);

    return mode;
}


QByteArray  ArnItemNet::ObjectModeToString( Arn::ObjectMode mode)
{
    QByteArray  modeString;
    if (mode.is( mode.Pipe))   modeString += "P";
    if (mode.is( mode.BiDir))  modeString += "B";
    if (mode.is( mode.Save))   modeString += "S";

    return modeString;
}


void  ArnItemNet::setModeString( const QByteArray& modeString)
{
    Arn::ObjectMode  mode = stringToObjectMode( modeString);
    addMode( mode);
}


QByteArray  ArnItemNet::getModeString()  const
{
    Arn::ObjectMode  mode = getMode();
    return ObjectModeToString( mode);
}


/// Must be threaded
void  ArnItemNet::sendNewItemMonEvent( const QString& path, bool isOld)
{
    int  type = isOld ? ArnMonEventType::ItemFound : ArnMonEventType::ItemCreated;
    sendMonEvent( type, path.toUtf8(), true);
}


/// Must be threaded
void  ArnItemNet::sendMonEvent( int type, const QByteArray& data, bool isLocal)
{
    ArnEvMonitor  ev( type, data, isLocal, _sessionHandler);
    sendArnEventLink( &ev);
}


void  ArnItemNet::setBlockEcho( bool blockEcho)
{
    _blockEcho = blockEcho;
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


void  ArnItemNet::nextEchoSeq()
{
    _curEchoSeq = (_curEchoSeq + 1) % 100;
}


void ArnItemNet::resetEchoSeq()
{
    _curEchoSeq = -1;
}


void  ArnItemNet::setEchoSeq( qint8 echoSeq)
{
    _curEchoSeq = echoSeq;
}


qint8  ArnItemNet::echoSeq()  const
{
    return isPipeMode() ? -1 : _curEchoSeq;
}


bool  ArnItemNet::isEchoSeqOld( qint8 receivedEchoSeq)
{
    if ((_curEchoSeq < 0) || (receivedEchoSeq < 0))  return false;
    return receivedEchoSeq != _curEchoSeq;
}


void  ArnItemNet::resetDirtyValue()
{
    _dirty     = false;
    _iniMaster = false;
    _iniSlave  = false;
    _syncFlux  = false;
    resetOnlyEcho();
}


void  ArnItemNet::resetDirtyMode()
{
    _dirtyMode = false;
}


bool  ArnItemNet::isDirtyValue()  const
{
    return _dirty;
}


bool  ArnItemNet::isDirtyMode()  const
{
    return _dirtyMode;
}


bool  ArnItemNet::isLeadValueUpdate()
{
    if (_dirty)  return false;

    _dirty = true;
    return true;
}


bool  ArnItemNet::isLeadModeUpdate()
{
    if (_dirtyMode)  return false;

    _dirtyMode = true;
    return true;
}


bool  ArnItemNet::isBlock( quint32 sendId)
{
    return (_blockEcho && (sendId == itemId()));  // Update was initiated from this Item, it can be blocked ...
}


void  ArnItemNet::setIniMaster( bool iniMaster)
{
    _iniMaster =  iniMaster;
    _iniSlave &= !iniMaster;
}


void  ArnItemNet::setIniSlave( bool iniSlave)
{
    _iniSlave   =  iniSlave;
    _iniMaster &= !iniSlave;
}


bool  ArnItemNet::isMasterAtStart()  const
{
    return !_iniSlave && (_iniMaster || isMaster());
}


void  ArnItemNet::setSyncFlux( bool isSyncFlux)
{
    _syncFlux = isSyncFlux;
}


bool  ArnItemNet::isSyncFlux()  const
{
    return _syncFlux;
}


quint32  ArnItemNet::localUpdateSinceStop()  const
{
    return localUpdateCount() - _updateCountStop;
}


void  ArnItemNet::onConnectStop()
{
    _updateCountStop = localUpdateCount();
}


void  ArnItemNet::arnEvent( QEvent* ev, bool isAlienThread)
{
    ArnBasicItem::arnEvent( ev, isAlienThread);
    if (ArnEvent::isArnEvent( ev->type())) {
        ArnEvent* e = static_cast<ArnEvent*>( ev);
        if (!e->target())  return;  // This ArnItemNet has been deleted
    }

    //// Must support threaded
    //// Only for a Monitor
    if (!_isMonitor)  return;

    int  evIdx = ev->type() - ArnEvent::baseType();
    switch (evIdx) {
    case ArnEvent::Idx::LinkCreate:
    {
        ArnEvLinkCreate*  e = static_cast<ArnEvLinkCreate*>( ev);
        if (e->isLastLink()) {
            // qDebug() << "ArnItemNet Mon create: path=" << e->path() << " inPath=" << path();
            sendNewItemMonEvent( e->path());
        }
        break;
    }
    case ArnEvent::Idx::ModeChange:
    {
        ArnEvModeChange*  e = static_cast<ArnEvModeChange*>( ev);
        Arn::XStringMap  xsm;
        xsm.add("path", e->path()).add("mode", ObjectModeToString( e->mode()));
        // qDebug() << "ArnItemNet Mode change: data=" << xsm.toXString() << " inPath=" << path();
        sendMonEvent( ArnMonEventType::ItemModeChg, xsm.toXString(), true);
        break;
    }
    default:;
    }
}



ArnItemNetEar::ArnItemNetEar( QObject* parent)
    : ArnItem( parent)
{
}


void  ArnItemNetEar::customEvent( QEvent* ev)
{
    int  evIdx = ev->type() - ArnEvent::baseType();
    switch (evIdx) {
    case ArnEvent::Idx::LinkCreate:
    {
        ArnEvLinkCreate*  e = static_cast<ArnEvLinkCreate*>( ev);
        if (e->isLastLink() && e->arnLink()->isFolder()) {
            // qDebug() << "ArnItemNetEar create tree: path=" << e->path();
            emit arnTreeCreated( e->path());
        }
        break;
    }
    case ArnEvent::Idx::Retired:
    {
        ArnEvRetired*  e = static_cast<ArnEvRetired*>( ev);
        QString  destroyPath = e->isBelow() ? e->startLink()->linkPath() : path();
        if (Arn::debugLinkDestroy)  qDebug() << "ArnItemNetEar retired: path=" << destroyPath
                                             << " isGlobal=" << e->isGlobal()
                                             << "inPath=" << path();
        if (e->startLink()->isFolder()) {
            emit arnTreeDestroyed( destroyPath, e->isGlobal());
        }
        break;
    }
    default:;
    }

    return ArnItem::customEvent( ev);
}
