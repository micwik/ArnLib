// Copyright (C) 2010-2022 Michael Wiklund.
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

#include "ArnLink.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnInc/ArnEvent.hpp"
#include <QCoreApplication>
#include <QThread>
#include <limits>
#include <QDebug>

QAtomicInt ArnLink::_idCount(1);


struct ArnLinkValue {
    QString  valueString;
    QByteArray  valueByteArray;
    QVariant  valueVariant;
    volatile ARNREAL  valueReal;
    volatile int  valueInt;
    quint32  localUpdateCount;  // Also ignored updates (ignoreSameValue) are included

    ArnLinkValue() {
        valueReal = 0.0;
        valueInt  = 0;
        localUpdateCount = 0;
    }
};


ArnLink::ArnLink( ArnLink *parent, const QString& name, Arn::LinkFlags flags)
{
    static ArnLinkList  nullArnLinkList;

    QString  name_ = Arn::convertBaseName( name, Arn::NameF());

    _parent          = 0;
    _objectName      = name_;
    _isFolder        = flags.is( flags.Folder);
    _isProvider      = name_.endsWith('!');
    _type            = Arn::DataType::Null;
    _twin            = 0;
    _subscribeTab    = 0;
    _mutex           = 0;
    _children        = _isFolder ? new ArnLinkList : &nullArnLinkList;
    _val             = _isFolder ? 0 : new ArnLinkValue;
    _isPipeMode      = false;
    _isSaveMode      = false;
    _hasBeenSetup    = false;
    _syncMode        = 0;
    _id              = quint32( _idCount.fetchAndAddRelaxed(1));
    _refCount        = -1;  // Mark no reference, Ok to delete
    _zeroRefCount    = 0;
    _isRetired       = false;
    _retireType      = RetireType::None;
    setParent( parent);
    resetHave();
}


ArnLink::~ArnLink()
{
    if (Arn::debugLinkDestroy)  qDebug() << "Destructor link: path=" << linkPath();
    if (_mutex)
        delete _mutex;
    if (_subscribeTab)
        delete _subscribeTab;

    if (_twin) {
        _twin->_twin = 0;  // points to this object
        delete _twin;
        _twin = 0;
    }

    setParent(0);
}


void  ArnLink::resetHave()
{
    _haveInt       = false;
    _haveReal      = false;
    _haveString    = false;
    _haveByteArray = false;
    _haveVariant   = false;
}


void  ArnLink::needInt( bool* isOk)
{
    if (!_haveInt) {
        bool  isOk2 = true;  // Default
        switch (_type) {
        case Arn::DataType::Real:
            _val->valueInt = int( _val->valueReal);
            break;
        case Arn::DataType::String:
            _val->valueInt = _val->valueString.toInt( &isOk2);
            break;
        case Arn::DataType::ByteArray:
            _val->valueInt = _val->valueByteArray.toInt( &isOk2);
            break;
        case Arn::DataType::Variant:
            _val->valueInt = _val->valueVariant.toInt( &isOk2);
            break;
        default:
            _val->valueInt = 0;
            isOk2 = false;
        }
        _haveInt = isOk2;
        if (isOk)
            *isOk = isOk2;
    }
}


void  ArnLink::needReal( bool* isOk)
{
    if (!_haveReal) {
        bool  isOk2 = true;  // Default
        switch (_type) {
        case Arn::DataType::Int:
            _val->valueReal = (ARNREAL)_val->valueInt;
            break;
#if defined( ARNREAL_FLOAT)
        case Arn::DataType::String:
            _val->valueReal = _val->valueString.toFloat( &isOk2);
            break;
        case Arn::DataType::ByteArray:
            _val->valueReal = _val->valueByteArray.toFloat( &isOk2);
            break;
        case Arn::DataType::Variant:
            _val->valueReal = _val->valueVariant.toFloat( &isOk2);
            break;
#else
        case Arn::DataType::String:
            _val->valueReal = _val->valueString.toDouble( &isOk2);
            break;
        case Arn::DataType::ByteArray:
            _val->valueReal = _val->valueByteArray.toDouble( &isOk2);
            break;
        case Arn::DataType::Variant:
            _val->valueReal = _val->valueVariant.toDouble( &isOk2);
            break;
#endif
        default:
            _val->valueReal = 0.0;
            isOk2 = false;
        }
        _haveReal = isOk2;
        if (isOk)
            *isOk = isOk2;
    }
}


QString  ArnLink::objectName()  const
{
    return _objectName;
}


ArnLink*  ArnLink::parent()  const
{
    return _parent;
}


void  ArnLink::setParent( ArnLink* parent)
{
    if (_parent)
        _parent->_children->removeOne( this);

    _parent = parent;

    if (_parent)
       _parent->_children->append( this);
}


const ArnLinkList&  ArnLink::children()  const
{
    return *_children;
}


//// This should in threaded: preserve order of setValue, optimize return of bytearray
void  ArnLink::doValueChanged( int sendId, const QByteArray* valueData,
                               const ArnLinkHandle& handleData)
{
    // qDebug() << "doValueChanged: isThr=" << (_mutex != 0)  << " isPipe=" << _isPipeMode <<
    //             " path=" << linkPath() << " value=" << (valueData ? *valueData : toByteArray());
    ArnEvValueChange ev( sendId, valueData, handleData);
    sendArnEvent( &ev);
}


void  ArnLink::sendEventsInThread( ArnEvent* ev, const ArnCoreItemList& recipients)
{
    int  len = recipients.size();
    for (int i = 0; i < len; ++i) {
        ArnCoreItem*  coreItem = recipients.at(i);
        ev->setAccepted( true);  // Default
        coreItem->sendArnEventItem( ev, false);
    }
}


/// Must be threaded
void  ArnLink::sendArnEvent( ArnEvent* ev)
{
    if (!_mutex) {  // Fast non threaded version
        // Copy of subsribeTab due to destroyEvent can change it
        if (_subscribeTab)
            sendEventsInThread( ev, ArnCoreItemList( *_subscribeTab));
        return;
    }

    _mutex->lock();

    ArnCoreItemList  subscrInThread;
    QThread*  curThread = QThread::currentThread();

    if (_subscribeTab && !_subscribeTab->isEmpty()) {
        foreach (ArnCoreItem* coreItem, *_subscribeTab) {
            if (coreItem->thread() == curThread) {
                subscrInThread += coreItem;
            }
            else {
                // Recipient in different thread
                ArnEvent*  evClone = ev->makeHeapClone();
                coreItem->sendArnEventItem( evClone, true, true);
            }
        }
    }

    _mutex->unlock();

    sendEventsInThread( ev, subscrInThread);
}


void  ArnLink::sendEventsDirRoot( ArnEvent* ev, ArnLink* startLink)
{
    ArnLink*  link = startLink;
    while (link) {
        // qDebug() << "sendEventsDirRoot: inLinkPath=" << link->linkPath() << " ev=" << ev->toString();
        link->sendArnEvent( ev);
        link = link->parent();
    }
}


void  ArnLink::sendEventArnM( ArnEvent* ev)
{
    bool  isMainThread = true;  // Default
    QObject*  objArnM = arnM();

    if (_mutex) {
        if (QThread::currentThread() != objArnM->thread()) {
            isMainThread = false;
            ArnEvent*  evClone = ev->makeHeapClone();
            QCoreApplication::postEvent( objArnM, evClone);
        }
    }

    if (isMainThread) {
        QCoreApplication::sendEvent( objArnM, ev);
    }
}


void  ArnLink::setValue( int value, int sendId, bool useUncrossed)
{
    if (!_val)  return;
    if (_twin  &&  !useUncrossed) {    // support for bidirectional function
        _twin->setValue( value, sendId, true);
        return;
    }

    if (_mutex)  _mutex->lock();
    resetHave();
    _val->valueInt = value;
    _type          = Arn::DataType::Int;
    _haveInt       = true;
    ++_val->localUpdateCount;
    if (_mutex)  _mutex->unlock();

    if (_mutex && _isPipeMode) {
        QByteArray  valueData = QByteArray( 1, char( Arn::ExportCode::String)) +
                                QByteArray::number( value);
        doValueChanged( sendId, &valueData);
    }
    else {
        doValueChanged( sendId);
    }
}


void  ArnLink::setValue( ARNREAL value, int sendId, bool useUncrossed)
{
    if (!_val)  return;
    if (_twin  &&  !useUncrossed) {    // support for bidirectional function
        _twin->setValue( value, sendId, true);
        return;
    }

    if (_mutex)  _mutex->lock();
    resetHave();
    _val->valueReal = value;
    _type           = Arn::DataType::Real;
    _haveReal       = true;
    ++_val->localUpdateCount;
    if (_mutex)  _mutex->unlock();

    if (_mutex && _isPipeMode) {
#if defined( ARNREAL_FLOAT)
        QByteArray  valueData = QByteArray( 1, char( Arn::ExportCode::String)) +
                                QByteArray::number( value, 'g', std::numeric_limits<float>::digits10);
#else
        QByteArray  valueData = QByteArray( 1, char( Arn::ExportCode::String)) +
                                QByteArray::number( value, 'g', std::numeric_limits<double>::digits10);
#endif
        doValueChanged( sendId, &valueData);
    }
    else {
        doValueChanged( sendId);
    }
}


void  ArnLink::setValue( const QString& value, int sendId, bool useUncrossed,
                         const ArnLinkHandle& handleData)
{
    if (!_val)  return;
    if (_twin  &&  !useUncrossed) {    // support for bidirectional function
        _twin->setValue( value, sendId, true, handleData);
        return;
    }

    if (_mutex)  _mutex->lock();
    resetHave();
    _val->valueString.resize(0);     // Avoid heap defragmentation
    _val->valueString += value;
    _type              = Arn::DataType::String;
    _haveString        = true;
    if (!handleData.flags().is( ArnLinkHandle::Flags::FromRemote))
        ++_val->localUpdateCount;
    if (_mutex)  _mutex->unlock();

    if (_mutex && (_isPipeMode || !handleData.isNull())) {
        QByteArray  valueData = QByteArray( 1, char( Arn::ExportCode::String)) +
                                value.toUtf8();
        doValueChanged( sendId, &valueData, handleData);
    }
    else {
        doValueChanged( sendId, 0, handleData);
    }
}


void  ArnLink::setValue( const QByteArray& value, int sendId, bool useUncrossed,
                         const ArnLinkHandle& handleData)
{
    if (!_val)  return;
    if (_twin  &&  !useUncrossed) {    // support for bidirectional function
        _twin->setValue( value, sendId, true, handleData);
        return;
    }

    if (_mutex)  _mutex->lock();
    resetHave();
    _val->valueByteArray.resize(0);     // Avoid heap defragmentation
    _val->valueByteArray += value;
    _type                 = Arn::DataType::ByteArray;
    _haveByteArray        = true;
    if (!handleData.flags().is( ArnLinkHandle::Flags::FromRemote))
        ++_val->localUpdateCount;
    if (_mutex)  _mutex->unlock();

    if (_mutex && (_isPipeMode || !handleData.isNull())) {
        QByteArray  valueData = QByteArray( 1, char( Arn::ExportCode::ByteArray)) + value;
        doValueChanged( sendId, &valueData, handleData);
    }
    else {
        doValueChanged( sendId, 0, handleData);
    }
}


void  ArnLink::setValue( const QVariant& value, int sendId, bool useUncrossed,
                         const ArnLinkHandle& handleData)
{
    if (!_val)  return;
    if (_twin  &&  !useUncrossed) {    // support for bidirectional function
        _twin->setValue( value, sendId, true, handleData);
        return;
    }

    if (_mutex)  _mutex->lock();
    resetHave();
    _val->valueVariant = value;
    _type              = Arn::DataType::Variant;
    _haveVariant       = true;
    if (!handleData.flags().is( ArnLinkHandle::Flags::FromRemote))
        ++_val->localUpdateCount;
    if (_mutex)  _mutex->unlock();

    if (_mutex && _isPipeMode) {
        // In a pipe only QVariant:s that can convert to QString is supported
        QByteArray  valueData = QByteArray( 1, char( Arn::ExportCode::String)) +
                                value.toString().toUtf8();
        doValueChanged( sendId, &valueData, handleData);
    }
    else {
        doValueChanged( sendId, 0, handleData);
    }
}


void  ArnLink::setIgnoredValue( const ArnLinkHandle& handleData)
{
    if (!_val)  return;

    if (_mutex)  _mutex->lock();
    if (!handleData.flags().is( ArnLinkHandle::Flags::FromRemote))
        ++_val->localUpdateCount;
    if (_mutex)  _mutex->unlock();
}


void ArnLink::setBits( int mask, int value, int sendId, bool useUncrossed)
{
    if (!_val)  return;

    if (_twin) {    // support for bidirectional function
        if (_isAtomicOpProvider && !_twin->_isAtomicOpProvider) {  // This twin is OpProvider
            return _twin->setBits( mask, value, sendId, useUncrossed);  // Act as OpProvider for setBits
        }
        if (!_twin->_isAtomicOpProvider) {  // Neither twin is OpProvider, send out event
            // qDebug() << "doSetBits: isThr=" << (_mutex != 0)  << " isPipe=" << _isPipeMode <<
            //            " path=" << linkPath() << " mask=" << mask << " value=" << value;
            ArnEvAtomicOp  ev( ArnEvAtomicOp::Op::BitSet, mask, value);
            _twin->sendArnEvent( &ev);
            return;
        }
        // Other twin is OpProvider (or faulty both is)
    }

    if (_mutex)  _mutex->lock();
    needInt();

    resetHave();
    int  newValue  = (_val->valueInt & ~mask) | (value & mask);
    _val->valueInt = newValue;
    _type          = Arn::DataType::Int;
    _haveInt       = true;
    ++_val->localUpdateCount;

    if (_mutex)  _mutex->unlock();

    if (_twin && !useUncrossed) {  // Update the OpProvider link before, as if it was requested there first
        _twin->setValue( newValue, sendId, true);
    }

    if (_mutex && _isPipeMode) {
        QByteArray  valueData = QByteArray( 1, char( Arn::ExportCode::String)) +
                                QByteArray::number( newValue);
        doValueChanged( sendId, &valueData);
    }
    else {
        doValueChanged( sendId);
    }
}


void ArnLink::addValue( int value, int sendId, bool useUncrossed)
{
    if (!_val)  return;

    if (_twin) {    // support for bidirectional function
        if (_isAtomicOpProvider && !_twin->_isAtomicOpProvider) {  // This twin is OpProvider
            return _twin->addValue( value, sendId, useUncrossed);  // Act as provider for addValue
        }
        if (!_twin->_isAtomicOpProvider) {  // Neither twin is OpProvider, send out event
            // qDebug() << "doAddValue: isThr=" << (_mutex != 0)  << " isPipe=" << _isPipeMode <<
            //            " path=" << linkPath() << " value=" << value;
            ArnEvAtomicOp  ev( ArnEvAtomicOp::Op::AddInt, value, QVariant());
            _twin->sendArnEvent( &ev);
            return;
        }
        // Other twin is OpProvider (or faulty both is)
    }

    if (_mutex)  _mutex->lock();
    needInt();

    resetHave();
    int  newValue  = _val->valueInt + value;
    _val->valueInt = newValue;
    _type          = Arn::DataType::Int;
    _haveInt       = true;
    ++_val->localUpdateCount;

    if (_mutex)  _mutex->unlock();

    if (_twin && !useUncrossed) {  // Update the OpProvider link before, as if it was requested there first
        _twin->setValue( newValue, sendId, true);
    }

    if (_mutex && _isPipeMode) {
        QByteArray  valueData = QByteArray( 1, char( Arn::ExportCode::String)) +
                                QByteArray::number( newValue);
        doValueChanged( sendId, &valueData);
    }
    else {
        doValueChanged( sendId);
    }
}


void  ArnLink::addValue( ARNREAL value, int sendId, bool useUncrossed)
{
    if (!_val)  return;

    if (_twin) {    // support for bidirectional function
        if (_isAtomicOpProvider && !_twin->_isAtomicOpProvider) {  // This twin is OpProvider
            return _twin->addValue( value, sendId, useUncrossed);  // Act as provider for addValue
        }
        if (!_twin->_isAtomicOpProvider) {  // Neither twin is OpProvider, send out event
            // qDebug() << "doAddValue: isThr=" << (_mutex != 0)  << " isPipe=" << _isPipeMode <<
            //            " path=" << linkPath() << " value=" << value;
            ArnEvAtomicOp  ev( ArnEvAtomicOp::Op::AddReal, value, QVariant());
            _twin->sendArnEvent( &ev);
            return;
        }
        // Other twin is OpProvider (or faulty both is)
    }

    if (_mutex)  _mutex->lock();
    needReal();

    resetHave();
    ARNREAL  newValue = _val->valueReal + value;
    _val->valueReal   = newValue;
    _type             = Arn::DataType::Real;
    _haveReal         = true;
    ++_val->localUpdateCount;

    if (_mutex)  _mutex->unlock();

    if (_twin && !useUncrossed) {  // Update the OpProvider link before, as if it was requested there first
        _twin->setValue( newValue, sendId, true);
    }

    if (_mutex && _isPipeMode) {
        QByteArray  valueData = QByteArray( 1, char( Arn::ExportCode::String)) +
                                QByteArray::number( newValue);
        doValueChanged( sendId, &valueData);
    }
    else {
        doValueChanged( sendId);
    }
}


int  ArnLink::toInt( bool* isOk)
{
    if (isOk)
        *isOk = true;  // Default
    if (!_val)  return 0;
    if (_mutex)  _mutex->lock();

    needInt( isOk);

    if (_mutex) {
        int retVal = _val->valueInt;
        _mutex->unlock();
        return retVal;
    }
    return _val->valueInt;
}


ARNREAL  ArnLink::toReal( bool* isOk)
{
    if (isOk)
        *isOk = true;  // Default
    if (!_val)  return 0.0;
    if (_mutex)  _mutex->lock();

    needReal( isOk);

    if (_mutex) {
        ARNREAL retVal = _val->valueReal;
        _mutex->unlock();
        return retVal;
    }
    return _val->valueReal;
}


QString  ArnLink::toString( bool* isOk)
{
    if (isOk)
        *isOk = true;  // Default
    if (!_val)  return QString();
    if (_mutex)  _mutex->lock();

    if (!_haveString) {
        bool  isOk2 = true;  // Default
        _val->valueString.resize(0);     // Avoid heap defragmentation
        switch (_type) {
        case Arn::DataType::Int:
            _val->valueString += QString::number(_val->valueInt, 10);
            break;
        case Arn::DataType::Real:
#if defined( ARNREAL_FLOAT)
            _val->valueString += QString::number(_val->valueReal, 'g', std::numeric_limits<float>::digits10);
#else
            _val->valueString += QString::number(_val->valueReal, 'g', std::numeric_limits<double>::digits10);
#endif
            break;
        case Arn::DataType::ByteArray:
            _val->valueString += QString::fromUtf8( _val->valueByteArray.constData(), _val->valueByteArray.size());
            break;
        case Arn::DataType::Variant:
            isOk2 = _val->valueVariant.canConvert( QVariant::String);
            _val->valueString += _val->valueVariant.toString();
            break;
        default:
            isOk2 = false;
        }
        _haveString = isOk2;
        if (isOk)
            *isOk = isOk2;
    }

    if (_mutex) {
        QString retVal = _val->valueString;
        _mutex->unlock();
        return retVal;
    }
    return _val->valueString;
}


QByteArray  ArnLink::toByteArray( bool* isOk)
{
    if (isOk)
        *isOk = true;  // Default
    if (!_val)  return QByteArray();
    if (_mutex)  _mutex->lock();

    if (!_haveByteArray) {
        bool  isOk2 = true;  // Default
        _val->valueByteArray.resize(0);     // Avoid heap defragmentation
        switch (_type) {
        case Arn::DataType::Int:
            _val->valueByteArray += QByteArray::number( _val->valueInt, 10);
            break;
        case Arn::DataType::Real:
#if defined( ARNREAL_FLOAT)
            _val->valueByteArray += QByteArray::number( _val->valueReal, 'g', std::numeric_limits<float>::digits10);
#else
            _val->valueByteArray += QByteArray::number( _val->valueReal, 'g', std::numeric_limits<double>::digits10);
#endif
            break;
        case Arn::DataType::String:
            _val->valueByteArray += _val->valueString.toUtf8();
            break;
        case Arn::DataType::Variant:
            isOk2 = _val->valueVariant.canConvert( QVariant::String);
            _val->valueByteArray += _val->valueVariant.toString().toUtf8();
            break;
        default:
            isOk2 = false;
        }
        _haveByteArray = isOk2;
        if (isOk)
            *isOk = isOk2;
    }

    if (_mutex) {
        QByteArray retVal = _val->valueByteArray;
        _mutex->unlock();
        return retVal;
    }
    return _val->valueByteArray;
}


QVariant  ArnLink::toVariant( bool* isOk)
{
    if (isOk)
        *isOk = true;  // Default
    if (!_val)  return QVariant();
    if (_mutex)  _mutex->lock();

    if (!_haveVariant) {
        bool  isOk2 = true;  // Default
        switch (_type) {
        case Arn::DataType::Int:
            _val->valueVariant = _val->valueInt;
            break;
        case Arn::DataType::Real:
            _val->valueVariant = _val->valueReal;
            break;
        case Arn::DataType::String:
            _val->valueVariant = _val->valueString;
            break;
        case Arn::DataType::ByteArray:
            _val->valueVariant = QString::fromUtf8( _val->valueByteArray.constData(),
                                                    _val->valueByteArray.size());
            break;
        default:
            _val->valueVariant = QVariant();
            isOk2 = false;
        }
        _haveVariant = isOk2;
        if (isOk)
            *isOk = isOk2;
    }

    if (_mutex) {
        QVariant retVal = _val->valueVariant;
        _mutex->unlock();
        return retVal;
    }
    return _val->valueVariant;
}


Arn::DataType  ArnLink::type()
{
    if (_mutex)  _mutex->lock();
    Arn::DataType  retVal = Arn::DataType::fromInt( _type);
    if (_mutex)  _mutex->unlock();

    return retVal;
}


QString  ArnLink::linkPath( Arn::NameF nameF)
{
    nameF.set( nameF.NoFolderMark, false);  // Foldermark '/' must be ...
    QString  path;
    QString  linkName;
    ArnLink*  link = this;

    while (link) {  // Backwards until root
        ArnLink*  parentLink = link->parent();
        if (!parentLink) {
            if (nameF.is( nameF.Relative))  break;  // Skip Root
            nameF.set( nameF.EmptyOk, true);  // Root is Empty node name
        }
        linkName = link->linkName( nameF);
        path.insert(0, linkName);
        link  = parentLink;
    }

    return path;
}


QString  ArnLink::linkName( Arn::NameF nameF)
{
    QString  retVal = convertBaseName( objectName(), nameF);
    if (this->isFolder() && !nameF.is(( nameF.NoFolderMark)))
        retVal += '/';

    return retVal;
}


uint  ArnLink::linkId()  const
{
    return _id;
}


void  ArnLink::setupEnd( const QString& path, Arn::ObjectSyncMode syncMode, Arn::LinkFlags flags)
{
    if (!_hasBeenSetup) {
        _hasBeenSetup = true;
        addSyncMode( syncMode);

        ArnEvLinkCreate  arnEvLinkCreate( path, this, flags.is( flags.LastLink));
        sendEventsDirRoot( &arnEvLinkCreate, parent());
    }
}


void  ArnLink::doModeChanged()
{
    ArnEvModeChange  arnEvModeChange( linkPath(), _id, getMode());
    sendEventsDirRoot( &arnEvModeChange, this);
}


ArnLink*  ArnLink::findLink( const QString& name)
{
    QString  name_ = Arn::convertBaseName( name, Arn::NameF());

    int  childNum = _children->size();
    for (int i = 0; i < childNum; i++) {
        ArnLink*  child = _children->at(i);

        if (child->objectName() == name_) {
            return child;
        }
    }

    return 0;
}


bool  ArnLink::isFolder( void)
{
    return _isFolder;
}


void  ArnLink::addSyncMode( Arn::ObjectSyncMode syncMode)
{
    if (_mutex)  _mutex->lock();
    _syncMode |= syncMode.toInt();
    if (_mutex)  _mutex->unlock();
}


Arn::ObjectSyncMode  ArnLink::syncMode()
{
    if (_mutex)  _mutex->lock();
    int  retVal = _syncMode;
    if (_mutex)  _mutex->unlock();
    return Arn::ObjectSyncMode::fromInt( retVal);
}


Arn::ObjectMode  ArnLink::getMode()
{
    Arn::ObjectMode  mode;
    if (_mutex)  _mutex->lock();
    mode.set( mode.Pipe, _isPipeMode);
    mode.set( mode.BiDir, _twin != 0);
    if (_mutex)  _mutex->unlock();
    mode.set( mode.Save, isSaveMode());

    return mode;
}


bool  ArnLink::isBiDirMode()
{
    return _twin != 0;   // Having a twin is bidirectional mode
}


void  ArnLink::setPipeMode( bool isPipeMode, bool alsoSetTwin)
{
    if (_mutex)  _mutex->lock();
    if (isPipeMode != _isPipeMode) {
        _isPipeMode = isPipeMode;
        if (_mutex)  _mutex->unlock();
        doModeChanged();
        if (_mutex)  _mutex->lock();
    }
    if (_twin  &&  alsoSetTwin) {
        if (_mutex)  _mutex->unlock();
        _twin->setPipeMode( isPipeMode, false);
        if (_mutex)  _mutex->lock();
    }
    if (_mutex)  _mutex->unlock();
}


bool  ArnLink::isPipeMode()
{
    if (_mutex)  _mutex->lock();
    bool  retVal = _isPipeMode;
    if (_mutex)  _mutex->unlock();
    return retVal;
}


void  ArnLink::setSaveMode( bool isSaveMode)
{
    ArnLink*  link = valueLink();  // SaveMode is always stored in ValueLink
    if (this != link)  return link->setSaveMode( isSaveMode);

    if (_mutex)  _mutex->lock();
    if (isSaveMode != _isSaveMode) {
        _isSaveMode = isSaveMode;
        if (_mutex)  _mutex->unlock();
        doModeChanged();
        return;
    }
    if (_mutex)  _mutex->unlock();
}


bool  ArnLink::isSaveMode()
{
    ArnLink*  link = valueLink();  // SaveMode is always stored in ValueLink
    if (this != link)  return link->isSaveMode();

    if (_mutex)  _mutex->lock();
    bool  retVal = _isSaveMode;
    if (_mutex)  _mutex->unlock();
    return retVal;
}


void  ArnLink::setAtomicOpProvider( bool isProvider)
{
    _isAtomicOpProvider = isProvider;
}


bool  ArnLink::isAtomicOpProvider()  const
{
    return _isAtomicOpProvider;
}


bool  ArnLink::isProvider()  const
{
    return _isProvider;
}


bool  ArnLink::isThreaded()  const
{
    return _mutex != 0;
}


//// Only used by main thread
void  ArnLink::setThreaded()
{
    if (_mutex)  return;  // Is already threaded

    ArnLink*  link = this;
    // All links (folders) in direction to root must be threaded due to ArnEvents.
    do {
        link->_mutex = new QMutex;
        link = _parent;
    } while( link && !link->_mutex);
}


void  ArnLink::lock()
{
    if (_mutex)  _mutex->lock();
}


void  ArnLink::unlock()
{
    if (_mutex)  _mutex->unlock();
}


QMutex*  ArnLink::getMutex()  const
{
    return _mutex;
}


QObject*  ArnLink::arnM( QObject* inArnM)
{
    static QObject*  storeArnM = 0;

    if (!storeArnM && inArnM)
        storeArnM = inArnM;
    return storeArnM;
}


bool  ArnLink::isRetired()
{
    if (_mutex)  _mutex->lock();
    bool  retVal = _isRetired;
    if (_mutex)  _mutex->unlock();
    return retVal;
}


uint  ArnLink::retireType()
{
    if (_mutex)  _mutex->lock();
    uint  retVal = _retireType;
    if (_mutex)  _mutex->unlock();
    return retVal;
}


/// Can only be called from main-thread
void  ArnLink::setRetired( RetireType retireType)
{
    if (_mutex)  _mutex->lock();
    if (Arn::debugLinkDestroy)  qDebug() << "setRetired: path=" << this->linkPath();

    _retireType = uint( retireType);
    _isRetired  = true;

    if (_mutex)  _mutex->unlock();
}


void  ArnLink::doRetired( ArnLink* startLink, bool isGlobal)
{
    if (Arn::debugLinkDestroy)  qDebug() << "doRetired: path=" << this->linkPath();
    if (startLink == this) {
        ArnEvRetired  arnEvRetired( startLink, true, isGlobal);
        sendEventsDirRoot( &arnEvRetired, parent());
    }
    ArnEvRetired  arnEvRetired( startLink, false, isGlobal);
    sendArnEvent( &arnEvRetired);
}


ArnLink*  ArnLink::twinLink()
{
    if (_mutex)  _mutex->lock();
    ArnLink*  retVal = _twin;
    if (_mutex)  _mutex->unlock();
    return retVal;
}


ArnLink*  ArnLink::valueLink()
{
    // Mutex not needed, all values are stable
    ArnLink*  retVal = _isProvider ? _twin : this;
    return retVal;
}


ArnLink*  ArnLink::providerLink()
{
    if (_mutex)  _mutex->lock();
    ArnLink*  retVal = _isProvider ? this : _twin;
    if (_mutex)  _mutex->unlock();
    return retVal;
}


ArnLink*  ArnLink::holderLink( bool useUncrossed)
{
    if (_mutex)  _mutex->lock();
    ArnLink*  retVal = (_twin && !useUncrossed) ? _twin : this;
    if (_mutex)  _mutex->unlock();
    return retVal;
}


QString  ArnLink::twinName()
{
    QString linkName = objectName();

    if (isProvider()) {  // Link is provider, twin is value
        return linkName.left( linkName.size() - 1);  // Remove '!' at end
    }
    else {  // Link is value, twin is provider
        return linkName + "!";
    }
}


/// Can only be called from main-thread
void  ArnLink::setRefCount( int count)
{
    ArnLink*  vLink = valueLink();

    if (vLink->_mutex)  vLink->_mutex->lock();
    vLink->_refCount = count;
    if (vLink->_mutex)  vLink->_mutex->unlock();
}


void  ArnLink::decZeroRefs()
{
    int  zeroRefCount = 0;
    ArnLink*  vLink = valueLink();

    if (vLink->_mutex)  vLink->_mutex->lock();
    if (vLink->_zeroRefCount > 0) {
        vLink->_zeroRefCount--;
        zeroRefCount = vLink->_zeroRefCount;
    }
    if (vLink->_mutex)  vLink->_mutex->unlock();
    if (Arn::debugLinkRef)  qDebug() << "link-decZeroRefs: path=" << this->linkPath()
                                     << " count=" << zeroRefCount;
}


bool  ArnLink::isLastZeroRef()
{
    bool  retVal    = false;
    ArnLink*  vLink = valueLink();

    if (vLink->_mutex)  vLink->_mutex->lock();
    retVal = (vLink->_refCount == 0) && (vLink->_zeroRefCount == 0);
    if (vLink->_mutex)  vLink->_mutex->unlock();

    return retVal;
}


/// Can only be called from main-thread
void  ArnLink::ref()
{
    ArnLink*  vLink = valueLink();

    if (vLink->_mutex)  vLink->_mutex->lock();
    if (vLink->_refCount <= 0)   // First reference, no other thread involved
        vLink->_refCount = 1;
    else
        vLink->_refCount++;
    if (vLink->_mutex)  vLink->_mutex->unlock();
    if (Arn::debugLinkRef)  qDebug() << "link-ref: path=" << this->linkPath() << " count=" << refCount();
}


bool  ArnLink::subscribe( ArnCoreItem* subscriber)
{
    if (!subscriber)  return false;  // Not valid subscriber

    if (_mutex)  _mutex->lock();
    if (!_subscribeTab)
        _subscribeTab = new ArnCoreItemList;

    *_subscribeTab += subscriber;
    if (_mutex)  _mutex->unlock();

    return true;  // Subsciber added Ok
}


bool  ArnLink::unsubscribe( ArnCoreItem* subscriber)
{
    if (!subscriber)  return false;  // Not valid subscriber
    if (!_subscribeTab)  return false;  // Not valid subscribe table

    if (_mutex)  _mutex->lock();
    bool  stat = _subscribeTab->removeOne( subscriber);
    if (_mutex)  _mutex->unlock();

    return stat;
}


void  ArnLink::deref()
{
    bool  isZeroRefs = false;
    ArnLink*  vLink  = valueLink();

    if (vLink->_mutex)  vLink->_mutex->lock();
    if (vLink->_refCount > 1)
        vLink->_refCount--;
    else {
        vLink->_refCount = 0;
        vLink->_zeroRefCount++;
        isZeroRefs = true;
    }
    if (vLink->_mutex)  vLink->_mutex->unlock();
    if (Arn::debugLinkRef)  qDebug() << "link-deref: path=" << this->linkPath() << " count=" << refCount();

    if (isZeroRefs) {  // This is last reference
        ArnEvZeroRef  arnEvZeroRef( this);
        sendEventArnM( &arnEvZeroRef);  // Will allways be received in main-thread (ArnM)
        //// Now this link might have been deleted
    }
}


int  ArnLink::refCount()
{
    int  retVal     = 0;
    ArnLink*  vLink = valueLink();

    if (vLink->_mutex)  vLink->_mutex->lock();
    retVal = vLink->_refCount;
    if (vLink->_mutex)  vLink->_mutex->unlock();

    return retVal;
}


quint32 ArnLink::localUpdateCount()
{
    quint32  retVal = 0;

    if (_mutex)  _mutex->lock();
    if (_val)
        retVal = _val->localUpdateCount;
    if (_mutex)  _mutex->unlock();

    return retVal;
}
