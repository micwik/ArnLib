// Copyright (C) 2010-2021 Michael Wiklund.
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

#include "ArnInc/MQFlags.hpp"
#include <ArnInc/XStringMap.hpp>
#include <ArnInc/Math.hpp>
#include <QMetaType>
#include <QMetaObject>
#include <QMetaEnum>
#include <QMapIterator>
#include <QDebug>
#include <limits.h>

namespace Arn {

EnumTxt::SubEnumEntry::SubEnumEntry( const EnumTxt& subEnum, uint bitMask, uint factor)
{
    _subEnum = &subEnum;
    _bitMask = bitMask;
    _bitPos  = log2( factor | 1);
}


EnumTxt::EnumTxt( const QMetaObject& metaObj, bool isFlag, const _InitEnumTxt* initTxt, const _InitSubEnum* initSubEnum,
                  const char* name)
    : _metaObj( metaObj)
{
    _txtStore   = arnNullptr;
    _subEnumTab = arnNullptr;
    _isFlag     = isFlag;
    _name       = name;
    setupFromMetaObject();
    setupTxt( initTxt);
    setupSubEnum( initSubEnum);
}


void  EnumTxt::setTxtRefAny( const char* txt, int enumVal, quint16 nameSpace)
{
    _enumTxtTab.insert( EnumTxtKey( enumVal, nameSpace, _isFlag), txt);
}


void  EnumTxt::setTxtRef( const char *txt, int enumVal, quint16 nameSpace)
{
    if (nameSpace > 0)
        setTxtRefAny( txt, enumVal, nameSpace);
}


void  EnumTxt::setTxt( const char* txt, int enumVal, quint16 nameSpace)
{
    if (nameSpace == 0)  return;  // Not allowed to change original enum texts

    if (!_txtStore)
        _txtStore = new QList<QByteArray>;

    int idx = _txtStore->size();
    *_txtStore += QByteArray( txt);

    setTxtRefAny( _txtStore->at( idx).constData(), enumVal, nameSpace);
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


int  EnumTxt::getEnumVal( const char* txt, int defaultVal, quint16 nameSpace, bool* isFound)  const
{
    EnumTxtKey  keyStart( _isFlag ? 1        : uint(INT_MIN), nameSpace, _isFlag);
    EnumTxtKey  keyStop(  _isFlag ? UINT_MAX : uint(INT_MAX), nameSpace, _isFlag);

    QMap<EnumTxtKey,const char*>::const_iterator  i = _enumTxtTab.lowerBound( keyStart);
    while (i != _enumTxtTab.end()) {
        const EnumTxtKey&  keyStored = i.key();
        if (keyStop < keyStored)  break;

        if (qstrcmp( i.value(), txt) == 0) {
            if (isFound)  *isFound = true;
            return int(keyStored._enumVal);  // Matching txt
        }
        ++i;
    }

    if (isFound)  *isFound = false;
    return defaultVal;  // Not found
}


int  EnumTxt::getEnumVal( const QString& txt, int defaultVal, quint16 nameSpace, bool* isFound)  const
{
    return getEnumVal( txt.toUtf8().constData(), defaultVal, nameSpace, isFound);
}


bool EnumTxt::getSubEnumVal( const char* txt, int& subEnumVal, uint& bitMask, quint16 nameSpace)  const
{
    if (_subEnumTab) {
        foreach (const SubEnumEntry& entry, *_subEnumTab) {
            bool isFound = false;
            int enumVal = entry._subEnum->getEnumVal( txt, -1, nameSpace, &isFound);
            if (isFound) {
                subEnumVal = enumVal << entry._bitPos;
                bitMask    = entry._bitMask;
                return true;
            }
        }
    }

    return false;  // Not found
}


bool EnumTxt::getSubEnumVal( const QString& txt, int& subEnumVal, uint& bitMask, quint16 nameSpace) const
{
    return getSubEnumVal( txt.toUtf8().constData(), subEnumVal, bitMask, nameSpace);
}


void  EnumTxt::addBitSet( XStringMap& xsm, quint16 nameSpace, bool neverHumanize)  const
{
    if (!_isFlag)  return;  // Only for flags

    EnumTxtKey  keyStart( 1,       nameSpace, _isFlag);
    EnumTxtKey  keyStop( UINT_MAX, nameSpace, _isFlag);
    uint  bitNum = 0;

    QMap<EnumTxtKey,const char*>::const_iterator  i = _enumTxtTab.lowerBound( keyStart);
    for (; i != _enumTxtTab.end(); ++i) {
        const EnumTxtKey&  keyStored = i.key();
        if (keyStop < keyStored)  break;

        uint  enumValStored = keyStored._enumVal;
        if (_subEnumMask & enumValStored)  // This is part of subEnum
            continue;

        QByteArray enumValTxt;
        if (keyStored._isSingleBit) {
            bitNum     = log2( enumValStored);
            enumValTxt = "B" + QByteArray::number( bitNum);
        }
        else {
            enumValTxt = numStr( enumValStored);
        }

        QByteArray  enumTxt;
        if ((nameSpace > 0) || neverHumanize) {  // Enum name is used as is
            enumTxt = i.value();
        }
        else {  // Enum name is humanized (Leading capital and separated words)
            enumTxt = humanize( QString::fromUtf8( i.value())).toUtf8();
        }

        xsm.add( enumValTxt, enumTxt);
    }

    if (_subEnumTab) {
        XStringMap xsmSub;
        int subEnumIdx = 0;
        foreach (const SubEnumEntry& entry, *_subEnumTab) {
            uint bitMask = entry._bitMask;
            QByteArray enumValTxt = "SE" + QByteArray::number( subEnumIdx) + ":" + numStr( bitMask);
            QByteArray enumTxt    = getTxt( bitMask, nameSpace);
            xsm.add( enumValTxt, enumTxt);

            xsmSub.clear();
            entry._subEnum->addEnumSet( xsmSub, nameSpace, neverHumanize);
            for (int i = 0; i < xsmSub.size(); ++i) {
                uint enumVal = (xsmSub.keyRef( i).toInt() << entry._bitPos) & bitMask;
                enumValTxt = "E" + QByteArray::number( subEnumIdx) + ":" + numStr( enumVal);
                xsm.add( enumValTxt, xsmSub.valueRef( i));
            }
            ++subEnumIdx;
        }
    }
}


QString  EnumTxt::getBitSet( quint16 nameSpace, bool neverHumanize)  const
{
    if (!_isFlag)  return QString();  // Only for flags

    XStringMap  xsm;
    addBitSet( xsm, nameSpace, neverHumanize);
    return QString::fromUtf8( xsm.toXString());
}


QString  EnumTxt::flagsToString( int val, quint16 nameSpace)  const
{
    if (!_isFlag)  return QString();  // Only for flags

    return flagsToStringList( val, nameSpace).join(" | ");
}


QStringList  EnumTxt::flagsToStringList( int val, quint16 nameSpace)  const
{
    if (!_isFlag)  return QStringList();  // Only for flags

    uint maxBitVal = (UINT_MAX >> 1) + 1;
    EnumTxtKey  keyStart( 1,        nameSpace, _isFlag);  // Zero is skipped
    EnumTxtKey  keyStop( maxBitVal, nameSpace, _isFlag);  // Combined bits are skipped
    QStringList  retVal;

    QMap<EnumTxtKey,const char*>::const_iterator  i = _enumTxtTab.lowerBound( keyStart);
    for (; i != _enumTxtTab.end(); ++i) {
        const EnumTxtKey&  keyStored = i.key();
        if (keyStop < keyStored)  break;

        uint enumValStored = keyStored._enumVal;
        if (_subEnumMask & enumValStored)
            continue;

        if (uint( val) & enumValStored) {
            retVal += QString::fromUtf8( i.value());
        }
    }
    if (_subEnumTab) {
        foreach (const SubEnumEntry& entry, *_subEnumTab) {
            int subEnum = (uint( val) & entry._bitMask) >> entry._bitPos;
            retVal += entry._subEnum->getTxt( subEnum, nameSpace);
        }
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

    uint maxBitVal = (UINT_MAX >> 1) + 1;
    EnumTxtKey  keyStart( 1,        nameSpace, _isFlag);
    EnumTxtKey  keyStop( maxBitVal, nameSpace, _isFlag);
    int  retVal = 0;

    QMap<EnumTxtKey,const char*>::iterator  i = _enumTxtTab.lowerBound( keyStart);
    for (; i != _enumTxtTab.end(); ++i) {
        const EnumTxtKey&  keyStored = i.key();
        if (keyStop < keyStored)  break;

        uint enumValStored = keyStored._enumVal;
        if (_subEnumMask & enumValStored)
            continue;

        if (flagStrings.contains( QString::fromUtf8( i.value())))
            retVal |= enumValStored;
    }
    if (_subEnumTab) {
        foreach (const SubEnumEntry& entry, *_subEnumTab) {
            EnumTxtKey  keyStart( uint(INT_MIN), nameSpace, false);
            EnumTxtKey  keyStop(  uint(INT_MAX), nameSpace, false);
            QMap<EnumTxtKey,const char*>::const_iterator  j = entry._subEnum->_enumTxtTab.lowerBound( keyStart);
            for (; true; ++j) {
                const EnumTxtKey&  keyStored = j.key();
                if (keyStop < keyStored)  break;

                if (flagStrings.contains( QString::fromUtf8( j.value()))) {
                    retVal &= ~entry._bitMask;
                    retVal |= (keyStored._enumVal << entry._bitPos) & entry._bitMask;
                    break;
                }
            }
        }
    }

    return retVal;
}


void  EnumTxt::addEnumSet( XStringMap& xsm, quint16 nameSpace, bool neverHumanize)  const
{
    EnumTxtKey  keyStart( _isFlag ? 1        : uint(INT_MIN), nameSpace, _isFlag);
    EnumTxtKey  keyStop(  _isFlag ? UINT_MAX : uint(INT_MAX), nameSpace, _isFlag);

    QMap<EnumTxtKey,const char*>::const_iterator  i = _enumTxtTab.lowerBound( keyStart);
    while (i != _enumTxtTab.end()) {
        const EnumTxtKey&  keyStored = i.key();
        if (keyStop < keyStored)  break;

        int  enumValStored = int(keyStored._enumVal);
        QByteArray  enumTxt;
        if ((nameSpace > 0) || neverHumanize) {  // Enum name is used as is
            enumTxt = i.value();
        }
        else {  // Enum name is humanized (Leading capital and separated words)
            enumTxt = humanize( QString::fromUtf8( i.value())).toUtf8();
        }
        xsm.add(QByteArray::number( enumValStored), enumTxt);
        ++i;
    }
}


QString  EnumTxt::getEnumSet( quint16 nameSpace, bool neverHumanize)  const
{
    XStringMap  xsm;
    addEnumSet( xsm, nameSpace, neverHumanize);
    return QString::fromUtf8( xsm.toXString());
}


void EnumTxt::addSubEnum( const EnumTxt& subEnum, uint bitMask, uint factor)
{
    if (subEnum.isFlag())  return;  // Not allowed with flags as subEnum

    if (!_subEnumTab)
        _subEnumTab = new QList<SubEnumEntry>;
    SubEnumEntry entry( subEnum, bitMask, factor);
    *_subEnumTab += entry;
    _subEnumMask |= bitMask;
}


void  EnumTxt::setMissingTxt( quint16 toNameSpace, quint16 fromNameSpace, bool neverHumanize)
{
    if (toNameSpace == 0)  return;  // Not allowed to change original enum texts
    if (toNameSpace == fromNameSpace)  return;

    XStringMap  xsmFrom;
    XStringMap  xsmTo;
    addEnumSet( xsmFrom, fromNameSpace);
    addEnumSet( xsmTo,   toNameSpace);

    for (int i = 0; i < xsmFrom.size(); ++i) {
        const QByteArray&  enumValKey = xsmFrom.keyRef( i);
        if (xsmTo.indexOf( enumValKey) < 0) {  // Missing enumval in target
            int  enumVal = enumValKey.toInt();
            const char*  enumTxt = getTxt( enumVal, fromNameSpace);  // Get the original txt ptr

            if ((fromNameSpace > 0) || neverHumanize) {  // Enum name is used as is
                setTxtRefAny( enumTxt, enumVal, toNameSpace);
            }
            else {  // Enum name is humanized (Leading capital and separated words)
                QByteArray  enumTxtHuman = humanize( QString::fromUtf8( enumTxt)).toUtf8();
                setTxt( enumTxtHuman.constData(), enumVal, toNameSpace);
            }
        }
    }
}


QString  EnumTxt::humanize( const QString& txt)
{
    QString  retVal;
    bool  wasDigit     = false;
    bool  wasUpper     = false;
    bool  wasLower     = false;
    bool  wasWantSpace = false;
    bool  wasSpace     = false;

    for (int i = 0; i < txt.size(); ++i) {
        QChar c = txt.at(i);
        bool isDigit   = c.isDigit();
        bool isUpper   = c.isUpper();
        bool isLower   = c.isLower();
        bool wantSpace = false;

        if (i > 0)
            c = c.toLower();

        if (i == 0)
            c = c.toUpper();
        else if (c == '_')
            c = ' ';
        else if ((i >= 2) && wasUpper && isLower && !wasWantSpace)
            retVal.insert( retVal.size() - 1, ' ');
        else if (wasLower && isUpper)
            wantSpace = true;
        else if (!wasDigit && isDigit && !wasSpace)
            wantSpace = true;
        else if (wasDigit && !isDigit)
            wantSpace = true;

        if (wantSpace)
            retVal += ' ';

        retVal += c;

        wasSpace     = c.isSpace();
        wasDigit     = isDigit;
        wasUpper     = isUpper;
        wasLower     = isLower;
        wasWantSpace = wantSpace;
    }

    return retVal;
}


void  EnumTxt::setupTxt( const _InitEnumTxt* initTxt)
{
    if (!initTxt)  return;  // Nothing to setup

    for (int i = 0; (initTxt[i].enumTxt || initTxt[i].enumVal || initTxt[i].ns); ++i) {
        if (initTxt[i].enumTxt)
            setTxtRef( initTxt[i].enumTxt, initTxt[i].enumVal, initTxt[i].ns);
        else
            setMissingTxt( initTxt[i].ns, initTxt[i].enumVal);
    }
}


void EnumTxt::setupSubEnum( const _InitSubEnum* initSubEnum)
{
    if (!initSubEnum)  return;  // Nothing to setup

    for (int i = 0; initSubEnum[i].enumTxtClass; ++i) {
        addSubEnum( *initSubEnum[i].enumTxtClass, initSubEnum[i].mask, initSubEnum[i].factor);
    }
}


QByteArray EnumTxt::numStr( uint num)
{
    if (num < 10)
        return QByteArray::number( num);
    else
        return "0x" + QByteArray::number( num, 16);
}

bool EnumTxt::isFlag() const
{
    return _isFlag;
}


const char*  EnumTxt::name()  const
{
    return _name;
}


int Arn::EnumTxt::enumCount()  const
{
    if (_metaObj.enumeratorCount() < 1)  return 0;

    QMetaEnum  metaEnum = _metaObj.enumerator(0);
    return metaEnum.keyCount();
}


void  EnumTxt::setupFromMetaObject()
{
    if (_metaObj.enumeratorCount() < 1)  return;

    QMetaEnum  metaEnum = _metaObj.enumerator(0);
    int  nKeys = metaEnum.keyCount();
    for (int i = 0; i < nKeys; ++i) {
        int  enumVal = metaEnum.value(i);
        setTxtRefAny( metaEnum.key(i), enumVal, 0);
    }
}


EnumTxt::EnumTxtKey::EnumTxtKey( uint enumVal, quint16 nameSpace, bool isFlag)
{
    _enumVal     = enumVal;
    _nameSpace   = nameSpace;
    _isFlag      = isFlag;
    _isSingleBit = isFlag ? isPower2( enumVal ) : false;
}


bool  EnumTxt::EnumTxtKey::operator <( const EnumTxt::EnumTxtKey& other)  const
{
    if (_isFlag != other._isFlag)  qWarning() << "ERROR EnumKey isFlag diff !!!";
    if (_nameSpace < other._nameSpace)  return true;
    if (_nameSpace > other._nameSpace)  return false;
    if (_isFlag) {  // Flags sorted: 1, 2, 4, 8 .. 0, 3, 5, 6, 7, 9 .. (i.e. Singlebits .. Others)
        if (_isSingleBit != other._isSingleBit)  return _isSingleBit;
        return _enumVal < other._enumVal;  // Flag: unsigned compare
    }
    else {  // Enums sorted: as signed integers
        return int(_enumVal) < int(other._enumVal);
    }
}

}
