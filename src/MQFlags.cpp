#include "ArnInc/MQFlags.hpp"
#include <QMetaType>
#include <QMetaObject>
#include <QMetaEnum>
#include <QDebug>

namespace Arn {

QString  mqfToString( const QMetaObject *metaObj, int val)
{
    if (!metaObj)  return QString();
    if (metaObj->enumeratorCount() < 1)  return QString();

    QString  retVal;
    QMetaEnum  metaEnum = metaObj->enumerator(0);
    int  nKeys = metaEnum.keyCount();
    for (int i = 0; i < nKeys; ++i) {
        int  enumVal = metaEnum.value(i);
        if ((!enumVal || ((enumVal & (enumVal - 1)) != 0)))
            continue;  // Not a single bit enum
        if (val & enumVal) {
            if (!retVal.isEmpty())
                retVal += " | ";
            retVal += metaEnum.key(i);
        }
    }

    return retVal;
}

}
