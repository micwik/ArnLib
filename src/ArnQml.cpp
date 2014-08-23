// Copyright (C) 2010-2014 Michael Wiklund.
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

#include "ArnInc/ArnQml.hpp"
#include "ArnInc/ArnQmlMSystem.hpp"
#include <QtQml>

using namespace Arn;


ArnItemQml::ArnItemQml( QObject* parent)
    : ArnItem( parent)
{
    _isCompleted = false;
}


QString  ArnItemQml::valueType()  const
{
    if (_valueType == QMetaType::Void)  return QString();

    const char*  typeName = QMetaType::typeName(_valueType);
    if (!typeName)  return QString();

    return typeName;
}


void  ArnItemQml::setValueType( const QString& typeName)
{
    if (typeName.isEmpty()) {
        _valueType = QMetaType::Void;
    }
    else {
        int  type = QMetaType::type( typeName.toLatin1().constData());
/*
        if (!type) {
            context()->throwError( QScriptContext::TypeError,
                                   "Setting unknown defaultType=" + typeName);
            return;
        }
        else
*/
            _valueType = type;
    }

    emit valueTypeChanged();
}


QString  ArnItemQml::path()  const
{
    return _path;
}


void  ArnItemQml::setPath( const QString& path)
{
    _path = path;
    if (_isCompleted)
        open( path);

    emit pathChanged();
}


void  ArnItemQml::setVariant( const QVariant& value)
{
    if (_valueType == QMetaType::Void)  // No valueType, no conversion
        ArnItem::setValue( value);
    else {  // Use valueType
        QVariant  val = value;
        if (val.convert( QVariant::Type( _valueType))) {
            ArnItem::setValue( val);
        }
        else {
            // context()->throwError( QScriptContext::TypeError,
            //                       "Can't convert to defaultType=" + defaultType());
        }
    }
}


void  ArnItemQml::setPipeMode( bool isPipeMode)
{
    if (isPipeMode)
        ArnItem::setPipeMode();
}


void  ArnItemQml::setMaster( bool isMaster)
{
    if (isMaster)
        ArnItem::setMaster();
}


void  ArnItemQml::setAutoDestroy( bool isAutoDestroy)
{
    if (isAutoDestroy)
        ArnItem::setAutoDestroy();
}


void  ArnItemQml::setSaveMode( bool isSaveMode)
{
    if (isSaveMode)
        ArnItem::setSaveMode();
}


void  ArnItemQml::classBegin()
{
}


void  ArnItemQml::componentComplete()
{
    _isCompleted = true;
    if (!_path.isEmpty())
        setPath( _path);
}


void ArnItemQml::itemUpdated(const ArnLinkHandle& handleData, const QByteArray* value)
{
    ArnItem::itemUpdated( handleData, value);

    emit valueChanged();
}


void  Arn::qmlSetup( QmlSetup flags)
{
    if (flags.is( flags.ArnLib)) {
        qmlRegisterType<ArnItemQml>("ArnLib", 1, 0, "ArnItem");
    }
    if (flags.is( flags.MSystem)) {
        qmlRegisterType<QmlMFileIO>("MSystem", 1, 0, "MFileIO");
    }
}
