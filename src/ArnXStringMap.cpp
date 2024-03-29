// Copyright (C) 2010-2022 Michael Wiklund.
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
#include <string.h>


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
    _hasBinCode = false;
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


const XStringMap::Options&  XStringMap::options()  const
{
    return _options;
}


void  XStringMap::setOptions( const XStringMap::Options& newOptions)
{
    _options = newOptions;
}


int  XStringMap::indexOf( const char* key, int from)  const
{
    if (key == arnNullptr)  return -1;  // Return Not found

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
    if (key == arnNullptr)  return *this;  // Not valid key

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


void  XStringMap::reverseOrder()
{
    if (_size <= 1)  return;

    QByteArray  key, val;
    for (int i = 0; i < _size / 2; ++i) {
        int  ir = _size - i - 1;
        key = _keyList.at( i);
        val = _valList.at( i);
        _keyList[ i].resize( 0);
        _valList[ i].resize( 0);
        _keyList[ i] += _keyList.at( ir);
        _valList[ i] += _valList.at( ir);
        _keyList[ ir].resize( 0);
        _valList[ ir].resize( 0);
        _keyList[ ir] += key;
        _valList[ ir] += val;
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
    bool  optFrame  = _options.is( Options::Frame);
    bool  optAnyKey = _options.is( Options::AnyKey);
    QByteArray  outXString;
    QByteArray  txtCoded;
    for (int i = 0; i < _size; ++i) {
        if (i > 0)
            outXString += ' ';
        const QByteArray&  key = _keyList.at(i);
        const QByteArray&  val = _valList.at(i);
        bool  useOrgKey = true;
        if (optAnyKey) {
            stringCode( txtCoded, key);
            if (_hasChgCode) {
                outXString += "^:";
                outXString += txtCoded;
                useOrgKey   = false;
            }
        }
        if (useOrgKey) {
            outXString += key;
        }
        stringCode( txtCoded, val);
        if (optFrame && !_hasBinCode) {
            QByteArray valLenTxt = QByteArray::number( val.size());
            bool useFrame = txtCoded.size() > val.size() + valLenTxt.size() + 4;
            if (useFrame) {
                outXString += "|=";
                outXString += valLenTxt;
                outXString += '<';
                outXString += val;
                outXString += '>';
                continue;
            }
        }
        if (!key.isEmpty() || val.isEmpty() || _hasEqChar) {
            outXString += '=';
        }
        outXString += txtCoded;
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
    if ((size < 0) || (size > inXString.size())) {
        size = inXString.size();
    }
    clear();
    if (size == 0)  return true;  // Nothing to load

    int  startPos = 0;
    QByteArray  key;
    QByteArray  val;
    QByteArray  txtCoded;

    forever {
        bool isFrame = false;
        if (startPos >= size) {     // if past end of line, finished
            break;
        }
        while ( inXString.at( startPos) == ' ') {   // Skip leading space before key (only possible by manual coding)
            ++startPos;
        }

        // Start getting key
        key.resize(0);  // Default empty key
        int  posEq  = inXString.indexOf( '=', startPos);
        int  posSep = inXString.indexOf( ' ', startPos);
        if (posSep < 0  ||  posSep >= size) {
            posSep = size; // last separator pos is set to end of string
        }
        if ((posEq >= 0) && (posEq < posSep)) {  // Key found
            int  keyOrgStart = startPos;
            int  keyOrgLen   = 0;
            if ((posEq > startPos) && (inXString.at( posEq - 1) == '|')) {  // Framed value
                isFrame = true;
                int  posFrSt     = inXString.indexOf( '<', posEq + 1);
                int  frameLen    = -1;
                bool  frameLenOk = false;
                if ((posFrSt > posEq) && (posFrSt < size - 1)) {
                    frameLen = inXString.mid( posEq + 1, posFrSt - posEq - 1).toInt( &frameLenOk);
                }
                int  posFrEn = posFrSt + frameLen + 1;
                if (frameLenOk && (posFrEn < size) && (inXString.at( posFrEn) == '>')) {
                    keyOrgLen = posEq - startPos - 1;
                    startPos  = posFrSt + 1;  // past '<'
                    posSep    = posFrEn + 1;  // past '>'
                }
                else  return false;  // Can't recover due to broken frame
            }
            else {
                keyOrgLen = posEq - startPos;
                startPos  = posEq + 1;     // past '='
            }
            if ((keyOrgLen >= 2) && (inXString.at( keyOrgStart) == '^') && (inXString.at( keyOrgStart + 1) == ':')) {  // Coded key
                txtCoded.resize(0);
                txtCoded.append( inXString.constData() + keyOrgStart + 2, keyOrgLen - 2);
                stringDecode( key, txtCoded);
            }
            else {
                key.append( inXString.constData() + keyOrgStart, keyOrgLen);
            }
        }

        // Start getting value
        if (isFrame) {
            val.resize(0);
            val.append( inXString.constData() + startPos, posSep - startPos - 1);
        }
        else {
            txtCoded.resize(0);
            txtCoded.append( inXString.constData() + startPos, posSep - startPos);
            stringDecode( val, txtCoded);
        }
        startPos = posSep + 1;     // past separator ' '

        add( key, val);
    }
    return true;
}


bool XStringMap::fromXString( const QString& inXString)
{
    return fromXString( inXString.toUtf8());
}


void  XStringMap::stringCode( QByteArray& dst, const QByteArray& src)  const
{
    bool  optRepeatLen = _options.is( Options::RepeatLen);
    bool  optNullTilde = _options.is( Options::NullTilde);
    bool  optAnyKey    = _options.is( Options::AnyKey);
    _hasBinCode = false;
    _hasChgCode = false;
    _hasEqChar  = false;

    int  srcSize = src.size();
    const char*  srcP = src.constData();

    dst.resize( 2 * srcSize);   // Max size of coded string
    char*  dstStart = dst.data();
    char*  dstP     = dstStart;
    char*  dstP0    = dstStart;
    const char*  srcRepLim = arnNullptr;

    bool  actNullTilde = false;
    qint16  lastChar   = -1;
    uchar  lastCharInc = 1;
    uchar  sameCount   = 0;
    uchar  srcChar;
    for (int i = 0; i < srcSize; ++i) {
        srcChar = uchar( *srcP++);
        if (optRepeatLen && (srcP >= srcRepLim)) {  // Optimize repeated chars
            if (srcChar == lastChar) {
                ++sameCount;
                if ((sameCount < 9) && (i < srcSize - 1))  continue;
                if (sameCount * lastCharInc > 2 ) {
                    *dstP++ = '\\';
                    *dstP++ = '0' + sameCount;
                    _hasChgCode = true;
                    sameCount = 0;
                    continue;
                }
                else {  // Last in source, 1 or 2 same
                    srcP     -= sameCount - 1;
                    i        -= sameCount - 1;
                }
                sameCount = 0;
            }
            else if (sameCount >= 1) {
                if (sameCount * lastCharInc > 2 ) {
                    *dstP++ = '\\';
                    *dstP++ = '0' + sameCount;
                    _hasChgCode = true;
                }
                else {  // Only 1 repeat, rewind last loop and redo
                    srcChar   = lastChar;
                    lastChar  = -1;
                    srcRepLim = srcP;
                    srcP     -= sameCount;
                    i        -= sameCount;
                }
                sameCount = 0;
            }
        }
        dstP0 = dstP;

        switch (srcChar) {
        case ' ':
            *dstP++ = '_';      // The coded string must not contain any ' '
            _hasChgCode = true;
            break;
        case '_':
            *dstP++ = '\\';
            *dstP++ = '_';
            _hasChgCode = true;
            break;
        case '\\':
            *dstP++ = '\\';
            *dstP++ = '\\';
            _hasChgCode = true;
            break;
        case '^':
            *dstP++ = '\\';
            *dstP++ = '^';
            _hasChgCode = true;
            break;
        case '~':
            if (actNullTilde) {
                *dstP++ = '\\';
                *dstP++ = '~';
                _hasChgCode = true;
            }
            else {
                *dstP++ = '~';
            }
            break;
        case '=':
            if (optAnyKey) {
                *dstP++ = '\\';
                *dstP++ = ':';
                _hasChgCode = true;
            }
            else {
                *dstP++ = '=';
            }
            _hasEqChar = true;
            break;
        case '\n':
            *dstP++ = '\\';
            *dstP++ = 'n';
            _hasBinCode = true;
            break;
        case '\r':
            *dstP++ = '\\';
            *dstP++ = 'r';
            _hasBinCode = true;
            break;
        case '\0':
            if (actNullTilde) {
                *dstP++ = '~';      // Null is a common value, substitute with single char
            }
            else if (optNullTilde) {
                *dstP++ = '^';      // Start the tilde substitution
                *dstP++ = '~';
                actNullTilde = true;
            }
            else {
                *dstP++ = '\\';
                *dstP++ = '0';
            }
            _hasBinCode = true;
            break;
        default:
            if (srcChar < 32) {      // 0 .. 31  Special control-char
                *dstP++ = '^';
                *dstP++ = char('A' + srcChar - 1);
                _hasBinCode = true;
            }
            else {             // Normal char (also UTF8 which is above 127)
                *dstP++ = char(srcChar);
            }
        }
        lastChar    = srcChar;
        lastCharInc = dstP - dstP0;
    }
    dst.resize( int( dstP - dstStart));   // Set the real used size for coded string
    _hasChgCode |= _hasBinCode;
}


void  XStringMap::stringDecode( QByteArray& dst, const QByteArray& src)  const
{
    int  srcSize = src.size();
    const char*  srcP = src.constData();

    dst.resize( srcSize * 5);   // Max size of decoded string. Worst for repeated chars like "A\9\9"
    char*  dstStart = dst.data();
    char*  dstP = dstStart;

    bool  actNullTilde = false;
    bool  escapeFlag   = false;
    bool  ctrlFlag     = false;
    qint16  lastChar = -1;
    qint16  dstChar;
    uchar  repeatCount;
    uchar  srcChar;
    for (int i = 0; i < srcSize; ++i) {
        dstChar    = -1;
        repeatCount = 1;
        srcChar = uchar( *srcP++);
        if (escapeFlag) {
            escapeFlag = false;
            switch (srcChar) {
            case 'n':
                dstChar = '\n';
                break;
            case 'r':
                dstChar = '\r';
                break;
            case '0':
                dstChar = '\0';
                break;
            case ':':
                dstChar = '=';
                break;
            default:
                if ((srcChar >= '1') && (srcChar <= '9')) {
                    dstChar     = lastChar;
                    repeatCount = srcChar - '0';
                }
                else {
                    dstChar = srcChar;
                }
            }
        }
        else if (ctrlFlag) {
            ctrlFlag = false;
            switch (srcChar) {
            case '~':
                dstChar = '\0';
                actNullTilde = true;
                break;
            default:
                if ((srcChar >= 'A') && (srcChar <= 'A' + 30)) {
                    dstChar  = uchar( srcChar - 'A' + 1);      // 1 .. 31  Special control-char
                }
            }
        }
        else {
            switch (srcChar) {
            case '_':
                dstChar = ' ';
                break;
            case '~':
                if (actNullTilde) {
                    dstChar = '\0';
                }
                else {
                    dstChar = '~';
                }
                break;
            case '\\':
                escapeFlag = true;
                break;
            case '^':
                ctrlFlag = true;
                break;
            default:        // Normal char (also UTF8 which is above 127)
                dstChar = srcChar;
            }
        }
        if (dstChar >= 0) {
            for (uchar i = 0; i < repeatCount; ++i) {
                *dstP++ = uchar( dstChar);
            }
            lastChar = dstChar;
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
