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

#include "ArnInc/ArnBasicItem.hpp"
#include "private/ArnBasicItem_p.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/ArnEvent.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnLink.hpp"
#include <QDataStream>
#include <QThreadStorage>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>


QAtomicInt ArnBasicItemPrivate::_idCount(1);


ArnBasicItemPrivate::ArnBasicItemPrivate()
{
    _reference       = arnNullptr;
    _eventHandler    = arnNullptr;
    _pendingEvChain  = arnNullptr;
    _id              = quint32(_idCount.fetchAndAddRelaxed(1));

    _useUncrossed    = false;
    _isStdEvHandler  = true;
    _isAssigning     = false;
    _ignoreSameValue = ArnM::defaultIgnoreSameValue();
    _isOnlyEcho      = true;  // Nothing else yet ...

    _syncMode          = quint8( Arn::ObjectSyncMode());
    _mode              = quint8( Arn::ObjectMode());
    _heritage          = ArnCoreItem::Heritage::None;
    _syncModeLinkShare = true;
}


ArnBasicItemPrivate::~ArnBasicItemPrivate()
{
}


ArnBasicItemPrivate& ArnBasicItemPrivate::addHeritage( ArnCoreItem::Heritage heritage)
{
    _heritage |= heritage;
    return *this;
}




void  ArnBasicItem::init()
{
    Q_D(ArnBasicItem);

    _link = 0;
    d->_eventHandler = getThreadEventHandler();  // The common event handler for this thread
    addHeritage( ArnCoreItem::Heritage::BasicItem);
}


ArnBasicItem::ArnBasicItem()
    : ArnCoreItem( *new ArnBasicItemPrivate)
{
    init();
}


ArnBasicItem::ArnBasicItem( ArnBasicItemPrivate& dd)
    : ArnCoreItem( dd)
{
    init();
}


ArnBasicItem::~ArnBasicItem()
{
    close();
}


void  ArnBasicItem::setupOpenItem( bool isFolder)
{
    Q_UNUSED(isFolder)
    Q_D(ArnBasicItem);

    addMode( Arn::ObjectMode::fromInt( d->_mode));  // Transfer modes to the link
}


bool  ArnBasicItem::openWithFlags( const QString& path, Arn::LinkFlags linkFlags)
{
    Q_D(ArnBasicItem);

    if (_link)
        close();

    Arn::ObjectSyncMode  syncMode = d->_syncModeLinkShare ? Arn::ObjectSyncMode::fromInt( d->_syncMode)
                                                          : Arn::ObjectSyncMode();
    _link = ArnM::link( path, linkFlags, syncMode);
    if (!_link)  return false;

    ArnEvRefChange ev(+1);
    sendArnEventLink( &ev);
    ArnM::changeRefCounter(+1);

    _link->subscribe( this);
    setupOpenItem( _link->isFolder());
#ifdef ArnBasicItem_INCPATH
    d->_path = path;
#endif

    return true;
}


bool  ArnBasicItem::open( const QString& path)
{
    return openWithFlags( path, Arn::LinkFlags::CreateAllowed);
}


void  ArnBasicItem::close()
{
    Q_D(ArnBasicItem);

    if (!_link)  return;

    _link->unsubscribe( this);
    // Now this item will not get ArnEvent updates in its

    ArnEvRefChange ev(-1);
    sendArnEventLink( &ev);
    ArnM::changeRefCounter(-1);

    if (d->_pendingEvChain) {
        d->_pendingEvChain->setTargetMutex(0);  // No mutex needed anymore
        d->_pendingEvChain->inhibitPendingChain();
    }

    _link->deref();
    _link        = 0;
    d->_syncMode = Arn::ObjectSyncMode();
    d->_mode     = Arn::ObjectMode();
}


void  ArnBasicItem::destroyLink( bool isGlobal)
{
    ArnM::destroyLink( _link, isGlobal);
}


bool  ArnBasicItem::isOpen()  const
{
    return _link != 0;
}


bool  ArnBasicItem::isFolder()  const
{
    if (!_link)  return false;

    return _link->isFolder();
}


bool  ArnBasicItem::isProvider()  const
{
    if (!_link)  return false;

    return _link->isProvider();
}


Arn::DataType  ArnBasicItem::type()  const
{
    if (!_link)  return Arn::DataType::Null;

    return _link->type();
}


uint  ArnBasicItem::linkId()  const
{
    if (!_link)  return 0;

    return _link->linkId();
}


int ArnBasicItem::refCount()  const
{
    if (!_link)  return -1;

    return qMax( _link->refCount(), 0);
}


void  ArnBasicItem::addSyncMode( Arn::ObjectSyncMode syncMode, bool linkShare)
{
    Q_D(ArnBasicItem);

    d->_syncModeLinkShare = linkShare;
    d->_syncMode |= syncMode;
    if (d->_syncModeLinkShare  &&  _link) {
        _link->addSyncMode( Arn::ObjectSyncMode::fromInt( d->_syncMode));
    }
}


void  ArnBasicItem::resetOnlyEcho()
{
    Q_D(ArnBasicItem);

    d->_isOnlyEcho = true;
}


void  ArnBasicItem::addIsOnlyEcho( quint32 sendId)
{
    Q_D(ArnBasicItem);

    if (sendId != d->_id)  // Originate from different Item, not an echo
        d->_isOnlyEcho = false;
}


bool  ArnBasicItem::isOnlyEcho() const
{
    Q_D(const ArnBasicItem);

    return d->_isOnlyEcho;
}


quint32  ArnBasicItem::localUpdateCount()  const
{
    if (!_link)  return 0;

    return _link->localUpdateCount();
}


uint  ArnBasicItem::retireType()
{
    return _link ? _link->retireType() : uint( ArnLink::RetireType::None);
}


Arn::ObjectSyncMode  ArnBasicItem::syncMode()  const
{
    Q_D(const ArnBasicItem);

    if (d->_syncModeLinkShare  &&  _link) {
        return _link->syncMode();
    }
    return Arn::ObjectSyncMode::fromInt( d->_syncMode);
}


ArnBasicItem&  ArnBasicItem::setBiDirMode()
{
    Q_D(ArnBasicItem);

    d->_mode |= Arn::ObjectMode::BiDir;
    if (!_link)  return *this;

    if (_link->isBiDirMode())  return *this;  // Already is bidirectional mode

    /// Bidirectional-mode is the pair of value & provider
    ArnLink*  twinLink = ArnM::addTwin( _link->linkPath(), _link, syncMode());
    twinLink->deref();

    return *this;
}


bool  ArnBasicItem::isBiDirMode()  const
{
    Q_D(const ArnBasicItem);

    if (!_link)  return d->mode().is( Arn::ObjectMode::BiDir);

    return _link->isBiDirMode();
}


ArnBasicItem&  ArnBasicItem::setPipeMode()
{
    Q_D(ArnBasicItem);

    d->_mode |=  Arn::ObjectMode::Pipe | Arn::ObjectMode::BiDir;
    d->_ignoreSameValue = false;
    if (!_link)  return *this;

    if (_link->isPipeMode())  return *this;  // Already is pipe mode

    //// Pipe-mode demands the pair of value & provider
    ArnLink*  twinLink = ArnM::addTwin( _link->linkPath(), _link, syncMode());
    _link->setPipeMode( true);
    twinLink->deref();

    return *this;
}


bool  ArnBasicItem::isPipeMode()  const
{
    Q_D(const ArnBasicItem);

    if (!_link)  return d->mode().is( Arn::ObjectMode::Pipe);

    return _link->isPipeMode();
}


ArnBasicItem&  ArnBasicItem::setSaveMode()
{
    Q_D(ArnBasicItem);

    d->_mode |= Arn::ObjectMode::Save;
    if (!_link)  return *this;

    _link->setSaveMode( true);
    return *this;
}


bool  ArnBasicItem::isSaveMode()  const
{
    Q_D(const ArnBasicItem);

    if (!_link)  return d->mode().is( Arn::ObjectMode::Save);

    return _link->isSaveMode();
}


void  ArnBasicItem::setAtomicOpProvider()
{
    if (!_link)  return;

    _link->setAtomicOpProvider( true);
    return;
}


bool  ArnBasicItem::isAtomicOpProvider()  const
{
    if (!_link)  return false;

    return _link->isAtomicOpProvider();
}


ArnBasicItem&  ArnBasicItem::setMaster()
{
    if (_link) {
        ArnM::errorLog( QString("Setting item/link as master"),
                            ArnError::AlreadyOpen);
    }
    addSyncMode( Arn::ObjectSyncMode::Master, true);
    return *this;
}


bool  ArnBasicItem::isMaster()  const
{
    return syncMode().is( Arn::ObjectSyncMode::Master);
}


ArnBasicItem&  ArnBasicItem::setAutoDestroy()
{
    if (_link) {
        ArnM::errorLog( QString("Setting item/link to autoDestroy"),
                            ArnError::AlreadyOpen);
    }
    addSyncMode( Arn::ObjectSyncMode::AutoDestroy, true);
    return *this;
}


bool  ArnBasicItem::isAutoDestroy()  const
{
    return syncMode().is( Arn::ObjectSyncMode::AutoDestroy);
}


void  ArnBasicItem::addMode( Arn::ObjectMode mode)
{
    Q_D(ArnBasicItem);

    d->_mode |= mode;  // Just in case, transfer all modes

    if (mode.is( mode.Pipe)) {
        setPipeMode();
    }
    else if (mode.is( mode.BiDir)) {
        setBiDirMode();
    }
    if (mode.is( mode.Save)) {
        setSaveMode();
    }
}


Arn::ObjectMode  ArnBasicItem::getMode()  const
{
    return getMode( _link);
}


/// Use with care, link must be "referenced" before use, otherwise it might have been deleted
Arn::ObjectMode  ArnBasicItem::getMode( ArnLink* link)  const
{
    Q_D(const ArnBasicItem);

    if (!link)  return Arn::ObjectMode::fromInt( d->_mode);

    return link->getMode();
}


void  ArnBasicItem::setIgnoreSameValue( bool isIgnore)
{
    Q_D(ArnBasicItem);

    d->_ignoreSameValue = isPipeMode() ? false : isIgnore;
}


bool  ArnBasicItem::isIgnoreSameValue()  const
{
    Q_D(const ArnBasicItem);

    return d->_ignoreSameValue;
}


QString  ArnBasicItem::path( Arn::NameF nameF)  const
{
    if (!_link)  return QString();

    return _link->linkPath( nameF);
}


QString  ArnBasicItem::name( Arn::NameF nameF)  const
{
    if (!_link)  return QString();

    return _link->linkName( nameF);
}


void  ArnBasicItem::setReference( void* reference)
{
    Q_D(ArnBasicItem);

    d->_reference = reference;
}


void*  ArnBasicItem::reference()  const
{
    Q_D(const ArnBasicItem);

    return d->_reference;
}


uint  ArnBasicItem::itemId()  const
{
    Q_D(const ArnBasicItem);

    return d->_id;
}


void  ArnBasicItem::arnImport( const QByteArray& data, int ignoreSame)
{
    ArnLinkHandle  handle;
    arnImport( data, ignoreSame, handle);
}


void  ArnBasicItem::arnImport( const QByteArray& data, int ignoreSame, ArnLinkHandle& handleData)
{
    if (!data.isEmpty()) {
        if (data.at(0) < 32) {  // Assume Export-code
            switch (Arn::ExportCode::fromInt( data.at(0))) {
            case Arn::ExportCode::Variant: {  // Legacy
                QVariant  value;
                QDataStream  stream( data);
                stream.setVersion( DATASTREAM_VER);
                quint8  dummy;  // Will get Export-code
                stream >> dummy >> value;
                setValue( value, ignoreSame, handleData);  // ArnLinkHandle not fully supported for QVariant
                return;
            }
            case Arn::ExportCode::VariantTxt: {
                int  sepPos = data.indexOf(':', 1);
                Q_ASSERT(sepPos > 0);

                QByteArray  typeName( data.constData() + 1, sepPos - 1);
                int  type = QMetaType::type( typeName.constData());
                if (!type) {
                    errorLog( QString("Import unknown text type:") + typeName.constData(),
                              ArnError::Undef);
                    return;
                }
                QVariant  value( QString::fromUtf8( data.constData() + sepPos + 1,
                                                    data.size() - sepPos - 1));
                if (!value.convert( QVariant::Type( type))) {
                    errorLog( QString("Can't' import data type:") + typeName.constData(),
                              ArnError::Undef);
                    return;
                }

                setValue( value, ignoreSame, handleData);  // ArnLinkHandle not fully supported for QVariant
                return;
            }
            case Arn::ExportCode::VariantBin: {
                if ((data.size() < 3) || (data.at(2) != DATASTREAM_VER)) {
                    errorLog( QString("Import not same DataStream version"),
                              ArnError::Undef);
                    return;
                }
                int  sepPos = data.indexOf(':', 3);
                Q_ASSERT(sepPos > 0);

                QByteArray  typeName( data.constData() + 3, sepPos - 3);
                int  type = QMetaType::type( typeName.constData());
                if (!type) {
                    errorLog( QString("Import unknown binary type:") + typeName.constData(),
                              ArnError::Undef);
                    return;
                }
#if QT_VERSION >= 0x050000
                void*  valData = QMetaType::create( type);
#else
                void*  valData = QMetaType::construct( type);
#endif
                Q_ASSERT( valData);
                QDataStream  stream( data);
                stream.setVersion( DATASTREAM_VER);
                stream.skipRawData( sepPos + 1);
#if QT_VERSION >= 0x060000
                QMetaType mt( type);
                bool isOk = mt.load( stream, valData);
#else
                bool isOk = QMetaType::load( stream, type, valData);
#endif
                if (!isOk) {
                    errorLog( QString("Can't' import binary type:") + typeName.constData(),
                              ArnError::Undef);
                    QMetaType::destroy( type, valData);
                    return;
                }

#if QT_VERSION >= 0x060000
                QVariant  value( mt, valData);
#else
                QVariant  value( type, valData);
#endif
                QMetaType::destroy( type, valData);

                setValue( value, ignoreSame, handleData);  // ArnLinkHandle not fully supported for QVariant
                return;
            }
            case Arn::ExportCode::ByteArray:
                setValue( data.mid(1), ignoreSame, handleData);
                return;
            case Arn::ExportCode::String:
                handleData.flags().set( ArnLinkHandle::Flags::Text);
                setValue( data.mid(1), ignoreSame, handleData);
                //setValue( QString::fromUtf8( data.constData() + 1, data.size() - 1),
                //          ignoreSame, handleData);
                return;
            default:  // Not supported code
                return;
            }
        }
    }
    // Normal printable data
    handleData.flags().set( ArnLinkHandle::Flags::Text);
    setValue( data, ignoreSame, handleData);
}


QByteArray  ArnBasicItem::arnExport()  const
{
    if (!_link)  return QByteArray();

    QByteArray  retVal;
    Arn::DataType  arnType = _link->type();

    if (arnType == Arn::DataType::Variant) {
        QVariant value = toVariant();
        QByteArray  typeName( value.typeName());
        int  type = QMetaType::type( typeName.constData());
        if (!type) {
            errorLog( QString("Export unknown type:") + typeName.constData(),
                      ArnError::Undef);
            return QByteArray();
        }

        if (value.canConvert( QVariant::String)) {  // Textual Variant
            retVal += char( Arn::ExportCode::VariantTxt);
            retVal += typeName;
            retVal += ':';
            retVal += value.toString().toUtf8();
        }
        else { // Binary Variant
            QDataStream  stream( &retVal, QIODevice::WriteOnly);
            stream.setVersion( DATASTREAM_VER);
            stream << quint8( Arn::ExportCode::VariantBin);
            stream << quint8(0);  // Spare
            stream << quint8( DATASTREAM_VER);
            stream.writeRawData( typeName.constData(), typeName.size());
            stream << quint8(':');
            if (!QMetaType::save( stream, type, value.constData())) {
                errorLog( QString("Can't export type:") + typeName,
                          ArnError::Undef);
                return QByteArray();
            }
        }
    }
    else if (arnType == Arn::DataType::ByteArray) {
        retVal = char( Arn::ExportCode::ByteArray) + toByteArray();
    }
    else {  // Expect only normal printable (could also be \n etc)
        retVal = toString().toUtf8();
        if (!retVal.isEmpty()) {
            if (retVal.at(0) < 32) {  // Starting char conflicting with Export-code
                retVal.insert( 0, char( Arn::ExportCode::String));  // Stuff String-code at pos 0
            }
        }
    }

    return retVal;
}


QString  ArnBasicItem::toString( bool* isOk)  const
{
    if (!_link)  return QString();

    return _link->toString( isOk);
}


QByteArray  ArnBasicItem::toByteArray( bool* isOk)  const
{
    if (!_link)  return QByteArray();

    return _link->toByteArray( isOk);
}


QVariant  ArnBasicItem::toVariant( bool* isOk)  const
{
    if (!_link)  return QVariant();

    return _link->toVariant( isOk);
}


int  ArnBasicItem::toInt( bool* isOk)  const
{
    if (!_link)  return 0;

    return _link->toInt( isOk);
}


double  ArnBasicItem::toDouble( bool* isOk)  const
{
    if (!_link)  return 0.0;

    return _link->toReal( isOk);
}


ARNREAL  ArnBasicItem::toReal( bool* isOk)  const
{
    if (!_link)  return 0.0;

    return _link->toReal( isOk);
}


bool  ArnBasicItem::toBool( bool* isOk)  const
{
    if (!_link)  return false;

    return _link->toInt( isOk) != 0;
}


uint  ArnBasicItem::toUInt( bool* isOk)  const
{
    if (!_link)  return 0;

    return _link->toByteArray( isOk).toUInt( isOk);
}


qint64  ArnBasicItem::toInt64( bool* isOk)  const
{
    if (!_link)  return 0;

    return _link->toByteArray( isOk).toLongLong( isOk);
}


quint64  ArnBasicItem::toUInt64( bool* isOk)  const
{
    if (!_link)  return 0;

    return _link->toByteArray( isOk).toULongLong( isOk);
}


ArnBasicItem&  ArnBasicItem::operator=( const ArnBasicItem& other)
{
    this->setValue( other);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator=( int val)
{
    this->setValue( val);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator=( ARNREAL val)
{
    this->setValue( val);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator=( const QString& val)
{
    this->setValue( val);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator=( const QByteArray& val)
{
    this->setValue( val);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator=( const char* val)
{
    this->setValue( val);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator=( uint val)
{
    this->setValue( val);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator=( qint64 val)
{
    this->setValue( val);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator=( quint64 val)
{
    this->setValue( val);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator=( const QVariant& val)
{
    this->setValue( val);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator+=( int val)
{
    this->addValue( val);
    return *this;
}


ArnBasicItem&  ArnBasicItem::operator+=( ARNREAL val)
{
    this->addValue( val);
    return *this;
}


void  ArnBasicItem::setValue( const ArnBasicItem& other, int ignoreSame)
{
    ArnLink *link = other._link;

    if (link) {
        switch (link->type()) {
        case Arn::DataType::Int:
            this->setValue( link->toInt(), ignoreSame);
            break;
        case Arn::DataType::Real:
            this->setValue( link->toReal(), ignoreSame);
            break;
        case Arn::DataType::String:
            this->setValue( link->toString(), ignoreSame);
            break;
        case Arn::DataType::ByteArray:
            this->setValue( link->toByteArray(), ignoreSame);
            break;
        case Arn::DataType::Variant:
            this->setValue( link->toVariant(), ignoreSame);
            break;
        case Arn::DataType::Null:
            //cerr << "Attempt to assign null value from "
            //        << other._link->linkPath() << " to "
            //        << this->_link->linkPath() << endl;
            errorLog( QString("Assigning Null"),
                      ArnError::ItemNotSet);
            break;
        default:
            // cerr << "Unknown type when assigning " << other._link->linkPath()
            //        << " to " << this->_link->linkPath() << endl;
            errorLog( QString("Assigning unknown type"),
                      ArnError::ItemNotSet);
        }
    }
    else {
        errorLog( QString("Assigning"),
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setValue( int value, int ignoreSame)
{
    Q_D(ArnBasicItem);

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useUncrossed);
            bool  isOk;
            if ((value == holderLink->toInt( &isOk)) && isOk) {
                holderLink->setIgnoredValue();
                return;
            }
        }
        d->_isAssigning = true;
        _link->setValue( value, d->_id, d->_useUncrossed);
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Assigning int:") + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setValue( ARNREAL value, int ignoreSame)
{
    Q_D(ArnBasicItem);

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useUncrossed);
            bool  isOk;
            if ((value == holderLink->toReal( &isOk)) && isOk) {
                holderLink->setIgnoredValue();
                return;
            }
        }
        d->_isAssigning = true;
        _link->setValue( value, d->_id, d->_useUncrossed);
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Assigning ARNREAL:") + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setValue( bool value, int ignoreSame)
{
    Q_D(ArnBasicItem);

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useUncrossed);
            bool  isOk;
            if ((value == (holderLink->toInt( &isOk) != 0)) && isOk) {
                holderLink->setIgnoredValue();
                return;
            }
        }
        d->_isAssigning = true;
        _link->setValue( value ? 1 : 0, d->_id, d->_useUncrossed);
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Assigning bool:") + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setValue( const QString& value, int ignoreSame)
{
    Q_D(ArnBasicItem);

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useUncrossed);
            bool  isOk;
            if ((value == holderLink->toString( &isOk)) && isOk) {
                holderLink->setIgnoredValue();
                return;
            }
        }
        d->_isAssigning = true;
        _link->setValue( value, d->_id, d->_useUncrossed);
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Assigning string:") + value,
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setValue( const QByteArray& value, int ignoreSame)
{
    Q_D(ArnBasicItem);

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useUncrossed);
            bool  isOk;
            if ((value == holderLink->toByteArray( &isOk)) && isOk) {
                holderLink->setIgnoredValue();
                return;
            }
        }
        d->_isAssigning = true;
        _link->setValue( value, d->_id, d->_useUncrossed);
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Assigning bytearray:") + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setValue( const QVariant& value, int ignoreSame)
{
    Q_D(ArnBasicItem);

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useUncrossed);
            bool  isOk;
            if ((value == holderLink->toVariant( &isOk)) && isOk) {
                holderLink->setIgnoredValue();
                return;
            }
        }
        d->_isAssigning = true;
        _link->setValue( value, d->_id, d->_useUncrossed);
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Assigning variant"),
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setValue( const char* value, int ignoreSame)
{
    setValue( QString::fromUtf8( value), ignoreSame);
}


void  ArnBasicItem::setValue( uint value, int ignoreSame)
{
    if (_link) {
        setValue( QByteArray::number( value), ignoreSame);
    }
    else {
        errorLog( QString("Assigning uint:") + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setValue( qint64 value, int ignoreSame)
{
    if (_link) {
        setValue( QByteArray::number( qlonglong( value)), ignoreSame);
    }
    else {
        errorLog( QString("Assigning int64:") + QString::number( qlonglong( value)),
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setValue( quint64 value, int ignoreSame)
{
    if (_link) {
        setValue( QByteArray::number( qulonglong( value)), ignoreSame);
    }
    else {
        errorLog( QString("Assigning uint64:") + QString::number( qulonglong( value)),
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setBits( int mask, int value, int ignoreSame)
{
    Q_D(ArnBasicItem);

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);  // TODO: mw bidir
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useUncrossed);
            bool  isOk;
            int  oldValue = holderLink->toInt( &isOk);
            if ((((oldValue & ~mask) | (value & mask)) == oldValue) && isOk) {
                holderLink->setIgnoredValue();
                return;
            }
        }
        d->_isAssigning = true;
        _link->setBits( mask, value, d->_id, d->_useUncrossed);
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Op setBits: mask=") + QString::number( mask) +
                  " value=" + QString::number( value), ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::addValue( int value)
{
    Q_D(ArnBasicItem);

    if (_link) {
        d->_isAssigning = true;
        _link->addValue( value, d->_id, d->_useUncrossed);
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Op addValue<int>: value=") + QString::number( value), ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::addValue( ARNREAL value)
{
    Q_D(ArnBasicItem);

    if (_link) {
        d->_isAssigning = true;
        _link->addValue( value, d->_id, d->_useUncrossed);
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Op addValue<real>: value=") + QString::number( value), ArnError::ItemNotOpen);
    }
}


QThread*  ArnBasicItem::thread()  const
{
    Q_D(const ArnBasicItem);

    if (d->_eventHandler)
        return d->_eventHandler->thread();
    else
        return 0;  // MW: Ok?  or QThread::currentThread();
}


/// Must be threaded
bool  ArnBasicItem::sendArnEventLink( ArnEvent* ev)
{
    if (!_link)  return false;

    _link->sendArnEvent( ev);
    return true;
}


void  ArnBasicItem::sendArnEventItem( ArnEvent* ev, bool isAlienThread, bool isLocked)
{
    Q_D(ArnBasicItem);

    if (!ev)  return;  // No event ...

    ev->setTarget( this);
    if (isAlienThread) {
        QMutex*  targetMutex = _link ? _link->getMutex() : 0;
        if (!isLocked)  // Not locked yet, assign mutex for target locking
            ev->setTargetMutex( targetMutex);

        ev->setTargetPendingChain( &d->_pendingEvChain);  //

        if (isLocked)  // Already locked, mutex will be used from now on
            ev->setTargetMutex( targetMutex);
    }

    arnEvent( ev, isAlienThread);
}


void  ArnBasicItem::arnEvent( QEvent* ev, bool isAlienThread)
{
    // Selected ArnEvent handler is called. Default is internal handler.
    // Selected handler must finish with ArnBasicItemEventHandler::defaultEvent( ev).
    // Warning!!! This ArnBasicItem might get deleted (ev->target == 0).

    Q_D(ArnBasicItem);

    if (isAlienThread) {
        if (!d->_eventHandler) {  // No handler
            delete ev;
            return;
        }
        QCoreApplication::postEvent( d->_eventHandler, ev);
    }
    else {
        if (d->_isStdEvHandler || !d->_eventHandler) {
            ArnBasicItemEventHandler::defaultEvent( ev);  // Optimized direct call
        }
        else {
            d->_eventHandler->event( ev);
        }
    }
}


void  ArnBasicItem::setEventHandler( QObject* eventHandler)
{
    Q_D(ArnBasicItem);

    d->_eventHandler   = eventHandler;
    d->_isStdEvHandler = false;
}


QObject*  ArnBasicItem::eventHandler()  const
{
    Q_D(const ArnBasicItem);

    return d->_eventHandler;
}


void ArnBasicItem::setUncrossed( bool isUncrossed)
{
    Q_D(ArnBasicItem);

    d->_useUncrossed = isUncrossed;
}


bool ArnBasicItem::isUncrossed()  const
{
    Q_D(const ArnBasicItem);

    return d->_useUncrossed || !isBiDirMode();
}


bool  ArnBasicItem::isAssigning()  const
{
    Q_D(const ArnBasicItem);

    return d->_isAssigning;
}


void  ArnBasicItem::setForceKeep( bool fk)
{
    setUncrossed( fk);
}


bool  ArnBasicItem::isForceKeep()  const
{
    return isUncrossed();
}


void  ArnBasicItem::setValue( const QByteArray& value, int ignoreSame, ArnLinkHandle& handleData)
{
    Q_D(ArnBasicItem);

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    ArnLinkHandle::Flags&  handleFlags = handleData.flags();
    QString  valueTxt;

    if (handleFlags.is( handleFlags.Text))
        valueTxt = QString::fromUtf8( value.constData(), value.size());

    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useUncrossed);
            bool  isOk;
            if (handleFlags.is( handleFlags.Text)) {
                if ((valueTxt == holderLink->toString( &isOk)) && isOk) {
                    holderLink->setIgnoredValue( handleData);
                    return;
                }
            }
            else {
                if ((value == holderLink->toByteArray( &isOk)) && isOk) {
                    holderLink->setIgnoredValue( handleData);
                    return;
                }
            }
        }
        d->_isAssigning = true;
        if (handleFlags.is( handleFlags.Text)) {
            handleFlags.set( handleFlags.Text, false);  // Text flag not needed anymore
            _link->setValue( valueTxt, d->_id, d->_useUncrossed, handleData);
        }
        else {
            _link->setValue( value, d->_id, d->_useUncrossed, handleData);
        }
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Assigning bytearray (ArnLinkHandle):") + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


void  ArnBasicItem::setValue( const QVariant& value, int ignoreSame, ArnLinkHandle& handleData)
{
    Q_D(ArnBasicItem);

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useUncrossed);
            bool  isOk;
            if ((value == holderLink->toVariant( &isOk)) && isOk) {
                holderLink->setIgnoredValue( handleData);
                return;
            }
        }
        d->_isAssigning = true;
        _link->setValue( value, d->_id, d->_useUncrossed, handleData);
        d->_isAssigning = false;
    }
    else {
        errorLog( QString("Assigning variant (ArnLinkHandle):"),
                  ArnError::ItemNotOpen);
    }
}


QStringList  ArnBasicItem::childItemsMain()  const
{
    // This must be run in main thread as childs only can be deleted there
    return ArnM::itemsMain( _link);
}


void  ArnBasicItem::errorLog( const QString& errText, ArnError err, void* reference)  const
{
    QString  itemText;
    if (_link) {
        itemText = " Item:" + _link->linkPath();
    }
    ArnM::errorLog( errText + itemText, err, reference);
}



ArnBasicItemEventHandler*  ArnBasicItem::getThreadEventHandler()
{
    static QThreadStorage<ArnBasicItemEventHandler*>  evHandlers;

    if (!evHandlers.hasLocalData())
        evHandlers.setLocalData( new ArnBasicItemEventHandler);

    return evHandlers.localData();
}


ArnBasicItemEventHandler::ArnBasicItemEventHandler( QObject* parent)
    : QObject( parent)
{
}


ArnBasicItemEventHandler::~ArnBasicItemEventHandler()
{
}


//// Must be threadsafe
void  ArnBasicItemEventHandler::defaultEvent( QEvent* ev)
{
    int  evIdx = ev->type() - ArnEvent::baseType();
    switch (evIdx) {
    case ArnEvent::Idx::ModeChange:
    {
        ArnEvModeChange*  e = static_cast<ArnEvModeChange*>( ev);
        ArnBasicItem*  target = static_cast<ArnBasicItem*>( e->target());
        if (!target)  return;  // No target, deleted/closed ...

        // qDebug() << "ArnBasicEvModeChange: path=" << e->path() << " mode=" << e->mode()
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
        ArnBasicItem*  target = static_cast<ArnBasicItem*>( e->target());
        if (!target)  return;  // No target, deleted/closed ...

        if (!e->isBelow()) {
            if (Arn::debugLinkDestroy)  qDebug() << "BasicItem arnLinkDestroyed: path=" << target->path();
            target->close();
            e->setTarget(0);  // target is not available any more
        }
        return;
    }
    default:;
    }
}


void  ArnBasicItemEventHandler::customEvent( QEvent* ev)
{
    defaultEvent( ev);
}
