// Copyright (C) 2010-2019 Michael Wiklund.
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

#include "ArnInc/ArnAdaptItem.hpp"
#include "private/ArnAdaptItem_p.hpp"
#include "ArnInc/ArnEvent.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QThread>
#include <QMutexLocker>
#include <QDebug>

#define MUTEX_CALL( funcCall ) \
    d->_mutex.lock(); \
    funcCall; \
    d->_mutex.unlock();

#define MUTEX_CALL_RET( funcCall ) \
    QMutexLocker mutexLocker( &d->_mutex); \
    return funcCall;


ArnAdaptItemPrivate::ArnAdaptItemPrivate()
    : _mutex( ARN_ModeRecursiveMutex )
    , _changedCB( 0 )
    , _linkDestroyedCB( 0 )
    , _arnEventCB( 0 )
{
}


ArnAdaptItemPrivate::~ArnAdaptItemPrivate()
{
}


void  ArnAdaptItem::init()
{
    addHeritage( ArnCoreItem::Heritage::AdaptItem);
}


ArnAdaptItem::ArnAdaptItem()
    : ArnBasicItem( *new ArnAdaptItemPrivate)
{
    init();
}


ArnAdaptItem::~ArnAdaptItem()
{
}


bool  ArnAdaptItem::open( const QString& path)
{
    Q_D(ArnAdaptItem);

    typedef Arn::LinkFlags  Flags;
    MUTEX_CALL( bool r = openWithFlags( path, Flags::CreateAllowed | Flags::Threaded))
    return r;
}


void  ArnAdaptItem::close()
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::close())
}


void  ArnAdaptItem::destroyLink( bool isGlobal)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::destroyLink( isGlobal))
}


bool  ArnAdaptItem::isOpen()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::isOpen())
    return r;
}


QString  ArnAdaptItem::path( Arn::NameF nameF)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL_RET( ArnBasicItem::path( nameF))
}


QString  ArnAdaptItem::name( Arn::NameF nameF)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL_RET( ArnBasicItem::name( nameF))
}


void  ArnAdaptItem::setReference( void* reference)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setReference( reference))
}


void*  ArnAdaptItem::reference()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( void* r = ArnBasicItem::reference())
    return r;
}


uint  ArnAdaptItem::itemId()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( uint r = ArnBasicItem::itemId())
    return r;
}


uint  ArnAdaptItem::linkId()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( uint r = ArnBasicItem::linkId())
    return r;
}


int  ArnAdaptItem::refCount()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( int r = ArnBasicItem::refCount())
    return r;
}


bool  ArnAdaptItem::isFolder()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::isFolder())
    return r;
}


bool  ArnAdaptItem::isProvider()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::isProvider())
    return r;
}


Arn::DataType  ArnAdaptItem::type()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( Arn::DataType r = ArnBasicItem::type())
    return r;
}


void  ArnAdaptItem::setIgnoreSameValue( bool isIgnore)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setIgnoreSameValue( isIgnore))
}


bool  ArnAdaptItem::isIgnoreSameValue()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::isIgnoreSameValue())
    return r;
}


void  ArnAdaptItem::addMode( Arn::ObjectMode mode)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::addMode( mode))
}


Arn::ObjectMode  ArnAdaptItem::getMode()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( Arn::ObjectMode r = ArnBasicItem::getMode())
    return r;
}


Arn::ObjectSyncMode  ArnAdaptItem::syncMode()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( Arn::ObjectSyncMode r = ArnBasicItem::syncMode())
    return r;
}


ArnAdaptItem&  ArnAdaptItem::setBiDirMode()
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setBiDirMode())
    return *this;
}


bool  ArnAdaptItem::isBiDirMode()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::isBiDirMode())
    return r;
}


ArnAdaptItem&  ArnAdaptItem::setPipeMode()
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setPipeMode())
    return *this;
}


bool  ArnAdaptItem::isPipeMode()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::isPipeMode())
    return r;
}


ArnAdaptItem&  ArnAdaptItem::setSaveMode()
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setSaveMode())
    return *this;
}


bool  ArnAdaptItem::isSaveMode()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::isSaveMode())
    return r;
}


ArnAdaptItem&  ArnAdaptItem::setMaster()
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setMaster())
    return *this;
}


bool  ArnAdaptItem::isMaster()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::isMaster())
    return r;
}


ArnAdaptItem&  ArnAdaptItem::setAutoDestroy()
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setAutoDestroy())
    return *this;
}


bool  ArnAdaptItem::isAutoDestroy()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::isAutoDestroy())
    return r;
}


void  ArnAdaptItem::arnImport( const QByteArray& data, int ignoreSame)
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::arnImport( data, ignoreSame))
}


QByteArray  ArnAdaptItem::arnExport()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL_RET( ArnBasicItem::arnExport())
}


int  ArnAdaptItem::toInt( bool* isOk)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( int r = ArnBasicItem::toInt( isOk))
    return r;
}


double  ArnAdaptItem::toDouble( bool* isOk)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( double r = ArnBasicItem::toDouble( isOk))
    return r;
}


ARNREAL  ArnAdaptItem::toReal( bool* isOk)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( ARNREAL r = ArnBasicItem::toReal( isOk))
    return r;
}


QString  ArnAdaptItem::toString( bool* isOk)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL_RET( ArnBasicItem::toString( isOk))
}


QByteArray  ArnAdaptItem::toByteArray( bool* isOk)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL_RET( ArnBasicItem::toByteArray( isOk))
}


QVariant  ArnAdaptItem::toVariant( bool* isOk)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL_RET( ArnBasicItem::toVariant( isOk))
}


bool  ArnAdaptItem::toBool( bool* isOk)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::toBool( isOk))
    return r;
}


uint  ArnAdaptItem::toUInt( bool* isOk)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( uint r = ArnBasicItem::toUInt( isOk))
    return r;
}


qint64  ArnAdaptItem::toInt64( bool* isOk)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( qint64 r = ArnBasicItem::toInt64( isOk))
    return r;
}


quint64  ArnAdaptItem::toUInt64( bool* isOk)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( quint64 r = ArnBasicItem::toUInt64( isOk))
    return r;
}


ArnAdaptItem&  ArnAdaptItem::operator=( const ArnAdaptItem& other)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( other))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator=( int val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( val))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator=( ARNREAL val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( val))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator=( const QString& val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( val))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator=( const QByteArray& val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( val))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator=( const QVariant& val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( val))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator=( const char* val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( val))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator=( uint val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( val))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator=( qint64 val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( val))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator=( quint64 val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( val))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator+=( int val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::addValue( val))
    return *this;
}


ArnAdaptItem&  ArnAdaptItem::operator+=( ARNREAL val)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::addValue( val))
    return *this;
}


void  ArnAdaptItem::setValue( const ArnAdaptItem& other, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( other, ignoreSame))
}


void  ArnAdaptItem::setValue( int value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( value, ignoreSame))
}


void  ArnAdaptItem::setValue( ARNREAL value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( value, ignoreSame))
}


void  ArnAdaptItem::setValue( bool value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( value, ignoreSame))
}


void  ArnAdaptItem::setValue( const QString& value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( value, ignoreSame))
}


void  ArnAdaptItem::setValue( const QByteArray& value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( value, ignoreSame))
}


void  ArnAdaptItem::setValue( const QVariant& value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( value, ignoreSame))
}


void  ArnAdaptItem::setValue( const char* value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( value, ignoreSame))
}


void  ArnAdaptItem::setValue( uint value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( value, ignoreSame))
}


void  ArnAdaptItem::setValue( qint64 value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( value, ignoreSame))
}


void  ArnAdaptItem::setValue( quint64 value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setValue( value, ignoreSame))
}


void  ArnAdaptItem::setBits( int mask, int value, int ignoreSame)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setBits( mask, value, ignoreSame))
}


void  ArnAdaptItem::addValue( int value)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::addValue( value))
}


void  ArnAdaptItem::addValue( ARNREAL value)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::addValue( value))
}


ARN_RecursiveMutex& ArnAdaptItem::mutex()  const
{
    Q_D(const ArnAdaptItem);

    return d->_mutex;
}


QThread*  ArnAdaptItem::thread()  const
{
    return QThread::currentThread();  // Same thread as now running (no real affinity)
}


void  ArnAdaptItem::setChangedCallback( ArnAdaptItem::ChangedCB changedCB)
{
    Q_D(ArnAdaptItem);

    d->_changedCB = changedCB;
}


ArnAdaptItem::ChangedCB  ArnAdaptItem::ChangedCallback()  const
{
    Q_D(const ArnAdaptItem);

    return d->_changedCB;
}


void  ArnAdaptItem::setLinkDestroyedCallback( ArnAdaptItem::LinkDestroyedCB linkDestroyedCB)
{
    Q_D(ArnAdaptItem);

    d->_linkDestroyedCB = linkDestroyedCB;
}


ArnAdaptItem::LinkDestroyedCB  ArnAdaptItem::linkDestroyedCallback()  const
{
    Q_D(const ArnAdaptItem);

    return d->_linkDestroyedCB;
}


void ArnAdaptItem::setArnEventCallback( ArnEventCB evCB)
{
    Q_D(ArnAdaptItem);

    d->_arnEventCB = evCB;
}


ArnAdaptItem::ArnEventCB  ArnAdaptItem::arnEventCallback()  const
{
    Q_D(const ArnAdaptItem);

    return d->_arnEventCB;
}


void  ArnAdaptItem::setUncrossed( bool isUncrossed)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::setUncrossed( isUncrossed))
}


bool  ArnAdaptItem::isUncrossed()  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::isUncrossed())
    return r;
}


bool  ArnAdaptItem::sendArnEventLink( ArnEvent* ev)
{
    Q_D(ArnAdaptItem);

    MUTEX_CALL( bool r = ArnBasicItem::sendArnEventLink( ev))
    return r;
}


void  ArnAdaptItem::sendArnEventItem( ArnEvent* ev, bool isAlienThread, bool isLocked)
{
    Q_UNUSED(isLocked)

    if (!ev)  return;  // No event ...

    ev->setTarget( this);

    arnEvent( ev, isAlienThread);

    if (isAlienThread) {
        delete ev;
    }
}


void  ArnAdaptItem::arnEvent( QEvent* ev, bool isAlienThread)
{
    Q_UNUSED(isAlienThread)
    Q_D(ArnAdaptItem);

    //int  evIdx = ev->type() - ArnEvent::baseType();
    int  evIdx = ArnEvent::toIdx( ev->type());

    if (d->_arnEventCB) {
        (*(d->_arnEventCB))( ev, evIdx);
    }

    switch (evIdx) {
    case ArnEvent::Idx::ValueChange:
    {
        if (!d->_changedCB)  return;
        ArnEvValueChange*   e = static_cast<ArnEvValueChange*>( ev);
        ArnAdaptItem*  target = static_cast<ArnAdaptItem*>( e->target());
        if (!target)  return;  // No target, deleted/closed ...

        QByteArray  val = e->valueData() ? *e->valueData() : target->toByteArray();
        (*(d->_changedCB))( *target, val);
        // qDebug() << "ArnAdaptEvValueChange: path=" << target->path()
        //          << " value=" << val;
        return;
    }
    case ArnEvent::Idx::ModeChange:
    {
        ArnEvModeChange*    e = static_cast<ArnEvModeChange*>( ev);
        ArnAdaptItem*  target = static_cast<ArnAdaptItem*>( e->target());
        if (!target)  return;  // No target, deleted/closed ...

        QMutexLocker mutexLocker( &target->mutex());  // Force atomic operation on target

        // qDebug() << "ArnAdaptEvModeChange: path=" << e->path() << " mode=" << e->mode()
        //          << " inItemPath=" << target->path();
        if (!target->isFolder()) {
            if (e->mode().is( Arn::ObjectMode::Pipe)) {  // Pipe-mode never IgnoreSameValue
                target->setIgnoreSameValue(false);
            }
        }
        return;
    }
    case ArnEvent::Idx::Retired:
    {
        ArnEvRetired*  e = static_cast<ArnEvRetired*>( ev);
        ArnAdaptItem*  target = static_cast<ArnAdaptItem*>( e->target());
        if (!target)  return;  // No target, deleted/closed ...

        QMutexLocker mutexLocker( &target->mutex());  // Force atomic operation on target

        if (!e->isBelow()) {
            if (Arn::debugLinkDestroy)  qDebug() << "AdaptItem arnLinkDestroyed: path=" << target->path();
            if (d->_linkDestroyedCB) {
                (*(d->_linkDestroyedCB))( *target);
            }
            target->close();
            e->setTarget(0);  // target is not available any more
        }
        return;
    }
    default:;
    }
}


void  ArnAdaptItem::errorLog( const QString& errText, ArnError err, void* reference)  const
{
    Q_D(const ArnAdaptItem);

    MUTEX_CALL( ArnBasicItem::errorLog( errText, err, reference))
}


ArnAdaptItem::ArnAdaptItem( ArnAdaptItemPrivate& dd)
    : ArnBasicItem( dd)
{
    init();
}
