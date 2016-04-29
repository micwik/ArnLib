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

#include "ArnInc/ArnMonitor.hpp"
#include "private/ArnMonitor_p.hpp"
#include "ArnInc/ArnMonEvent.hpp"
#include "ArnInc/ArnClient.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnInc/ArnEvent.hpp"
#include "ArnInc/ArnItemB.hpp"
#include "ArnItemNet.hpp"
#include "ArnSync.hpp"
#include "ArnLink.hpp"
#include <QDebug>
#include <QTime>


ArnMonitorPrivate::ArnMonitorPrivate()
{
    _arnClient    = 0;
    _reference    = 0;
    _localMonItem = 0;
}


ArnMonitorPrivate::~ArnMonitorPrivate()
{
}


void  ArnMonitor::init()
{
}


ArnMonitor::ArnMonitor( QObject* parent)
    : ArnItemB( parent)
    , d_ptr( new ArnMonitorPrivate)
{
    init();
}


ArnMonitor::ArnMonitor(const QString& path, QObject* parent)
    : ArnItemB( parent)
    , d_ptr( new ArnMonitorPrivate)
{
    init();
    start( path);
}


ArnMonitor::ArnMonitor( ArnMonitorPrivate& dd, QObject* parent)
    : ArnItemB( parent)
    , d_ptr( &dd)
{
    init();
}


ArnMonitor::~ArnMonitor()
{
    Q_D(ArnMonitor);

    if (d->_localMonItem) {
        delete d->_localMonItem;
    }

    delete d_ptr;
}


void  ArnMonitor::setClient( ArnClient *client)
{
    Q_D(ArnMonitor);

    d->_arnClient = client;
}


void  ArnMonitor::setClient( const QString& id)
{
    Q_D(ArnMonitor);

    d->_arnClient = ArnClient::getClient( id);
}


QString  ArnMonitor::clientId()  const
{
    Q_D(const ArnMonitor);

    if (!d->_arnClient)  return QString();
    return d->_arnClient->id();
}


ArnClient*  ArnMonitor::client()  const
{
    Q_D(const ArnMonitor);

    return d->_arnClient;
}


void  ArnMonitor::setMonitorPath( const QString& path, ArnClient *client)
{
    Q_D(ArnMonitor);

    start( path, client ? client : d->_arnClient.data());
}


bool  ArnMonitor::start( const QString& path, ArnClient* client)
{
    Q_D(ArnMonitor);

    d->_arnClient = client;
    d->_monitorPath = inPathConvert( Arn::fullPath( path));
    if (!d->_monitorPath.endsWith("/"))  d->_monitorPath += "/";

    if (d->_localMonItem) {
        delete d->_localMonItem;
        d->_localMonItem = 0;
    }

    if (d->_arnClient) {
        d->_arnClient.data()->getLocalRemotePath( d->_monitorPath, d->_localMountPath, d->_remoteMountPath);
        // No recursion due to no emit of arnItemCreated as this is a folder
        if (!open( d->_monitorPath))  return false;
        // Force an itemNet (if not already exist), as this is a folder
        bool isNew;
        d->_arnClient->newNetItem( d->_monitorPath, Arn::ObjectSyncMode::Normal, &isNew);

        if (Arn::debugMonitor)  qDebug() << "Monitor start: new=" << isNew << " path=" << d->_monitorPath;
        if (isNew) {
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorStart Before";
            QMetaObject::invokeMethod( this,
                                       "sendArnMonEvent",
                                       Qt::QueuedConnection,  // make sure started after all setup is done in this thread
                                       Q_ARG( int, ArnMonEventType::MonitorStart));
            //// Check thread event sequence by big delay
            // QTime  ts = QTime::currentTime();
            // while (QTime::currentTime() < ts.addSecs(5));
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorStart After (delay)";
        }
        else {
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorReStart Before";
            QMetaObject::invokeMethod( this,
                                       "sendArnMonEvent",
                                       Qt::QueuedConnection,  // make sure restarted after all setup is done in this thread
                                       Q_ARG( int, ArnMonEventType::MonitorReStart));
            if (Arn::debugMonitorTest)  qDebug() << "ArnMonitor-Test: Invoke monitorReStart After (delay)";
        }
    }
    else {  // No client, do local monitor. 2 items used, "sensor" (ArnItemNet) and this ArnMonitor to same path.
        if (!open( d->_monitorPath))  return false;

        d->_localMonItem = new ArnItemNet(0);
        if (!d->_localMonItem->open( d->_monitorPath)) {
            close();
            return false;
        }

        d->_localMonItem->setNetId( linkId());

        if (Arn::debugMonitor)  qDebug() << "Monitor start (local-items): path="
                                         << d->_monitorPath << " id=" << linkId();
        QMetaObject::invokeMethod( this,
                                   "setupLocalMonitorItem",
                                   Qt::QueuedConnection  // make sure called after all setup is done in this thread
                                   );
    }

    connect( this, SIGNAL(arnLinkDestroyed()), this, SIGNAL(monitorClosed()));
    return true;
}


bool  ArnMonitor::start( const QString& path)
{
    return start( path, client());
}


QString  ArnMonitor::monitorPath()  const
{
    Q_D(const ArnMonitor);

    return d->_monitorPath;
}


void  ArnMonitor::reStart()
{
    Q_D(ArnMonitor);

    d->_foundChilds.clear();
    sendArnMonEvent( ArnMonEventType::MonitorReStart);
}


void  ArnMonitor::setReference( void* reference)
{
    Q_D(ArnMonitor);

    d->_reference = reference;
}


void*  ArnMonitor::reference()  const
{
    Q_D(const ArnMonitor);

    return d->_reference;
}


void  ArnMonitor::sendArnMonEvent( int type, const QByteArray& data)
{
    Q_D(ArnMonitor);

    if (!isOpen())  return;

    ArnEvMonitor  ev( type, data, true, d->_arnClient);
    sendArnEventLink( &ev);
}


void  ArnMonitor::setupLocalMonitorItem()
{
    Q_D(ArnMonitor);

    if (!d->_localMonItem)  return;

    ArnSync::setupMonitorItem( d->_localMonItem);
}


void  ArnMonitor::dispatchArnMonEvent( int type, const QByteArray& data, bool isLocal)
{
    Q_D(ArnMonitor);

    if (Arn::debugMonitor && isLocal) {
        qDebug() << "Dipatch Arn event (local): type=" << ArnMonEventType::txt().getTxt( type)
                 << " data=" << data << " monPath=" << d->_monitorPath;
    }

    switch (type) {
    case ArnMonEventType::ItemCreated:
        // Fall throu
    case ArnMonEventType::ItemFound:
        doEventItemFoundCreated( type, data, isLocal);
        break;
    case ArnMonEventType::ItemDeleted:
        doEventItemDeleted( data, isLocal);
        break;
    case ArnMonEventType::ItemModeChg:
        doEventItemModeChg( data, isLocal);
        break;
    case ArnMonEventType::MonitorReStart:
        if (!d->_arnClient && d->_localMonItem) {  // Local monitor event
            //// Send NewItemEvent for any existing direct children (also folders)
            ArnSync::doChildsToEvent( d->_localMonItem);
        }
        break;
    default:
        break;  // This event is not handled
    }
}


void  ArnMonitor::doEventItemFoundCreated( int type, const QByteArray& data, bool isLocal)
{
    Q_D(ArnMonitor);

    QString  foundRemotePath = QString::fromUtf8( data.constData(), data.size());
    QString  foundLocalPath  = toLocalPath( foundRemotePath);

    if (type == ArnMonEventType::ItemCreated) {
        // "created" (new fresh) is special case of "found"
        emit arnItemCreated( outPathConvert( foundLocalPath));
    }

    if (Arn::debugMonitor && !isLocal) {
        qDebug() << "Dipatch Arn event: type=" << ArnMonEventType::txt().getTxt( type)
                 << " remotePath=" << foundRemotePath << " localPath=" << foundLocalPath
                 << " monPath=" << d->_monitorPath;
    }

    QString childPath = Arn::childPath( d->_monitorPath, foundLocalPath);
    // qDebug() << "### arnMon dispArnEv: childPath=" << childPath;
    if (!childPath.isEmpty()) {  // Should always be none emty ...
        if (!d->_foundChilds.contains( childPath)) {  // Only send events about unknown items
            d->_foundChilds += childPath;
            // qDebug() << "### arnMon dispArnEv: Emit chFound childPath=" << childPath;
            emit arnChildFound( outPathConvert( childPath));
            if (childPath.endsWith('/'))  emit arnChildFoundFolder( outPathConvert( childPath));
            else                          emit arnChildFoundLeaf( outPathConvert( childPath));
        }
    }
}


void  ArnMonitor::doEventItemDeleted( const QByteArray& data, bool isLocal)
{
    Q_D(ArnMonitor);

    QString  delRemotePath = QString::fromUtf8( data.constData(), data.size());
    QString  delLocalPath  = toLocalPath( delRemotePath);

    if (Arn::debugMonitor && !isLocal) {
        qDebug() << "Deleted Arn event: remotePath=" << delRemotePath
                 << " localPath=" << delLocalPath << " monPath=" << d->_monitorPath;
    }

    QString  childPath = Arn::childPath( d->_monitorPath, delLocalPath);
    if (childPath == delLocalPath) {  // A child has been deleted
        foundChildDeleted( delLocalPath);
        emit arnChildDeleted( childPath);
    }
    emit arnItemDeleted( delLocalPath);
}


void  ArnMonitor::doEventItemModeChg( const QByteArray& data, bool isLocal)
{
    Q_D(ArnMonitor);

    Arn::XStringMap  xsm( data);
    QString  itemRemotePath = xsm.valueString("path");
    QString  itemLocalPath  = toLocalPath( itemRemotePath);
    Arn::ObjectMode  mode = ArnItemNet::stringToObjectMode( xsm.value("mode"));

    if (Arn::debugMonitor && !isLocal) {
        qDebug() << "ModeChg Arn event: remotePath=" << itemRemotePath
                 << " localPath=" << itemLocalPath << "mode=" << mode.toString()
                 << " monPath=" << d->_monitorPath;
    }

    QString  childPath = Arn::childPath( d->_monitorPath, itemLocalPath);
    if (childPath == itemLocalPath) {  // A child has been deleted
        emit arnChildModeChanged( childPath, mode);
    }
    emit arnItemModeChanged( itemLocalPath, mode);
}


void  ArnMonitor::foundChildDeleted( const QString& path)
{
    Q_D(ArnMonitor);

    int  i = d->_foundChilds.indexOf( inPathConvert( path));
    // qDebug() << "### arnMon foundChDel: path=" << path << " i=" << i;
    if (i >= 0) {
        d->_foundChilds.removeAt(i);
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


void  ArnMonitor::customEvent( QEvent* ev)
{
    Q_D(ArnMonitor);

    int  evIdx = ev->type() - ArnEvent::baseType();
    switch (evIdx) {
    case ArnEvent::Idx::Monitor:
    {
        ArnEvMonitor*  e = static_cast<ArnEvMonitor*>( ev);
        ArnBasicItem*  target = static_cast<ArnBasicItem*>( e->target());
        if (!target)  break;  // No target, deleted/closed ...

        // qDebug() << "ArnMonitor Ev Monitor:";
        if (e->sessionHandler() == d->_arnClient)  // Event is for this session (client)
            dispatchArnMonEvent( e->monEvType(), e->data(), e->isLocal());
        break;
    }
    case ArnEvent::Idx::Retired:
    {
        ArnEvRetired*  e = static_cast<ArnEvRetired*>( ev);
        ArnBasicItem*  target = static_cast<ArnBasicItem*>( e->target());
        if (!target)  break;  // No target, deleted/closed ...

        QString  destroyPath = e->isBelow() ? e->startLink()->linkPath() : target->path();
        // qDebug() << "ArnMonitor Ev Retired: path=" << destroyPath << " inPath=" << target->path();
        dispatchArnMonEvent( ArnMonEventType::ItemDeleted, destroyPath.toUtf8(), true);
        break;
    }
    default:
        break;
    }

    ArnItemB::customEvent( ev);
}


QString  ArnMonitor::toLocalPath( const QString& remotePath)  const
{
    Q_D(const ArnMonitor);

    return Arn::changeBasePath( d->_remoteMountPath, d->_localMountPath, remotePath);
}
