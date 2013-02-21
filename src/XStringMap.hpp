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

#ifndef XSTRINGMAP_HPP
#define XSTRINGMAP_HPP

#include "ArnLib_global.hpp"
#include <QObject>
#include <QVector>
#include <QByteArray>
#include <QStringList>

class ARNLIBSHARED_EXPORT XStringMap : public QObject
{
Q_OBJECT
public:
    explicit  XStringMap( QObject* parent = 0);
    explicit  XStringMap( const QByteArray& xString, QObject* parent = 0);
    ~XStringMap();

    int  size()  const { return _size; }
    void  clear();
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

private:
    void  init();
    void  checkSpace();

    QVector<QByteArray>  _keyList;
    QVector<QByteArray>  _valList;
    int  _size;
    QByteArray  _nullValue;
};


void XStringMapTest();

#endif // XSTRINGMAP_HPP
