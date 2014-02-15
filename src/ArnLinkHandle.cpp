// Copyright (C) 2010-2013 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt 4 and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these other
// libraries is subject to their respective license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE.LGPL included in the packaging of this file.
// In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General
// Public License version 3.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and
// conditions contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//

#include "ArnInc/ArnLinkHandle.hpp"
#include <QDebug>


void ArnLinkHandle::init()
{
    _codes = Normal;
    _data  = 0;
}


ArnLinkHandle::ArnLinkHandle()
{
    init();
}


ArnLinkHandle::ArnLinkHandle( const ArnLinkHandle& other)
{
    _codes = other._codes;
    _flags = other._flags;
    if (other._data)
        _data = new HandleData( *other._data);
    else
        _data = 0;
}


ArnLinkHandle::ArnLinkHandle( const ArnLinkHandle::Flags& flags)
{
    init();
    _flags = flags;
}


ArnLinkHandle::~ArnLinkHandle()
{
    if (_data)
        delete _data;
}


ArnLinkHandle&  ArnLinkHandle::add( Code code, const QVariant& value)
{
    if (code == Normal)  return *this;

    if (!_data)
        _data = new HandleData;
    _data->insert( code, value);
    _codes |= code;
    return *this;
}


bool  ArnLinkHandle::has( Code code)  const
{
    return _codes.testFlag( code);
}


bool  ArnLinkHandle::isNull()  const
{
    return (_codes == Normal) && (_flags == Flags());
}


const QVariant&  ArnLinkHandle::valueRef( Code code)  const
{
    static QVariant  nullValue;

    if (!_data || !has( code))  // Should not be used ...
        return nullValue;

    return _data->constFind( code).value();
}
