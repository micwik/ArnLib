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

#ifndef MQFLAGS_HPP
#define MQFLAGS_HPP

#include "ArnLib_global.hpp"
#include <QFlags>
#include <QMap>
#include <QStringList>
#include <QObject>
#include <QDebug>

namespace Arn {
class XStringMap;
class EnumTxt;
}

struct QMetaObject;


namespace Arn {

typedef struct {
    int  ns;
    int  enumVal;
    const char*  enumTxt;
} _InitEnumTxt;

typedef struct {
    EnumTxt*  enumTxtClass;
    uint  mask;
    uint  factor;
} _InitSubEnum;


//! Class Enum text.
/*!
<b>Example usage</b> \n \code
class AllowClassT {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        None      = 0x00,
        Read      = 0x01,
        Create    = 0x04,
        Delete    = 0x08,
        //! Convenience, allow all
        All       = 0xff
    };
    MQ_DECLARE_FLAGSTXT( AllowClassT)

    enum NS {NsEnum, NsHuman};

    MQ_DECLARE_ENUM_NSTXT(
        { NsHuman, Read,   "Allow Read" },
        { NsHuman, Delete, "Allow Delete" }
    )
};
MQ_DECLARE_OPERATORS_FOR_FLAGS( AllowClassT)

class ConnectStatT {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        Init = 0,
        Connected,
        Error,
        Disconnected,
        TriedAll
    };
    MQ_DECLARE_ENUMTXT( ConnectStatT)

    enum NS {NsEnum, NsHuman};
    MQ_DECLARE_ENUM_NSTXT(
        { NsHuman, Init,     "Initialized" },
        { NsHuman, Error,    "Connect error" },
        { NsHuman, TriedAll, "Tried all" },
        { NsHuman, MQ_NSTXT_FILL_MISSING_FROM( NsEnum) }
    )
};
\endcode
*/
class EnumTxt
{
public:
    EnumTxt( const QMetaObject& metaObj, bool isFlag, const _InitEnumTxt* initTxt, const _InitSubEnum* initSubEnum,
             const char* name);

    void  setTxtRef( const char* txt, int enumVal, quint16 nameSpace);

    //! Set an additional text for an enum val in a namespace
    /*! The namespace with index 0 is the standard namespace that automatically
     *  gets its texts from the definition of the enum.
     *
     *  <b>Example usage</b> \n \code
     *  AllowClassT  allow;
     *  allow.txt().setTxt("Test - Create", allow.Create, AllowClassT::NsHuman);
     *  allow = allow.Create;
     *  qDebug() << allow.toString() << allow.toString( AllowClassT::NsHuman)
     *  \endcode
     *  \param[in] txt is the new enum text.
     *  \param[in] enumVal is the referenced value.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \see getTxt();
     */
    void  setTxt( const char* txt, int enumVal, quint16 nameSpace);

    //! Returns the text for a enum value in a namespace.
    /*! \param[in] enumVal is the referenced value.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \return the enum text.
     *  \see setTxt();
     */
    const char*  getTxt( int enumVal, quint16 nameSpace = 0)  const;

    //! Set an additional text for an enum val in a namespace
    /*! \param[in] txt is the new enum text.
     *  \param[in] enumVal is the referenced value.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \see setTxt();
     *  \see getTxtString();
     */
    void  setTxtString( const QString& txt, int enumVal, quint16 nameSpace);

    //! Returns the text for a enum value in a namespace.
    /*! \param[in] enumVal is the referenced value.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \return the enum text.
     *  \see setTxt();
     *  \see setTxtString();
     */
    QString  getTxtString( int enumVal, quint16 nameSpace = 0)  const;

    //! Returns the enum value for a text in a namespace.
    /*! \param[in] txt is the enum text.
     *  \param[in] defaultVal is the returned value when txt is not found.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[out] isFound returns status when pointer is none null.
     *  \return the enum value.
     *  \see setTxt();
     */
    int  getEnumVal( const char* txt, int defaultVal = 0, quint16 nameSpace = 0, bool* isFound = arnNullptr)  const;

    //! Returns the enum value for a text in a namespace.
    /*! \param[in] txt is the enum text.
     *  \param[in] defaultVal is the returned value when txt is not found.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[out] isFound returns status when pointer is none null.
     *  \return the enum value.
     *  \see setTxt();
     *  \see setTxtString();
     */
    int  getEnumVal( const QString& txt, int defaultVal = 0, quint16 nameSpace = 0, bool* isFound = arnNullptr)  const;

    //! Returns the enum value and mask for a subEnum text in a namespace.
    /*! \param[in] txt is the subEnum text.
     *  \param[out] subEnumVal is the returned value when txt is found as a subEnum.
     *  \param[out] bitMask is the returned value when txt is found as a subEnum.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \retval is true when txt is found as a subEnum.
     *  \see setTxt();
     *  \see setTxtString();
     */
    bool  getSubEnumVal( const char* txt, int& subEnumVal, uint& bitMask, quint16 nameSpace = 0)  const;

    //! Returns the enum value and mask for a subEnum text in a namespace.
    /*! \param[in] txt is the subEnum text.
     *  \param[out] subEnumVal is the returned value when txt is found as a subEnum.
     *  \param[out] bitMask is the returned value when txt is found as a subEnum.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \retval is true when txt is found as a subEnum.
     *  \see setTxt();
     *  \see setTxtString();
     */
    bool  getSubEnumVal( const QString& txt, int& subEnumVal, uint& bitMask, quint16 nameSpace = 0)  const;

    //! Adds bit set for enum flags to a XStringMap
    /*! <b>Example</b> \n \code
     *  Arn::XStringMap  xsm;
     *  xsm.add("T", "Test");
     *  AllowClassT::txt().addBitSet( xsm);
     *  \endcode
     *  wiil give xsm containing: T=Test B0=Read B2=Create B3=Delete
     *  \param[out] xsm is the XStringMap to be added to.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \see humanize()
     */
    void  addBitSet( Arn::XStringMap& xsm, quint16 nameSpace = 0, bool neverHumanize = false)  const;

    //! returns the bit set string for enum flags
    /*! Example
     *  > qDebug() << AllowClassT::txt().getBitSet();
     *  wiil print: "B0=Read B2=Create B3=Delete"
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \return the bit set string.
     *  \see humanize()
     */
    QString  getBitSet( quint16 nameSpace = 0, bool neverHumanize = false)  const;

    //! returns text string for enum flags
    /*! <b>Example</b> \n \code
     *  AllowClassT  allow;
     *  allow = allow.Create | allow.Delete;
     *  qDebug() << AllowClassT::txt().flagsToString( allow);
     *  \endcode
     *  wiil print: "Create | Delete"
     *  \param[in] val is the flags enum value.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \return the flags text string.
     */
    QString  flagsToString( int val, quint16 nameSpace = 0)  const;

    //! returns string list for enum flags
    /*! <b>Example</b> \n \code
     *  AllowClassT  allow;
     *  allow = allow.Create | allow.Delete;
     *  QStringList  allowList = AllowClassT::txt().flagsToStringList( allow);
     *  \endcode
     *  wiil give allowList containing: "Create", "Delete"
     *  \param[in] val is the flags enum value.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \return the flags string list.
     */
    QStringList  flagsToStringList( int val, quint16 nameSpace = 0)  const;

    //! returns enum flags from string
    /*! <b>Example</b> \n \code
     *  QString flagString = "Create | Delete";
     *  int val = AllowClassT::txt().flagsFromString( flagString);
     *  \endcode
     *  wiil give val containing: 0xc (0x4 + 0x8)
     *  \param[in] flagString is the flags text.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \return the flags enum value.
     */
    int  flagsFromString( const QString& flagString, quint16 nameSpace = 0);

    //! returns enum flags from string list
    /*! <b>Example</b> \n \code
     *  QStringList flagStrings;
     *  flagStrings << "Create" << "Delete";
     *  int val = AllowClassT::txt().flagsFromString( flagStrings);
     *  \endcode
     *  wiil give val containing: 0xc (0x4 + 0x8)
     *  \param[in] flagStrings is the flags text list.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \return the flags enum value.
     */
    int  flagsFromStringList( const QStringList& flagStrings, quint16 nameSpace = 0);

    //! Adds enum set to a XStringMap
    /*! <b>Example</b> \n \code
     *  Arn::XStringMap  xsm;
     *  xsm.add("T", "Test");
     *  ConnectStatT::txt().addEnumSet( xsm);
     *  \endcode
     *  wiil give xsm containing: T=Test 0=Init 1=Connected 2=Error 3=Disconnected 4=Tried all
     *  \param[out] xsm is the XStringMap to be added to.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \see humanize()
     */
    void  addEnumSet( Arn::XStringMap& xsm, quint16 nameSpace = 0, bool neverHumanize = false)  const;

    //! returns the enum set string
    /*! Example
     *  > qDebug() << ConnectStatT::txt().getEnumSet();
     *  wiil print: "0=Init 1=Connected 2=Error 3=Disconnected 4=Tried_all"
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \return the enum set string.
     *  \see humanize()
     */
    QString  getEnumSet( quint16 nameSpace = 0, bool neverHumanize = false)  const;

    //! Adds an other EnumTxt as a subEnum to this flag EnumTxt
    /*! <b>Example</b> \n \code
     *  MyFlags::txt().addSubEnum( ConnectStatT::txt(), MyFlags::ConnStat, MyFlags::ConnStatB0);
     *  \endcode
     *  \param[in] subEnum is the other EnumTxt to be included as a subEnum.
     *  \param[in] bitMask is used for selecting the subEnum among the flags.
     *  \param[in] factor is multiplying the base enum before it is masked in as subEnum to flags.
     *  \return the flags enum value.
     */
    void  addSubEnum( const EnumTxt& subEnum, uint bitMask, uint factor);

    //! returns the name of the enum (class)
    /*! Example
     *  > qDebug() << ConnectStatT::txt().name();
     *  wiil print: "ConnectStatT"
     *  \return the enum (class) name.
     */
    const char*  name()  const;

    //! returns number of enumerators in the enum (class)
    /*! Example
     *  > qDebug() << ConnectStatT::txt().enumCount();
     *  wiil print: 5
     *  \return the count of enumerators.
     */
    int  enumCount()  const;

    //! Copies missing enum texts from one namespace to another
    /*! The standard 0 namespace contains all enum texts as defined and can not be altered.
     *  All the other wanted namespaces can have customized enum texts, but then there
     *  can be enum values without a text in such namespace.
     *  This function can be used to fill in those missing texts from another namespace,
     *  which typically is 0 as it contains all texts.
     *  \param[in] toNameSpace is the altered one. Can not be 0.
     *  \param[in] fromNameSpace is the one to copy from.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \see humanize()
     */
    void  setMissingTxt( quint16 toNameSpace, quint16 fromNameSpace = 0, bool neverHumanize = false);

    //! returns the humanized text
    /*! The input text can be Chamel-case or '_' word separeted.
     *  First output char will always be upper case and the following chars will always
     *  be lower case.
     *
     *  <b>Example output</b> \n \code
     *  "MySimpelCase" ==> "My simpel case"
     *  "My_Simpel_case" ==> "My simpel case"
     *  "count123ms" ==> "Count 123 ms"
     *  "DDTIsBad" ==> "DDT is bad"
     *  \endcode
     *  \param[in] txt is the text to be humanized.
     *  \return the humanized text.
     */
    static QString  humanize( const QString& txt);

    //! Returns true if this is a flag usage.
    /*! \retval is true when this is a flag, false when ordinary enum.
     */
    bool isFlag() const;

private:
    struct EnumTxtKey {
        uint  _enumVal;
        quint16  _nameSpace;
        bool  _isFlag;
        bool  _isSingleBit;  // Only for flags

        EnumTxtKey( uint enumVal, quint16 nameSpace, bool isFlag);
        bool  operator <( const EnumTxtKey& other)  const;
    };
    struct SubEnumEntry {
        const EnumTxt* _subEnum;
        uint  _bitMask;
        uchar  _bitPos;
        SubEnumEntry( const EnumTxt& subEnum, uint bitMask, uint factor);
    };

    void  setTxtRefAny( const char* txt, int enumVal, quint16 nameSpace);
    void  setupFromMetaObject();
    void  setupTxt( const _InitEnumTxt* initTxt);
    void  setupSubEnum( const _InitSubEnum* initSubEnum);
    static QByteArray numStr( uint num);

    const QMetaObject&  _metaObj;
    QMap<EnumTxtKey,const char*>  _enumTxtTab;
    QList<QByteArray>*  _txtStore;
    QList<SubEnumEntry>*  _subEnumTab;
    const char*  _name;
    uint _subEnumMask;
    bool  _isFlag;
};

}


#define MQ_NSTXT_FILL_MISSING   0, 0
#define MQ_NSTXT_FILL_MISSING_FROM( FromNs)   FromNs, 0


/// Flags
#define MQ_DECLARE_FLAGS( FEStruct) \
    Q_DECLARE_FLAGS(F, E) \
    F  f; \
    inline FEStruct(F v_ = F(QFlag(0))) : f( v_)  {setup(0);} \
    inline FEStruct(E e_) : f( e_)  {setup(0);} \
    inline static E  flagIf( bool test, E e)  {return test ? e : E(0);} \
    inline bool  is(E e)  const {return f.testFlag(e);} \
    inline bool  isAny(E e)  const {return ((f & e) != 0) && (e != 0 || f == 0 );} \
    inline FEStruct&  set(E e, bool v_ = true)  {f = v_ ? (f | e) : (f & ~e); return *this;} \
    inline void  setBits(E e, int v_)  {f = (f & ~e) | E(v_);} \
    inline static FEStruct  fromInt( int v_)  {return FEStruct( F( v_));} \
    inline int  toInt()  const {return f;} \
    inline operator int()  const {return f;} \
    inline bool  operator!()  const {return !f;} \
    inline void setup( char* dummy) {Q_UNUSED(dummy)}

#define MQ_DECLARE_FLAGSTXT( FEStruct) \
    MQ_DECLARE_FLAGS( FEStruct) \
    static Arn::EnumTxt&  txt()  {static Arn::EnumTxt in( staticMetaObject, true, _setNs(0), _setSe(0), \
                                                          #FEStruct); return in;} \
    inline static const Arn::_InitEnumTxt* _setNs( const Arn::_InitEnumTxt* ieTxt)  {return ieTxt;} \
    inline static const Arn::_InitSubEnum* _setSe( const Arn::_InitSubEnum* isEnum) {return isEnum;} \
    inline static const char*  name()  {return txt().name();} \
    inline QString  toString( quint16 nameSpace = 0)  const {return txt().flagsToString( f, nameSpace);} \
    inline static FEStruct  fromString( const QString& text, quint16 nameSpace = 0) \
      {return FEStruct( F( txt().flagsFromString( text, nameSpace)));}


#define MQ_DECLARE_FLAGS_NSTXT(...) \
    static const Arn::_InitEnumTxt* _setNs(int dummy) { \
        Q_UNUSED(dummy) \
        static Arn::_InitEnumTxt  initTxt[] = { __VA_ARGS__ , { 0, 0, arnNullptr }}; \
        return initTxt; \
    };

#define MQ_DECLARE_SUBETXT(...) \
    static const Arn::_InitSubEnum* _setSe( int dummy) { \
        Q_UNUSED(dummy) \
        static Arn::_InitSubEnum  initSubEnum[] = { __VA_ARGS__ , { arnNullptr, 0, 0 }}; \
        return initSubEnum; \
    };

#define MQ_SUBETXT_ADD_RELDEF( EStruct, Mask, Factor) \
    { &EStruct::txt(), Mask, Factor}

#define MQ_SUBETXT_ADD_ABSDEF( EStruct, Mask) \
    MQ_SUBETXT_ADD_RELDEF( EStruct, Mask, 1)

#define MQ_SUBETXT_ADD_RELOP( EStruct, Mask, Factor) \
    inline void setSubEnum( EStruct::E v_) { \
        setBits( Mask, v_ * Factor); \
    }

#define MQ_SUBETXT_ADD_ABSOP( EStruct, Mask) \
    MQ_SUBETXT_ADD_RELOP( EStruct, Mask, 1) \

#define MQ_DECLARE_OPERATORS_FOR_FLAGS( FEStruct) \
    Q_DECLARE_OPERATORS_FOR_FLAGS( FEStruct::F)


/// Enums
#define MQ_DECLARE_ENUM( EStruct) \
    E  e; \
    inline EStruct(E v_ = E(0)) : e( v_)  {setup(0);} \
    inline static EStruct  fromInt( int v_)  {return EStruct( E( v_));} \
    inline int  toInt()  const {return e;} \
    inline operator int()  const {return e;} \
    inline bool  operator!()  const {return !e;} \
    inline void setup( char* dummy) {Q_UNUSED(dummy)}

#define MQ_DECLARE_ENUMTXT( EStruct) \
    MQ_DECLARE_ENUM( EStruct) \
    static Arn::EnumTxt&  txt()  {static Arn::EnumTxt in( staticMetaObject, false, _setNs(0), arnNullptr, \
                                                          #EStruct); return in;} \
    inline static const Arn::_InitEnumTxt* _setNs( const Arn::_InitEnumTxt* ieTxt) {return ieTxt;} \
    inline static const char*  name()  {return txt().name();} \
    inline QString  toString( quint16 nameSpace = 0)  const {return txt().getTxtString( e, nameSpace);} \
    inline static EStruct  fromString( const QString& text, quint16 nameSpace = 0) \
      {return EStruct( E( txt().getEnumVal( text, 0, nameSpace)));}


#define MQ_DECLARE_ENUM_NSTXT(...) \
    static const Arn::_InitEnumTxt* _setNs(int dummy) { \
        Q_UNUSED(dummy) \
        static Arn::_InitEnumTxt  initTxt[] = { __VA_ARGS__ , { 0, 0, 0 }}; \
        return initTxt; \
    };


#endif // MQFLAGS_HPP
