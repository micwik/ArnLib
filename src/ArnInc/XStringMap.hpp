// Copyright (C) 2010-2013 Michael Wiklund.
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

#ifndef XSTRINGMAP_HPP
#define XSTRINGMAP_HPP

#include "ArnLib_global.hpp"
#include <QVector>
#include <QByteArray>
#include <QStringList>
#include <QVariant>


namespace Arn {

//! Container class with string representation for serialized data.
/*!
The primary usage is for creating and parsing serialized data.
it's optimized for giving an easy readable representation which never contains
char codes below 32 (space).

This class can store data with a key like QMaps. There is a guarantied order of
storing, i.e. its not sorted like QMaps.

The stored data can be ascii as well as binary.

Following mapping is done when serialized to the XString.
Special codes below 32: code 0 -> "\0",  code 10 -> "\n", code 13 -> "\r"
General codes below 32: code 1 -> "^A", code 2 -> "^B" and so on to code 31
code 32 (space) -> "_", "_" -> "\_", "^" -> "\^", "\" -> "\\"

The XString can be imported to the XStringMap. To get back stored values,
XStringMap is Queried with the keys or by index.

\code
    Arn::XStringMap xsm;
    xsm.add("", "put");
    xsm.add("id", "level");
    xsm.add("val", QByteArray::number(12));
    qDebug() << "XString: " << xsm.toXString();
\endcode
This will print "XString: put id=level val=12"
*/
class ARNLIBSHARED_EXPORT XStringMap
{
public:
    explicit  XStringMap();
    explicit  XStringMap( const QByteArray& xString);
    explicit  XStringMap( const QVariantMap& variantMap);
    ~XStringMap();

    int  size()  const { return _size; }
    void  clear( bool freeMem = false);
    void  squeeze();

    int  indexOf( const char* key, int from = 0)  const;
    int  indexOf( const QByteArray& key, int from = 0)  const;
    int  indexOf( const QString& key, int from = 0)  const;
    int  indexOfValue( const QByteArray& value, int from = 0)  const;
    int  indexOfValue( const QString& value, int from = 0)  const;
    int  maxEnumOf( const char* keyPrefix)  const;

    XStringMap&  add( const char* key, const QByteArray& val);
    XStringMap&  add( const char* key, const char* val);
    XStringMap&  add( const char* keyPrefix, uint eNum, const QByteArray& val);
    XStringMap&  add( const QByteArray& key, const QByteArray& val);
    XStringMap&  add( const char* key, const QString& val);
    XStringMap&  add( const char* keyPrefix, uint eNum, const QString& val);
    XStringMap&  add( const QByteArray& key, const QString& val);
    XStringMap&  add( const QString& key, const QString& val);
    XStringMap&  add( const XStringMap& other);
    XStringMap&  add( const QVariantMap& variantMap);

    void  set( int i, const QByteArray& val);
    void  set( const char* key, const QByteArray& val);
    void  set( const char* key, const char* val);
    void  set( const QByteArray& key, const QByteArray& val);
    void  set( const char* key, const QString& val);
    void  set( const QByteArray& key, const QString& val);
    void  set( const QString& key, const QString& val);

    const QByteArray&  keyRef( int i)  const;
    QByteArray  key( int i, const char* def = 0)  const;
    QByteArray  key( const QByteArray& value, const char* def = 0)  const;
    QByteArray  key( const QString& value, const char* def = 0)  const;
    QString  keyString( int i, const QString& def = QString())  const;
    QString  keyString( const QString& value, const QString& def = QString())  const;

    const QByteArray&  valueRef( int i)  const;
    QByteArray  value( int i, const char* def = 0)  const;
    QByteArray  value( const char* key, const char* def = 0)  const;
    QByteArray  value( const char* keyPrefix, uint eNum, const char* def = 0)  const;
    QByteArray  value( const QByteArray& key, const char* def = 0)  const;
    QByteArray  value( const QByteArray& key, const QByteArray& def)  const;
    QString  valueString( int i, const QString& def = QString())  const;
    QString  valueString( const char* key, const QString& def = QString())  const;
    QString  valueString( const char* keyPrefix, uint eNum, const QString& def = QString())  const;
    QString  valueString( const QByteArray& key, const QString& def = QString())  const;
    QString  valueString( const QString& key, const QString& def = QString())  const;

    void  remove( int index);
    void  remove( const char* key);
    void  remove( const QByteArray& key);
    void  remove( const QString& key);

    QByteArray  toXString()  const;
    bool  fromXString( const QByteArray& inXString, int size=-1);

    void  setEmptyKeysToValue();
    QStringList  keys()  const;
    QStringList  values()  const;
    QVariantMap  toVariantMap()  const;

    static void  stringCode( QByteArray& dst, const QByteArray& src);
    static void  stringDecode( QByteArray& dst, const QByteArray& src);

    inline void  append( const char* key, const QByteArray& val)
    {add( key, val);}
    inline void  append( const char* key, const char* val)
    {add( key, val);}
    inline void  append( const char* keyPrefix, uint eNum, const QByteArray& val)
    {add( keyPrefix, eNum, val);}
    inline void  append( const QByteArray& key, const QByteArray& val)
    {add( key, val);}
    inline void  append( const char* key, const QString& val)
    {add( key, val);}
    inline void  append( const char* keyPrefix, uint eNum, const QString& val)
    {add( keyPrefix, eNum, val);}
    inline void  append( const QByteArray& key, const QString& val)
    {add( key, val);}
    inline void  append( const QString& key, const QString& val)
    {add( key, val);}
    inline void  append( const XStringMap& other)
    {add( other);}
    inline void  append( const QVariantMap& other)
    {add( other);}

    inline XStringMap&  operator+=( const XStringMap& other)
    {return add( other);}
    inline XStringMap&  operator+=( const QVariantMap& other)
    {return add( other);}

private:
    void  init();
    void  checkSpace();

    QVector<QByteArray>  _keyList;
    QVector<QByteArray>  _valList;
    int  _size;
    QByteArray  _nullValue;
};

void XStringMapTest();

}  // Arn::

#endif // XSTRINGMAP_HPP
