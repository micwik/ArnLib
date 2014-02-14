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

#include "ArnInc/ArnMonitor.hpp"
#include "ArnInc/ArnClient.hpp"
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


void  ArnMonitor::setMonitorPath( QString path, ArnClient *client)
{
    if (client)
        _arnClient = client;
    _monitorPath = path;
    if (!_monitorPath.endsWith("/"))  _monitorPath += "/";

    if (_arnClient) {
        // No recursion due to no emit of arnItemCreated as this is a folder
        // ItemNet is not threadsafe, only use in connect etc
        //_itemNet = _arnClient->newNetItem( _monitorPath, ArnItem::SyncMode::Monitor);
        bool isNew;
        _itemNet = _arnClient->newNetItem( _monitorPath, ArnItem::SyncMode::Normal, &isNew);
        if (!_itemNet)  return;

        connect( _itemNet, SIGNAL(arnEvent(QByteArray,QByteArray,bool)),
                 this, SLOT(dispatchArnEvent(QByteArray,QByteArray,bool)));

        if (isNew) {
            if (Arn::debugMonitor)  qDebug() << "ArnMonitor-Test: Invoke monitorStart Before";
            QMetaObject::invokeMethod( this,
                                       "emitArnEvent",
                                       Qt::QueuedConnection,  // make sure started after all setup is done in this thread
                                       Q_ARG( QByteArray, "monitorStart"));
            //// Check thread event sequence by big delay
            // QTime  ts = QTime::currentTime();
            // while (QTime::currentTime() < ts.addSecs(5));
            if (Arn::debugMonitor)  qDebug() << "ArnMonitor-Test: Invoke monitorStart After (delay)";
        }
        else {
            if (Arn::debugMonitor)  qDebug() << "ArnMonitor-Test: Invoke monitorReStart Before";
            QMetaObject::invokeMethod( this,
                                       "emitArnEvent",
                                       Qt::QueuedConnection,  // make sure restarted after all setup is done in this thread
                                       Q_ARG( QByteArray, "monitorReStart"));
            if (Arn::debugMonitor)  qDebug() << "ArnMonitor-Test: Invoke monitorReStart After (delay)";
        }
    }
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


void  ArnMonitor::dispatchArnEvent( QByteArray type, QByteArray data, bool isLocal)
{
    if (isLocal)  return;  // Don't handle local events

    QString  foundPath;
    // "created" (new fresh) is special case of "found"
    if (type == "itemCreated") {
        foundPath = QString::fromUtf8( data.constData(), data.size());
        emit arnItemCreated( foundPath);
    }
    else if (type == "itemFound") {
        foundPath = QString::fromUtf8( data.constData(), data.size());
    }

    QString childPath = Arn::childPath( _monitorPath, foundPath);
    // qDebug() << "### arnMon dispArnEv: childPath=" << childPath;
    if (!childPath.isEmpty()) {
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
