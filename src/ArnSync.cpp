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

#include "ArnSync.hpp"
#include "ArnItemNet.hpp"
#include "ArnLink.hpp"
#include "ArnInc/ArnClient.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QTcpSocket>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <limits.h>

using Arn::XStringMap;


ArnSync::ArnSync( QTcpSocket *socket, bool isClientSide, QObject *parent)
    : QObject( parent)
{
    _socket          = socket;  // Note: client side does not own socket ...
    _isClientSide    = isClientSide;
    _isSending       = false;
    _isClosed        = isClientSide;  // Server start as not closed
    _queueNumCount   = 0;
    _queueNumDone    = 0;
    _dataRemain.clear();

    connect( _socket, SIGNAL(disconnected()), this, SLOT(disConnected()));
    connect( _socket, SIGNAL(readyRead()), this, SLOT(socketInput()));
    connect( _socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sendNext()));

    if (isClientSide) {
        _isConnected = false;
        connect( _socket, SIGNAL(connected()), this, SLOT(connected()));
    }
    else {
        _socket->setParent( this);  // Server side takes ownerchip of socket
        _isConnected = true;
    }
}


ArnSync::~ArnSync()
{
}


void  ArnSync::sendXSMap( const XStringMap& xsMap)
{
    send( xsMap.toXString());
}


void  ArnSync::send( const QByteArray& xString)
{
    if (!_isConnected) {
        return;
    }

    QByteArray  sendString;
    sendString += xString;
    if (Arn::debugRecInOut)  qDebug() << "Rec-Out: " << sendString;
    sendString += "\r\n";
    _socket->write( sendString);
}


/// Common setup of ItemNet for both server and client
void  ArnSync::setupItemNet( ArnItemNet* itemNet, uint netId)
{
    itemNet->setNetId( netId);
    _itemNetMap.insert( netId, itemNet);

    connect( itemNet, SIGNAL(goneDirty(const ArnLinkHandle&)),
             this, SLOT(addToFluxQue(const ArnLinkHandle&)));
    connect( itemNet, SIGNAL(goneDirtyMode()), this, SLOT(addToModeQue()));
    connect( itemNet, SIGNAL(arnLinkDestroyed()), this, SLOT(linkDestroyedHandle()));
    connect( itemNet, SIGNAL(arnEvent(QByteArray,QByteArray,bool)),
             this, SLOT(doArnEvent(QByteArray,QByteArray,bool)));
}


/// Client ...
ArnItemNet*  ArnSync::newNetItem( const QString& path,
                                  const QString& localMountPath, const QString& remoteMountPath,
                                  Arn::ObjectSyncMode syncMode, bool* isNewPtr)
{
    ArnItemNet*  itemNet = new ArnItemNet( this);
    if (!itemNet->open( path))  return 0;

    uint  netId = itemNet->linkId(); // Use clients linkId as netID for this Item
    if (_itemNetMap.contains( netId)) {  // Item is already synced by this client
        if (isNewPtr) {  // Allow duplicate ref, indicate this is not new
            delete itemNet;
            itemNet = _itemNetMap.value( netId, 0);
            itemNet->addSyncMode( syncMode, true);
            *isNewPtr = false;
            return itemNet;
        }
        else { // Not allow duplicate ref, return error;
            qDebug() << "Arn netSync Item already synced: path=" << itemNet->path();
            delete itemNet;
            return 0;
        }
    }
    if (isNewPtr)
        *isNewPtr = true;

    itemNet->setLocalMountPath( localMountPath);
    itemNet->setRemoteMountPath( remoteMountPath);
    itemNet->addSyncMode( syncMode, true);
    setupItemNet( itemNet, netId);
    itemNet->setBlockEcho( true);    // Client gives no echo to avoid endless looping

    if (_isClosed)  return itemNet;

    _syncQueue.enqueue( itemNet);
    // qDebug() << "Sync EnQueue: id=" << itemNet->netId() << " path=" << itemNet->path();

    if (!_isSending)
        sendNext();

    return itemNet;
}


void ArnSync::close()
{
    if (_isClosed)  return;

    _isClosed = true;
    if (!_isSending)
        sendNext();
    if (!_isConnected)
        clearQueues();
}


void ArnSync::closeFinal()
{
    sendExit();
    _socket->disconnectFromHost();
    _isConnected = false;
}


void ArnSync::clearQueues()
{
    _syncQueue.clear();
    _modeQueue.clear();
    _fluxItemQueue.clear();
    _fluxRecPool += _fluxPipeQueue;
    _fluxPipeQueue.clear();
}


void  ArnSync::socketInput()
{
    _dataReadBuf.resize( _socket->bytesAvailable());
    int nbytes = _socket->read( _dataReadBuf.data(), _dataReadBuf.size());
    if (nbytes <= 0)  return; // No bytes / error
    if (_isClosed)  return;

    _dataReadBuf.resize( nbytes);
    _dataRemain += _dataReadBuf;

    QByteArray  xString;
    int pos;
    while ((pos = _dataRemain.indexOf("\n")) >= 0) {
        // Set xString to string before \n
        xString.resize(0);
        xString.append( _dataRemain.constData(), pos);
        _dataRemain.remove(0, pos + 1);     // Set remain to string after \n

        xString.replace('\r', "");         // Remove any \r
        _commandMap.fromXString( xString); // Load command Map
        _replyMap.clear();                  // Reset reply Map

        if (Arn::debugRecInOut)  qDebug() << "Rec-in: " << xString;
        doCommand();

        if (_replyMap.size()) {
            sendXSMap( _replyMap);
            // _replySendingCount++;
            // cout << "REPLY: |" << _replyMap.toXString() << "|" << endl;
        }
    }
}


void  ArnSync::doCommand()
{
    // int stat = 0;
    uint stat = ArnError::Ok;
    QByteArray command = _commandMap.value(0);

    /// Received replies
    if (command.startsWith('R')) {
        emit replyRecord( _commandMap);
    }
    /// Received commands
    else if (command == "flux") {
        stat = doCommandFlux();
    }
    else if (command == "event") {
        stat = doCommandEvent();
    }
    else if (command == "get") {
        stat = doCommandGet();
    }
    else if (command == "set") {
        stat = doCommandSet();
    }
    else if (command == "sync") {
        stat = doCommandSync();
    }
    else if (command == "mode") {
        stat = doCommandMode();
    }
    else if (command == "nosync") {
        stat = doCommandNoSync();
    }
    else if (command == "destroy") {
        stat = doCommandDestroy();
    }
    else if (command == "ls") {
        stat = doCommandLs();
    }
    else if (command == "ver") {
        _replyMap.add(ARNRECNAME, "Rver").add("data", "Arn ver 1.0");
    }
    else if (command == "exit") {
        _socket->disconnectFromHost();
    }
    /// Error for Server or Client
    else if (command == "err") {
        qDebug() << "REC-ERR: |" << _commandMap.toXString() << "|";
    }
    else {
        stat = ArnError::RecUnknown;
        _replyMap.add(ARNRECNAME, "err");
        _replyMap.add("data", QByteArray("Unknown record:") + command);
    }
    if (_replyMap.size()) {
        _replyMap.add("stat", QByteArray::number( stat));
    }
}


uint  ArnSync::doCommandSync()
{
    QByteArray   path = _commandMap.value("path");
    QByteArray  smode = _commandMap.value("smode");
    uint        netId = _commandMap.value("id").toUInt();

    ArnItemNet*  itemNet = new ArnItemNet;
    if (!itemNet->open( path)) {
        delete itemNet;
        return ArnError::CreateError;
    }

    setupItemNet( itemNet, netId);
    itemNet->addSyncModeString( smode, false);  // SyncMode is only for the item (session), not the link

    Arn::ObjectSyncMode  syncMode = itemNet->syncMode();
    if (syncMode.is( syncMode.Monitor)) {
        setupMonitorItem( itemNet);
    }
    if (!itemNet->getModeString().isEmpty()) {   // If non default mode
        itemNet->modeUpdate( Arn::ObjectMode());    // Make server send the current mode to client
    }
    if ((itemNet->type() != Arn::DataType::Null)
    && !(itemNet->syncMode().is( syncMode.Master))) {  // Only send non Null Value to non master
        itemNet->itemUpdated( ArnLinkHandle()); // Make server send the current value to client
    }

    return ArnError::Ok;
}


/// Can be called booth for remote and local monitoring
void  ArnSync::setupMonitorItem(ArnItemNet *itemNet)
{
    //// Activate NewItemEvent to be sent for future created posterity (also not direct children)
    itemNet->setMonitor( true);

    //// Send NewItemEvent for any existing (direct) children (also folders)
    doChildsToEvent( itemNet);
}


void  ArnSync::doChildsToEvent( ArnItemNet *itemNet)
{
    //// Send NewItemEvent for any existing direct children (also folders)
    QString  path = itemNet->path();
    QStringList  childList = itemNet->childItemsMain();
    foreach (QString childName, childList) {
        itemNet->emitNewItemEvent( Arn::makePath( path, childName), true);
    }
}


uint  ArnSync::doCommandMode()
{
    uint  netId = _commandMap.value("id").toUInt();
    QByteArray  data = _commandMap.value("data");

    ArnItemNet*  itemNet = _itemNetMap.value( netId, 0);
    if (!itemNet) {
        return ArnError::NotFound;
    }

    itemNet->setModeString( data);
    return ArnError::Ok;
}


uint ArnSync::doCommandNoSync()
{
    uint  netId    = _commandMap.value("id").toUInt();

    ArnItemNet*  itemNet = _itemNetMap.value( netId, 0);
    if (!itemNet) {  // Not existing item is ok, maybe destroyed before sync
        return ArnError::Ok;
    }

    removeItemNet( itemNet);

    return ArnError::Ok;
}


uint  ArnSync::doCommandDestroy()
{
    uint  netId    = _commandMap.value("id").toUInt();

    ArnItemNet*  itemNet = _itemNetMap.value( netId, 0);
    if (!itemNet) {  // Not existing item is ok, maybe destroyed before sync
        return ArnError::Ok;
    }

    itemNet->setDisable();  // Defunc the item to prevent sending destroy record (MW: problem with twin)
    itemNet->destroyLink();

    return ArnError::Ok;
}


uint  ArnSync::doCommandFlux()
{
    uint       netId = _commandMap.value("id").toUInt();
    QByteArray  type = _commandMap.value("type");
    QByteArray  nqrx = _commandMap.value("nqrx");
    QByteArray  seq  = _commandMap.value("seq");
    QByteArray  data = _commandMap.value("data");

    bool  isOnlyEcho = type.contains("E");

    ArnLinkHandle  handleData;
    if (!nqrx.isEmpty())
        handleData.add( ArnLinkHandle::QueueFindRegexp,
                        QVariant( QRegExp( QString::fromUtf8( nqrx.constData(), nqrx.size()))));

    if (!seq.isEmpty())
        handleData.add( ArnLinkHandle::SeqNo,
                        QVariant( seq.toInt()));

    ArnItemNet*  itemNet = _itemNetMap.value( netId, 0);
    if (!itemNet) {
        return ArnError::NotFound;
    }

    bool  isIgnoreSame = isOnlyEcho;
    if (!isOnlyEcho || !itemNet->getMode().is( Arn::ObjectMode::Pipe))  // Echo to Pipe is ignored
        itemNet->arnImport( data, isIgnoreSame, handleData);
    return ArnError::Ok;
}


uint  ArnSync::doCommandEvent()
{
    uint       netId = _commandMap.value("id").toUInt();
    QByteArray  type = _commandMap.value("type");
    QByteArray  data = _commandMap.value("data");

    ArnItemNet*  itemNet = _itemNetMap.value( netId, 0);
    if (!itemNet) {
        return ArnError::NotFound;
    }

    itemNet->emitArnEvent( type, data, false);
    return ArnError::Ok;
}


uint  ArnSync::doCommandSet()
{
    QByteArray  path = _commandMap.value("path");
    QByteArray  data = _commandMap.value("data");

    _replyMap.add(ARNRECNAME, "Rset").add("path", path);

    ArnItem  item;
    if (!item.open( path)) {
        return ArnError::CreateError;
    }

    item.arnImport( data);
    return ArnError::Ok;
}


uint  ArnSync::doCommandGet()
{
    QByteArray  path = _commandMap.value("path");

    _replyMap.add(ARNRECNAME, "Rget").add("path", path);

    ArnItem  item;
    if (!item.open( path)) {
        return ArnError::CreateError;
    }

    _replyMap.add("data", item.arnExport());
    return 0;
}


uint  ArnSync::doCommandLs()
{
    QByteArray  path = _commandMap.value("path");
    _replyMap.add(ARNRECNAME, "Rls").add("path", path);

   if (ArnM::isFolder( path)) {
        QStringList  subitems = ArnM::items( path);
        int  nItems = subitems.size();

        for (int i = 0; i < nItems; ++i) {
            _replyMap.add("item", uint(i + 1), subitems.at(i));
        }
    }
    else {
        return ArnError::NotFound;
    }

    return ArnError::Ok;
}


void  ArnSync::connected()
{
    bool  wasClosed = _isClosed;
    _isConnected = true;
    _isClosed    = false;
    _syncQueue.clear();
    if (wasClosed)
        clearQueues();

    /// All the existing netItems must be synced
    ArnItemNet*  itemNet;
    QByteArray  mode;
    QMapIterator<uint,ArnItemNet*>  i( _itemNetMap);
    while (i.hasNext()) {
        i.next();
        itemNet = i.value();

        if (wasClosed) {
            itemNet->resetDirty();
            itemNet->resetDirtyMode();
        }

        _syncQueue.enqueue( itemNet);
        mode = itemNet->getModeString();
        // If non default mode that isn't already in modeQueue
        if (!mode.isEmpty()  &&  !itemNet->isDirtyMode()) {
            _modeQueue.enqueue( itemNet);
        }
        if ((itemNet->type() != Arn::DataType::Null)                  // Only send non Null Value ...
        && (!itemNet->isPipeMode())                                   // from non pipe ..
        && (itemNet->syncMode().is( Arn::ObjectSyncMode::Master))) {  // which is master
            itemNet->itemUpdated( ArnLinkHandle());  // Make client send the current value to server
        }
    }
    sendNext();
}


void  ArnSync::disConnected()
{
    _isConnected = false;
    _isSending = false;

    if (_isClientSide) {  // Client
        if (_isClosed) {
            clearQueues();
        }
    }
    else {  // Server
        // Make a list of netId to AutoDestroy
        QList<uint>  destroyList;
        foreach (ArnItemNet* itemNet, _itemNetMap) {
            if (itemNet->syncMode().is( Arn::ObjectSyncMode::AutoDestroy)) {
                destroyList += itemNet->netId();
                // qDebug() << "Server-disconnect: destroyList path=" << itemNet->path();
            }
        }
        // Destroy from list, twins dissapears in pair
        foreach (uint netId, destroyList) {
            ArnItemNet* itemNet = _itemNetMap.value( netId, 0);
            if (itemNet) {  // if this itemNet still exist
                // qDebug() << "Server-disconnect: Destroy path=" << itemNet->path();
                itemNet->destroyLink( true);  // The itemNet will be destroyed (gblobally)
            }
        }

        deleteLater();
    }
}


void  ArnSync::linkDestroyedHandle()
{
    ArnItemNet*  itemNet = qobject_cast<ArnItemNet*>( sender());
    if (!itemNet) {
        ArnM::errorLog( QString(tr("Can't get ArnItemNet sender for itemRemove")),
                            ArnError::Undef);
        return;
    }

    if (Arn::debugLinkDestroy)  qDebug() << "itemRemove: netId=" << itemNet->netId() << " path=" << itemNet->path();
    removeItemNet( itemNet);
    destroyToFluxQue( itemNet);
}


void  ArnSync::removeItemNet( ArnItemNet* itemNet)
{
    if (!itemNet)  return;

    int s;
    s = _itemNetMap.remove( itemNet->netId());
    // qDebug() << "... remove from itemMap num=" << s;
    s = _syncQueue.removeAll( itemNet);
    // qDebug() << "... remove from syncQueue num=" << s;
    s = _modeQueue.removeAll( itemNet);
    // qDebug() << "... remove from modeQueue num=" << s;
    s = _fluxItemQueue.removeAll( itemNet);
    // qDebug() << "... remove from fluxQueue num=" << s;
    ++s;  // Gets rid of warning

    itemNet->deleteLater();
}


void  ArnSync::doArnEvent( const QByteArray& type, const QByteArray& data, bool isLocal)
{
    ArnItemNet*  itemNet = qobject_cast<ArnItemNet*>( sender());
    if (!itemNet) {
        ArnM::errorLog( QString(tr("Can't get ArnItemNet sender for doArnEvent")),
                            ArnError::Undef);
        return;
    }

    if (isLocal) {  // Local events are always sent to remote side
        eventToFluxQue( itemNet->netId(), type, data);
    }

    if (type == "monitorStart") {
        if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: monitorStart Event";

        Arn::ObjectSyncMode  syncMode = itemNet->syncMode();
        if (isLocal && _isClientSide) {  // Client Side
            itemNet->addSyncMode( syncMode.Monitor, true);  // Will demand monitor if resynced (e.g. server restart)
        }
        else if (!isLocal && !_isClientSide && !syncMode.is( syncMode.Monitor)) {  // Server side
            setupMonitorItem( itemNet);  // Item will function as a Monitor
            itemNet->addSyncMode( syncMode.Monitor, false);  // Indicate is Monitor, prevent duplicate setup
        }
    }
    if (type == "monitorReStart") {
        if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: monitorReStart Event";

        if (!isLocal && !_isClientSide) {  // Server side
            //// Send NewItemEvent for any existing direct children (also folders)
            doChildsToEvent( itemNet);
        }
    }
}


void  ArnSync::addToFluxQue( const ArnLinkHandle& handleData)
{
    if (_isClosed)  return;

    ArnItemNet*  itemNet = qobject_cast<ArnItemNet*>( sender());
    if (!itemNet) {
        ArnM::errorLog( QString(tr("Can't get ArnItemNet sender for itemChanged")),
                            ArnError::Undef);
        return;
    }

    if (itemNet->isPipeMode()) {
        if (itemNet->isOnlyEcho()) {
            // qDebug() << "Flux skip pipe echo: path=" << itemNet->path() << " data=" << itemNet->arnExport()
            //          << "itemId=" << itemNet->itemId();
            itemNet->resetDirty();  // Arm for more updates
            return;  // Don't send any echo to a Pipe
        }

        FluxRec*  fluxRec = getFreeFluxRec();
        fluxRec->xString += makeFluxString( itemNet, handleData);
        itemNet->resetDirty();
        if (handleData.has( ArnLinkHandle::QueueFindRegexp)) {
            QRegExp  rx = handleData.valueRef( ArnLinkHandle::QueueFindRegexp).toRegExp();
            // qDebug() << "AddFluxQueue Pipe QOW: rx=" << rx.pattern();
            int i;
            for (i = 0; i < _fluxPipeQueue.size(); ++i) {
                FluxRec*&  fluxRecQ = _fluxPipeQueue[i];
                _syncMap.fromXString( fluxRecQ->xString);
                QString  fluxDataStrQ = _syncMap.valueString("data");
                if (rx.indexIn( fluxDataStrQ) >= 0) {  // Match
                    // qDebug() << "AddFluxQueue Pipe QOW match: old:"
                    //          << fluxRecQ->xString << "  new:" << fluxRec->xString;
                    _fluxRecPool += fluxRecQ;  // Free item to be replaced
                    fluxRecQ = fluxRec;
                    i = -1;  // Mark match
                    break;
                }
            }
            if (i >= 0) {  // No match
                // qDebug() << "AddFluxQueue Pipe QOW nomatch:"
                //          << fluxRec->xString;
                _fluxPipeQueue.enqueue( fluxRec);
            }
        }
        else {  // Normal Pipe
            // qDebug() << "AddFluxQueue Pipe:"
            //          << fluxRec->xString;
            _fluxPipeQueue.enqueue( fluxRec);
        }
    }
    else {  // Normal Item
        if (itemNet->syncMode().is( Arn::ObjectSyncMode::Master)
        &&  itemNet->isOnlyEcho())
        {
            itemNet->resetDirty();  // Arm for more updates
            return;  // Don't send any echo to a Master
        }

        itemNet->setQueueNum( ++_queueNumCount);
        _fluxItemQueue.enqueue( itemNet);
    }

    if (!_isSending) {
        sendNext();
    }
}


void  ArnSync::eventToFluxQue( uint netId, const QByteArray& type, const QByteArray& data)
{
    if (!netId)  return;  // Not valid id, item was disabled
    if (_isClosed)  return;

    FluxRec*  fluxRec = getFreeFluxRec();
    _syncMap.clear();
    _syncMap.add(ARNRECNAME, "event");
    _syncMap.add("id", QByteArray::number( netId));
    _syncMap.add("type", type);
    _syncMap.add("data", data);
    fluxRec->xString += _syncMap.toXString();
    _fluxPipeQueue.enqueue( fluxRec);

    if (!_isSending) {
        sendNext();
    }
}


void  ArnSync::destroyToFluxQue( ArnItemNet* itemNet)
{
    if (itemNet->isDisable())  return;
    if (_isClosed)  return;

    bool  isGlobal = itemNet->isRetiredGlobal() || !_isClientSide;  // Server allways Global destroy
    FluxRec*  fluxRec = getFreeFluxRec();
    _syncMap.clear();
    _syncMap.add(ARNRECNAME, isGlobal ? "destroy" : "nosync")
            .add("id", QByteArray::number( itemNet->netId()));
    fluxRec->xString += _syncMap.toXString();
    _fluxPipeQueue.enqueue( fluxRec);

    if (!_isSending) {
        sendNext();
    }
}


ArnSync::FluxRec*  ArnSync::getFreeFluxRec()
{
    FluxRec*  fluxRec;

    if (_fluxRecPool.empty()) {
        fluxRec = new FluxRec;
    }
    else {
        fluxRec = _fluxRecPool.takeLast();
    }
    fluxRec->xString.resize(0);
    fluxRec->queueNum = ++_queueNumCount;

    return fluxRec;
}


void  ArnSync::addToModeQue()
{
    if (_isClosed)  return;

    ArnItemNet*  itemNet = qobject_cast<ArnItemNet*>( sender());
    if (!itemNet) {
        ArnM::errorLog( QString(tr("Can't get ArnItemNet sender for itemModeChanged")),
                            ArnError::Undef);
        return;
    }

    _modeQueue.enqueue( itemNet);

    if (!_isSending) {
        sendNext();
    }
}


void  ArnSync::sendNext()
{
    _isSending = false;

    if (!_isConnected || !_socket->isValid())  return;

    ArnItemNet*  itemNet;

    if (!_syncQueue.isEmpty()) {
        itemNet = _syncQueue.dequeue();
        sendSyncItem( itemNet);
        _isSending = true;
    }
    else if (!_modeQueue.isEmpty()) {
        itemNet = _modeQueue.dequeue();
        sendModeItem( itemNet);
        itemNet->resetDirtyMode();
        _isSending = true;
    }
    else {  // Flux queues - send entity with lowest queue number
        int  itemQueueNum = _fluxItemQueue.isEmpty() ? _queueNumDone + INT_MAX : _fluxItemQueue.head()->queueNum();
        int  pipeQueueNum = _fluxPipeQueue.isEmpty() ? _queueNumDone + INT_MAX : _fluxPipeQueue.head()->queueNum;
        int  itemQueueRel = itemQueueNum - _queueNumDone;
        int  pipeQueueRel = pipeQueueNum - _queueNumDone;

        if ((itemQueueRel < INT_MAX) || (pipeQueueRel < INT_MAX)) { // At least 1 flux queue not empty
            if (itemQueueRel < pipeQueueRel) {  // Item flux queue
                _queueNumDone = itemQueueNum;

                itemNet = _fluxItemQueue.dequeue();
                sendFluxItem( itemNet);
                itemNet->resetDirty();
            }
            else {  // Pipe flux queue
                _queueNumDone = pipeQueueNum;

                FluxRec*  fluxRec = _fluxPipeQueue.dequeue();
                _fluxRecPool += fluxRec;
                send( fluxRec->xString);
            }
            _isSending = true;
        }
        else {  // Nothing more to send
            if (_isClosed)
                closeFinal();
        }
    }
}


QByteArray  ArnSync::makeFluxString(const ArnItemNet* itemNet, const ArnLinkHandle& handleData)
{
    QByteArray  type;
    if (itemNet->isOnlyEcho())  type += "E";

    _syncMap.clear();
    _syncMap.add(ARNRECNAME, "flux").add("id", QByteArray::number( itemNet->netId()));

    if (!type.isEmpty())
        _syncMap.add("type", type);

    if (handleData.has( ArnLinkHandle::QueueFindRegexp))
        _syncMap.add("nqrx", handleData.valueRef( ArnLinkHandle::QueueFindRegexp).toRegExp().pattern());
    else if (handleData.has( ArnLinkHandle::SeqNo))
        _syncMap.add("seq", QByteArray::number( handleData.valueRef( ArnLinkHandle::SeqNo).toInt()));

    _syncMap.add("data", itemNet->arnExport());

    return _syncMap.toXString();
}


void  ArnSync::sendFluxItem( const ArnItemNet* itemNet)
{
    if (!itemNet || !itemNet->isOpen()) {
        sendNext();  // Warning: this is recursion while not existing items
        return;
    }

    send( makeFluxString( itemNet, ArnLinkHandle()));
}


void  ArnSync::sendSyncItem( ArnItemNet* itemNet)
{
    if (!itemNet  ||  !itemNet->isOpen()) {
        sendNext();  // Warning: this is recursion while not existing items
        return;
    }

    _syncMap.clear();
    _syncMap.add(ARNRECNAME, "sync");
    _syncMap.add("path", itemNet->toRemotePath());
    _syncMap.add("id", QByteArray::number( itemNet->netId()));
    QByteArray  smode = itemNet->getSyncModeString();
    if (!smode.isEmpty()) {
        _syncMap.add("smode", smode);
    }

    if (Arn::debugShareObj)  qDebug() << "Send sync: localPath=" << itemNet->path()
                                      << ", " << _syncMap.toXString();
    sendXSMap( _syncMap);
}


void  ArnSync::sendModeItem( ArnItemNet* itemNet)
{
    if (!itemNet  ||  !itemNet->isOpen()) {
        sendNext();  // Warning: this is recursion while not existing items
        return;
    }

    _syncMap.clear();
    _syncMap.add(ARNRECNAME, "mode");
    _syncMap.add("id", QByteArray::number( itemNet->netId()));
    _syncMap.add("data", itemNet->getModeString());
    sendXSMap( _syncMap);
}


void ArnSync::sendExit()
{
    XStringMap xm;
    xm.add(ARNRECNAME, "exit");

    sendXSMap( xm);
}

