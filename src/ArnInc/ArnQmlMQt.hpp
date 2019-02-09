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

#ifndef ARNQMLMQT_HPP
#define ARNQMLMQT_HPP

#include "ArnQml.hpp"
#include <QObject>


namespace Arn {

class QmlMQtObject : public QObject, public QML_PARSER_STATUS
{
    Q_OBJECT
#ifdef QML_Qt4
    Q_INTERFACES( QDeclarativeParserStatus)
    Q_PROPERTY( QDeclarativeListProperty<QObject> data READ data DESIGNABLE false)
#else
    Q_INTERFACES( QQmlParserStatus)
    Q_PROPERTY( QQmlListProperty<QObject> data READ data DESIGNABLE false)
#endif

    Q_PROPERTY( QmlMQtObject* parent READ parentItem WRITE setParentItem NOTIFY parentChanged DESIGNABLE false FINAL)

    Q_CLASSINFO("DefaultProperty", "data")

public:
    QmlMQtObject( QmlMQtObject* parent = 0);
    virtual  ~QmlMQtObject();

    QmlMQtObject*  parentItem()  const;
    void  setParentItem( QmlMQtObject* parent);

    QML_LIST_PROPERTY<QObject>  data();
    static void  data_append( QML_LIST_PROPERTY<QObject>* prop, QObject* obj);
    static int  data_count( QML_LIST_PROPERTY<QObject>* prop);
    static QObject*  data_at( QML_LIST_PROPERTY<QObject>* prop, int index);
    static void  data_clear( QML_LIST_PROPERTY<QObject>* prop);

    virtual void  classBegin();
    virtual void  componentComplete();

public slots:

signals:
    void  parentChanged( QmlMQtObject* obj);
    void  completed();

private:
};

}

#endif // ARNQMLMQT_HPP
