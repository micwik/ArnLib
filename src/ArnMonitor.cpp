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

#include "ArnInc/ArnMonitor.hpp"
#include "ArnInc/ArnMonEvent.hpp"
#include "ArnInc/ArnClient.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnSync.hpp"
#include "ArnItemNet.hpp"
#include <QDebug>
#include <QTime>

ArnMonitor::ArnMonitor( QObject* parent) :
    QObject( parent)
{
    _arnClient = 0;
    _itemNet = 0;
}


void  ArnMonitor::setClient(ArnClient *client)
{
    _arnClient = client;
}


void  ArnMonitor::setClient( const QString& id)
{
    _arnClient = ArnClient::getClient( id);
}


QString  ArnMonitor::clientId()  const
{
    if (!_arnClient)  return QString();
    return _arnClient->id();
}


ArnClient*  ArnMonitor::client()  const
{
    return _arnClient;
}


void  ArnMonitor::setMonitorPath( const QString& path, ArnClient *client)
{
    start( path, client ? client : _arnClient.data());
}


bool  ArnMonitor::start( const QString& path, ArnClient* client)
{
    _arnClient = client;
    _monitorPath = inPathConvert( Arn::fullPath( path));
    if (!_monitorPath.endsWith("/"))  _monitorPath += "/";

    if (_itemNet) {
        disconnect( _itemNet, 0, this, 0);
        if (_itemNet->parent() == this)
            _itemNet->deleteLater();
        _itemNet = 0;
    }

    if (_arnClient) {
        // No recursion due to no emit of arnItemCreated as this is a folder
        // ItemNet is not threadsafe, only use in connect etc
        bool isNew;
        _itemNet = _arnClient->newNetItem( _monitorPath, Arn::ObjectSyncMode::Normal, &isNew);
        if (!_itemNet)  return false;

        connect( _itemNet, SIGNAL(arnMonEvent(int,QByteArray,bool)),
                 this, SLOT(dispatchArnMonEvent(int,QByteArray,bool)));

        if (Arn::debugMonitor)  qDebug() << "Monitor start: new=" << isNew << " path=" << _monitorPath;
        if (isNew) {
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorStart Before";
            QMetaObject::invokeMethod( this,
                                       "emitArnMonEvent",
                                       Qt::QueuedConnection,  // make sure started after all setup is done in this thread
                                       Q_ARG( int, ArnMonEvent::Type::MonitorStart));
            //// Check thread event sequence by big delay
            // QTime  ts = QTime::currentTime();
            // while (QTime::currentTime() < ts.addSecs(5));
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorStart After (delay)";
        }
        else {
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorReStart Before";
            QMetaObject::invokeMethod( this,
                                       "emitArnMonEvent",
                                       Qt::QueuedConnection,  // make sure restarted after all setup is done in this thread
                                       Q_ARG( int, ArnMonEvent::Type::MonitorReStart));
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorReStart After (delay)";
        }
    }
    else {  // No client, do local monitor
        _itemNet = new ArnItemNet( this);
        if (!_itemNet->open( _monitorPath)) {
            delete _itemNet;
            _itemNet = 0;
            return false;
        }
        _itemNet->setNetId( _itemNet->linkId());

        connect( _itemNet, SIGNAL(arnMonEvent(int,QByteArray,bool)),
                 this, SLOT(dispatchArnMonEvent(int,QByteArray,bool)), Qt::QueuedConnection);
        //MW: Todo connect( _itemNet, SIGNAL(arnLinkDestroyed()),

        if (Arn::debugMonitor)  qDebug() << "Monitor start (local-items): path="
                                         << _monitorPath << " id=" << _itemNet->netId();
        QMetaObject::invokeMethod( this,
                                   "setupLocalMonitorItem",
                                   Qt::QueuedConnection  // make sure called after all setup is done in this thread
                                   );
    }

    connect( _itemNet, SIGNAL(arnLinkDestroyed()), this, SIGNAL(monitorClosed()));
    return true;
}


void  ArnMonitor::reStart()
{
    _foundChilds.clear();
    emitArnMonEvent( ArnMonEvent::Type::MonitorReStart);
}


void  ArnMonitor::emitArnMonEvent( int type, const QByteArray& data)
{
    if (!_itemNet)  return;

    QMetaObject::invokeMethod( _itemNet,
                               "emitArnMonEvent",
                               Qt::AutoConnection,
                               Q_ARG( int, type),
                               Q_ARG( QByteArray, data));
}


void  ArnMonitor::setupLocalMonitorItem()
{
    ArnSync::setupMonitorItem( _itemNet);
}


void  ArnMonitor::dispatchArnMonEvent( int type, const QByteArray& data, bool isLocal)
{
    if (Arn::debugMonitor && isLocal) {
        qDebug() << "Dipatch Arn event (local): type=" << ArnMonEvent::idToText( type)
                 << " data=" << data << " monPath=" << _monitorPath;
    }

    switch (type) {
    case ArnMonEvent::Type::ItemCreated:
        // Fall throu
    case ArnMonEvent::Type::ItemFound:
        doEventItemFoundCreated( type, data, isLocal);
        break;
    case ArnMonEvent::Type::ItemDeleted:
        doEventItemDeleted( data, isLocal);
        break;
    case ArnMonEvent::Type::MonitorReStart:
        if (!_arnClient) {  // Local monitor event
            //// Send NewItemEvent for any existing direct children (also folders)
            ArnSync::doChildsToEvent( _itemNet);
        }
        break;
    default:
        break;  // This event is not handled
    }
}


void ArnMonitor::doEventItemFoundCreated( int type, const QByteArray& data, bool isLocal)
{
    QString  foundRemotePath = QString::fromUtf8( data.constData(), data.size());
    QString  foundLocalPath  = _itemNet->toLocalPath( foundRemotePath);

    if (type == ArnMonEvent::Type::ItemCreated) {
        // "created" (new fresh) is special case of "found"
        emit arnItemCreated( outPathConvert( foundLocalPath));
    }

    if (Arn::debugMonitor && !isLocal) {
        qDebug() << "Dipatch Arn event: type=" << ArnMonEvent::idToText( type)
                 << " remotePath=" << foundRemotePath << " localPath=" << foundLocalPath
                 << " monPath=" << _monitorPath;
    }

    QString childPath = Arn::childPath( _monitorPath, foundLocalPath);
    // qDebug() << "### arnMon dispArnEv: childPath=" << childPath;
    if (!childPath.isEmpty()) {  // Should always be none emty ...
        if (!_foundChilds.contains( childPath)) {  // Only send events about unknown items
            _foundChilds += childPath;
            // qDebug() << "### arnMon dispArnEv: Emit chFound childPath=" << childPath;
            emit arnChildFound( outPathConvert( childPath));
            if (childPath.endsWith('/'))  emit arnChildFoundFolder( outPathConvert( childPath));
            else                          emit arnChildFoundLeaf( outPathConvert( childPath));
        }
    }
}


void ArnMonitor::doEventItemDeleted( const QByteArray& data, bool isLocal)
{
    QString  delRemotePath = QString::fromUtf8( data.constData(), data.size());
    QString  delLocalPath  = _itemNet->toLocalPath( delRemotePath);

    if (Arn::debugMonitor && !isLocal) {
        qDebug() << "Deleted Arn event: remotePath=" << delRemotePath
                 << " localPath=" << delLocalPath << " monPath=" << _monitorPath;
    }

    QString  childPath = Arn::childPath( _monitorPath, delLocalPath);
    if (childPath == delLocalPath) {  // A child has been deleted
        foundChildDeleted( delLocalPath);
        emit arnChildDeleted( childPath);
    }
    emit arnItemDeleted( delLocalPath);
}


void  ArnMonitor::foundChildDeleted( const QString& path)
{
    int  i = _foundChilds.indexOf( inPathConvert( path));
    // qDebug() << "### arnMon foundChDel: path=" << path << " i=" << i;
    if (i >= 0) {
        _foundChilds.removeAt(i);
    }
}


QString  ArnMonitor::outPathConvert( const QString& path)
{
    return path;  // No conversion as standard
}


QString  ArnMonitor::inPathConvert( const QString& path)
{
    return path;  // No conversion as standard
}
