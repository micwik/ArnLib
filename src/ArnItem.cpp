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

#include "ArnInc/ArnItem.hpp"
#include "private/ArnItem_p.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnLink.hpp"
#include <QTimerEvent>
#include <QBasicTimer>
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
        QMetaMethod::fromSignal( static_cast<void (ArnItem::*)(const QString&)>(&ArnItem::changed)));
QMetaMethod  ArnItem::_metaSignalChangedByteArray(
        QMetaMethod::fromSignal( static_cast<void (ArnItem::*)(const QByteArray&)>(&ArnItem::changed)));
QMetaMethod  ArnItem::_metaSignalChangedVariant(
        QMetaMethod::fromSignal( static_cast<void (ArnItem::*)(const QVariant&)>(&ArnItem::changed)));
#endif


class MQBasicTimer : public QBasicTimer
{
public:
    MQBasicTimer()
    {
        _interval = 0;
    }

    int  interval()  const {return _interval;}
    void  setInterval( int interval)  {_interval = interval;}
    void  start(QObject* obj)  {QBasicTimer::start( _interval, obj);}
    void  start( int msec, QObject* obj)
    {
        _interval = msec;
        QBasicTimer::start( _interval, obj);
    }

private:
    int  _interval;
};


ArnItemPrivate::ArnItemPrivate()
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


ArnItemPrivate::~ArnItemPrivate()
{
    if (_delayTimer)
        delete _delayTimer;
}


void  ArnItem::init()
{
}


ArnItem::ArnItem( QObject *parent)
    : ArnItemB( *new ArnItemPrivate, parent)
{
    init();
}


ArnItem::ArnItem( const QString& path, QObject *parent)
    : ArnItemB( *new ArnItemPrivate, parent)
{
    init();
    this->open( path);
}


ArnItem::ArnItem( const ArnItem& itemTemplate, const QString& path, QObject *parent)
    : ArnItemB( *new ArnItemPrivate, parent)
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


ArnItem::ArnItem( ArnItemPrivate& dd, QObject* parent)
    : ArnItemB( dd, parent)
{
    init();
}


void  ArnItem::itemCreatedBelow( const QString& path)
{
    emit arnItemCreated( path);
}


void  ArnItem::itemModeChangedBelow( const QString& path, uint linkId, Arn::ObjectMode mode)
{
    emit arnModeChanged( path, linkId, mode);
}


void  ArnItem::timerEvent( QTimerEvent* ev)
{
    Q_D(ArnItem);

    if (d->_delayTimer && (ev->timerId() == d->_delayTimer->timerId())) {
        // qDebug() << "ArnItem delay doUpdate: path=" << path();
        doItemUpdate( ArnLinkHandle::null());
    }

    return ArnItemB::timerEvent( ev);
}


int ArnItem::delayTimerId()  const
{
    Q_D(const ArnItem);

    if (!d->_delayTimer)  return 0;

    return d->_delayTimer->timerId();
}


ArnItem&  ArnItem::setTemplate( bool isTemplate)
{
    Q_D(ArnItem);

    d->_isTemplate = isTemplate;
    return *this;
}


bool  ArnItem::isTemplate()  const
{
    Q_D(const ArnItem);

    return d->_isTemplate;
}


void  ArnItem::setDelay( int delay)
{
    Q_D(ArnItem);

    if (!d->_delayTimer) {
        d->_delayTimer = new MQBasicTimer;
    }
    d->_delayTimer->setInterval( delay);
}


int ArnItem::delay() const
{
    Q_D(const ArnItem);

    if (!d->_delayTimer)  return 0;

    return d->_delayTimer->interval();
}


bool  ArnItem::isDelayPending()  const
{
    Q_D(const ArnItem);

    return d->_delayTimer && d->_delayTimer->isActive();
}


void ArnItem::bypassDelayPending()
{
    if (isDelayPending()) {
        doItemUpdate( ArnLinkHandle::null());
    }
}


ArnItem&  ArnItem::operator=( const ArnItem& other)
{
    this->setValue( other);
    return *this;
}


ArnItem&  ArnItem::operator=( int val)
{
    this->setValue( val);
    return *this;
}


ArnItem&  ArnItem::operator=( ARNREAL val)
{
    this->setValue( val);
    return *this;
}


ArnItem&  ArnItem::operator=( const QString& val)
{
    this->setValue( val);
    return *this;
}


ArnItem&  ArnItem::operator=( const QByteArray& val)
{
    this->setValue( val);
    return *this;
}


ArnItem&  ArnItem::operator=( const char* val)
{
    this->setValue( val);
    return *this;
}


ArnItem&  ArnItem::operator=( uint val)
{
    this->setValue( val);
    return *this;
}


ArnItem&  ArnItem::operator=( qint64 val)
{
    this->setValue( val);
    return *this;
}


ArnItem&  ArnItem::operator=( quint64 val)
{
    this->setValue( val);
    return *this;
}


ArnItem&  ArnItem::operator=( const QVariant& val)
{
    this->setValue( val);
    return *this;
}


void  ArnItem::toggleBool()
{
    bool value = toBool();

    setValue( !value);
}


#if QT_VERSION >= 0x050000

void  ArnItem::connectNotify( const QMetaMethod &signal)
{
    Q_D(ArnItem);

    if (signal == _metaSignalChanged) {
        d->_emitChanged++;
    }
    else if (signal == _metaSignalChangedInt) {
        d->_emitChangedInt++;
    }
    else if (signal == _metaSignalChangedReal) {
        d->_emitChangedReal++;
    }
    else if (signal == _metaSignalChangedBool) {
        d->_emitChangedBool++;
    }
    else if (signal == _metaSignalChangedString) {
        d->_emitChangedString++;
    }
    else if (signal == _metaSignalChangedByteArray) {
        d->_emitChangedByteArray++;
    }
    else if (signal == _metaSignalChangedVariant) {
        d->_emitChangedVariant++;
    }
}


void  ArnItem::disconnectNotify( const QMetaMethod &signal)
{
    Q_D(ArnItem);

    if (signal == _metaSignalChanged) {
        d->_emitChanged--;
    }
    else if (signal == _metaSignalChangedInt) {
        d->_emitChangedInt--;
    }
    else if (signal == _metaSignalChangedReal) {
        d->_emitChangedReal--;
    }
    else if (signal == _metaSignalChangedBool) {
        d->_emitChangedBool--;
    }
    else if (signal == _metaSignalChangedString) {
        d->_emitChangedString--;
    }
    else if (signal == _metaSignalChangedByteArray) {
        d->_emitChangedByteArray--;
    }
    else if (signal == _metaSignalChangedVariant) {
        d->_emitChangedVariant--;
    }
}


#else

void  ArnItem::connectNotify( const char *signal)
{
    Q_D(ArnItem);

    if (QLatin1String( signal) == SIGNAL(changed())) {
        d->_emitChanged++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(int))) {
        d->_emitChangedInt++;
    }
#ifdef ARNREAL_FLOAT
    else if (QLatin1String( signal) == SIGNAL(changed(float))) {
#else
    else if (QLatin1String( signal) == SIGNAL(changed(double))) {
#endif
        d->_emitChangedReal++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(bool))) {
        d->_emitChangedBool++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QString))) {
        d->_emitChangedString++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QByteArray))) {
        d->_emitChangedByteArray++;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QVariant))) {
        d->_emitChangedVariant++;
    }
}


void  ArnItem::disconnectNotify( const char *signal)
{
    Q_D(ArnItem);

    if (QLatin1String( signal) == SIGNAL(changed())) {
        d->_emitChanged--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(int))) {
        d->_emitChangedInt--;
    }
#ifdef ARNREAL_FLOAT
    else if (QLatin1String( signal) == SIGNAL(changed(float))) {
#else
    else if (QLatin1String( signal) == SIGNAL(changed(double))) {
#endif
        d->_emitChangedReal--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(bool))) {
        d->_emitChangedBool--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QString))) {
        d->_emitChangedString--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QByteArray))) {
        d->_emitChangedByteArray--;
    }
    else if (QLatin1String( signal) == SIGNAL(changed(QVariant))) {
        d->_emitChangedVariant--;
    }
}

#endif


void  ArnItem::itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value)
{
    Q_D(ArnItem);

    if (!value) {  // Update of item with no data supplied
        if (d->_delayTimer) {
            if (!d->_delayTimer->isActive()) {
                d->_delayTimer->start( this);
                // qDebug() << "ArnItem delay start: path=" << path();
            }
        }
        else {
            doItemUpdate( handleData);
        }
    }
    else {  // Update of item with data supplied (pipe in multi-thread)
        if (d->_emitChanged) {
            emit changed();
        }
        if (d->_emitChangedInt) {
            emit changed( int( value->toInt()));
        }
        if (d->_emitChangedReal) {
#if defined( ARNREAL_FLOAT)
            emit changed( float( value->toFloat()));
#else
            emit changed( double( value->toDouble()));
#endif
        }
        if (d->_emitChangedBool) {
            emit changed( bool( value->toInt() != 0));
        }
        if (d->_emitChangedString) {
            emit changed( QString::fromUtf8( value->constData(), value->size()));
        }
        if (d->_emitChangedByteArray) {
            emit changed( *value);
        }
        if (d->_emitChangedVariant) {
            // Can only handle printable value ...
            emit changed( QVariant( QString::fromUtf8( value->constData(), value->size())));
        }
        resetOnlyEcho();  // Nothing else yet ...
    }
}


void  ArnItem::doItemUpdate( const ArnLinkHandle& handleData)
{
    Q_UNUSED(handleData);
    Q_D(ArnItem);


    if (d->_delayTimer ) {
        d->_delayTimer->stop();
    }

    if (d->_emitChanged) {
        emit changed();
    }
    if (d->_emitChangedInt) {
        emit changed( toInt());
    }
    if (d->_emitChangedReal) {
        emit changed( toReal());
    }
    if (d->_emitChangedBool) {
        emit changed( toBool());
    }
    if (d->_emitChangedString) {
        emit changed( toString());
    }
    if (d->_emitChangedByteArray) {
        emit changed( toByteArray());
    }
    if (d->_emitChangedByteArray) {
        emit changed( toVariant());
    }
    resetOnlyEcho();  // Nothing else yet ...
}


void  ArnItem::modeUpdate( Arn::ObjectMode mode, bool isSetup)
{
    ArnItemB::modeUpdate( mode, isSetup); // must be called for base-class update
    if (isSetup)  return;

    emit modeChanged( mode);
}


ArnItem::~ArnItem()
{
}


QTextStream &operator<<( QTextStream& out, const ArnItem& item)
{
    out << item.toString();
    return out;
}




