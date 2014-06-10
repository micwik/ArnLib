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
#include "ArnInc/ArnClient.hpp"
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


void  ArnMonitor::setClient(ArnClient *client, QString id)
{
    _arnClient = client;
    if (_arnClient && !id.isNull())
        _arnClient->setId( id);
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


void  ArnMonitor::setMonitorPath( QString path, ArnClient *client)
{
    start( path, client ? client : _arnClient.data());
}


bool ArnMonitor::start(const QString& path, ArnClient* client)
{
    _arnClient = client;
    _monitorPath = Arn::fullPath( path);
    if (!_monitorPath.endsWith("/"))  _monitorPath += "/";

    if (_arnClient) {
        // No recursion due to no emit of arnItemCreated as this is a folder
        // ItemNet is not threadsafe, only use in connect etc
        bool isNew;
        _itemNet = _arnClient->newNetItem( _monitorPath, Arn::ObjectSyncMode::Normal, &isNew);
        if (!_itemNet)  return false;

        connect( _itemNet, SIGNAL(arnEvent(QByteArray,QByteArray,bool)),
                 this, SLOT(dispatchArnEvent(QByteArray,QByteArray,bool)));

        if (Arn::debugMonitor)  qDebug() << "Monitor start: new=" << isNew << " path=" << _monitorPath;
        if (isNew) {
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorStart Before";
            QMetaObject::invokeMethod( this,
                                       "emitArnEvent",
                                       Qt::QueuedConnection,  // make sure started after all setup is done in this thread
                                       Q_ARG( QByteArray, "monitorStart"));
            //// Check thread event sequence by big delay
            // QTime  ts = QTime::currentTime();
            // while (QTime::currentTime() < ts.addSecs(5));
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorStart After (delay)";
        }
        else {
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorReStart Before";
            QMetaObject::invokeMethod( this,
                                       "emitArnEvent",
                                       Qt::QueuedConnection,  // make sure restarted after all setup is done in this thread
                                       Q_ARG( QByteArray, "monitorReStart"));
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

        connect( _itemNet, SIGNAL(arnEvent(QByteArray,QByteArray,bool)),
                 this, SLOT(dispatchArnEvent(QByteArray,QByteArray,bool)), Qt::QueuedConnection);
        //MW: Todo connect( _itemNet, SIGNAL(arnLinkDestroyed()),

        if (Arn::debugMonitor)  qDebug() << "Monitor start (local-items): path="
                                         << _monitorPath << " id=" << _itemNet->netId();
        QMetaObject::invokeMethod( this,
                                   "setupLocalMonitorItem",
                                   Qt::QueuedConnection  // make sure called after all setup is done in this thread
                                   );
    }
    return true;
}


void  ArnMonitor::reStart()
{
    _foundChilds.clear();
    emitArnEvent("monitorReStart");
}


void  ArnMonitor::emitArnEvent( QByteArray type, QByteArray data)
{
    if (!_itemNet)  return;

    QMetaObject::invokeMethod( _itemNet,
                               "emitArnEvent",
                               Qt::AutoConnection,
                               Q_ARG( QByteArray, type),
                               Q_ARG( QByteArray, data));
}


void  ArnMonitor::setupLocalMonitorItem()
{
    ArnSync::setupMonitorItem( _itemNet);
}


void  ArnMonitor::dispatchArnEvent( QByteArray type, QByteArray data, bool isLocal)
{
    ArnItemNet*  itemNet = qobject_cast<ArnItemNet*>( sender());
    if (!itemNet) {
        ArnM::errorLog( QString(tr("Can't get ArnItemNet sender for dispatchArnEvent")),
                            ArnError::Undef);
        return;
    }

    if (Arn::debugMonitor && isLocal) {
        qDebug() << "Dipatch Arn event (local): type=" << type
                 << " data=" << data << " id=" << itemNet->netId();
    }

    QString  foundRemotePath = QString::fromUtf8( data.constData(), data.size());
    QString  foundPath = itemNet->toLocalPath( foundRemotePath);

    if (type == "itemCreated") {
        //// "created" (new fresh) is special case of "found"
        emit arnItemCreated( foundPath);
    }
    else if (type == "itemFound") {
    }
    else if ((type == "monitorReStart") && !_arnClient) {  // Local monitor event
        //// Send NewItemEvent for any existing direct children (also folders)
        ArnSync::doChildsToEvent( itemNet);
        return;
    }
    else {
        return;  // This event is not handled
    }

    //// Continue with found items
    if (Arn::debugMonitor && !isLocal) {
        qDebug() << "Dipatch Arn event: type=" << type
                 << " remotePath=" << foundRemotePath << " localPath=" << foundPath;
    }

    QString childPath = Arn::childPath( _monitorPath, foundPath);
    // qDebug() << "### arnMon dispArnEv: childPath=" << childPath;
    if (!childPath.isEmpty()) {  // Should always be none emty ...
        if (!_foundChilds.contains( childPath)) {  // Only send events about unknown items
            _foundChilds += childPath;
            // qDebug() << "### arnMon dispArnEv: Emit chFound childPath=" << childPath;
            emit arnChildFound( childPath);
            if (childPath.endsWith('/'))  emit arnChildFoundFolder( childPath);
            else                          emit arnChildFoundLeaf( childPath);
        }
    }
}


void  ArnMonitor::foundChildDeleted( QString path)
{
    int  i = _foundChilds.indexOf( path);
    // qDebug() << "### arnMon foundChDel: path=" << path << " i=" << i;
    if (i >= 0) {
        _foundChilds.removeAt(i);
    }
}
