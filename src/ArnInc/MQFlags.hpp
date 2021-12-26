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
    inline void  setup( char* dummy) {Q_UNUSED(dummy)}

#define MQ_DECLARE_FLAGSTXT( FEStruct) \
    MQ_DECLARE_FLAGS( FEStruct) \
    static Arn::EnumTxt&  txt()  {static Arn::EnumTxt in( &staticMetaObject, true, _setNs(0), _setSe(0), \
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
    inline void  setSubEnum( EStruct::E v_) { \
        setBits( Mask, v_ * Factor); \
    } \
    inline EStruct::E  getSubEnum_##EStruct() { \
        return EStruct::E( (f & Mask) / Factor); \
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
    static Arn::EnumTxt&  txt()  {static Arn::EnumTxt in( &staticMetaObject, false, _setNs(0), arnNullptr, \
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
class AllowLevelT {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        Low = 0,
        Mid,
        High
    };
    MQ_DECLARE_ENUMTXT( AllowLevelT)
};

class AllowClassT {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        None       = 0x00,
        Read       = 0x01,
        AllowLevB0 = 0x02,
        AllowLevB1 = 0x04,
        Create     = 0x08,
        Delete     = 0x10,
        /// SubEnums
        AllowLev   = AllowLevB0 | AllowLevB1,
        //! Convenience, allow all
        All        = 0xff
    };
    MQ_DECLARE_FLAGSTXT( AllowClassT)

    MQ_DECLARE_SUBETXT(
        MQ_SUBETXT_ADD_RELDEF( AllowLevelT, AllowLev, AllowLevB0)
    )
    MQ_SUBETXT_ADD_RELOP( AllowLevelT, AllowLev, AllowLevB0)

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
        { NsHuman, MQ_NSTXT_FILL_MISSING_FROM( NsEnum) }
    )
};
\endcode
*/
class EnumTxt
{
public:
    struct IncludeMode {
        enum E {
            OnlySingle1Bits,
            OnlyMulti1Bits,  // Also include number 0 (no 1 bits at all)
            OnlySubEnumBits,
            AnyButSubEnumBits,
            Any
        };
        MQ_DECLARE_ENUM( IncludeMode)
    };

    EnumTxt( const QMetaObject* metaObj, bool isFlag, const _InitEnumTxt* initTxt, const _InitSubEnum* initSubEnum,
             const char* name);

    //! Create a dynamic runtime handled EnumTxt
    /*! This is used for handling general Enums that is not statically assigned via QMetaObject.
     *  <b>Example usage</b> \n \code
     *  Arn::EnumTxt myFlags( true, "MyFlags");
     *  myFlags.loadBitSet( "B0=Flag1 B5=Flag2 0=None 0x21=FlagAll");
     *  \endcode
     *  \param[in] flag is true when using Flags (bitSet), otherwise use plain Enums.
     *  \param[in] name is the name of these Enums / Flags.
     */
    explicit EnumTxt( bool isFlag = false, const QString& name = QString());

    ~EnumTxt();

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
     *  wiil print: "Create" and "Test - Create"
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

    //! Returns the shifted enum value and the mask for a subEnum text in a namespace.
    /*! The enum value returned is shifted (with factor) to directly fit the flags enum.
     *  <b>Example usage</b> \n \code
     *  int  subEnumVal;
     *  uint  bitMask;
     *  AllowClassT::txt().getSubEnumVal( "Mid", subEnumVal, bitMask);
     *  qDebug() << subEnumVal << bitMask
     *  \endcode
     *  wiil print: 2 and 6
     *  \param[in] txt is the subEnum text.
     *  \param[out] subEnumVal is the returned shifted value when txt is found as a subEnum.
     *  \param[out] bitMask is the returned value when txt is found as a subEnum.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \retval is true when txt is found as a subEnum.
     *  \see setTxt();
     *  \see addSubEnum();
     */
    bool  getSubEnumVal( const char* txt, int& subEnumVal, uint& bitMask, quint16 nameSpace = 0)  const;

    //! Returns the shifted enum value and the mask for a subEnum text in a namespace.
    /*! The enum value returned is shifted (with factor) to directly fit the flags enum.
     *  <b>Example usage</b> \n \code
     *  int  subEnumVal;
     *  uint  bitMask;
     *  AllowClassT::txt().getSubEnumVal( "High", subEnumVal, bitMask);
     *  qDebug() << subEnumVal << bitMask
     *  \endcode
     *  wiil print: 4 and 6
     *  \param[in] txt is the subEnum text.
     *  \param[out] subEnumVal is the returned shifted value when txt is found as a subEnum.
     *  \param[out] bitMask is the returned value when txt is found as a subEnum.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \retval is true when txt is found as a subEnum.
     *  \see setTxtString();
     *  \see addSubEnum();
     */
    bool  getSubEnumVal( const QString& txt, int& subEnumVal, uint& bitMask, quint16 nameSpace = 0)  const;

    //! Adds enum flags to a XStringMap
    /*! <b>Example</b> \n \code
     *  Arn::XStringMap  xsm;
     *  xsm.add("T", "Test");
     *  AllowClassT::txt().addFlagsTo( xsm, IncludeMode::OnlySingle1Bits, 0, true);
     *  \endcode
     *  wiil give xsm containing:
     *  T=Test B0=Read B3=Create B4=Delete
     *  \param[out] xsm is the XStringMap to be added to.
     *  \param[in] incMode specifies what to include (SingleBits / MultiBits / SubEnumBits / "Any").
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \see addBitSetTo()
     *  \see humanize()
     */
    void  addFlagsTo( Arn::XStringMap& xsm, const IncludeMode& incMode, quint16 nameSpace = 0, bool neverHumanize = false)  const;

    //! Adds sub enum flags to a XStringMap
    /*! <b>Example</b> \n \code
     *  Arn::XStringMap  xsm;
     *  xsm.add("T", "Test");
     *  AllowClassT::txt().addSubEnumTo( xsm, 0, true);
     *  \endcode
     *  wiil give xsm containing:
     *  T=Test SE6:B1=AllowLev E0=Low E1=Mid E2=High
     *  \param[out] xsm is the XStringMap to be added to.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \see addBitSetTo()
     *  \see humanize()
     */
    void  addSubEnumTo( Arn::XStringMap& xsm, quint16 nameSpace = 0, bool neverHumanize = false)  const;

    //! Adds bit set for enum flags to a XStringMap
    /*! <b>Example</b> \n \code
     *  Arn::XStringMap  xsm;
     *  xsm.add("T", "Test");
     *  AllowClassT::txt().addBitSetTo( xsm, 0, true);
     *  \endcode
     *  wiil give xsm containing:
     *  T=Test B0=Read B3=Create B4=Delete 0=None 0xff=All SE6:B1=AllowLev E0=Low E1=Mid E2=High
     *  \param[out] xsm is the XStringMap to be added to.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \see addFlagsTo()
     *  \see addSubEnumTo()
     *  \see humanize()
     */
    void  addBitSetTo( Arn::XStringMap& xsm, quint16 nameSpace = 0, bool neverHumanize = false)  const;

    void  addBitSet( Arn::XStringMap& xsm, quint16 nameSpace = 0, bool neverHumanize = false)  const  // Legacy
          {addBitSetTo( xsm, nameSpace, neverHumanize);}

    //! returns the bit set string for enum flags
    /*! Example
     *  > qDebug() << AllowClassT::txt().getBitSet();
     *  wiil print: "B0=Read B3=Create B4=Delete 0=None 0xff=All SE6:B1=AllowLev E0=Low E1=Mid E2=High"
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \return the bit set string.
     *  \see humanize()
     */
    QString  getBitSet( quint16 nameSpace = 0, bool neverHumanize = false)  const;

    //! Adds all sub enum plain and shifted to a XStringMap
    /*! Also adds bitmask and name of the sub enum
     *  All enums must have unique names
     *  <b>Example</b> \n \code
     *  Arn::XStringMap  xsm;
     *  xsm.add("T", "Test");
     *  AllowClassT::txt().addSubEnumPlainTo( xsm, 0, true);
     *  \endcode
     *  wiil give xsm containing:
     *  T=Test 6=AllowLev 0=Low 2=Mid 4=High
     *  \param[out] xsm is the XStringMap to be added to.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \see addBitSetTo()
     *  \see humanize()
     */
    void  addSubEnumPlainTo( Arn::XStringMap& xsm, quint16 nameSpace = 0, bool neverHumanize = false)  const;

    //! returns text string for enum flags
    /*! <b>Example</b> \n \code
     *  AllowClassT  allow;
     *  allow = allow.Create | allow.Delete;
     *  allow.setSubEnum( AllowLevelT::Mid);
     *  qDebug() << allow.txt().flagsToString( allow);
     *  \endcode
     *  wiil print: "Create | Delete | Mid"
     *  \param[in] val is the flags enum value.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \return the flags text string.
     *  \see flagsToStringList()
     */
    QString  flagsToString( int val, quint16 nameSpace = 0)  const;

    //! returns string list for enum flags
    /*! <b>Example</b> \n \code
     *  AllowClassT  allow;
     *  allow = allow.Create | allow.Delete;
     *  allow.setSubEnum( AllowLevelT::Low);
     *  QStringList  allowList = allow.txt().flagsToStringList( allow);
     *  \endcode
     *  wiil give allowList containing: "Create", "Delete", "Low"
     *  \param[in] val is the flags enum value.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \return the flags string list.
     *  \see flagsToString()
     */
    QStringList  flagsToStringList( int val, quint16 nameSpace = 0)  const;

    //! returns enum flags from string
    /*! <b>Example</b> \n \code
     *  QString flagString = "Create | Delete | Low";
     *  int val = AllowClassT::txt().flagsFromString( flagString);
     *  \endcode
     *  wiil give val: 0x18 (0x08 + 0x10 + 0x00)
     *  \param[in] flagString is the flags text.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \return the flags enum value.
     *  \see flagsFromStringList()
     */
    int  flagsFromString( const QString& flagString, quint16 nameSpace = 0);

    //! returns enum flags from string list
    /*! <b>Example</b> \n \code
     *  QStringList flagStrings;
     *  flagStrings << "Create" << "Delete" << "High";
     *  int val = AllowClassT::txt().flagsFromString( flagStrings);
     *  \endcode
     *  wiil give val: 0x1c (0x08 + 0x10 + 0x04)
     *  \param[in] flagStrings is the flags text list.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \return the flags enum value.
     *  \see flagsFromString()
     */
    int  flagsFromStringList( const QStringList& flagStrings, quint16 nameSpace = 0);

    //! Adds enum set to a XStringMap
    /*! <b>Example</b> \n \code
     *  Arn::XStringMap  xsm;
     *  xsm.add("T", "Test");
     *  ConnectStatT::txt().addEnumSetTo( xsm);
     *  \endcode
     *  wiil give xsm containing: T=Test 0=Init 1=Connected 2=Error 3=Disconnected 4=Tried all
     *  \param[out] xsm is the XStringMap to be added to.
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \see getEnumSet()
     *  \see humanize()
     */
    void  addEnumSetTo( Arn::XStringMap& xsm, quint16 nameSpace = 0, bool neverHumanize = false)  const;

    void  addEnumSet( Arn::XStringMap& xsm, quint16 nameSpace = 0, bool neverHumanize = false)  const  // Legacy
          {addEnumSetTo( xsm, nameSpace, neverHumanize);}

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

    //! returns a string list containing the most basic texts
    /*! For a EnumSet this is the complete text list.
     *  For a BitSet this is the texts for all the single 1-bits except those used for SubEnums.
     *  Example
     *  > qDebug() << AllowClassT::txt().getBasicTextList( 0, true);
     *  wiil print: "Read" "Create" "Delete"
     *  \param[in] nameSpace is the usage set for this enum, e.g human readable.
     *  \param[in] neverHumanize if true never applies the enum text humanize algorithm.
     *  \return the basic string list.
     *  \see humanize()
     */
    QStringList  getBasicTextList( quint16 nameSpace = 0, bool neverHumanize = false)  const;

    //! Adds an other EnumTxt as a subEnum to this flag EnumTxt
    /*! <b>Example</b> \n \code
     *  // In class usage example, following is done behind the scene
     *  AllowClassT::txt().addSubEnum( AlowLevelT::txt(), AllowClassT::AllowLev, AllowClassT::AllowLevB0);
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
    /*! \retval is true when this is a flag, false when plain enum.
     */
    bool  isFlag()  const;

    //! Clear this dynamic instance to its starting state
    /*! Clear will do nothing if this instance was created staticly with a QMetaObject.
     */
    void  clear();

    //! Loads the instance by an enum set XStringMap
    /*! <b>Example output</b> \n \code
     *  Arn::XStringMap  xsm( "0=Arn 1=Is 0x2=Great");
     *  Arn::EnumTxt  myEnum;
     *  myEnum.loadEnumSet( xsm, "MyEnum");
     *  \endcode
     *  \param[in] xsm is the XStringMap containing the enum representation.
     *  \param[in] name is the name of this enum collection.
     *  \retval returns true if successful.
     */
    bool  loadEnumSet( const Arn::XStringMap& xsm, const QString& name = QString());

    //! Loads the instance by an enum set XString
    /*! <b>Example output</b> \n \code
     *  QString  xstr( "0=Arn 0x1=Is 2=Great");
     *  Arn::EnumTxt  myEnum;
     *  myEnum.loadEnumSet( xstr, "MyEnum");
     *  \endcode
     *  \param[in] xstr is the XString containing the enum representation.
     *  \param[in] name is the name of this enum collection.
     *  \retval returns true if successful.
     */
    bool  loadEnumSet( const QString& xstr, const QString& name = QString());

    //! Loads the instance by an bit set (flags) XStringMap
    /*! <b>Example output</b> \n \code
     *  Arn::XStringMap  xsm( "B0=Read B3=Create 0=None SE6:B1=AllowLev E0=Low E1=Mid E2=High");
     *  Arn::EnumTxt  myFlags;
     *  myFlags.loadBitSet( xsm, "MyFlags");
     *  \endcode
     *  \param[in] xsm is the XStringMap containing the flags representation.
     *  \param[in] name is the name of this flag collection.
     *  \retval returns true if successful.
     */
    bool  loadBitSet( const Arn::XStringMap& xsm, const QString& name = QString());

    //! Loads the instance by an bit set (flags) XString
    /*! <b>Example output</b> \n \code
     *  QString  xstr( "B0=Read B3=Create 0=None SE6:B1=AllowLev E0=Low E1=Mid E2=High");
     *  Arn::EnumTxt  myFlags;
     *  myFlags.loadBitSet( xstr, "MyFlags");
     *  \endcode
     *  \param[in] xsm is the XStringMap containing the flags representation.
     *  \param[in] name is the name of this flag collection.
     *  \retval returns true if successful.
     */
    bool  loadBitSet( const QString& xstr, const QString& name = QString());

    //! Returns number of subEnums in this bitSet (class)
    /*! Example
     *  > qDebug() << AllowClassT::txt().subEnumCount();
     *  wiil print: 1
     *  \return the count of sub enums.
     */
    int  subEnumCount()  const;

    //! Returns the name of a SubEnum
    /*! Name is taken by the registered bitmask of the SubEnum.
     *  When using static creation of this instance, there must be a declared enum for the
     *  bitmask of the SubEnum. E.g. in AllowClassT this enum is AllowLev.
     *  When using dynamic creation of flags, loading a BitSet, the name of the SubEnum
     *  is set in the process. This is also true when using addSubEnum().
     *  Example
     *  > qDebug() << AllowClassT::txt().subEnumNameAt( 0);
     *  wiil print: "AllowLev"
     *  \param[in] idx is the index of the SubEnum.
     *  \param[in] nameSpace is the nameSpace for the registered name (by bitMask).
     *  \retval returns the SubEnum name for inbound idx, otherwise QString().
     */
    QString  subEnumNameAt( int idx, quint16 nameSpace = 0)  const;

    //! Returns a pointer to a SubEnum
    /*! <b>Example output</b> \n \code
     *  QString  xstr( "B0=Read B3=Create 0=None SE6:B1=AllowLev E0=Low E1=Mid E2=High");
     *  Arn::EnumTxt  myFlags;
     *  myFlags.loadBitSet( xstr, "MyFlags");
     *  const Arn::EnumTxt*  sube = myFlags.subEnumAt( 0);
     *  qDebug() << sube->getEnumSet( 0, false);
     *  \endcode
     *  wiil print: "0=Low 1=Mid 2=High"
     *  \param[in] idx is the index of the SubEnum.
     *  \retval returns a pointer to SubEnum for inbound idx, otherwise arnNullPtr.
     */
    const EnumTxt*  subEnumAt( int idx)  const;

    static QByteArray  numToStr( uint num);
    static uint  strToNum( const QByteArray& str, bool* isOk = arnNullptr);
    static uchar strToBitpos( const QByteArray& str, bool* isOk = arnNullptr);

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
        bool  _isLocalOwn;
        SubEnumEntry( const EnumTxt& subEnum, uint bitMask, uint factor, bool isLocalOwn);
    };

    void  setTxtRefAny( const char* txt, int enumVal, quint16 nameSpace);
    void  addSubEnumAny( const EnumTxt& subEnum, uint bitMask, uint factor, bool isLocalOwn);
    void  setupFromMetaObject();
    void  setupTxt( const _InitEnumTxt* initTxt);
    void  setupSubEnum( const _InitSubEnum* initSubEnum);

    const QMetaObject*  _metaObj;
    QMap<EnumTxtKey,const char*>  _enumTxtTab;
    QList<QByteArray>*  _txtStore;
    QList<SubEnumEntry>*  _subEnumTab;
    QByteArray  _name;
    uint _subEnumMask;
    bool  _isFlag;
};

}

#endif // MQFLAGS_HPP
