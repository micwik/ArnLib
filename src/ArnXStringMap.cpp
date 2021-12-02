// Copyright (C) 2010-2019 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. Usage of these other libraries is subject to their respective
// license agreements.
//
// The MIT License (MIT) Usage
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this file to deal in its contained Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software in this file.
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

#include "ArnInc/XStringMap.hpp"
#include <QMetaType>
#include <QDebug>
#include <limits>


namespace Arn {

QByteArray  XStringMap::_nullValue;


XStringMap::XStringMap()
{
    init();
}


XStringMap::XStringMap( const XStringMap& other)
    : _keyList( other._keyList)
    , _valList( other._valList)
    , _size(    other._size)
{
}


XStringMap::XStringMap( const QByteArray& xString)
{
    init();
    fromXString( xString);
}


XStringMap::XStringMap( const QVariantMap& variantMap)
{
    init();
    add( variantMap);
}


XStringMap::~XStringMap()
{
}


XStringMap&  XStringMap::operator=( const XStringMap& other)
{
    _keyList = other._keyList;
    _valList = other._valList;
    _size    = other._size;

    return *this;
}


void  XStringMap::init()
{
    clear( true);
}


void  XStringMap::clear( bool freeMem)
{
    _size = 0;

    if (freeMem) {
        _keyList.clear();
        _valList.clear();
    }
}


void XStringMap::squeeze()
{
    _keyList.resize( _size);
    _valList.resize( _size);
    _keyList.squeeze();
    _valList.squeeze();
}


int  XStringMap::indexOf( const char* key, int from)  const
{
    if (key == 0)  return -1;  // Return Not found

    for (int i = from; i < _size; ++i) {
        if (_keyList.at(i) == key) {
            return i;   // Return index of found key
        }
    }
    return -1;  // Return Not found
}


int  XStringMap::indexOf( const QByteArray& key, int from)  const
{
    for (int i = from; i < _size; ++i) {
        if (_keyList.at(i) == key) {
            return i;   // Return index of found key
        }
    }
    return -1;  // Return Not found
}


int  XStringMap::indexOf( const QString& key, int from) const
{
    return indexOf( key.toUtf8(), from);
}


int  XStringMap::indexOfValue( const QByteArray& value, int from)  const
{
    for (int i = from; i < _size; ++i) {
        if (_valList.at(i) == value) {
            return i;   // Return index of found value
        }
    }
    return -1;  // Return Not found
}


int  XStringMap::indexOfValue( const QString& value, int from)  const
{
    return indexOfValue( value.toUtf8(), from);
}


int  XStringMap::maxEnumOf( const char* keyPrefix)  const
{
    int  maxEnum = -1;
    int  keyPrefixLen = int( strlen( keyPrefix));
    bool isOk;
    for (int i = 0; i < _size; ++i) {
        if (_keyList.at(i).startsWith( keyPrefix)) {
            int num = _keyList.at(i).mid( keyPrefixLen).toInt( &isOk);
            if (isOk && (num > maxEnum))
                maxEnum = num;
        }
    }
    return maxEnum;
}


XStringMap&  XStringMap::add( const char* key, const QByteArray& val)
{
    if (key == 0)  return *this;  // Not valid key

    checkSpace();
    _keyList[ _size].resize(0);     // Avoid Heap reallocation
    _valList[ _size].resize(0);
    _keyList[ _size] += key;
    _valList[ _size] += val;
    ++_size;

    return *this;
}


XStringMap&  XStringMap::add( const char* key, const char* val)
{
    return add( key, QByteArray( val));
}


XStringMap&  XStringMap::add( const char* keyPrefix, uint eNum, const QByteArray& val)
{
    QByteArray  key;
    key += keyPrefix;
    key += QByteArray::number( eNum);

    return add( key.constData(), val);
}


XStringMap&  XStringMap::add( const QByteArray& key, const QByteArray& val)
{
    return add( key.constData(), val);
}


XStringMap&  XStringMap::add( const char* key, const QString& val)
{
    return add( key, val.toUtf8());
}


XStringMap&  XStringMap::add( const char* keyPrefix, uint eNum, const QString& val)
{
    return add( keyPrefix, eNum, val.toUtf8());
}


XStringMap&  XStringMap::add( const QByteArray& key, const QString& val)
{
    return add( key, val.toUtf8());
}


XStringMap&  XStringMap::add( const QString& key, const QString& val)
{
    return add( key.toUtf8(), val.toUtf8());
}


XStringMap&  XStringMap::add( const XStringMap& other)
{
    for (int i = 0; i < other._size; ++i) {
        add( other._keyList.at(i), other._valList.at(i));
    }

    return *this;
}


XStringMap&  XStringMap::add( const QVariantMap& variantMap)
{
    QMapIterator<QString,QVariant>  i( variantMap);

    while (i.hasNext()) {
        i.next();
        const QVariant&  value = i.value();
        QByteArray  valBytes = value.type() == int(QMetaType::QByteArray) ? value.toByteArray()
                                                                          : value.toString().toUtf8();
        add( i.key().toUtf8(), valBytes);
    }

    return *this;
}


XStringMap&  XStringMap::addNum( const char* key, int val)
{
    return add( key, QByteArray::number( val, 10));
}


XStringMap&  XStringMap::addNum( const QByteArray& key, int val)
{
    return add( key, QByteArray::number( val, 10));
}


XStringMap&  XStringMap::addNum( const QString& key, int val)
{
    return add( key, QString::number( val, 10));
}


XStringMap&  XStringMap::addNum( const char* key, uint val)
{
    return add( key, QByteArray::number( val, 10));
}


XStringMap&  XStringMap::addNum( const QByteArray& key, uint val)
{
    return add( key, QByteArray::number( val, 10));
}


XStringMap&  XStringMap::addNum( const QString& key, uint val)
{
    return add( key, QString::number( val, 10));
}


XStringMap&  XStringMap::addNum( const char* key, double val, int precision)
{
    int  prec = precision >= 0 ? precision : std::numeric_limits<double>::digits10;
    return add( key, QByteArray::number( val, 'g', prec));
}


XStringMap&  XStringMap::addNum( const QByteArray& key, double val, int precision)
{
    int  prec = precision >= 0 ? precision : std::numeric_limits<double>::digits10;
    return add( key, QByteArray::number( val, 'g', prec));
}


XStringMap&  XStringMap::addNum(const QString& key, double val, int precision)
{
    int  prec = precision >= 0 ? precision : std::numeric_limits<double>::digits10;
    return add( key, QString::number( val, 'g', prec));
}


XStringMap&  XStringMap::addValues( const QStringList& stringList)
{
    foreach (const QString& str, stringList) {
        add("", str);
    }

    return *this;
}


XStringMap&  XStringMap::set( int i, const QByteArray& val)
{
    if ((i < 0) || (i >= _size))  return *this;  // Not valid index

    _valList[i].resize(0);  // Avoid Heap reallocation
    _valList[i] += val;

    return *this;
}


XStringMap&  XStringMap::set( int i, const QString& val)
{
    return set( i, val.toUtf8());
}


XStringMap&  XStringMap::set( const char* key, const QByteArray& val)
{
    int  i = indexOf( key);
    if (i < 0)
        add( key, val);
    else
        set( i, val);

    return *this;
}


XStringMap&  XStringMap::set( const char* key, const char* val)
{
    return set( key, QByteArray( val));
}


XStringMap&  XStringMap::set( const QByteArray& key, const QByteArray& val)
{
    return set( key.constData(), val);
}


XStringMap&  XStringMap::set( const char* key, const QString& val)
{
    return set( key, val.toUtf8());
}


XStringMap&  XStringMap::set( const QByteArray& key, const QString& val)
{
    return set( key, val.toUtf8());
}


XStringMap&  XStringMap::set( const QString& key, const QString& val)
{
    return set( key.toUtf8(), val.toUtf8());
}


XStringMap&  XStringMap::setKey( int i, const QByteArray& key)
{
    if ((i < 0) || (i >= _size))  return *this;  // Not valid index

    _keyList[i].resize(0);  // Avoid Heap reallocation
    _keyList[i] += key;

    return *this;
}


const QByteArray&  XStringMap::keyRef( int i)  const
{
    if ((i < 0) || (i >= _size))  return _nullValue;

    return _keyList.at(i);
}


QByteArray  XStringMap::key( int i, const char* def)  const
{
    if ((i < 0) || (i >= _size))  return def ? QByteArray( def) : QByteArray();

    return _keyList.at(i);
}


QByteArray  XStringMap::key( const QByteArray& value, const char* def)  const
{
    int  i = indexOfValue( value);
    if (i < 0)  return def ? QByteArray( def) : QByteArray();

    return _keyList.at(i);
}


QByteArray  XStringMap::key( const QString& value, const char* def)  const
{
    return key( value.toUtf8(), def);
}


QString  XStringMap::keyString( int i, const QString& def)  const
{
    if ((i < 0) || (i >= _size))  return def;

    const QByteArray&  key = _keyList.at(i);
    return QString::fromUtf8( key.constData(), key.size());
}


QString  XStringMap::keyString( const QString& value, const QString& def)  const
{
    int  i = indexOfValue( value);
    return keyString( i, def);
}


const QByteArray&  XStringMap::valueRef( int i)  const
{
    if ((i < 0) || (i >= _size))  return _nullValue;

    return _valList.at(i);
}


QByteArray  XStringMap::value( int i, const char* def)  const
{
    if ((i < 0) || (i >= _size))  return def ? QByteArray( def) : QByteArray();

    return _valList.at(i);
}


QByteArray  XStringMap::value( const char* key, const char* def)  const
{
    int  i = indexOf( key);
    if (i < 0)  return def ? QByteArray( def) : QByteArray();

    return _valList.at(i);
}


QByteArray  XStringMap::value( const char* keyPrefix, uint eNum, const char* def)  const
{
    if (!keyPrefix) {
        keyPrefix = "";
    }
    QByteArray  key;
    key += keyPrefix;
    key += QByteArray::number( eNum);

    return value( key, def);
}


QByteArray  XStringMap::value( const QByteArray& key, const char* def)  const
{
    int  i = indexOf( key);
    if (i < 0)  return def ? QByteArray( def) : QByteArray();

    return _valList.at(i);
}


QByteArray  XStringMap::value( const QByteArray& key, const QByteArray& def)  const
{
    int  i = indexOf( key);
    if (i < 0) {
        return def;
    }
    return _valList.at(i);
}


QString  XStringMap::valueString( int i, const QString& def)  const
{
    if ((i < 0) || (i >= _size))  return def;

    const QByteArray&  val = _valList.at(i);
    return QString::fromUtf8( val.constData(), val.size());
}


QString  XStringMap::valueString( const char* key, const QString& def)  const
{
    int  i = indexOf( key);
    return valueString( i, def);
}


QString  XStringMap::valueString( const char* keyPrefix, uint eNum, const QString& def)  const
{
    if (!keyPrefix)  keyPrefix = "";

    QByteArray  key;
    key += keyPrefix;
    key += QByteArray::number( eNum);

    return valueString( key.constData(), def);
}


QString  XStringMap::valueString( const QByteArray& key, const QString& def) const
{
    int  i = indexOf( key);
    return valueString( i, def);
}


QString  XStringMap::valueString( const QString& key, const QString& def) const
{
    int  i = indexOf( key);
    return valueString( i, def);
}


XStringMap&  XStringMap::remove( int index)
{
    if ((index < 0) || (index >= _size))  return *this;

    for (int i = index; i < _size - 1; ++i) {
        _keyList[i].resize(0);     // Avoid Heap reallocation
        _valList[i].resize(0);
        _keyList[i] += _keyList.at(i + 1);
        _valList[i] += _valList.at(i + 1);
    }
    --_size;

    return *this;
}


XStringMap&  XStringMap::remove( const char* key)
{
    return remove( indexOf( key));
}


XStringMap&  XStringMap::remove( const QByteArray& key)
{
    return remove( indexOf( key));
}


XStringMap&  XStringMap::remove( const QString& key)
{
    return remove( indexOf( key.toUtf8()));
}


XStringMap&  XStringMap::removeValue( const QByteArray& val)
{
    return remove( indexOfValue( val));
}


XStringMap&  XStringMap::removeValue( const QString& val)
{
    return remove( indexOfValue( val.toUtf8()));
}


void  XStringMap::setEmptyKeysToValue()
{
    for (int i = 0; i < _size; ++i) {
        if (_keyList.at(i).isEmpty()) {
            _keyList[i].resize(0);     // Avoid Heap reallocation
            _keyList[i] += _valList.at(i);
        }
    }
}


QStringList  XStringMap::keys()  const
{
    QStringList  retList;
    for (int i = 0; i < _size; ++i) {
        const QByteArray&  key = _keyList.at(i);
        retList += QString::fromUtf8( key.constData(), key.size());
    }
    return retList;
}


QStringList  XStringMap::values( const char* keyPrefix)  const
{
    if (!keyPrefix)  keyPrefix = "";

    QStringList  retList;
    for (int i = 0; i < _size; ++i) {
        if (*keyPrefix && !_keyList.at(i).startsWith( keyPrefix))  continue;

        const QByteArray&  value = _valList.at(i);
        retList += QString::fromUtf8( value.constData(), value.size());
    }
    return retList;
}


MQVariantMap XStringMap::toVariantMap( bool useStringVal)  const
{
    MQVariantMap  retMap;

    for (int i = 0; i < _size; ++i) {
        const QByteArray&  key = _keyList.at(i);
        const QByteArray&  value = _valList.at(i);
        retMap.insert( QString::fromUtf8( key.constData(), key.size()),
                            useStringVal ? QVariant( QString::fromUtf8( value))
                                         : QVariant( value));
    }
    return retMap;
}


QByteArray  XStringMap::toXString()  const
{
    QByteArray  outXString;
    QByteArray  valCoded;
    for (int i = 0; i < _size; ++i) {
        if (i > 0)
            outXString += ' ';
        const QByteArray&  key = _keyList.at(i);
        const QByteArray&  val = _valList.at(i);
        if (!key.isEmpty() || val.isEmpty() || val.contains('=')) {
            outXString += key;
            outXString += '=';
        }
        stringCode( valCoded, val);
        outXString += valCoded;
    }
    return outXString;
}


QString  XStringMap::toXStringString()  const
{
    QByteArray xstr = toXString();
    return QString::fromUtf8( xstr.constData(), xstr.size());
}


bool  XStringMap::fromXString( const QByteArray& inXString, int size)
{
    if (size < 0  ||  size > inXString.size()) {
        size = inXString.size();
    }
    clear();
    if (size == 0)  return true;  // Nothing to load

    int  startPos = 0;
    QByteArray  key;
    QByteArray  val;
    QByteArray  valCoded;

    forever {
        if (startPos >= size) {     // if past end of line, finished
            break;
        }
        while ( inXString.at( startPos) == ' ') {   // Skip leading space before key (only possible by manual coding)
            ++startPos;
        }

        // Start getting key
        key.resize(0);  // Default empty key
        int  posEq = inXString.indexOf('=', startPos);
        int  posSp = inXString.indexOf(' ', startPos);
        if (posSp < 0  ||  posSp >= size) {
            posSp = size; // last pos is set to end of string
        }
        if ((posEq >= 0) && (posEq < posSp)) {  // Key found
            key.append( inXString.constData() + startPos, posEq - startPos);
            startPos = posEq + 1;     // past '='
        }

        // Start getting value
        valCoded.resize(0);
        valCoded.append( inXString.constData() + startPos, posSp - startPos);
        stringDecode( val, valCoded);
        startPos = posSp + 1;     // past ' '

        add( key, val);
    }
    return true;
}


bool XStringMap::fromXString( const QString& inXString)
{
    return fromXString( inXString.toUtf8());
}


void  XStringMap::stringCode( QByteArray& dst, const QByteArray& src)
{
    int  srcSize = src.size();
    const char*  srcP = src.constData();

    dst.resize( 2 * srcSize);   // Max size of coded string
    char*  dstStart = dst.data();
    char*  dstP = dstStart;

    uchar c;
    for (int i = 0; i < srcSize; ++i) {
        c = uchar( *srcP++);
        switch (c) {
        case ' ':
            *dstP++ = '_';      // The coded string must not contain any ' '
            break;
        case '_':
            *dstP++ = '\\';
            *dstP++ = '_';
            break;
        case '\\':
            *dstP++ = '\\';
            *dstP++ = '\\';
            break;
        case '^':
            *dstP++ = '\\';
            *dstP++ = '^';
            break;
        case '\n':
            *dstP++ = '\\';
            *dstP++ = 'n';
            break;
        case '\r':
            *dstP++ = '\\';
            *dstP++ = 'r';
            break;
        case '\0':
            *dstP++ = '\\';
            *dstP++ = '0';
            break;
        default:
            if (c < 32) {      // 0 .. 31  Special control-char
                *dstP++ = '^';
                *dstP++ = char('A' + c - 1);
            }
            else {             // Normal char (also UTF8 which is above 127)
                *dstP++ = char(c);
            }
        }
    }
    dst.resize( int( dstP - dstStart));   // Set the real used size for coded string
}


void  XStringMap::stringDecode( QByteArray& dst, const QByteArray& src)
{
    int  srcSize = src.size();
    const char*  srcP = src.constData();

    dst.resize( srcSize);   // Max size of decoded string
    char*  dstStart = dst.data();
    char*  dstP = dstStart;

    bool  escapeFlag = false;
    bool  ctrlFlag   = false;
    uchar c;
    for (int i = 0; i < srcSize; ++i) {
        c = uchar( *srcP++);
        if (escapeFlag) {
            escapeFlag = false;
            switch (c) {
            case 'n':
                *dstP++ = '\n';
                break;
            case 'r':
                *dstP++ = '\r';
                break;
            case '0':
                *dstP++ = '\0';
                break;
            default:
                *dstP++ = char(c);
            }
        }
        else if (ctrlFlag) {
            ctrlFlag = false;
            *dstP++ = char( c - 'A' + 1);      // 0 .. 31  Special control-char
        }
        else {
            switch (c) {
            case '_':
                *dstP++ = ' ';
                break;
            case '\\':
                escapeFlag = true;
                break;
            case '^':
                ctrlFlag = true;
                break;
            default:        // Normal char (also UTF8 which is above 127)
                *dstP++ = char(c);
            }
        }
    }
    dst.resize( int( dstP - dstStart));   // Set the real used size for decoded string
}


XStringMap&  XStringMap::operator+=( const QVariantMap& other)
{
    return add( other);
}


XStringMap&  XStringMap::operator+=( const XStringMap& other)
{
    return add( other);
}


QByteArray XStringMap::info()
{
    return "XStringMap ver=" ARNXSTRINGMAP_VER;
}


void  XStringMap::checkSpace()
{
    if (_size >= _keyList.size()) {     // If out of space allocate more
        int  newCapacity = (_size > 0) ? (2 * _size) : 8;
        _keyList.reserve( newCapacity);  // Force exact min amount space beeing guaranted, i.e. no extra margin
        _valList.reserve( newCapacity);
        _keyList.resize( newCapacity);
        _valList.resize( newCapacity);
    }
}

}  // Arn::
