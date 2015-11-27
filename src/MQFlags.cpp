#include "ArnInc/MQFlags.hpp"
#include <QMetaType>
#include <QMetaObject>
#include <QMetaEnum>
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


void  MQFTxt::setTxtRef( int nameSpace, int enumVal, const char *txt)
{
    _enumStr.insert( (nameSpace << 16) + enumVal, txt);
}


void MQFTxt::setTxt(int nameSpace, int enumVal, const char* txt)
{
    if (!_txtStore)
        _txtStore = new QList<QByteArray>;

    int idx = _txtStore->size();
    *_txtStore += QByteArray( txt);

    setTxtRef( nameSpace, enumVal, _txtStore->at( idx).constData());
}


const char *MQFTxt::getTxt( int nameSpace, int enumVal)  const
{
    return _enumStr.value( (nameSpace << 16) + enumVal);
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

}
