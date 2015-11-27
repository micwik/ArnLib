#include "ArnInc/MQFlags.hpp"
#include <ArnInc/XStringMap.hpp>
#include <QMetaType>
#include <QMetaObject>
#include <QMetaEnum>
#include <QMapIterator>
#include <QDebug>

namespace Arn {

QString  mqfToString( const QMetaObject& metaObj, int val)
{
    if (metaObj.enumeratorCount() < 1)  return QString();

    QString  retVal;
    QMetaEnum  metaEnum = metaObj.enumerator(0);
    int  nKeys = metaEnum.keyCount();
    for (int i = 0; i < nKeys; ++i) {
        uint  enumVal = metaEnum.value(i);
        if (!isPower2( enumVal))
            continue;  // Not a single bit enum
        if (val & enumVal) {
            if (!retVal.isEmpty())
                retVal += " | ";
            retVal += metaEnum.key(i);
        }
    }

    return retVal;
}


bool  isPower2( uint x)
{
    return x && ((x & (x - 1)) == 0);
}


MQFTxt::MQFTxt( const QMetaObject& metaObj)
    : _metaObj( metaObj)
{
    _txtStore = 0;
    setupFromMetaObject();
}


void  MQFTxt::setTxtRef( quint16 nameSpace, quint16 enumVal, const char *txt)
{
    _enumStr.insert( toEnumIndex( nameSpace, enumVal), txt);
}


void  MQFTxt::setTxt( quint16 nameSpace, quint16 enumVal, const char* txt)
{
    if (!_txtStore)
        _txtStore = new QList<QByteArray>;

    int idx = _txtStore->size();
    *_txtStore += QByteArray( txt);

    setTxtRef( nameSpace, enumVal, _txtStore->at( idx).constData());
}


const char*  MQFTxt::getTxt( quint16 nameSpace, quint16 enumVal)  const
{
    return _enumStr.value( toEnumIndex( nameSpace, enumVal), "");
}


void  MQFTxt::setTxtString( quint16 nameSpace, quint16 enumVal, const QString& txt)
{
    setTxt( nameSpace, enumVal, txt.toUtf8().constData());
}


QString  MQFTxt::getTxtString( quint16 nameSpace, quint16 enumVal) const
{
    return QString::fromUtf8( getTxt( nameSpace, enumVal));
}


QString  MQFTxt::makeBitSet( quint16 nameSpace)
{
    quint32  idxStart = toEnumIndex( nameSpace, 0);
    quint32  idxStop  = toEnumIndex( nameSpace, 0xffff);
    XStringMap  xsm;
    int  bitNum = 0;

    QMap<quint32,const char*>::iterator  i = _enumStr.lowerBound( idxStart);
    while (i != _enumStr.end()) {
        quint32  enumIndexStored = i.key();
        if (enumIndexStored > idxStop)  break;

        while (toEnumIndex( nameSpace, 1 << bitNum) < enumIndexStored)
            ++bitNum;
        xsm.add("B" + QByteArray::number( bitNum), QByteArray( i.value()));
        ++i;
    }

    return QString::fromUtf8( xsm.toXString());
}


void  MQFTxt::setupFromMetaObject()
{
    if (_metaObj.enumeratorCount() < 1)  return;

    QMetaEnum  metaEnum = _metaObj.enumerator(0);
    int  nKeys = metaEnum.keyCount();
    for (int i = 0; i < nKeys; ++i) {
        int  enumVal = metaEnum.value(i);
        if (!isPower2( enumVal))
            continue;  // Not a single bit enum
        setTxtRef( 0, enumVal, metaEnum.key(i));
    }
}


quint32  MQFTxt::toEnumIndex( quint16 nameSpace, quint16 enumVal)
{
    return (nameSpace << 16) | enumVal;
}

}
