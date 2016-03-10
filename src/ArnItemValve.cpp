// Copyright (C) 2010-2016 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. Usage of these other libraries is subject to their respective
// license agreements.
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
#include "private/ArnItemValve_p.hpp"


ArnItemValvePrivate::ArnItemValvePrivate()
{
    _switchValue = true;
    _targetItem  = 0;
}


ArnItemValvePrivate::~ArnItemValvePrivate()
{
}


ArnItemValve::ArnItemValve( QObject *parent)
    : ArnItemB( *new ArnItemValvePrivate, parent)
{
}


ArnItemValve::ArnItemValve( ArnItemValvePrivate& dd, QObject* parent)
    : ArnItemB( dd, parent)
{
}


bool  ArnItemValve::setTarget( ArnItemB* targetItem, ArnItemValve::SwitchMode mode)
{
    Q_D(ArnItemValve);

    d->_targetItem = targetItem;
    d->_switchMode = mode;
    doControl();

    return true;
}


ArnItemValve::SwitchMode  ArnItemValve::switchMode()  const
{
    Q_D(const ArnItemValve);

    return d->_switchMode;
}


bool  ArnItemValve::toBool()  const
{
    Q_D(const ArnItemValve);

    if (isOpen())
        return ArnItemB::toBool();
    else
        return d->_switchValue;
}


ArnItemValve&  ArnItemValve::operator=( bool value)
{
    setValue( value);
    return *this;
}


void  ArnItemValve::setValue( bool value)
{
    Q_D(ArnItemValve);

    if (isOpen())
        ArnItemB::setValue( value, Arn::SameValue::Ignore);
    else if (value != d->_switchValue) {
        d->_switchValue = value;
        doControl();
        emit changed( d->_switchValue);
    }
}


void  ArnItemValve::itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value)
{
    Q_D(ArnItemValve);

    ArnItemB::itemUpdated( handleData, value);

    if (value)
        d->_switchValue = (value->toInt() != 0);
    else
        d->_switchValue = ArnItemB::toBool();

    doControl();
    emit changed( d->_switchValue);
}


void  ArnItemValve::doControl()
{
    Q_D(ArnItemValve);

    if (!d->_targetItem)  return;  // No target to control

    if (d->_switchMode.is( SwitchMode::InStream))
        (d->_targetItem->*&ArnItemValve::setEnableUpdNotify)( d->_switchValue);  // Control target changed() signal
    if (d->_switchMode.is( SwitchMode::OutStream))
        (d->_targetItem->*&ArnItemValve::setEnableSetValue)( d->_switchValue);  // Control target assign value
}
