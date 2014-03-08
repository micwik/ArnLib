// Copyright (C) 2010-2014 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt 4 and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these other
// libraries is subject to their respective license agreements.
//
// The MIT License (MIT)
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of the software in this file (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and
// conditions contained in a signed written agreement between you and Michael Wiklund.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
// THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef MQFLAGS_HPP
#define MQFLAGS_HPP

#include <QFlags>

/// Flags
#define MQ_DECLARE_FLAGS( FEStruct) \
    Q_DECLARE_FLAGS(F, E) \
    F  f; \
    inline FEStruct(F v_ = F(0)) : f( v_)  {} \
    inline FEStruct(E e_) : f( e_)  {} \
    inline static E  flagIf( bool test, E e)  {return test ? e : E(0);} \
    inline bool  is(E e)  const {return f.testFlag(e);} \
    inline bool  isAny(E e)  const {return ((f & e) != 0) && (e != 0 || f == 0 );} \
    inline FEStruct&  set(E e, bool v_ = true)  {f = v_ ? (f | e) : (f & ~e); return *this;} \
    inline static FEStruct  fromInt( int v_)  {return FEStruct( F( v_));} \
    inline int  toInt()  const {return f;} \
    inline operator int()  const {return f;} \
    inline bool  operator!()  const {return !f;}

//inline bool testFlag(Enum f) const { return (i & f) == f && (f != 0 || i == 0 ); }

#define MQ_DECLARE_OPERATORS_FOR_FLAGS( FEStruct) \
    Q_DECLARE_OPERATORS_FOR_FLAGS( FEStruct::F)


/// Enums
#define MQ_DECLARE_ENUM( EStruct) \
    E  e; \
    inline EStruct(E v_ = E(0)) : e( v_)  {} \
    inline static EStruct  fromInt( int v_)  {return EStruct( E( v_));} \
    inline int  toInt()  const {return e;} \
    inline operator int()  const {return e;} \
    inline bool  operator!()  const {return !e;}


#endif // MQFLAGS_HPP
