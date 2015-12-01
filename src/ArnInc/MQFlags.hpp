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

#ifndef MQFLAGS_HPP
#define MQFLAGS_HPP

#include <QFlags>
#include <QMap>
#include <QStringList>

namespace Arn {
class XStringMap;
}


class QMetaObject;

namespace Arn {

bool  isPower2( uint x);

class MQFTxt
{
public:
    MQFTxt( const QMetaObject& metaObj, bool isFlag);

    void  setTxtRef( const char* txt, int enumVal, quint16 nameSpace = 0);
    void  setTxt( const char* txt, int enumVal, quint16 nameSpace = 0);
    const char*  getTxt( int enumVal, quint16 nameSpace = 0)  const;
    void  setTxtString( const QString& txt, int enumVal, quint16 nameSpace = 0);
    QString  getTxtString( int enumVal, quint16 nameSpace = 0)  const;
    int  getEnumVal( const char* txt, int defaultVal = 0, quint16 nameSpace = 0);
    int  getEnumVal( const QString& txt, int defaultVal = 0, quint16 nameSpace = 0);

    void  addBitSet( Arn::XStringMap& xsm, quint16 nameSpace = 0);
    QString  getBitSet( quint16 nameSpace = 0);
    QString  flagsToString( int val, quint16 nameSpace = 0);
    QStringList  flagsToStringList( int val, quint16 nameSpace = 0);
    int  flagsFromString( const QString& flagString, quint16 nameSpace = 0);
    int  flagsFromStringList( const QStringList& flagStrings, quint16 nameSpace = 0);

    void  addEnumSet( Arn::XStringMap& xsm, quint16 nameSpace = 0);
    QString  getEnumSet( quint16 nameSpace = 0);

private:
    struct EnumTxtKey {
        uint  _enumVal;
        quint16  _nameSpace;
        bool  _isFlag;

        EnumTxtKey( uint enumVal, quint16 nameSpace, bool isFlag)
        {_enumVal = enumVal;  _nameSpace = nameSpace;  _isFlag = isFlag;}
        bool  operator <( const EnumTxtKey& other)  const;
    };

    void  setupFromMetaObject();

    const QMetaObject&  _metaObj;
    QMap<EnumTxtKey,const char*>  _enumTxtTab;
    QList<QByteArray>*  _txtStore;
    bool  _isFlag;
};

}


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

#define MQ_DECLARE_FLAGSTXT( FEStruct) \
    MQ_DECLARE_FLAGS( FEStruct) \
    static Arn::MQFTxt&  txt()  {static Arn::MQFTxt in( staticMetaObject, true); return in;} \
    inline QString  toString( quint16 nameSpace = 0)  const {return txt().flagsToString( f, nameSpace);} \
    inline static FEStruct  fromString( const QString& text, quint16 nameSpace = 0) \
      {return FEStruct( F( txt().flagsFromString( text, nameSpace)));}


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

#define MQ_DECLARE_ENUMTXT( EStruct) \
    MQ_DECLARE_ENUM( EStruct) \
    static Arn::MQFTxt&  txt()  {static Arn::MQFTxt in( staticMetaObject, false); return in;} \
    inline QString  toString( quint16 nameSpace = 0)  const {return txt().getTxtString( e, nameSpace);} \
    inline static EStruct  fromString( const QString& text, quint16 nameSpace = 0) \
      {return EStruct( E( txt().getEnumVal( text, 0, nameSpace)));}


#endif // MQFLAGS_HPP
