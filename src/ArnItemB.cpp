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

#include "ArnItemB.hpp"
#include "Arn.hpp"
#include <QDataStream>
#include <QUuid>
#include <QTimer>
#include <QMetaObject>
#include <QDebug>


QAtomicInt ArnItemB::_idCount(1);


void  ArnItemB::init()
{
    _link       = 0;
    _reference  = 0;
    _id         = _idCount.fetchAndAddRelaxed(1);

    _useForceKeep    = false;
    _blockEcho       = false;
    _ignoreSameValue = ArnM::defaultIgnoreSameValue();
    _isOnlyEcho      = true;  // Nothing else yet ...

    _syncMode = SyncMode();
    _mode     = Mode();
    _syncModeLinkShare = true;
}


ArnItemB::ArnItemB( QObject *parent)
            : QObject( parent)
{
    init();
}


void  ArnItemB::setupOpenItem( bool isFolder)
{
    addMode( _mode);  // Transfer modes to the link
    modeUpdate(true);
    connect( _link, SIGNAL(retired()), this, SLOT(doArnLinkDestroyed()));

    if (isFolder) {
        connect( _link, SIGNAL(linkCreatedBelow(ArnLink*)),
                 this, SLOT(arnLinkCreatedBelow(ArnLink*)));
        connect( _link, SIGNAL(modeChangedBelow(QString,uint)),
                 this, SLOT(arnModeChangedBelow(QString,uint)));
    }
    else {
        //// Optimize: Only one changed() should be connected depending on thread & pipeMode
        connect( _link, SIGNAL(changed(uint,const ArnLinkHandle&)),
                 this, SLOT(linkValueUpdated(uint,const ArnLinkHandle&)));
        connect( _link, SIGNAL(changed(uint,QByteArray,ArnLinkHandle)),
                 this, SLOT(linkValueUpdated(uint,QByteArray,ArnLinkHandle)));
        connect( _link, SIGNAL(modeChanged(QString,uint)), this, SLOT(modeUpdate()));
    }
}


bool  ArnItemB::open( const QString &path, bool isFolder)
{
    SyncMode  syncMode = _syncModeLinkShare ? _syncMode : SyncMode();
    ArnLink::Flags  flags;
    _link = ArnM::link( path, flags.flagIf( isFolder, flags.Folder) | flags.CreateAllowed, syncMode);
    if (!_link)  return false;

    setupOpenItem( isFolder);
    return true;
}


bool  ArnItemB::open( const QString &path)
{
    bool  isFolder = path.endsWith("/");
    return open( path, isFolder);
}


bool  ArnItemB::openUuidPipe( const QString &path)
{
    QUuid  uuid = QUuid::createUuid();
    QString  fullPath = path + uuid.toString();
    bool  stat = open( fullPath, false);
    setPipeMode();
    return stat;
}


bool  ArnItemB::openFolder( const QString &path)
{
    return open( path, true);
}


bool  ArnItemB::open( const ArnItemB& folder, const QString& itemName, bool isFolder)
{
    ArnLink *parent = folder._link;

    SyncMode  syncMode = _syncModeLinkShare ? _syncMode : SyncMode();
    ArnLink::Flags  flags;
    _link = ArnM::link( parent, itemName,
                            flags.flagIf( isFolder, flags.Folder) | flags.CreateAllowed, syncMode);
    if (!_link)  return false;

    setupOpenItem( isFolder);
    return true;
}


bool  ArnItemB::open( const ArnItemB& folder, const QString& itemName)
{
    bool  isFolder = itemName.endsWith("/");
    return open( folder, itemName, isFolder);
}


void  ArnItemB::close()
{
    if (!_link)  return;

    _link->deref();
    _link     = 0;
    _syncMode = SyncMode();
    _mode     = Mode();
}


void  ArnItemB::destroyLink()
{
    ArnM::destroyLink( _link);
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


bool  ArnItemB::isBiDir()  const
{
    if (!_link)  return false;

    return _link->isProvider();
}


ArnLink::Type  ArnItemB::type()  const
{
    if (!_link)  return ArnLink::Type::Null;

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

    if (_link->isPipeMode()) {  // Pipe-mode never IgnoreSameValue
        setIgnoreSameValue(false);
    }
}


void  ArnItemB::addSyncMode( SyncMode syncMode, bool linkShare)
{
    _syncModeLinkShare = linkShare;
    _syncMode.f |= syncMode.f;
    if (_syncModeLinkShare  &&  _link) {
        _link->addSyncMode( _syncMode.f);
    }
}


ArnItemB::SyncMode  ArnItemB::syncMode()  const
{
    if (_syncModeLinkShare  &&  _link) {
        return ArnItemB::SyncMode::fromInt( _link->syncMode());
    }
    return _syncMode;
}


ArnItemB&  ArnItemB::setBiDirMode()
{
    _mode.f |= _mode.BiDir;
    if (!_link)  return *this;

    if (_link->isBiDirMode())  return *this;  // Already is bidirectional mode

    /// Bidirectional-mode is the pair of value & provider
    ArnLink*  twinLink = ArnM::addTwin( _link, syncMode());
    twinLink->deref();

    return *this;
}


bool  ArnItemB::isBiDirMode()  const
{
    if (!_link)  return _mode.is( _mode.BiDir);

    return _link->isBiDirMode();
}


ArnItemB&  ArnItemB::setPipeMode()
{
    _mode.f |= _mode.Pipe | _mode.BiDir;
    if (!_link)  return *this;

    if (_link->isPipeMode())  return *this;  // Already is pipe mode

    _ignoreSameValue = false;
    // Pipe-mode demands the pair of value & provider
    ArnLink*  twinLink = ArnM::addTwin( _link, syncMode());
    _link->setPipeMode( true);
    twinLink->deref();

    return *this;
}


bool  ArnItemB::isPipeMode()  const
{
    if (!_link)  return _mode.is( _mode.Pipe);

    return _link->isPipeMode();
}


ArnItemB&  ArnItemB::setSaveMode()
{
    _mode.f |= _mode.Save;
    if (!_link)  return *this;

    _link->setSaveMode( true);
    return *this;
}


bool  ArnItemB::isSaveMode()  const
{
    if (!_link)  return _mode.is( _mode.Save);

    return _link->isSaveMode();
}


ArnItemB&  ArnItemB::setMaster()
{
    if (_link) {
        ArnM::errorLog( QString(tr("Setting item/link as master")),
                            ArnError::AlreadyOpen);
    }
    addSyncMode( SyncMode::Master, true);
    return *this;
}


bool  ArnItemB::isMaster()  const
{
    return syncMode().is( SyncMode::Master);
}


ArnItemB&  ArnItemB::setAutoDestroy()
{
    if (_link) {
        ArnM::errorLog( QString(tr("Setting item/link to autoDestroy")),
                            ArnError::AlreadyOpen);
    }
    addSyncMode( SyncMode::AutoDestroy, true);
    return *this;
}


bool  ArnItemB::isAutoDestroy()  const
{
    return syncMode().is( SyncMode::AutoDestroy);
}


void  ArnItemB::addMode( Mode mode)
{
    _mode.f |= mode.f;  // Just in case, transfer all modes

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


ArnItemB::Mode  ArnItemB::getMode()  const
{
    return getMode( _link);
}


/// Use with care, link must be "referenced" before use, otherwise it might have been deleted
ArnItemB::Mode  ArnItemB::getMode( ArnLink* link)  const
{
    if (!link)  return _mode;

    Mode  mode;
    if (link->isPipeMode())   mode.f |= mode.Pipe;
    if (link->isBiDirMode())  mode.f |= mode.BiDir;
    if (link->isSaveMode())   mode.f |= mode.Save;

    return mode;
}


void  ArnItemB::setIgnoreSameValue( bool isIgnore)
{
    _ignoreSameValue = isPipeMode() ? false :isIgnore;
}


bool  ArnItemB::isIgnoreSameValue()
{
    return _ignoreSameValue;
}


QString  ArnItemB::path( ArnLink::NameF nameF)  const
{
    if (!_link)  return QString();

    return _link->linkPath( nameF);
}


QString  ArnItemB::name( ArnLink::NameF nameF)  const
{
    if (!_link)  return QString();

    return _link->linkName( nameF);
}


void  ArnItemB::arnImport( const QByteArray& data, int ignoreSame)
{
    ArnLinkHandle  handle;
    arnImport( data, ignoreSame, handle);
}


void  ArnItemB::arnImport( const QByteArray& data, int ignoreSame, ArnLinkHandle& handleData)
{
    if (!data.isEmpty()) {
        if (data.at(0) < 32) {  // Assume Export-code
            switch (ExportCode::fromInt( data.at(0))) {
            case ExportCode::Variant: {  // Legacy
                QVariant  value;
                QDataStream  stream( data);
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
                    errorLog( QString(tr("Import unknown type:") + typeName.constData()),
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
                int  sepPos = data.indexOf(':', 1);
                Q_ASSERT(sepPos > 0);

                QByteArray  typeName( data.constData() + 1, sepPos - 1);
                int  type = QMetaType::type( typeName.constData());
                if (!type) {
                    errorLog( QString(tr("Import unknown type:") + typeName.constData()),
                              ArnError::Undef);
                    return;
                }
                QVariant  value( type, data.constData() + sepPos + 1);

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
    //setValue( QString::fromUtf8( data.constData(), data.size()), ignoreSame, handleData);
}


QByteArray  ArnItemB::arnExport()  const
{
    if (!_link)  return QByteArray();

    QByteArray  retVal;
    ArnLink::Type  arnType = _link->type();

    if (arnType == ArnLink::Type::Variant) {
        QVariant value = toVariant();
        const char*  typeName = value.typeName();
        if (!typeName)
            typeName = "<Invalid>";
        int  type = QMetaType::type( typeName);
        if (!type) {
            errorLog( QString(tr("Export unknown type:") + typeName),
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
#if 0       //// Legacy
            QDataStream  stream( &retVal, QIODevice::WriteOnly);
            stream << qint8( ExportCode::Variant) << value;
            // retVal will contain Export-code at pos 0 followed by the QVariant
#else
            QDataStream  stream( &retVal, QIODevice::WriteOnly);
            stream << qint8( ExportCode::VariantBin) << typeName << ":";
            if (!QMetaType::save( stream, type, value.constData())) {
                errorLog( QString(tr("Can't export type:") + typeName),
                          ArnError::Undef);
                return QByteArray();
            }
#endif
        }
    }
    else if (arnType == ArnLink::Type::ByteArray) {
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

    return _link->toDouble();
}


bool  ArnItemB::toBool() const
{
    if (!_link)  return false;

    return _link->toInt() != 0;
}


void  ArnItemB::setValue( const ArnItemB& other, int ignoreSame)
{
    ArnLink *link = other._link;

    if (link) {
        switch (link->type()) {
        case ArnLink::Type::Int:
            this->setValue( link->toInt(), ignoreSame);
            break;
        case ArnLink::Type::Double:
            this->setValue( link->toDouble(), ignoreSame);
            break;
        case ArnLink::Type::String:
            this->setValue( link->toString(), ignoreSame);
            break;
        case ArnLink::Type::ByteArray:
            this->setValue( link->toByteArray(), ignoreSame);
            break;
        case ArnLink::Type::Variant:
            this->setValue( link->toVariant(), ignoreSame);
            break;
        case ArnLink::Type::Null:
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
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == _link->holderLink( _useForceKeep)->toInt()) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( QByteArray::number( value), _id, _useForceKeep,
                      ArnLinkHandle( ArnLinkHandle::Flags::Text));
        else
            _link->setValue( value, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning int:")) + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( double value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == _link->holderLink( _useForceKeep)->toDouble()) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( QByteArray::number( value), _id, _useForceKeep,
                      ArnLinkHandle( ArnLinkHandle::Flags::Text));
        else
            _link->setValue( value, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning double:")) + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( bool value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == (_link->holderLink( _useForceKeep)->toInt() != 0)) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( QByteArray::number( value ? 1 : 0), _id, _useForceKeep,
                      ArnLinkHandle( ArnLinkHandle::Flags::Text));
        else
            _link->setValue( value ? 1 : 0, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning bool:")) + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( const QString& value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == _link->holderLink( _useForceKeep)->toString()) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( value.toUtf8(), _id, _useForceKeep,
                      ArnLinkHandle( ArnLinkHandle::Flags::Text));
        else
            _link->setValue( value, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning string:")) + value,
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( const QByteArray& value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == _link->holderLink( _useForceKeep)->toByteArray()) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( value, _id, _useForceKeep,
                      ArnLinkHandle());
        else
            _link->setValue( value, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning bytearray:")) + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( const QVariant& value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == _link->holderLink( _useForceKeep)->toVariant()) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            // QVariant is not realy supported for pipe in threaded usage
            trfValue( value.toString().toUtf8(), _id, _useForceKeep,
                      ArnLinkHandle( ArnLinkHandle::Flags::Text));
        else
            _link->setValue( value, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning variant")),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::setValue( const QByteArray& value, int ignoreSame, ArnLinkHandle& handleData)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    ArnLinkHandle::Flags&  handleFlags = handleData._flags;
    QString  valueTxt;

    if (handleFlags.is( handleFlags.Text))
        valueTxt = QString::fromUtf8( value.constData(), value.size());

    if (_link) {
        if (isIgnoreSame) {
            if (handleFlags.is( handleFlags.Text)) {
                if (valueTxt == _link->holderLink( _useForceKeep)->toString())
                    return;
            }
            else {
                if (value == _link->holderLink( _useForceKeep)->toByteArray())
                    return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( value, _id, _useForceKeep, handleData);
        else
            if (handleFlags.is( handleFlags.Text)) {
                handleFlags.set( handleFlags.Text, false);  // Text flag not needed anymore
                _link->setValue( valueTxt, _id, _useForceKeep, handleData);
            }
            else
                _link->setValue( value, _id, _useForceKeep, handleData);
    }
    else {
        errorLog( QString(tr("Assigning bytearray (ArnLinkHandle):")) + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItemB::trfValue( const QByteArray& value, int sendId, bool forceKeep,
                         const ArnLinkHandle& handleData)
{
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


void  ArnItemB::errorLog( QString errText, ArnError err, void* reference)  const
{
    QString  itemText;
    if (_link) {
        itemText = " Item:" + _link->linkPath();
    }
    ArnM::errorLog( errText + itemText, err, reference);
}


void  ArnItemB::linkValueUpdated( uint sendId, const ArnLinkHandle& handleData)
{
    if (_blockEcho  &&  sendId == _id) {  // This update was initiated from this Item, it can be blocked ...
        return;
    }
    _isOnlyEcho = (sendId == _id) ? _isOnlyEcho : false;

    itemUpdate( handleData);
}


void  ArnItemB::linkValueUpdated( uint sendId, QByteArray value, ArnLinkHandle handleData)
{
    if (_blockEcho  &&  sendId == _id) {  // This update was initiated from this Item, it can be blocked ...
        return;
    }
    _isOnlyEcho = (sendId == _id) ? _isOnlyEcho : false;

    itemUpdate( handleData, &value);
}


void  ArnItemB::arnLinkCreatedBelow( ArnLink* link)
{
    if (!link->isFolder()) {
        itemCreatedBelow( link->linkPath());
    }
}


void  ArnItemB::arnModeChangedBelow( QString path, uint linkId)
{
    ArnLink::Flags  flags;
    ArnLink*  link = ArnM::link( path, flags.SilentError);
    if (!link)  return;  // Item has been lost (deleted?)

    itemModeChangedBelow( path, linkId, getMode( link));
    link->deref();  // Release the link-reference
}


void  ArnItemB::doArnLinkDestroyed()
{
    qDebug() << "Item arnLinkDestroyed: path=" << path();
    emit arnLinkDestroyed();
    close();
}


ArnItemB::~ArnItemB()
{
    close();
}
