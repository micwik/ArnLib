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

    ArnLinkValue() {
        valueReal = 0.0;
        valueInt  = 0;
    }
};


void ArnLink::resetHave()
{
    _haveInt       = false;
    _haveReal      = false;
    _haveString    = false;
    _haveByteArray = false;
    _haveVariant   = false;
}


//// This should in threaded: preserve order of setValue, optimize return of bytearray
void ArnLink::emitChanged( int sendId, const ArnLinkHandle& handleData)
{
    // qDebug() << "emitChanged: isThr=" << _isThreaded << " isPipe=" << _isPipeMode <<
    //            " path=" << linkPath() << " value=" << toByteArray();
    if (_mutex && (_isPipeMode || !handleData.isNull()))
        emit changed( sendId, toByteArray(), handleData);
    else
        emit changed( sendId, handleData);
}


void ArnLink::setValue( int value, int sendId, bool forceKeep)
{
    if (!_val)  return;
    if (_twin  &&  !forceKeep) {    // support for bidirectional function
        _twin->setValue( value, sendId, true);
        return;
    }

    if (_mutex)  _mutex->lock();
    resetHave();
    _val->valueInt = value;
    _type.e        = _type.Int;
    _haveInt       = true;
    if (_mutex)  _mutex->unlock();

    emitChanged( sendId);
}


void ArnLink::setValue( ARNREAL value, int sendId, bool forceKeep)
{
    if (!_val)  return;
    if (_twin  &&  !forceKeep) {    // support for bidirectional function
        _twin->setValue( value, sendId, true);
        return;
    }

    if (_mutex)  _mutex->lock();
    resetHave();
    _val->valueReal = value;
    _type.e         = _type.Real;
    _haveReal       = true;
    if (_mutex)  _mutex->unlock();

    emitChanged( sendId);
}


void ArnLink::setValue( const QString& value, int sendId, bool forceKeep,
                        const ArnLinkHandle& handleData)
{
    if (!_val)  return;
    if (_twin  &&  !forceKeep) {    // support for bidirectional function
        _twin->setValue( value, sendId, true, handleData);
        return;
    }

    if (_mutex)  _mutex->lock();
    resetHave();
    _val->valueString.resize(0);     // Avoid heap defragmentation
    _val->valueString += value;
    _type.e            = _type.String;
    _haveString        = true;
    if (_mutex)  _mutex->unlock();

    emitChanged( sendId, handleData);
}


void ArnLink::setValue( const QByteArray& value, int sendId, bool forceKeep,
                        const ArnLinkHandle& handleData)
{
    if (!_val)  return;
    if (_twin  &&  !forceKeep) {    // support for bidirectional function
        _twin->setValue( value, sendId, true, handleData);
        return;
    }

    if (_mutex)  _mutex->lock();
    resetHave();
    _val->valueByteArray.resize(0);     // Avoid heap defragmentation
    _val->valueByteArray += value;
    _type.e               = _type.ByteArray;
    _haveByteArray        = true;
    if (_mutex)  _mutex->unlock();

    emitChanged( sendId, handleData);
}


void ArnLink::setValue( const QVariant& value, int sendId, bool forceKeep)
{
    if (!_val)  return;
    if (_twin  &&  !forceKeep) {    // support for bidirectional function
        _twin->setValue( value, sendId, true);
        return;
    }

    if (_mutex)  _mutex->lock();
    resetHave();
    _val->valueVariant = value;
    _type.e            = _type.Variant;
    _haveVariant       = true;
    if (_mutex)  _mutex->unlock();

    emitChanged( sendId);
}


void ArnLink::trfValue( const QByteArray& value, int sendId, bool forceKeep, ArnLinkHandle handleData)
{
    ArnLinkHandle::Flags&  handleFlags = handleData._flags;

    if (handleFlags.is( handleFlags.Text)) {
        handleFlags.set( handleFlags.Text, false);  // Text flag not needed anymore
        setValue( QString::fromUtf8( value.constData(), value.size()),
                  sendId, forceKeep, handleData);
    }
    else
        setValue( value, sendId, forceKeep, handleData);
}


int ArnLink::toInt()
{
    if (!_val)  return 0;
    if (_mutex)  _mutex->lock();

    if (!_haveInt) {
        switch (_type.e) {
        case Arn::DataType::Real:
            _val->valueInt = (int)_val->valueReal;
            break;
        case Arn::DataType::String:
            _val->valueInt = _val->valueString.toInt();
            break;
        case Arn::DataType::ByteArray:
            _val->valueInt = _val->valueByteArray.toInt();
            break;
        case Arn::DataType::Variant:
            _val->valueInt = _val->valueVariant.toInt();
            break;
        default:
            _val->valueInt = 0;
        }
        _haveInt = true;
    }

    if (_mutex) {
        int retVal = _val->valueInt;
        _mutex->unlock();
        return retVal;
    }
    return _val->valueInt;
}


ARNREAL  ArnLink::toReal()
{
    if (!_val)  return 0.0;
    if (_mutex)  _mutex->lock();

    if (!_haveReal) {
        switch (_type.e) {
        case Arn::DataType::Int:
            _val->valueReal = (ARNREAL)_val->valueInt;
            break;
#if defined( ARNREAL_FLOAT)
        case Arn::DataType::String:
            _val->valueReal = _val->valueString.toFloat();
            break;
        case Arn::DataType::ByteArray:
            _val->valueReal = _val->valueByteArray.toFloat();
            break;
        case Arn::DataType::Variant:
            _val->valueReal = _val->valueVariant.toFloat();
            break;
#else
        case Arn::DataType::String:
            _val->valueReal = _val->valueString.toDouble();
            break;
        case Arn::DataType::ByteArray:
            _val->valueReal = _val->valueByteArray.toDouble();
            break;
        case Arn::DataType::Variant:
            _val->valueReal = _val->valueVariant.toDouble();
            break;
#endif
        default:
            _val->valueReal = 0.0;
        }
        _haveReal = true;
    }

    if (_mutex) {
        ARNREAL retVal = _val->valueReal;
        _mutex->unlock();
        return retVal;
    }
    return _val->valueReal;
}


QString ArnLink::toString()
{
    if (!_val)  return QString();
    if (_mutex)  _mutex->lock();

    if (!_haveString) {
        _val->valueString.resize(0);     // Avoid heap defragmentation
        switch (_type.e) {
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
            _val->valueString += _val->valueVariant.toString();
            break;
        default:;
        }
        _haveString  = true;
    }

    if (_mutex) {
        QString retVal = _val->valueString;
        _mutex->unlock();
        return retVal;
    }
    return _val->valueString;
}


QByteArray ArnLink::toByteArray()
{
    if (!_val)  return QByteArray();
    if (_mutex)  _mutex->lock();

    if (!_haveByteArray) {
        _val->valueByteArray.resize(0);     // Avoid heap defragmentation
        switch (_type.e) {
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
            _val->valueByteArray += _val->valueVariant.toString().toUtf8();
            break;
        default:;
        }
        _haveByteArray  = true;
    }

    if (_mutex) {
        QByteArray retVal = _val->valueByteArray;
        _mutex->unlock();
        return retVal;
    }
    return _val->valueByteArray;
}


QVariant ArnLink::toVariant( void)
{
    if (!_val)  return QVariant();
    if (_mutex)  _mutex->lock();

    if (!_haveVariant) {
        switch (_type.e) {
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
            _val->valueVariant = _val->valueByteArray;
            break;
        default:
            _val->valueVariant = QVariant();
        }
        _haveVariant = true;
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
    Arn::DataType  retVal = Arn::DataType( _type.e);
    if (_mutex)  _mutex->unlock();

    return retVal;
}


QString ArnLink::linkPath( Arn::NameF nameF)
{
    nameF.set( nameF.NoFolderMark, false);  // Foldermark '/' must be ...
    QString  path;
    QString  linkName;
    ArnLink*  link = this;

    while (link) {  // Backwards until root
        ArnLink*  parentLink = qobject_cast<ArnLink*>( link->parent());
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


QString ArnLink::linkName( Arn::NameF nameF)
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


ArnLink::ArnLink( ArnLink *parent, const QString& name, Arn::LinkFlags flags)
        : QObject(0)
{
    QString  name_ = Arn::convertBaseName( name, Arn::NameF());

    QObject::setParent((QObject*) parent);
    QObject::setObjectName( name_);
    _isFolder        = flags.is( flags.Folder);
    _isProvider      = name_.endsWith('!');
    _type.e          = Arn::DataType::Null;
    _twin            = 0;
    _mutex           = 0;
    _val             = _isFolder ? 0 : new ArnLinkValue;
    _isPipeMode      = false;
    _isSaveMode      = false;
    _hasBeenSetup    = false;
    _syncMode        = 0;
    _id              = _idCount.fetchAndAddRelaxed(1);
    _refCount        = -1;  // Mark no reference, Ok to delete
    _zeroRefCount    = 0;
    _isRetired       = false;
    _isRetiredGlobal = false;
}


void  ArnLink::setupEnd( const QString& path, Arn::ObjectSyncMode syncMode)
{
    if (!_hasBeenSetup) {
        _hasBeenSetup = true;
        addSyncMode( syncMode);

        ArnEvLinkCreate  arnEvLinkCreate;
        arnEvLinkCreate.path    = path;
        arnEvLinkCreate.arnLink = this;
        ArnLink*  link = this;
        forever {
            link = qobject_cast<ArnLink*>( link->parent());
            if (!link)  break;
            if (link->_refCount > 0)
                QCoreApplication::sendEvent( link, &arnEvLinkCreate);
        }
    }
}


void  ArnLink::doModeChanged()
{
    if (thread() == QThread::currentThread()) {
        ArnEvModeChange  arnEvModeChange;
        arnEvModeChange.path   = linkPath();
        arnEvModeChange.linkId = _id;
        QCoreApplication::sendEvent( this, &arnEvModeChange);
    }
    else {
        ArnEvModeChange  *arnEvModeChange = new ArnEvModeChange;
        arnEvModeChange->path   = linkPath();
        arnEvModeChange->linkId = _id;
        QCoreApplication::postEvent( this, arnEvModeChange);
    }
}


ArnLink::~ArnLink()
{
    if (Arn::debugLinkDestroy)  qDebug() << "Destructor link: path=" << linkPath();
    if (_mutex)
        delete _mutex;
    if (_twin) {
        _twin->_twin = 0;  // points to this object
        delete _twin;
        _twin = 0;
    }
}


ArnLink*  ArnLink::findLink( const QString& name)
{
    QString  name_ = Arn::convertBaseName( name, Arn::NameF());
    QObjectList  children = this->children();

    for (int i = 0; i < children.size(); i++) {
        QObject*  child = children.at(i);

        if (child->objectName() == name_) {
            return qobject_cast<ArnLink*>( child);
        }
    }

    return 0;
}


bool ArnLink::isFolder( void)
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


bool  ArnLink::isProvider()  const
{
    return _isProvider;
}


bool  ArnLink::isThreaded()  const
{
    return _mutex != 0;
}


void  ArnLink::setThreaded()
{
    if (!_mutex)
        _mutex = new QMutex;
}


void ArnLink::lock()
{
    if (_mutex)  _mutex->lock();
}


void ArnLink::unlock()
{
    if (_mutex)  _mutex->unlock();
}


bool  ArnLink::event( QEvent* ev)
{
    QEvent::Type  type = ev->type();
    if (type == ArnEvLinkCreate::type()) {
        ArnEvLinkCreate*  e = static_cast<ArnEvLinkCreate*>( ev);
        // qDebug() << "ArnEvLinkCreate: path=" << e->path << " inLinkPath=" << linkPath();
        emit linkCreatedBelow( e->arnLink);
        return true;
    }
    if (type == ArnEvModeChange::type()) {
        ArnEvModeChange*  e = static_cast<ArnEvModeChange*>( ev);
        // qDebug() << "ArnEvModeChange: path=" << e->path << " inLinkPath=" << linkPath();
        emit modeChanged( e->path, e->linkId);
        ArnLink*  link = this;
        forever {
            link = qobject_cast<ArnLink*>( link->parent());
            if (!link)  break;
            // qDebug() << "ArnEvModeChange: path=" << e->path << " inParentLinkPath=" << link->linkPath();
            if (link->_refCount > 0) {
                emit modeChangedBelow( e->path, e->linkId);
            }
        }
        return true;
    }

    return QObject::event( ev);
}


bool  ArnLink::isRetired()
{
    if (_mutex)  _mutex->lock();
    bool  retVal = _isRetired;
    if (_mutex)  _mutex->unlock();
    return retVal;
}


bool  ArnLink::isRetiredGlobal()
{
    if (_mutex)  _mutex->lock();
    bool  retVal = _isRetiredGlobal;
    if (_mutex)  _mutex->unlock();
    return retVal;
}


/// Can only be called from main-thread
void  ArnLink::setRetired( bool isGlobal)
{
    if (_mutex)  _mutex->lock();
    if (Arn::debugLinkDestroy)  qDebug() << "setRetired: path=" << this->linkPath();

    _isRetiredGlobal = isGlobal;
    _isRetired       = true;

    if (_mutex)  _mutex->unlock();
}


void  ArnLink::doRetired()
{
    if (Arn::debugLinkDestroy)  qDebug() << "doRetired: path=" << this->linkPath();
    emit retired();
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
    // MW: Mutex not needed, all values are stable (?)
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


ArnLink*  ArnLink::holderLink( bool forceKeep)
{
    if (_mutex)  _mutex->lock();
    ArnLink*  retVal = (_twin && !forceKeep) ? _twin : this;
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


void ArnLink::decZeroRefs()
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


bool ArnLink::isLastZeroRef()
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


/// Can be called from any thread any time
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
        emit zeroRef( this);  // Will be received in main-thread
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


