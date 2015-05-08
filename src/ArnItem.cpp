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

#include "ArnInc/ArnItem.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnLink.hpp"
#include <QTimer>
#include <QMetaObject>
#include <QDebug>


#if QT_VERSION >= 0x050000
//// Store meta methods for the "changed..." signals, used later for comparison
QMetaMethod  ArnItem::_metaSignalChanged(
        QMetaMethod::fromSignal( static_cast<void (ArnItem::*)()>(&ArnItem::changed)));
QMetaMethod  ArnItem::_metaSignalChangedInt(
        QMetaMethod::fromSignal( static_cast<void (ArnItem::*)(int)>(&ArnItem::changed)));
QMetaMethod  ArnItem::_metaSignalChangedReal(
        QMetaMethod::fromSignal( static_cast<void (ArnItem::*)(ARNREAL)>(&ArnItem::changed)));
QMetaMethod  ArnItem::_metaSignalChangedBool(
        QMetaMethod::fromSignal( static_cast<void (ArnItem::*)(bool)>(&ArnItem::changed)));
QMetaMethod  ArnItem::_metaSignalChangedString(
        QMetaMethod::fromSignal( static_cast<void (ArnItem::*)(QString)>(&ArnItem::changed)));
QMetaMethod  ArnItem::_metaSignalChangedByteArray(
        QMetaMethod::fromSignal( static_cast<void (ArnItem::*)(QByteArray)>(&ArnItem::changed)));
QMetaMethod  ArnItem::_metaSignalChangedVariant(
        QMetaMethod::fromSignal( static_cast<void (ArnItem::*)(QVariant)>(&ArnItem::changed)));
#endif


void  ArnItem::init()
{
    _delayTimer = 0;
    _isTemplate = false;

    _emitChanged          = 0;
    _emitChangedInt       = 0;
    _emitChangedReal      = 0;
    _emitChangedBool      = 0;
    _emitChangedString    = 0;
    _emitChangedByteArray = 0;
}


ArnItem::ArnItem( QObject *parent)
            : ArnItemB( parent)
{
    init();
}


ArnItem::ArnItem( const QString& path, QObject *parent)
            : ArnItemB( parent)
{
    init();
    this->open( path);
}


ArnItem::ArnItem( const ArnItem& itemTemplate, const QString& path, QObject *parent)
            : ArnItemB( parent)
{
    init();
    if (itemTemplate.isTemplate()) {  // Template mode: Copy syncMode & Mode from template to this Item
        this->addSyncMode( itemTemplate.syncMode(), true);
        this->addMode( itemTemplate.getMode());
        this->open( path);
    }
    else {
        ArnM::errorLog( QString(tr("Should be template path=")) + path,
                        ArnError::CreateError);
    }
}


void  ArnItem::itemCreatedBelow( QString path)
{
    emit arnItemCreated( path);
}


void  ArnItem::itemModeChangedBelow( QString path, uint linkId, Arn::ObjectMode mode)
{
    emit arnModeChanged( path, linkId, mode);
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


void  ArnItem::setDelay( int delay)
{
    if (!_delayTimer) {
        _delayTimer = new QTimer( this);
        connect( _delayTimer, SIGNAL(timeout()), this, SLOT(timeoutItemUpdate()));
    }
    _delayTimer->setInterval( delay);
}


ArnItem&  ArnItem::operator=( const ArnItem& other)
{
    this->setValue( other);
    return *this;
}


ArnItem&  ArnItem::operator=( int other)
{
    this->setValue( other);
    return *this;
}


ArnItem&  ArnItem::operator=( ARNREAL other)
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


void  ArnItem::setValue( const char* value, int ignoreSame)
{
    setValue( QString::fromUtf8( value), ignoreSame);
}


void  ArnItem::toggleBool()
{
    bool value = toBool();

    setValue( !value);
}


#if QT_VERSION >= 0x050000

void  ArnItem::connectNotify( const QMetaMethod &signal)
{
    if (signal == _metaSignalChanged) {
        _emitChanged++;
    }
    else if (signal == _metaSignalChangedInt) {
        _emitChangedInt++;
    }
    else if (signal == _metaSignalChangedReal) {
        _emitChangedReal++;
    }
    else if (signal == _metaSignalChangedBool) {
        _emitChangedBool++;
    }
    else if (signal == _metaSignalChangedString) {
        _emitChangedString++;
    }
    else if (signal == _metaSignalChangedByteArray) {
        _emitChangedByteArray++;
    }
    else if (signal == _metaSignalChangedVariant) {
        _emitChangedVariant++;
    }
}


void  ArnItem::disconnectNotify( const QMetaMethod &signal)
{
    if (signal == _metaSignalChanged) {
        _emitChanged--;
    }
    else if (signal == _metaSignalChangedInt) {
        _emitChangedInt--;
    }
    else if (signal == _metaSignalChangedReal) {
        _emitChangedReal--;
    }
    else if (signal == _metaSignalChangedBool) {
        _emitChangedBool--;
    }
    else if (signal == _metaSignalChangedString) {
        _emitChangedString--;
    }
    else if (signal == _metaSignalChangedByteArray) {
        _emitChangedByteArray--;
    }
    else if (signal == _metaSignalChangedVariant) {
        _emitChangedVariant--;
    }
}


#else

void  ArnItem::connectNotify( const char *signal)
{
    if (QLatin1String( signal) == SIGNAL(changed())) {
        _emitChanged++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(int))) {
        _emitChangedInt++;
    }
#ifdef ARNREAL_FLOAT
    else if (QLatin1String( signal) == SIGNAL(changed(float))) {
#else
    else if (QLatin1String( signal) == SIGNAL(changed(double))) {
#endif
        _emitChangedReal++;
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
#ifdef ARNREAL_FLOAT
    else if (QLatin1String( signal) == SIGNAL(changed(float))) {
#else
    else if (QLatin1String( signal) == SIGNAL(changed(double))) {
#endif
        _emitChangedReal--;
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

#endif


void  ArnItem::itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value)
{
    if (!value) {  // Update of item with no data supplied
        if (_delayTimer) {
            if (!_delayTimer->isActive()) {
                _delayTimer->start();
            }
        }
        else {
            doItemUpdate( handleData);
        }
    }
    else {  // Update of item with data supplied (pipe in multi-thread)
        if (_emitChanged) {
            emit changed();
        }
        if (_emitChangedInt) {
            emit changed( int( value->toInt()));
        }
        if (_emitChangedReal) {
#if defined( ARNREAL_FLOAT)
            emit changed( float( value->toFloat()));
#else
            emit changed( double( value->toDouble()));
#endif
        }
        if (_emitChangedBool) {
            emit changed( bool( value->toInt() != 0));
        }
        if (_emitChangedString) {
            emit changed( QString::fromUtf8( value->constData(), value->size()));
        }
        if (_emitChangedByteArray) {
            emit changed( *value);
        }
        if (_emitChangedVariant) {
            // Can only handle printable value ...
            emit changed( QVariant( QString::fromUtf8( value->constData(), value->size())));
        }
        resetOnlyEcho();  // Nothing else yet ...
    }
}


void  ArnItem::doItemUpdate( const ArnLinkHandle& handleData)
{
    Q_UNUSED(handleData);

    if (_delayTimer ) {
        _delayTimer->stop();
    }

    if (_emitChanged) {
        emit changed();
    }
    if (_emitChangedInt) {
        emit changed( toInt());
    }
    if (_emitChangedReal) {
        emit changed( toReal());
    }
    if (_emitChangedBool) {
        emit changed( toBool());
    }
    if (_emitChangedString) {
        emit changed( toString());
    }
    if (_emitChangedByteArray) {
        emit changed( toByteArray());
    }
    if (_emitChangedByteArray) {
        emit changed( toVariant());
    }
    resetOnlyEcho();  // Nothing else yet ...
}


void  ArnItem::timeoutItemUpdate()
{
    doItemUpdate( ArnLinkHandle());
}


void  ArnItem::modeUpdate( bool isSetup)
{
    ArnItemB::modeUpdate( isSetup); // must be called for base-class update
    if (isSetup)  return;

    emit modeChanged( getMode());
}


ArnItem::~ArnItem()
{
}


QTextStream &operator<<( QTextStream& out, const ArnItem& item)
{
    out << item.toString();
    return out;
}

