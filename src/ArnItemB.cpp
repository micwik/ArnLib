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

#include "ArnInc/ArnItemB.hpp"
#include "private/ArnItemB_p.hpp"
#include "ArnInc/ArnEvent.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QCoreApplication>
#include <QDebug>



ArnItemBPrivate::ArnItemBPrivate()
{
    _blockEcho       = false;
    _enableUpdNotify = true;
    _enableSetValue  = true;
}


ArnItemBPrivate::~ArnItemBPrivate()
{
}


void  ArnItemB::init()
{
}


ArnItemB::ArnItemB( QObject *parent)
    : QObject( parent)
    , d_ptr( new ArnItemBPrivate)
{
    init();
}


ArnItemB::ArnItemB( ArnItemBPrivate& dd, QObject* parent)
    : QObject( parent)
    , d_ptr( &dd)
{
    init();
}


ArnItemB::~ArnItemB()
{
    delete d_ptr;
}


bool  ArnItemB::openWithFlags( const QString& path, Arn::LinkFlags linkFlags)
{
    bool  retVal = ArnBasicItem::openWithFlags( path, linkFlags);
    modeUpdate( getMode(), true);
    return retVal;
}


bool  ArnItemB::open( const QString& path)
{
    return openWithFlags( path, Arn::LinkFlags::CreateAllowed);
}


bool  ArnItemB::openUuid( const QString& path)
{
    QString  uuidPath = Arn::uuidPath( path);
    bool  stat = open( uuidPath);

    return stat;
}


bool  ArnItemB::openUuidPipe( const QString& path)
{
    bool  stat = openUuid( path);

    setPipeMode();
    return stat;
}


bool  ArnItemB::openFolder( const QString& path)
{
    QString  folderPath = path;
    if (!Arn::isFolderPath( folderPath))
        folderPath += '/';

    return open( folderPath);
}


void  ArnItemB::modeUpdate( Arn::ObjectMode mode, bool isSetup)
{
    Q_UNUSED(mode)
    Q_UNUSED(isSetup)
}


void  ArnItemB::setBlockEcho( bool blockEcho)
{
    Q_D(ArnItemB);

    d->_blockEcho = blockEcho;
}


void  ArnItemB::setEnableSetValue( bool enable)
{
    Q_D(ArnItemB);

    d->_enableSetValue = enable;
}


void  ArnItemB::setEnableUpdNotify( bool enable)
{
    Q_D(ArnItemB);

    d->_enableUpdNotify = enable;
}


void  ArnItemB::arnImport( const QByteArray& data, int ignoreSame)
{
    ArnLinkHandle  handle;
    arnImport( data, ignoreSame, handle);
}


void  ArnItemB::arnImport( const QByteArray& data, int ignoreSame, ArnLinkHandle& handleData)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::arnImport( data, ignoreSame, handleData);
}


void  ArnItemB::setValue( const ArnItemB& other, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( other, ignoreSame);
}


void  ArnItemB::setValue( int value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( value, ignoreSame);
}


void  ArnItemB::setValue( ARNREAL value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( value, ignoreSame);
}


void  ArnItemB::setValue( bool value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( value, ignoreSame);
}


void  ArnItemB::setValue( const QString& value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( value, ignoreSame);
}


void  ArnItemB::setValue( const QByteArray& value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( value, ignoreSame);
}


void  ArnItemB::setValue( const QVariant& value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( value, ignoreSame);
}


void  ArnItemB::setValue( uint value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( value, ignoreSame);
}


void  ArnItemB::setValue( qint64 value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( value, ignoreSame);
}


void  ArnItemB::setValue( quint64 value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( value, ignoreSame);
}


void  ArnItemB::itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value)
{
    Q_UNUSED(handleData);
    Q_UNUSED(value);
}


void  ArnItemB::itemCreatedBelow( const QString& path)
{
    Q_UNUSED(path);
}


void  ArnItemB::itemModeChangedBelow( const QString& path, uint linkId, Arn::ObjectMode mode)
{
    Q_UNUSED(path);
    Q_UNUSED(linkId);
    Q_UNUSED(mode);
}


void  ArnItemB::setValue( const QByteArray& value, int ignoreSame, ArnLinkHandle& handleData)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    ArnBasicItem::setValue( value, ignoreSame, handleData);
}


void  ArnItemB::arnEvent( QEvent* ev, bool isAlienThread)
{
    if (!ev)  return;  // No event ...

    if (isAlienThread) {
        QCoreApplication::postEvent( this, ev);
    }
    else {
        customEvent( ev);
    }
}


void  ArnItemB::customEvent( QEvent* ev)
{
    // This has to handle all demands of ArnEvent handling, i.e finish with ArnBasicItem::arnEvent(...)
    // customEvent() in inherited class to ArnItemB must call this member at end.

    doEvent( ev);
    ArnBasicItem::arnEvent( ev, false);  // Will call selected ArnEventhandler
}


void  ArnItemB::doEvent( QEvent* ev)
{
    Q_D(ArnItemB);

    int  evIdx = ev->type() - ArnEvent::baseType();
    switch (evIdx) {
    case ArnEvent::Idx::ValueChange:
    {
        ArnEvValueChange*  e = static_cast<ArnEvValueChange*>( ev);
        // qDebug() << "ArnEvValueChange: inItemPath=" << path();
        quint32  sendId = e->sendId();
        quint32  id = itemId();
        if (d->_blockEcho  &&  sendId == id)  // Update was initiated from this Item, it can be blocked ...
            return;

        addIsOnlyEcho( sendId);

        if (d->_enableUpdNotify)
            itemUpdated( e->handleData(), e->valueData());
        return;
    }
    case ArnEvent::Idx::LinkCreate:
    {
        ArnEvLinkCreate*  e = static_cast<ArnEvLinkCreate*>( ev);
        // qDebug() << "ArnEvLinkCreate: path=" << e->path() << " inItemPath=" << path();
        if (!Arn::isFolderPath( e->path())) {  // Only created leaves are passed on
            itemCreatedBelow( e->path());
        }
        return;
    }
    case ArnEvent::Idx::ModeChange:
    {
        ArnEvModeChange*  e = static_cast<ArnEvModeChange*>( ev);
        // qDebug() << "ArnEvModeChange: path=" << e->path() << " mode=" << e->mode()
        //          << " inItemPath=" << path();
        if (isFolder())
            itemModeChangedBelow( e->path(), e->linkId(),e->mode());
        else
            modeUpdate( e->mode());
        return;
    }
    case ArnEvent::Idx::Retired:
    {
        ArnEvRetired*  e = static_cast<ArnEvRetired*>( ev);
        if (!e->isBelow()) {
            if (Arn::debugLinkDestroy)  qDebug() << "Item arnLinkDestroyed: path=" << path();
            emit arnLinkDestroyed();
        }
        return;
    }
    default:;
    }
}
