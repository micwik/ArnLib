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

#ifndef ARN_XSTRINGMAP_HPP
#define ARN_XSTRINGMAP_HPP

#include "ArnLib_global.hpp"
#include "MQFlagsBase.hpp"
#include <QVector>
#include <QByteArray>
#include <QStringList>
#include <QVariant>

#define ARNXSTRINGMAP_VER   "3.0"

typedef QMultiMap<QString,QVariant> MQVariantMap;

namespace Arn {

class XStringMapOptions {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        None       = 0x00,
        NullTilde  = 0x01,
        RepeatLen  = 0x02,
        //! Convenience
        Supported  = 0x07
    };
    MQ_DECLARE_FLAGS( XStringMapOptions)
};
MQ_DECLARE_OPERATORS_FOR_FLAGS( XStringMapOptions)


//! Container class with string representation for serialized data.
/*!
The primary usage is for creating and parsing serialized data.
it's optimized for giving an easy readable representation which never contains
char codes below 32 (space).

This class can store data with a key like QMaps. There is a guarantied order of
storing, i.e. its not sorted like QMaps.

The stored data can be ascii as well as binary.

Following mapping is done when serialized to the XString:
> Special codes below 32: code 0 -> "\0",   code 10 -> "\n",  code 13 -> "\r"    <Br>
> General codes below 32: code 1 -> "^A",  code 2 -> "^B"  and so on to code 31  <Br>
> code 32 (space) -> "_",  "_" -> "\\_",  "^" -> "\^",  "\" -> "\\"              <Br>

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
    typedef XStringMapOptions  Options;

    XStringMap();
    /// Make shallow copy (Qt style)
    XStringMap( const XStringMap& other);
    explicit  XStringMap( const QByteArray& xString);
    explicit  XStringMap( const QVariantMap& variantMap);
    ~XStringMap();
    /// Make shallow copy (Qt style)
    XStringMap&  operator=( const XStringMap& other);

    int  size()  const { return _size; }
    void  clear( bool freeMem = false);
    void  squeeze();
    const Options&  options()  const;
    void  setOptions( const Options& newOptions);

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

    XStringMap&  addNum( const char* key, int val);
    XStringMap&  addNum( const QByteArray& key, int val);
    XStringMap&  addNum( const QString& key, int val);
    XStringMap&  addNum( const char* key, uint val);
    XStringMap&  addNum( const QByteArray& key, uint val);
    XStringMap&  addNum( const QString& key, uint val);
    XStringMap&  addNum( const char* key, double val, int precision = -1);
    XStringMap&  addNum( const QByteArray& key, double val, int precision = -1);
    XStringMap&  addNum( const QString& key, double val, int precision = -1);

    XStringMap&  addValues( const QStringList& stringList);

    XStringMap&  set( int i, const QByteArray& val);
    XStringMap&  set( int i, const QString& val);
    XStringMap&  set( const char* key, const QByteArray& val);
    XStringMap&  set( const char* key, const char* val);
    XStringMap&  set( const QByteArray& key, const QByteArray& val);
    XStringMap&  set( const char* key, const QString& val);
    XStringMap&  set( const QByteArray& key, const QString& val);
    XStringMap&  set( const QString& key, const QString& val);

    XStringMap&  setKey( int i, const QByteArray& key);

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

    XStringMap&  remove( int index);
    XStringMap&  remove( const char* key);
    XStringMap&  remove( const QByteArray& key);
    XStringMap&  remove( const QString& key);
    XStringMap&  removeValue( const QByteArray& val);
    XStringMap&  removeValue( const QString& val);

    QByteArray  toXString()  const;
    QString  toXStringString()  const;
    bool  fromXString( const QByteArray& inXString, int size=-1);
    bool  fromXString( const QString& inXString);

    void  setEmptyKeysToValue();
    void  reverseOrder();
    QStringList  keys()  const;
    QStringList  values( const char* keyPrefix = 0)  const;
    MQVariantMap  toVariantMap( bool useStringVal)  const;

    void  stringCode( QByteArray& dst, const QByteArray& src)  const;
    void  stringDecode( QByteArray& dst, const QByteArray& src)  const;

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

    XStringMap&  operator+=( const XStringMap& other);
    XStringMap&  operator+=( const QVariantMap& other);

    QByteArray  info();

private:
    void  init();
    void  checkSpace();

    QVector<QByteArray>  _keyList;
    QVector<QByteArray>  _valList;
    int  _size;
    Options _options;
    static QByteArray  _nullValue;
};

}  // Arn::

#endif // ARN_XSTRINGMAP_HPP
