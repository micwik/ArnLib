// Copyright (C) 2010-2015 Michael Wiklund.
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

#include "ArnInc/MQFlags.hpp"
#include <ArnInc/XStringMap.hpp>
#include <QMetaType>
#include <QMetaObject>
#include <QMetaEnum>
#include <QMapIterator>
#include <QDebug>
#include <limits.h>

namespace Arn {

bool  isPower2( uint x)
{
    return x && ((x & (x - 1)) == 0);
}


EnumTxt::EnumTxt(const QMetaObject& metaObj, bool isFlag, const _InitEnumTxt* initTxt)
    : _metaObj( metaObj)
{
    _txtStore = 0;
    _isFlag   = isFlag;
    setupFromMetaObject();
    setupTxt( initTxt);
}


void  EnumTxt::setTxtRef( const char *txt, int enumVal, quint16 nameSpace)
{
    _enumTxtTab.insert( EnumTxtKey( enumVal, nameSpace, _isFlag), txt);
}


void  EnumTxt::setTxt( const char* txt, int enumVal, quint16 nameSpace)
{
    if (!_txtStore)
        _txtStore = new QList<QByteArray>;

    int idx = _txtStore->size();
    *_txtStore += QByteArray( txt);

    setTxtRef( _txtStore->at( idx).constData(), enumVal, nameSpace);
}


const char*  EnumTxt::getTxt( int enumVal, quint16 nameSpace)  const
{
    return _enumTxtTab.value( EnumTxtKey( enumVal, nameSpace, _isFlag), "");
}


void  EnumTxt::setTxtString( const QString& txt, int enumVal, quint16 nameSpace)
{
    setTxt( txt.toUtf8().constData(), enumVal, nameSpace);
}


QString  EnumTxt::getTxtString( int enumVal, quint16 nameSpace)  const
{
    return QString::fromUtf8( getTxt( enumVal, nameSpace));
}


int  EnumTxt::getEnumVal( const char* txt, int defaultVal, quint16 nameSpace)
{
    EnumTxtKey  keyStart( _isFlag ? 0        : INT_MIN, nameSpace, _isFlag);
    EnumTxtKey  keyStop(  _isFlag ? UINT_MAX : INT_MAX, nameSpace, _isFlag);

    QMap<EnumTxtKey,const char*>::iterator  i = _enumTxtTab.lowerBound( keyStart);
    while (i != _enumTxtTab.end()) {
        const EnumTxtKey&  keyStored = i.key();
        if (keyStop < keyStored)  break;

        if (qstrcmp( i.value(), txt) == 0)  return int(keyStored._enumVal);  // Matching txt
        ++i;
    }

    return defaultVal;  // Not found
}


int  EnumTxt::getEnumVal( const QString& txt, int defaultVal, quint16 nameSpace)
{
    return getEnumVal( txt.toUtf8().constData(), defaultVal, nameSpace);
}


void  EnumTxt::addBitSet( XStringMap& xsm, quint16 nameSpace)
{
    if (!_isFlag)  return;  // Only for flags

    EnumTxtKey  keyStart( 0,       nameSpace, _isFlag);
    EnumTxtKey  keyStop( UINT_MAX, nameSpace, _isFlag);
    uint  bitNum = 0;

    QMap<EnumTxtKey,const char*>::iterator  i = _enumTxtTab.lowerBound( keyStart);
    while (i != _enumTxtTab.end()) {
        const EnumTxtKey&  keyStored = i.key();
        if (keyStop < keyStored)  break;

        uint  enumValStored = keyStored._enumVal;
        while (uint(1 << bitNum) < enumValStored)
            ++bitNum;
        xsm.add("B" + QByteArray::number( bitNum), QByteArray( i.value()));
        ++i;
    }
}


QString  EnumTxt::getBitSet( quint16 nameSpace)
{
    if (!_isFlag)  return QString();  // Only for flags

    XStringMap  xsm;
    addBitSet( xsm, nameSpace);
    return QString::fromUtf8( xsm.toXString());
}


QString  EnumTxt::flagsToString( int val, quint16 nameSpace)
{
    if (!_isFlag)  return QString();  // Only for flags

    return flagsToStringList( val, nameSpace).join(" | ");
}


QStringList  EnumTxt::flagsToStringList( int val, quint16 nameSpace)
{
    if (!_isFlag)  return QStringList();  // Only for flags

    EnumTxtKey  keyStart( 0,       nameSpace, _isFlag);
    EnumTxtKey  keyStop( UINT_MAX, nameSpace, _isFlag);
    QStringList  retVal;

    QMap<EnumTxtKey,const char*>::iterator  i = _enumTxtTab.lowerBound( keyStart);
    while (i != _enumTxtTab.end()) {
        const EnumTxtKey&  keyStored = i.key();
        if (keyStop < keyStored)  break;

        if (val & keyStored._enumVal) {
            retVal += QString::fromUtf8( i.value());
        }
        ++i;
    }

    return retVal;
}


int  EnumTxt::flagsFromString( const QString& flagString, quint16 nameSpace)
{
    if (!_isFlag)  return 0;  // Only for flags

    return flagsFromStringList( flagString.split(" | "), nameSpace);
}


int  EnumTxt::flagsFromStringList( const QStringList& flagStrings, quint16 nameSpace)
{
    if (!_isFlag)  return 0;  // Only for flags

    EnumTxtKey  keyStart( 0,       nameSpace, _isFlag);
    EnumTxtKey  keyStop( UINT_MAX, nameSpace, _isFlag);
    int  retVal = 0;

    QMap<EnumTxtKey,const char*>::iterator  i = _enumTxtTab.lowerBound( keyStart);
    while (i != _enumTxtTab.end()) {
        const EnumTxtKey&  keyStored = i.key();
        if (keyStop < keyStored)  break;

        if (flagStrings.contains( QString::fromUtf8( i.value())))
            retVal |= keyStored._enumVal;
        ++i;
    }

    return retVal;
}


void  EnumTxt::addEnumSet( XStringMap& xsm, quint16 nameSpace)
{
    if (_isFlag)  return;  // Only for non flags

    EnumTxtKey  keyStart( INT_MIN, nameSpace, _isFlag);
    EnumTxtKey  keyStop(  INT_MAX, nameSpace, _isFlag);

    QMap<EnumTxtKey,const char*>::iterator  i = _enumTxtTab.lowerBound( keyStart);
    while (i != _enumTxtTab.end()) {
        const EnumTxtKey&  keyStored = i.key();
        if (keyStop < keyStored)  break;

        int  enumValStored = int(keyStored._enumVal);
        xsm.add(QByteArray::number( enumValStored), QByteArray( i.value()));
        ++i;
    }
}


QString  EnumTxt::getEnumSet( quint16 nameSpace)
{
    if (_isFlag)  return QString();  // Only for non flags

    XStringMap  xsm;
    addEnumSet( xsm, nameSpace);
    return QString::fromUtf8( xsm.toXString());
}


void  EnumTxt::setupTxt( const _InitEnumTxt* initTxt)
{
    if (!initTxt)  return;  // Nothing to setup

    for (int i = 0; initTxt[i].enumTxt; ++i) {
        setTxtRef( initTxt[i].enumTxt, initTxt[i].enumVal, initTxt[i].ns);
    }
}


void  EnumTxt::setupFromMetaObject()
{
    if (_metaObj.enumeratorCount() < 1)  return;

    QMetaEnum  metaEnum = _metaObj.enumerator(0);
    int  nKeys = metaEnum.keyCount();
    for (int i = 0; i < nKeys; ++i) {
        int  enumVal = metaEnum.value(i);
        if (_isFlag && !isPower2( enumVal))
            continue;  // Not a single bit for flags
        setTxtRef( metaEnum.key(i), enumVal);
    }
}


bool  EnumTxt::EnumTxtKey::operator <( const EnumTxt::EnumTxtKey& other)  const
{
    if (_isFlag != other._isFlag)  qWarning() << "ERROR EnumKey isFlag diff !!!";
    if (_nameSpace < other._nameSpace)  return true;
    if (_nameSpace > other._nameSpace)  return false;
    if (_isFlag)
        return _enumVal < other._enumVal;  // Flag: unsigned compare
    else
        return int(_enumVal) < int(other._enumVal);
}

}
