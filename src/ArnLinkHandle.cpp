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

#include "ArnInc/ArnLinkHandle.hpp"
#include <QDebug>


//// WARNING !!!  This class can have a this-pointer that is zero (0), for optimization.

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
    if (&other == 0) {  // other is NULL
        init();
        return;
    }

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
    if (this == 0)  return;  // Shouldn't be called ...

    if (_data)
        delete _data;
}


const ArnLinkHandle&  ArnLinkHandle::null()
{
    return *static_cast<ArnLinkHandle*>(0);
}


const ArnLinkHandle::Flags&  ArnLinkHandle::flags()  const
{
    static Flags  nullFlags;
    if (this == 0)  return nullFlags;  // ArnHandle is NULL

    return _flags;
}


ArnLinkHandle::Flags&  ArnLinkHandle::flags()
{
    static Flags  nullFlags;
    if (this == 0)  return nullFlags;  // ArnHandle is NULL, should not be used ...

    return _flags;
}


ArnLinkHandle&  ArnLinkHandle::add( Code code, const QVariant& value)
{
    if (this == 0)  return *this;  // ArnHandle is NULL
    if (code == Normal)  return *this;  // Adding nothing

    if (!_data)
        _data = new HandleData;
    _data->insert( code, value);
    _codes |= code;
    return *this;
}


bool  ArnLinkHandle::has( Code code)  const
{
    if (this == 0)  return false;  // ArnHandle is NULL

    return _codes.testFlag( code);
}


bool  ArnLinkHandle::isNull()  const
{
    if (this == 0)  return true;  // ArnHandle is NULL

    return (_codes == Normal) && (_flags == Flags());
}


const QVariant&  ArnLinkHandle::valueRef( Code code)  const
{
    static QVariant  nullValue;

    if (this == 0)  return nullValue;  // ArnHandle is NULL

    if (!_data || !has( code))  // Should not be used ...
        return nullValue;

    return _data->constFind( code).value();
}
