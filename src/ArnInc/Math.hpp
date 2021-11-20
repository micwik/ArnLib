// Copyright (C) 2010-2021 Michael Wiklund.
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

#ifndef ARN_MATH_H
#define ARN_MATH_H

#include <QtGlobal>


namespace Arn
{
    int  _mod_i( int x, int y);
    qlonglong  _mod_ll( qlonglong x, qlonglong y);
    int  _log2_u( uint x);
    int  _log2_ull( qulonglong x);

    template <typename T>
    inline T mod( T x, T y)
    {
        return (sizeof(T) == sizeof(qlonglong)) ? T( _mod_ll( qlonglong( x), qlonglong( y)))
                                                : T( _mod_i( int( x), int( y)));
    }

    template <typename T>
    T  circVal( T x, T lo, T hi)
    {
        return mod( x - lo, hi - lo) + lo;
    }

    template <typename T>
    bool isPower2( T x)
    {
        return x && ((x & (x - 1)) == 0);
    }

    template <typename T>
    inline int log2( T x)
    {
        return (sizeof(T) == sizeof(qulonglong)) ? _log2_ull( qulonglong( x)) : _log2_u( uint( x));
    }

    template <typename T>
    T minLim( const T& x, const T& lim)
    {
        return x < lim ? lim : x;
    }

    template <typename T>
    T maxLim( const T& x, const T& lim)
    {
        return x > lim ? lim : x;
    }

    template <typename T>
    T rangeLim( const T& x, const T& min, const T& max)
    {
        return x < min ? min : (x > max ? max : x);
    }
}

#endif // ARN_MATH_H
