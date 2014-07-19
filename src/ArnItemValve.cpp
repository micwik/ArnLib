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

#include "ArnInc/ArnItemValve.hpp"


ArnItemValve::ArnItemValve( QObject *parent)
    : ArnItemB( parent)
{
    _switchValue = true;
    _targetItem  = 0;
}


bool  ArnItemValve::setTarget( ArnItemB* targetItem, ArnItemValve::SwitchMode mode)
{
    _targetItem = targetItem;
    _switchMode = mode;
    doControl();

    return true;
}


ArnItemValve::SwitchMode  ArnItemValve::switchMode()  const
{
    return _switchMode;
}


bool  ArnItemValve::toBool()  const
{
    if (isOpen())
        return ArnItemB::toBool();
    else
        return _switchValue;
}


ArnItemValve&  ArnItemValve::operator=( bool value)
{
    setValue( value);
    return *this;
}


void  ArnItemValve::setValue( bool value)
{
    if (isOpen())
        ArnItemB::setValue( value, Arn::SameValue::Ignore);
    else if (value != _switchValue) {
        _switchValue = value;
        doControl();
        emit changed( _switchValue);
    }
}


void  ArnItemValve::itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value)
{
    ArnItemB::itemUpdated( handleData, value);

    if (value)
        _switchValue = (value->toInt() != 0);
    else
        _switchValue = ArnItemB::toBool();

    doControl();
    emit changed( _switchValue);
}


void  ArnItemValve::doControl()
{
    if (!_targetItem)  return;  // No target to control

    if (_switchMode.is( SwitchMode::InStream))
        (_targetItem->*&ArnItemValve::setEnableUpdNotify)( _switchValue);  // Control target changed() signal
    if (_switchMode.is( SwitchMode::OutStream))
        (_targetItem->*&ArnItemValve::setEnableSetValue)( _switchValue);  // Control target assign value
}
