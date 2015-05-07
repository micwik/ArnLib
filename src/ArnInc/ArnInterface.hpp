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

#ifndef ARNINTERFACE_HPP
#define ARNINTERFACE_HPP

#include "ArnLib_global.hpp"
#include "ArnM.hpp"


class ARNLIBSHARED_EXPORT ArnInterface : public QObject
{
    Q_OBJECT
    //! See ArnM::info()
    Q_PROPERTY( QString info  READ info  NOTIFY dummyNotifier)
public:
    //! Action when assigning same value to an ArnItem
    enum SameValue {
        //! Assigning same value generates an update of the _Arn Data Object_
        SameValue_Accept = Arn::SameValue::Accept,
        //! Assigning same value is ignored
        SameValue_Ignore = Arn::SameValue::Ignore,
        //! Assigning same value gives default action set in ArnM or ArnItem
        SameValue_DefaultAction = Arn::SameValue::DefaultAction
    };
    Q_ENUMS( SameValue)

    //! Data type of an _Arn Data Object_
    enum DataType {
        DataType_Null      = Arn::DataType::Null,
        DataType_Int       = Arn::DataType::Int,
        DataType_Double    = Arn::DataType::Double,
        DataType_Real      = Arn::DataType::Real,
        DataType_ByteArray = Arn::DataType::ByteArray,
        DataType_String    = Arn::DataType::String,
        DataType_Variant   = Arn::DataType::Variant
    };
    Q_ENUMS( DataType)

    //! General global mode of an _Arn Data Object_
    enum ObjectMode {
        //! A two way object, typically for validation or pipe
        ObjectMode_BiDir = Arn::ObjectMode::BiDir,
        //! Implies _BiDir_ and all data is preserved as a stream
        ObjectMode_Pipe  = Arn::ObjectMode::Pipe,
        //! Data is persistent and will be saved
        ObjectMode_Save  = Arn::ObjectMode::Save
    };
    Q_ENUMS( ObjectMode)

    //! Selects a format for path or item name
    enum NameF {
        //! Empty not ok,  Path: Absolute  Item: FolderMark
        NameF_Default      = Arn::NameF::Default,
        //! Only on discrete names, no effect on path. "test/" ==> "test"
        NameF_NoFolderMark = Arn::NameF::NoFolderMark,
        //! Path: "/@/test" ==> "//test", Item: "@" ==> ""
        NameF_EmptyOk      = Arn::NameF::EmptyOk,
        //! Only on path, no effect on discrete names. "/test/value" ==> "test/value"
        NameF_Relative     = Arn::NameF::Relative
    };
    Q_ENUMS( NameF)

//! \cond ADV
    explicit  ArnInterface( QObject* parent = 0) : QObject( parent) {}

    QString  info()                             {return QString::fromUtf8( ArnM::instance().info().constData());}
//! \endcond

public slots:

    //! See ArnM::valueVariant()
    QVariant  value( const QString& path)       {return ArnM::instance().valueVariant( path);}

    //! See ArnM::valueVariant()
    QVariant  variant( const QString& path)     {return ArnM::instance().valueVariant( path);}

    //! See ArnM::valueString()
    QString  string( const QString& path)       {return ArnM::instance().valueString( path);}

    //! See ArnM::valueByteArray()
    QByteArray  bytes( const QString& path)     {return ArnM::instance().valueByteArray( path);}

    //! See ArnM::valueDouble()
#ifdef ARNREAL_FLOAT
    float  num( const QString& path)            {return ArnM::instance().valueDouble( path);}
#else
    double  num( const QString& path)           {return ArnM::instance().valueDouble( path);}
#endif

    //! See ArnM::valueInt()
    int  intNum( const QString& path)           {return ArnM::instance().valueInt( path);}

    //! See ArnM::items()
    QStringList  items( const QString& path)    {return ArnM::instance().items( path);}

    //! See ArnM::exist()
    bool  exist(const QString& path)            {return ArnM::instance().exist( path);}

    //! See ArnM::isFolder()
    bool  isFolder( const QString& path)        {return ArnM::instance().isFolder( path);}

    //! See ArnM::isLeaf()
    bool  isLeaf( const QString& path)          {return ArnM::instance().isLeaf( path);}

    //! See ArnM::setValue()
    void  setValue( const QString& path, const QVariant& value)
    {ArnM::instance().setValue( path, value);}

    //! See ArnM::setValue()
    void  setVariant( const QString& path, const QVariant& value, const QString& typeName = QString())
    {ArnM::instance().setValue( path, value, typeName.toLatin1().constData());}

    //! See ArnM::setValue()
    void  setString( const QString& path, const QString& value)
    {ArnM::instance().setValue( path, value);}

    //! See ArnM::setValue()
    void  setBytes( const QString& path, const QByteArray& value)
    {ArnM::instance().setValue( path, value);}

    //! See ArnM::setValue()
#ifdef ARNREAL_FLOAT
    void  setNum( const QString& path, float value)
#else
    void  setNum( const QString& path, double value)
#endif
    {ArnM::instance().setValue( path, value);}

    //! See ArnM::setValue()
    void  setIntNum( const QString& path, int value)
    {ArnM::instance().setValue( path, value);}

    //// "static" help functions

    //! See Arn::isFolderPath()
    bool  isFolderPath( const QString& path)    {return Arn::isFolderPath( path);}

    //! See Arn::isProviderPath()
    bool  isProviderPath( const QString& path)  {return Arn::isProviderPath( path);}

    //! See Arn::itemName()
    QString  itemName( const QString& path)     {return Arn::itemName( path);}

    //! See Arn::twinPath()
    QString  twinPath( const QString& path)     {return Arn::twinPath( path);}

    //! See Arn::changeBasePath()
    QString  changeBasePath( const QString& oldBasePath, const QString& newBasePath, const QString& path)
    {return Arn::changeBasePath( oldBasePath, newBasePath, path);}

    //! See Arn::childPath()
    QString  childPath( const QString &parentPath, const QString &posterityPath)
    {return Arn::childPath( parentPath, posterityPath);}

    //! See Arn::makePath()
    QString  makePath( const QString &parentPath, const QString &itemName)
    {return Arn::makePath( parentPath, itemName);}

    //! See Arn::providerPath()
    QString  providerPath( const QString& path, bool giveProviderPath = true)
    {return  Arn::providerPath( path, giveProviderPath);}

//! \cond ADV
signals:
    void  dummyNotifier();
//! \endcond
};

#endif // ARNINTERFACE_HPP
