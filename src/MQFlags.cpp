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


MQFTxt::MQFTxt( const QMetaObject& metaObj, bool isFlag)
    : _metaObj( metaObj)
{
    _txtStore = 0;
    _isFlag   = isFlag;
    setupFromMetaObject();
}


void  MQFTxt::setTxtRef( const char *txt, int enumVal, quint16 nameSpace)
{
    _enumTxtTab.insert( EnumTxtKey( enumVal, nameSpace, _isFlag), txt);
}


void  MQFTxt::setTxt( const char* txt, int enumVal, quint16 nameSpace)
{
    if (!_txtStore)
        _txtStore = new QList<QByteArray>;

    int idx = _txtStore->size();
    *_txtStore += QByteArray( txt);

    setTxtRef( _txtStore->at( idx).constData(), enumVal, nameSpace);
}


const char*  MQFTxt::getTxt( int enumVal, quint16 nameSpace)  const
{
    return _enumTxtTab.value( EnumTxtKey( enumVal, nameSpace, _isFlag), "");
}


void  MQFTxt::setTxtString( const QString& txt, int enumVal, quint16 nameSpace)
{
    setTxt( txt.toUtf8().constData(), enumVal, nameSpace);
}


QString  MQFTxt::getTxtString( int enumVal, quint16 nameSpace)  const
{
    return QString::fromUtf8( getTxt( enumVal, nameSpace));
}


void  MQFTxt::addBitSet( XStringMap& xsm, quint16 nameSpace)
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


QString  MQFTxt::getBitSet( quint16 nameSpace)
{
    if (!_isFlag)  return QString();  // Only for flags

    XStringMap  xsm;
    addBitSet( xsm, nameSpace);
    return QString::fromUtf8( xsm.toXString());
}


QString  MQFTxt::flagsToString( int val, quint16 nameSpace)
{
    if (!_isFlag)  return QString();  // Only for flags

    return flagsToStringList( val, nameSpace).join(" | ");
}


QStringList  MQFTxt::flagsToStringList( int val, quint16 nameSpace)
{
    if (!_isFlag)  return QStringList();  // Only for flags

    EnumTxtKey  keyStart( 0,       nameSpace, _isFlag);
    EnumTxtKey  keyStop( UINT_MAX, nameSpace, _isFlag);
    XStringMap  xsm;
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


void  MQFTxt::setupFromMetaObject()
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


bool  MQFTxt::EnumTxtKey::operator <( const MQFTxt::EnumTxtKey& other)  const
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
