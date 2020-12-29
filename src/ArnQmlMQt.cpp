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

#include "ArnInc/ArnQmlMQt.hpp"


namespace Arn {

QmlMQtObject::QmlMQtObject( QmlMQtObject* parent)
    : QObject(parent)
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
    return QML_LIST_PROPERTY<QObject>( this, arnNullptr, data_append, data_count, data_at, data_clear);
}


void  QmlMQtObject::data_append( QML_LIST_PROPERTY<QObject>* prop, QObject* obj)
{
    if (!obj)  return;

    QmlMQtObject*  that = static_cast<QmlMQtObject*>( prop->object);
    obj->setParent( that);
}


ARN_SIZETYPE  QmlMQtObject::data_count( QML_LIST_PROPERTY<QObject>* prop)
{
    QmlMQtObject*  that = static_cast<QmlMQtObject*>( prop->object);
    return that->children().count();
}


QObject*  QmlMQtObject::data_at( QML_LIST_PROPERTY<QObject>* prop, ARN_SIZETYPE index)
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
}


void  QmlMQtObject::classBegin()
{
}


void  QmlMQtObject::componentComplete()
{
    emit completed();
}

}  // Arn::
