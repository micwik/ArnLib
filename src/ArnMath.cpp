// Copyright (C) 2010-2022 Michael Wiklund.
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

#include "ArnInc/Math.hpp"


namespace Arn {

//// Helper special
template <typename T>
static inline T _modGeneric( T x, T y)
{
    T  remainder = x % y;
    if (((x >= 0) && (y >= 0)) || ((x <= 0) && (y <= 0)))
        return remainder;
    return remainder == 0 ? 0 : remainder + y;
}


template <typename T>
static inline int _log2Generic( T x)
{
    uint bits = sizeof(T) * 4;
    uint n = 0;
    while (x > 1) {
        if (x >> bits) {
            x >>= bits;
            n += bits;
        }
        bits >>= 1;
    }
    return int( n);
}


int  _mod_i( int x, int y)
{
    return _modGeneric( x, y);
}


qlonglong  _mod_ll( qlonglong x, qlonglong y)
{
    return _modGeneric( x, y);
}


int  _log2_u( uint x)
{
    if (x == 0)  return -1;

#ifdef __GNUC__
    return (sizeof(uint) * 8) - 1 - __builtin_clz( x);
#else
    return _log2Generic( x);
#endif
}


int  _log2_ull( qulonglong x)
{
    if (x == 0)  return -1;

#ifdef __GNUC__
    return (sizeof(qulonglong) * 8) - 1 - __builtin_clzll( x);
#else
    return _log2Generic( x);
#endif
}

}
