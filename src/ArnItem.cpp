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

#include "ArnItem.hpp"
#include "Arn.hpp"
#include <QDataStream>
#include <QUuid>
#include <QTimer>
#include <QMetaObject>
#include <QDebug>


QAtomicInt ArnItem::_idCount(1);


void  ArnItem::init()
{
    _link       = 0;
    _reference  = 0;
    _delayTimer = 0;
    _id         = _idCount.fetchAndAddRelaxed(1);

    _useForceKeep    = false;
    _blockEcho       = false;
    _isTemplate      = false;
    _ignoreSameValue = Arn::defaultIgnoreSameValue();
    _isOnlyEcho      = true;  // Nothing else yet ...

    _syncMode = SyncMode();
    _mode     = Mode();
    _syncModeLinkShare = true;

    _emitChanged          = 0;
    _emitChangedInt       = 0;
    _emitChangedDouble    = 0;
    _emitChangedBool      = 0;
    _emitChangedString    = 0;
    _emitChangedByteArray = 0;
}


ArnItem::ArnItem( QObject *parent)
            : QObject( parent)
{
    init();
}


ArnItem::ArnItem( const QString& path, QObject *parent)
            : QObject( parent)
{
    init();
    this->open( path);
}


ArnItem::ArnItem( const ArnItem& folder_template, const QString& itemName_path, QObject *parent)
            : QObject( parent)
{
    init();
    /// Double usage 2 modes: template, folder
    if (folder_template.isTemplate()) {  // Template mode: Copy syncMode & Mode from template to this Item
        this->addSyncMode( folder_template.syncMode(), true);
        this->addMode( folder_template.getMode());
        this->open( itemName_path);  // (path)
    }
    else {  // folder mode: Open itemName based on folder item
        this->open( folder_template, itemName_path);  // (folder, itemName)
    }
}


void  ArnItem::setupOpenItem( bool isFolder)
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
        connect( _link, SIGNAL(changed(uint)), this, SLOT(linkValueUpdated(uint)));
        connect( _link, SIGNAL(changed(uint,QByteArray)),
                 this, SLOT(linkValueUpdated(uint,QByteArray)));
        connect( _link, SIGNAL(modeChanged(QString,uint)), this, SLOT(modeUpdate()));
    }
}


bool  ArnItem::open( const QString &path, bool isFolder)
{
    SyncMode  syncMode = _syncModeLinkShare ? _syncMode : SyncMode();
    ArnLink::Flags  flags;
    _link = Arn::link( path, flags.flagIf( isFolder, flags.Folder) | flags.CreateAllowed, syncMode);
    if (!_link)  return false;

    setupOpenItem( isFolder);
    return true;
}


bool  ArnItem::open( const QString &path)
{
    bool  isFolder = path.endsWith("/");
    return open( path, isFolder);
}


bool  ArnItem::openUuidPipe( const QString &path)
{
    QUuid  uuid = QUuid::createUuid();
    QString  fullPath = path + uuid.toString();
    bool  stat = open( fullPath, false);
    setPipeMode();
    return stat;
}


bool  ArnItem::openFolder( const QString &path)
{
    return open( path, true);
}


bool  ArnItem::open( const ArnItem& folder, const QString& itemName, bool isFolder)
{
    ArnLink *parent = folder._link;

    SyncMode  syncMode = _syncModeLinkShare ? _syncMode : SyncMode();
    ArnLink::Flags  flags;
    _link = Arn::link( parent, itemName,
                            flags.flagIf( isFolder, flags.Folder) | flags.CreateAllowed, syncMode);
    if (!_link)  return false;

    setupOpenItem( isFolder);
    return true;
}


bool  ArnItem::open( const ArnItem& folder, const QString& itemName)
{
    bool  isFolder = itemName.endsWith("/");
    return open( folder, itemName, isFolder);
}


void  ArnItem::close()
{
    if (!_link)  return;

    _link->deref();
    _link     = 0;
    _syncMode = SyncMode();
    _mode     = Mode();
}


void  ArnItem::destroyLink()
{
    Arn::destroyLink( _link);
}


bool  ArnItem::isOpen()  const
{
    return _link != 0;
}

/*
bool  ArnItem::isEmpty()  const
{
    return true;
}
*/

bool  ArnItem::isFolder()  const
{
    if (!_link)  return false;

    return _link->isFolder();
}


bool  ArnItem::isBiDir()  const
{
    if (!_link)  return false;

    return _link->isProvider();
}


ArnLink::Type  ArnItem::type()  const
{
    if (!_link)  return ArnLink::Type::Null;

    return _link->type();
}


uint  ArnItem::linkId()  const
{
    if (!_link)  return 0;

    return _link->linkId();
}


void  ArnItem::modeUpdate( bool /*isSetup*/)
{
    if (_link->isPipeMode()) {  // Pipe-mode never IgnoreSameValue
        setIgnoreSameValue(false);
    }
}


void  ArnItem::addSyncMode( SyncMode syncMode, bool linkShare)
{
    _syncModeLinkShare = linkShare;
    _syncMode.f |= syncMode.f;
    if (_syncModeLinkShare  &&  _link) {
        _link->addSyncMode( _syncMode.f);
    }
}


ArnItem::SyncMode  ArnItem::syncMode()  const
{
    if (_syncModeLinkShare  &&  _link) {
        return ArnItem::SyncMode::F( _link->syncMode());
    }
    return _syncMode;
}


void  ArnItem::itemUpdateEnd()
{
    resetOnlyEcho();  // Nothing else yet ...
}


ArnItem&  ArnItem::setTemplate( bool isTemplate)
{
    _isTemplate = isTemplate;
    return *this;
}


bool  ArnItem::isTemplate()  const
{
    return _isTemplate;
}


ArnItem&  ArnItem::setBiDirMode()
{
    _mode.f |= _mode.BiDir;
    if (!_link)  return *this;

    if (_link->isBiDirMode())  return *this;  // Already is bidirectional mode

    /// Bidirectional-mode is the pair of value & provider
    ArnLink*  twinLink = Arn::addTwin( _link, syncMode());
    twinLink->deref();

    return *this;
}


bool  ArnItem::isBiDirMode()  const
{
    if (!_link)  return _mode.is( _mode.BiDir);

    return _link->isBiDirMode();
}


ArnItem&  ArnItem::setPipeMode()
{
    _mode.f |= _mode.Pipe | _mode.BiDir;
    if (!_link)  return *this;

    if (_link->isPipeMode())  return *this;  // Already is pipe mode

    _ignoreSameValue = false;
    // Pipe-mode demands the pair of value & provider
    ArnLink*  twinLink = Arn::addTwin( _link, syncMode());
    _link->setPipeMode( true);
    twinLink->deref();

    return *this;
}


bool  ArnItem::isPipeMode()  const
{
    if (!_link)  return _mode.is( _mode.Pipe);

    return _link->isPipeMode();
}


ArnItem&  ArnItem::setSaveMode()
{
    _mode.f |= _mode.Save;
    if (!_link)  return *this;

    _link->setSaveMode( true);
    return *this;
}


bool  ArnItem::isSaveMode()  const
{
    if (!_link)  return _mode.is( _mode.Save);

    return _link->isSaveMode();
}


ArnItem&  ArnItem::setMaster()
{
    if (_link) {
        Arn::errorLog( QString(tr("Setting item/link as master")),
                            ArnError::AlreadyOpen);
    }
    addSyncMode( SyncMode::Master, true);
    return *this;
}


bool  ArnItem::isMaster()  const
{
    return syncMode().is( SyncMode::Master);
}


ArnItem&  ArnItem::setAutoDestroy()
{
    if (_link) {
        Arn::errorLog( QString(tr("Setting item/link to autoDestroy")),
                            ArnError::AlreadyOpen);
    }
    addSyncMode( SyncMode::AutoDestroy, true);
    return *this;
}


bool  ArnItem::isAutoDestroy()  const
{
    return syncMode().is( SyncMode::AutoDestroy);
}


void  ArnItem::addMode( Mode mode)
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


ArnItem::Mode  ArnItem::getMode()  const
{
    return getMode( _link);
}


/// Use with care, link must be "referenced" before use, otherwise it might have been deleted
ArnItem::Mode  ArnItem::getMode( ArnLink* link)  const
{
    if (!link)  return _mode;

    Mode  mode;
    if (link->isPipeMode())   mode.f |= mode.Pipe;
    if (link->isBiDirMode())  mode.f |= mode.BiDir;
    if (link->isSaveMode())   mode.f |= mode.Save;

    return mode;
}


void  ArnItem::setIgnoreSameValue( bool isIgnore)
{
    _ignoreSameValue = isPipeMode() ? false :isIgnore;
}


bool  ArnItem::isIgnoreSameValue()
{
    return _ignoreSameValue;
}


QString  ArnItem::path( ArnLink::NameF nameF)  const
{
    if (!_link)  return QString();

    return _link->linkPath( nameF);
}


QString  ArnItem::name( ArnLink::NameF nameF)  const
{
    if (!_link)  return QString();

    return _link->linkName( nameF);
}


void  ArnItem::setDelay( int delay)
{
    if (!_delayTimer) {
        _delayTimer = new QTimer;
        _delayTimer->setInterval( delay);
        connect( _delayTimer, SIGNAL(timeout()), this, SLOT(doItemUpdate()));
    }
}


void  ArnItem::arnImport( const QByteArray& data, int ignoreSame)
{
    if (!data.isEmpty()) {
        if (data.at(0) < 32) {  // Assume SetAs-code
            switch (static_cast<ArnLink::Type::E>( data.at(0))) {
            case ArnLink::Type::Variant: {
                    //qDebug() << "ArnImport Variant: size=" << data.size();
                    QVariant  value;
                    QDataStream  stream( data);
                    quint8  dummy;  // Will get SetAs-code
                    stream >> dummy >> value;
                    //qDebug() << "ArnImport dataType=" << value.typeName();
                    setValue( value, ignoreSame);
                    return;
                }
            case ArnLink::Type::ByteArray:
                setValue( data.mid(1), ignoreSame);
                return;
            default:  // Not supported code
                return;
            }
        }
    }
    setValue( data, ignoreSame);    // Normal printable data
}


QByteArray  ArnItem::arnExport()  const
{
    if (!_link)  return QByteArray();

    QByteArray  retVal;
    if (_link->type().e == ArnLink::Type::Variant) {
        QDataStream  stream( &retVal, QIODevice::WriteOnly);
        QVariant value = toVariant();
        stream << qint8( ArnLink::Type::Variant) << value;
        // retVal will contain SetAs-code at pos 0 followed by the QVariant

        //qDebug() << "ArnExport dataType=" << value.typeName();
        //qDebug() << "ArnExport Variant: size=" << retVal.size();
    }
    else {
        retVal = toByteArray();
        if (!retVal.isEmpty()) {
            if (retVal.at(0) < 32) {  // Starting char is conflicting with SetAs-code
                retVal.insert( 0, char( ArnLink::Type::ByteArray));  // Stuff ByteArray-code at pos 0
            }
        }
    }

    return retVal;
}


QString  ArnItem::toString() const
{
    if (!_link)  return QString();

    return _link->toString();
}


QByteArray  ArnItem::toByteArray() const
{
    if (!_link)  return QByteArray();

    return _link->toByteArray();
}


QVariant  ArnItem::toVariant() const
{
    if (!_link)  return QVariant();

    return _link->toVariant();
}


int  ArnItem::toInt() const
{
    if (!_link)  return 0;

    return _link->toInt();
}


double  ArnItem::toDouble() const
{
    if (!_link)  return 0.0;

    return _link->toDouble();
}


bool  ArnItem::toBool() const
{
    if (!_link)  return false;

    return _link->toInt() != 0;
}


ArnItem&  ArnItem::operator=( const ArnItem& other)
{
    ArnLink *link = other._link;

    if (link) {
        switch (link->type().e) {
        case ArnLink::Type::Int:
            this->setValue( link->toInt());
            break;
        case ArnLink::Type::Double:
            this->setValue( link->toDouble());
            break;
        case ArnLink::Type::String:
            this->setValue( link->toString());
            break;
        case ArnLink::Type::ByteArray:
            this->setValue( link->toByteArray());
            break;
        case ArnLink::Type::Variant:
            this->setValue( link->toVariant());
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

    return *this;
}


ArnItem&  ArnItem::operator=( int other)
{
    this->setValue( other);
    return *this;
}


ArnItem&  ArnItem::operator=( double other)
{
    this->setValue( other);
    return *this;
}


ArnItem&  ArnItem::operator=( const QString& other)
{
    this->setValue( other);
    return *this;
}


ArnItem&  ArnItem::operator=( const QByteArray& other)
{
    this->setValue( other);
    return *this;
}


ArnItem&  ArnItem::operator=( const char* other)
{
    this->setValue( other);
    return *this;
}


ArnItem&  ArnItem::operator=( const QVariant& other)
{
    this->setValue( other);
    return *this;
}


void  ArnItem::setValue( int value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == _link->holderLink( _useForceKeep)->toInt()) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( QByteArray::number( value), _id, _useForceKeep);
        else
            _link->setValue( value, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning int:")) + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItem::setValue( double value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == _link->holderLink( _useForceKeep)->toDouble()) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( QByteArray::number( value), _id, _useForceKeep);
        else
            _link->setValue( value, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning double:")) + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItem::setValue( bool value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == (_link->holderLink( _useForceKeep)->toInt() != 0)) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( QByteArray::number( value ? 1 : 0), _id, _useForceKeep);
        else
            _link->setValue( value ? 1 : 0, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning bool:")) + QString::number( value),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItem::setValue( const QString& value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == _link->holderLink( _useForceKeep)->toString()) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( value.toUtf8(), _id, _useForceKeep);
        else
            _link->setValue( value, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning string:")) + value,
                  ArnError::ItemNotOpen);
    }
}


void  ArnItem::setValue( const QByteArray& value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == _link->holderLink( _useForceKeep)->toByteArray()) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( value, _id, _useForceKeep);
        else
            _link->setValue( value, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning bytearray:")) + QString::fromUtf8( value.constData(), value.size()),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItem::setValue( const QVariant& value, int ignoreSame)
{
    bool  isIgnoreSame = (ignoreSame < 0) ? isIgnoreSameValue() : (ignoreSame != 0);
    if (_link) {
        if (isIgnoreSame) {
            if (value == _link->holderLink( _useForceKeep)->toVariant()) {
                return;
            }
        }
        if (_link->isPipeMode() && _link->isThreaded())
            trfValue( value.toString().toUtf8(), _id, _useForceKeep);
        else
            _link->setValue( value, _id, _useForceKeep);
    }
    else {
        errorLog( QString(tr("Assigning variant")),
                  ArnError::ItemNotOpen);
    }
}


void  ArnItem::setValue( const char* value, int ignoreSame)
{
        setValue( QByteArray( value), ignoreSame);
}


void  ArnItem::toggleBool()
{
    bool value = this->toBool();

    this->setValue( !value);
}


void  ArnItem::trfValue( QByteArray value, int sendId, bool forceKeep)
{
    QMetaObject::invokeMethod( _link, "trfValue", Qt::QueuedConnection,
                               Q_ARG( QByteArray, value),
                               Q_ARG( int, sendId),
                               Q_ARG( bool, forceKeep));

}


void  ArnItem::connectNotify( const char *signal)
{
    if (QLatin1String( signal) == SIGNAL(changed())) {
        _emitChanged++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(int))) {
        _emitChangedInt++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(double))) {
        _emitChangedDouble++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(bool))) {
        _emitChangedBool++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QString))) {
        _emitChangedString++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QByteArray))) {
        _emitChangedByteArray++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QVariant))) {
        _emitChangedVariant++;
    }
}


void  ArnItem::disconnectNotify( const char *signal)
{
    if (QLatin1String( signal) == SIGNAL(changed())) {
        _emitChanged--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(int))) {
        _emitChangedInt--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(double))) {
        _emitChangedDouble--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(bool))) {
        _emitChangedBool--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QString))) {
        _emitChangedString--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QByteArray))) {
        _emitChangedByteArray--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QVariant))) {
        _emitChangedVariant--;
    }
}


QStringList  ArnItem::childItemsMain()  const
{
    // This must be run in main thread as childs only can be deleted there
    return Arn::itemsMain( _link);
}


void  ArnItem::errorLog( QString errText, ArnError err, void* reference)
{
    QString  itemText;
    if (_link) {
        itemText = " Item:" + _link->linkPath();
    }
    Arn::errorLog( errText + itemText, err, reference);
}


void  ArnItem::linkValueUpdated( uint sendId)
{
    if (_blockEcho  &&  sendId == _id) {  // This update was initiated from this Item, it can be blocked ...
        return;
    }
    _isOnlyEcho = (sendId == _id) ? _isOnlyEcho : false;

    if (_delayTimer) {
        if (!_delayTimer->isActive()) {
            _delayTimer->start();
        }
    }
    else {
        doItemUpdate();
    }
}


void  ArnItem::linkValueUpdated( uint sendId, QByteArray value)
{
    if (_blockEcho  &&  sendId == _id) {  // This update was initiated from this Item, it can be blocked ...
        return;
    }
    _isOnlyEcho = (sendId == _id) ? _isOnlyEcho : false;

    itemUpdateStart();
    if (_emitChanged) {
        emit changed();
    }
    if (_emitChangedInt) {
        emit changed( int( value.toInt()));
    }
    if (_emitChangedDouble) {
        emit changed( double( value.toDouble()));
    }
    if (_emitChangedBool) {
        emit changed( bool( value.toInt() != 0));
    }
    if (_emitChangedString) {
        emit changed( QString::fromUtf8( value.constData(), value.size()));
    }
    if (_emitChangedByteArray) {
        emit changed( value);
    }
    if (_emitChangedVariant) {
        // Can only handle printable value ...
        emit changed( QVariant( QString::fromUtf8( value.constData(), value.size())));
    }
    itemUpdateEnd();
}


void  ArnItem::doItemUpdate()
{
    if (_delayTimer ) {
        _delayTimer->stop();
    }

    itemUpdateStart();
    if (_emitChanged) {
        emit changed();
    }
    if (_emitChangedInt) {
        emit changed( int(_link->toInt()));
    }
    if (_emitChangedDouble) {
        emit changed( double(_link->toDouble()));
    }
    if (_emitChangedBool) {
        emit changed( bool(_link->toInt() != 0));
    }
    if (_emitChangedString) {
        emit changed(_link->toString());
    }
    if (_emitChangedByteArray) {
        emit changed(_link->toByteArray());
    }
    if (_emitChangedByteArray) {
        emit changed(_link->toVariant());
    }
    itemUpdateEnd();
}


void  ArnItem::arnLinkCreatedBelow( ArnLink* link)
{
    if (!link->isFolder()) {
        emit arnItemCreated( link->linkPath());
    }
}


void  ArnItem::arnModeChangedBelow( QString path, uint linkId)
{
    ArnLink::Flags  flags;
    ArnLink*  link = Arn::link( path, flags.SilentError);
    if (!link)  return;  // Item has been lost (deleted?)

    emit arnModeChanged( path, linkId, getMode( link));
    link->deref();  // Release the link-reference
}


void  ArnItem::doArnLinkDestroyed()
{
    qDebug() << "Item arnLinkDestroyed: path=" << path();
    emit arnLinkDestroyed();
    close();
}


ArnItem::~ArnItem()
{
    close();
}


QTextStream &operator<<( QTextStream& out, const ArnItem& item)
{
    out << item.toString();
    return out;
}
