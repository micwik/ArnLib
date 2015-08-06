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

#include "ArnInc/ArnItemB.hpp"
#include "private/ArnItemB_p.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/ArnEvent.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnLink.hpp"
#include <QDataStream>
#include <QUuid>
#include <QTimer>
#include <QMetaObject>
#include <QDebug>


QAtomicInt ArnItemBPrivate::_idCount(1);


ArnItemBPrivate::ArnItemBPrivate()
{
    _reference = 0;
    _id        = _idCount.fetchAndAddRelaxed(1);

    _useForceKeep    = false;
    _blockEcho       = false;
    _enableSetValue  = true;
    _enableUpdNotify = true;
    _ignoreSameValue = ArnM::defaultIgnoreSameValue();
    _isOnlyEcho      = true;  // Nothing else yet ...

    _syncMode          = Arn::ObjectSyncMode();
    _mode              = Arn::ObjectMode();
    _syncModeLinkShare = true;
}


ArnItemBPrivate::~ArnItemBPrivate()
{
}


void  ArnItemB::init()
{
    _link = 0;
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
    close();
    delete d_ptr;
}


void  ArnItemB::setupOpenItem( bool isFolder)
{
    Q_D(ArnItemB);

    connect( _link, SIGNAL(retired()), this, SLOT(doArnLinkDestroyed()));

    if (!isFolder) {
        //// Optimize: Only one changed() should be connected depending on thread & pipeMode
        connect( _link, SIGNAL(changed(uint,const ArnLinkHandle&)),
                 this, SLOT(linkValueUpdated(uint,const ArnLinkHandle&)));
        connect( _link, SIGNAL(changed(uint,QByteArray,ArnLinkHandle)),
                 this, SLOT(linkValueUpdated(uint,QByteArray,ArnLinkHandle)));
    }
    addMode( d->_mode);  // Transfer modes to the link
    modeUpdate(true);
}


bool  ArnItemB::open( const QString& path)
{
    Q_D(ArnItemB);

    if (_link)
        close();

    Arn::ObjectSyncMode  syncMode = d->_syncModeLinkShare ? d->_syncMode : Arn::ObjectSyncMode();
    _link = ArnM::link( path,  Arn::LinkFlags::CreateAllowed, syncMode);
    if (!_link)  return false;

    _link->subscribe( this);
    setupOpenItem( _link->isFolder());
#ifdef ARNITEMB_INCPATH
    d->_path = path;
#endif
    return true;
}


bool  ArnItemB::openUuid( const QString& path)
{
    QUuid  uuid = QUuid::createUuid();
    bool  isProvider = Arn::isProviderPath( path);

    QString  uuidPath = isProvider ? Arn::twinPath( path) : path;  // Allways Requester path (no "!")
    uuidPath += uuid.toString();
    if (isProvider)
        uuidPath = Arn::twinPath( uuidPath);  // Restore original "!"

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


void  ArnItemB::close()
{
    Q_D(ArnItemB);

    if (!_link)  return;

    _link->deref( this);
    _link        = 0;
    d->_syncMode = Arn::ObjectSyncMode();
    d->_mode     = Arn::ObjectMode();
}


void  ArnItemB::destroyLink( bool isGlobal)
{
    ArnM::destroyLink( _link, isGlobal);
}


bool  ArnItemB::isOpen()  const
{
    return _link != 0;
}


bool  ArnItemB::isFolder()  const
{
    if (!_link)  return false;

    return _link->isFolder();
}


bool  ArnItemB::isProvider()  const
{
    if (!_link)  return false;

    return _link->isProvider();
}


Arn::DataType  ArnItemB::type()  const
{
    if (!_link)  return Arn::DataType::Null;

    return _link->type();
}


uint  ArnItemB::linkId()  const
{
    if (!_link)  return 0;

    return _link->linkId();
}


void  ArnItemB::modeUpdate( bool isSetup)
{
    Q_UNUSED(isSetup);

    if (isPipeMode()) {  // Pipe-mode never IgnoreSameValue
        setIgnoreSameValue(false);
    }
}


void  ArnItemB::addSyncMode( Arn::ObjectSyncMode syncMode, bool linkShare)
{
    Q_D(ArnItemB);

    d->_syncModeLinkShare = linkShare;
    d->_syncMode.f |= syncMode.f;
    if (d->_syncModeLinkShare  &&  _link) {
        _link->addSyncMode( d->_syncMode);
    }
}


void  ArnItemB::resetOnlyEcho()
{
    Q_D(ArnItemB);

    d->_isOnlyEcho = true;
}


bool  ArnItemB::isOnlyEcho() const
{
    Q_D(const ArnItemB);

    return d->_isOnlyEcho;
}


void  ArnItemB::setBlockEcho( bool blockEcho)
{
    Q_D(ArnItemB);

    d->_blockEcho = blockEcho;
}


bool ArnItemB::isRetiredGlobal()
{
    return _link ? _link->isRetiredGlobal() : false;
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


Arn::ObjectSyncMode  ArnItemB::syncMode()  const
{
    Q_D(const ArnItemB);

    if (d->_syncModeLinkShare  &&  _link) {
        return Arn::ObjectSyncMode::fromInt( _link->syncMode());
    }
    return d->_syncMode;
}


ArnItemB&  ArnItemB::setBiDirMode()
{
    Q_D(ArnItemB);

    d->_mode.set( Arn::ObjectMode::BiDir);
    if (!_link)  return *this;

    if (_link->isBiDirMode())  return *this;  // Already is bidirectional mode

    /// Bidirectional-mode is the pair of value & provider
    ArnLink*  twinLink = ArnM::addTwin( _link->linkPath(), _link, syncMode());
    twinLink->deref();

    return *this;
}


bool  ArnItemB::isBiDirMode()  const
{
    Q_D(const ArnItemB);

    if (!_link)  return d->_mode.is( Arn::ObjectMode::BiDir);

    return _link->isBiDirMode();
}


ArnItemB&  ArnItemB::setPipeMode()
{
    Q_D(ArnItemB);

    d->_mode.set( Arn::ObjectMode::Pipe).set( Arn::ObjectMode::BiDir);
    if (!_link)  return *this;

    if (_link->isPipeMode())  return *this;  // Already is pipe mode

    d->_ignoreSameValue = false;
    //// Pipe-mode demands the pair of value & provider
    ArnLink*  twinLink = ArnM::addTwin( _link->linkPath(), _link, syncMode());
    _link->setPipeMode( true);
    twinLink->deref();

    return *this;
}


bool  ArnItemB::isPipeMode()  const
{
    Q_D(const ArnItemB);

    if (!_link)  return d->_mode.is( Arn::ObjectMode::Pipe);

    return _link->isPipeMode();
}


ArnItemB&  ArnItemB::setSaveMode()
{
    Q_D(ArnItemB);

    d->_mode.set( Arn::ObjectMode::Save);
    if (!_link)  return *this;

    _link->setSaveMode( true);
    return *this;
}


bool  ArnItemB::isSaveMode()  const
{
    Q_D(const ArnItemB);

    if (!_link)  return d->_mode.is( Arn::ObjectMode::Save);

    return _link->isSaveMode();
}


ArnItemB&  ArnItemB::setMaster()
{
    if (_link) {
        ArnM::errorLog( QString(tr("Setting item/link as master")),
                            ArnError::AlreadyOpen);
    }
    addSyncMode( Arn::ObjectSyncMode::Master, true);
    return *this;
}


bool  ArnItemB::isMaster()  const
{
    return syncMode().is( Arn::ObjectSyncMode::Master);
}


ArnItemB&  ArnItemB::setAutoDestroy()
{
    if (_link) {
        ArnM::errorLog( QString(tr("Setting item/link to autoDestroy")),
                            ArnError::AlreadyOpen);
    }
    addSyncMode( Arn::ObjectSyncMode::AutoDestroy, true);
    return *this;
}


bool  ArnItemB::isAutoDestroy()  const
{
    return syncMode().is( Arn::ObjectSyncMode::AutoDestroy);
}


void  ArnItemB::addMode( Arn::ObjectMode mode)
{
    Q_D(ArnItemB);

    d->_mode.f |= mode.f;  // Just in case, transfer all modes

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


Arn::ObjectMode  ArnItemB::getMode()  const
{
    return getMode( _link);
}


/// Use with care, link must be "referenced" before use, otherwise it might have been deleted
Arn::ObjectMode  ArnItemB::getMode( ArnLink* link)  const
{
    Q_D(const ArnItemB);

    if (!link)  return d->_mode;

    return link->getMode();
}


void  ArnItemB::setIgnoreSameValue( bool isIgnore)
{
    Q_D(ArnItemB);

    d->_ignoreSameValue = isPipeMode() ? false : isIgnore;
}


bool  ArnItemB::isIgnoreSameValue()  const
{
    Q_D(const ArnItemB);

    return d->_ignoreSameValue;
}


QString  ArnItemB::path( Arn::NameF nameF)  const
{
    if (!_link)  return QString();

    return _link->linkPath( nameF);
}


QString  ArnItemB::name( Arn::NameF nameF)  const
{
    if (!_link)  return QString();

    return _link->linkName( nameF);
}


void  ArnItemB::setReference( void* reference)
{
    Q_D(ArnItemB);

    d->_reference = reference;
}


void*  ArnItemB::reference()  const
{
    Q_D(const ArnItemB);

    return d->_reference;
}


uint  ArnItemB::itemId()  const
{
    Q_D(const ArnItemB);

    return d->_id;
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

    if (!data.isEmpty()) {
        if (data.at(0) < 32) {  // Assume Export-code
            switch (ExportCode::fromInt( data.at(0))) {
            case ExportCode::Variant: {  // Legacy
                QVariant  value;
                QDataStream  stream( data);
                stream.setVersion( DATASTREAM_VER);
                quint8  dummy;  // Will get Export-code
                stream >> dummy >> value;
                setValue( value, ignoreSame);  // ArnLinkHandle not supported for QVariant
                return;
            }
            case ExportCode::VariantTxt: {
                int  sepPos = data.indexOf(':', 1);
                Q_ASSERT(sepPos > 0);

                QByteArray  typeName( data.constData() + 1, sepPos - 1);
                int  type = QMetaType::type( typeName.constData());
                if (!type) {
                    errorLog( QString(tr("Import unknown text type:") + typeName.constData()),
                              ArnError::Undef);
                    return;
                }
                QVariant  value( QString::fromUtf8( data.constData() + sepPos + 1,
                                                    data.size() - sepPos - 1));
                if (!value.convert( QVariant::Type( type))) {
                    errorLog( QString(tr("Can't' import data type:") + typeName.constData()),
                              ArnError::Undef);
                    return;
                }

                setValue( value, ignoreSame);  // ArnLinkHandle not supported for QVariant
                return;
            }
            case ExportCode::VariantBin: {
                if ((data.size() < 3) || (data.at(2) != DATASTREAM_VER)) {
                    errorLog( QString(tr("Import not same DataStream version")),
                              ArnError::Undef);
                    return;
                }
                int  sepPos = data.indexOf(':', 3);
                Q_ASSERT(sepPos > 0);

                QByteArray  typeName( data.constData() + 3, sepPos - 3);
                int  type = QMetaType::type( typeName.constData());
                if (!type) {
                    errorLog( QString(tr("Import unknown binary type:") + typeName.constData()),
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
                if (!QMetaType::load( stream, type, valData)) {
                    errorLog( QString(tr("Can't' import binary type:") + typeName.constData()),
                              ArnError::Undef);
                    QMetaType::destroy( type, valData);
                    return;
                }

                QVariant  value( type, valData);
                QMetaType::destroy( type, valData);

                setValue( value, ignoreSame);  // ArnLinkHandle not supported for QVariant
                return;
            }
            case ExportCode::ByteArray:
                setValue( data.mid(1), ignoreSame, handleData);
                return;
            case ExportCode::String:
                handleData._flags.set( handleData._flags.Text);
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
    handleData._flags.set( handleData._flags.Text);
    setValue( data, ignoreSame, handleData);
}


QByteArray  ArnItemB::arnExport()  const
{
    if (!_link)  return QByteArray();

    QByteArray  retVal;
    Arn::DataType  arnType = _link->type();

    if (arnType == Arn::DataType::Variant) {
        QVariant value = toVariant();
        QByteArray  typeName( value.typeName());
        int  type = QMetaType::type( typeName.constData());
        if (!type) {
            errorLog( QString(tr("Export unknown type:") + typeName.constData()),
                      ArnError::Undef);
            return QByteArray();
        }

        if (value.canConvert( QVariant::String)) {  // Textual Variant
            retVal += char( ExportCode::VariantTxt);
            retVal += typeName;
            retVal += ':';
            retVal += value.toString().toUtf8();
        }
        else { // Binary Variant
            QDataStream  stream( &retVal, QIODevice::WriteOnly);
            stream.setVersion( DATASTREAM_VER);
            stream << quint8( ExportCode::VariantBin);
            stream << quint8(0);  // Spare
            stream << quint8( DATASTREAM_VER);
            stream.writeRawData( typeName.constData(), typeName.size());
            stream << quint8(':');
            if (!QMetaType::save( stream, type, value.constData())) {
                errorLog( QString(tr("Can't export type:") + typeName),
                          ArnError::Undef);
                return QByteArray();
            }
        }
    }
    else if (arnType == Arn::DataType::ByteArray) {
        retVal = char( ExportCode::ByteArray) + toByteArray();
    }
    else {  // Expect only normal printable (could also be \n etc)
        retVal = toString().toUtf8();
        if (!retVal.isEmpty()) {
            if (retVal.at(0) < 32) {  // Starting char conflicting with Export-code
                retVal.insert( 0, char( ExportCode::String));  // Stuff String-code at pos 0
            }
        }
    }

    return retVal;
}


QString  ArnItemB::toString() const
{
    if (!_link)  return QString();

    return _link->toString();
}


QByteArray  ArnItemB::toByteArray() const
{
    if (!_link)  return QByteArray();

    return _link->toByteArray();
}


QVariant  ArnItemB::toVariant() const
{
    if (!_link)  return QVariant();

    return _link->toVariant();
}


int  ArnItemB::toInt() const
{
    if (!_link)  return 0;

    return _link->toInt();
}


double  ArnItemB::toDouble() const
{
    if (!_link)  return 0.0;

    return _link->toReal();
}


ARNREAL  ArnItemB::toReal() const
{
    if (!_link)  return 0.0;

    return _link->toReal();
}


bool  ArnItemB::toBool() const
{
    if (!_link)  return false;

    return _link->toInt() != 0;
}


void  ArnItemB::setValue( const ArnItemB& other, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

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
            errorLog( QString(tr("Assigning Null")),
                      ArnError::ItemNotSet);
            break;
        default:
            // cerr << "Unknown type when assigning " << other._link->linkPath()
            //        << " to " << this->_link->linkPath() << endl;
            errorLog( QString(tr("Assigning unknown type")),
                      ArnError::ItemNotSet);
        }
    }
    else {
        errorLog( QString(tr("Assigning")),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( int value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useForceKeep);
            if ((holderLink->type() != Arn::DataType::Null) && (value == holderLink->toInt())) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( QByteArray::number( value), d->_id, d->_useForceKeep,
                      ArnLinkHandle( ArnLinkHandle::Flags::Text));
        else
            _link->setValue( value, d->_id, d->_useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning int:")) + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( ARNREAL value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useForceKeep);
            if ((holderLink->type() != Arn::DataType::Null) && (value == holderLink->toReal())) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( QByteArray::number( value), d->_id, d->_useForceKeep,
                      ArnLinkHandle( ArnLinkHandle::Flags::Text));
        else
            _link->setValue( value, d->_id, d->_useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning ARNREAL:")) + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( bool value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useForceKeep);
            if ((holderLink->type() != Arn::DataType::Null) && (value == (holderLink->toInt() != 0))) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( QByteArray::number( value ? 1 : 0), d->_id, d->_useForceKeep,
                      ArnLinkHandle( ArnLinkHandle::Flags::Text));
        else
            _link->setValue( value ? 1 : 0, d->_id, d->_useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning bool:")) + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( const QString& value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useForceKeep);
            if ((holderLink->type() != Arn::DataType::Null) && (value == holderLink->toString())) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( value.toUtf8(), d->_id, d->_useForceKeep,
                      ArnLinkHandle( ArnLinkHandle::Flags::Text));
        else
            _link->setValue( value, d->_id, d->_useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning string:")) + value,
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( const QByteArray& value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useForceKeep);
            if ((holderLink->type() != Arn::DataType::Null) && (value == holderLink->toByteArray())) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( value, d->_id, d->_useForceKeep,
                      ArnLinkHandle());
        else
            _link->setValue( value, d->_id, d->_useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning bytearray:")) + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( const QVariant& value, int ignoreSame)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useForceKeep);
            if ((holderLink->type() != Arn::DataType::Null) && (value == holderLink->toVariant())) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            // QVariant is not realy supported for pipe in threaded usage
            trfValue( value.toString().toUtf8(), d->_id, d->_useForceKeep,
                      ArnLinkHandle( ArnLinkHandle::Flags::Text));
        else
            _link->setValue( value, d->_id, d->_useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning variant")),
                  ArnError::ItemNotOpen);
    }
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


void  ArnItemB::setForceKeep( bool fk)
{
    Q_D(ArnItemB);

    d->_useForceKeep = fk;
}


bool  ArnItemB::isForceKeep()  const
{
    Q_D(const ArnItemB);

    return d->_useForceKeep;
}


void  ArnItemB::setValue( const QByteArray& value, int ignoreSame, ArnLinkHandle& handleData)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    ArnLinkHandle::Flags&  handleFlags = handleData._flags;
    QString  valueTxt;

    if (handleFlags.is( handleFlags.Text))
        valueTxt = QString::fromUtf8( value.constData(), value.size());

    if (_link) {
        if (isIgnoreSame) {
            ArnLink*  holderLink = _link->holderLink( d->_useForceKeep);
            if (handleFlags.is( handleFlags.Text)) {
                if ((holderLink->type() != Arn::DataType::Null) && (valueTxt == holderLink->toString()))
                    return;
            }
            else {
                if ((holderLink->type() != Arn::DataType::Null) && (value == holderLink->toByteArray()))
                    return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( value, d->_id, d->_useForceKeep, handleData);
        else
            if (handleFlags.is( handleFlags.Text)) {
                handleFlags.set( handleFlags.Text, false);  // Text flag not needed anymore
                _link->setValue( valueTxt, d->_id, d->_useForceKeep, handleData);
            }
            else
                _link->setValue( value, d->_id, d->_useForceKeep, handleData);
    }
    else {
        errorLog( QString(tr("Assigning bytearray (ArnLinkHandle):")) + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::trfValue( const QByteArray& value, int sendId, bool forceKeep,
                         const ArnLinkHandle& handleData)
{
    Q_D(ArnItemB);

    if (!d->_enableSetValue)  return;

    QMetaObject::invokeMethod( _link, "trfValue", Qt::QueuedConnection,
                               Q_ARG( QByteArray, value),
                               Q_ARG( int, sendId),
                               Q_ARG( bool, forceKeep),
                               Q_ARG( ArnLinkHandle, handleData));
}


QStringList  ArnItemB::childItemsMain()  const
{
    // This must be run in main thread as childs only can be deleted there
    return ArnM::itemsMain( _link);
}


void  ArnItemB::errorLog( const QString& errText, ArnError err, void* reference)  const
{
    QString  itemText;
    if (_link) {
        itemText = " Item:" + _link->linkPath();
    }
    ArnM::errorLog( errText + itemText, err, reference);
}


void  ArnItemB::linkValueUpdated( uint sendId, const ArnLinkHandle& handleData)
{
    Q_D(ArnItemB);

    if (d->_blockEcho  &&  sendId == d->_id)  // Update was initiated from this Item, it can be blocked ...
        return;

    d->_isOnlyEcho = (sendId == d->_id) ? d->_isOnlyEcho : false;

    if (d->_enableUpdNotify)
        itemUpdated( handleData);
}


void  ArnItemB::linkValueUpdated( uint sendId, const QByteArray& value, ArnLinkHandle handleData)
{
    Q_D(ArnItemB);

    if (d->_blockEcho  &&  sendId == d->_id)  // Update was initiated from this Item, it can be blocked ...
        return;

    d->_isOnlyEcho = (sendId == d->_id) ? d->_isOnlyEcho : false;

    if (d->_enableUpdNotify)
        itemUpdated( handleData, &value);
}


bool  ArnItemB::event( QEvent* ev)
{
    QEvent::Type  type = ev->type();
    if (type == ArnEvLinkCreate::type()) {
        ArnEvLinkCreate*  e = static_cast<ArnEvLinkCreate*>( ev);
        qDebug() << "ArnEvLinkCreate: path=" << e->path() << " inItemPath=" << path();
        if (!Arn::isFolderPath( e->path())) {
            itemCreatedBelow( e->path());
        }
        return true;
    }
    if (type == ArnEvModeChange::type()) {
        ArnEvModeChange*  e = static_cast<ArnEvModeChange*>( ev);
        // qDebug() << "ArnEvModeChange: path=" << e->path() << " mode=" << e->mode()
        //          << " inItemPath=" << path();
        if (isFolder())
            itemModeChangedBelow( e->path(), e->linkId(),e->mode());
        else
            modeUpdate();
        return true;
    }

    return QObject::event( ev);
}


void  ArnItemB::doArnLinkDestroyed()
{
    if (Arn::debugLinkDestroy)  qDebug() << "Item arnLinkDestroyed: path=" << path();
    emit arnLinkDestroyed();
    close();
}
