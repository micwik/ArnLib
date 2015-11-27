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

#include "ArnInc/ArnQmlMQt.hpp"


namespace Arn {

QmlMQtObject::QmlMQtObject( QmlMQtObject* parent)
{
}


QmlMQtObject::~QmlMQtObject()
{
}


QmlMQtObject*  QmlMQtObject::parentItem()  const
{
    return static_cast<QmlMQtObject*>( parent());
}


void  QmlMQtObject::setParentItem( QmlMQtObject* parent)
{
    setParent( parent);
    emit parentChanged( this);
}


QML_LIST_PROPERTY<QObject>  QmlMQtObject::data()
{
    return QML_LIST_PROPERTY<QObject>( this, 0, data_append, data_count, data_at, data_clear);
}


void  QmlMQtObject::data_append( QML_LIST_PROPERTY<QObject>* prop, QObject* obj)
{
    if (!obj)  return;

    QmlMQtObject*  that = static_cast<QmlMQtObject*>( prop->object);
    obj->setParent( that);
}


int  QmlMQtObject::data_count( QML_LIST_PROPERTY<QObject>* prop)
{
    QmlMQtObject*  that = static_cast<QmlMQtObject*>( prop->object);
    return that->children().count();
}


QObject*  QmlMQtObject::data_at( QML_LIST_PROPERTY<QObject>* prop, int index)
{
    QmlMQtObject*  that = static_cast<QmlMQtObject*>( prop->object);
    if ((index >= 0) && (index < that->children().count()))
        return that->children().at( index);
    else
        return 0;
}


void  QmlMQtObject::data_clear(QML_LIST_PROPERTY<QObject>* prop)
{
    QmlMQtObject*  that = static_cast<QmlMQtObject*>( prop->object);
    const QObjectList&  children = that->children();
    int  childCount = children.count();
    for (int index = 0; index < childCount; ++index)
        children.at(0)->setParent(0);
#if 0
// MW: needed logic ?
    if (that->componentComplete) {
        for (int index = 0; index < childCount; ++index)
            children.at(0)->setParent(0);
    }
    else {
        for (int index = 0 ;index < childCount; index++)
            QGraphicsItemPrivate::get(d->children.at(0))->setParentItemHelper(0, /*newParentVariant=*/0, /*thisPointerVariant=*/0);
    }
#endif
}


void  QmlMQtObject::classBegin()
{
}


void  QmlMQtObject::componentComplete()
{
    emit completed();
}

}  // Arn::
