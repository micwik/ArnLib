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


//! \cond ADV
class ARNLIBSHARED_EXPORT ArnInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString info  READ info )
public:
    explicit  ArnInterface( QObject* parent = 0) : QObject( parent) {}

    QString  info()                             {return QString::fromUtf8( ArnM::instance().info().constData());}

public slots:
    QVariant  value( const QString& path)       {return ArnM::instance().valueVariant( path);}
    QVariant  variant( const QString& path)     {return ArnM::instance().valueVariant( path);}
    QString  string( const QString& path)       {return ArnM::instance().valueString( path);}
    QByteArray  bytes( const QString& path)     {return ArnM::instance().valueByteArray( path);}
    double  num( const QString& path)           {return ArnM::instance().valueDouble( path);}
    int  intNum( const QString& path)           {return ArnM::instance().valueInt( path);}

    QStringList  items( const QString& path)    {return ArnM::instance().items( path);}
    bool  exist(const QString& path)            {return ArnM::instance().exist( path);}
    bool  isFolder( const QString& path)        {return ArnM::instance().isFolder( path);}
    bool  isLeaf( const QString& path)          {return ArnM::instance().isLeaf( path);}

    void  setValue( const QString& path, const QVariant& value)
    {ArnM::instance().setValue( path, value);}

    void  setVariant( const QString& path, const QVariant& value, const QString& typeName = QString())
    {ArnM::instance().setValue( path, value, typeName.toLatin1().constData());}

    void  setString( const QString& path, const QString& value)
    {ArnM::instance().setValue( path, value);}

    void  setBytes( const QString& path, const QByteArray& value)
    {ArnM::instance().setValue( path, value);}

    void  setNum( const QString& path, double value)
    {ArnM::instance().setValue( path, value);}

    void  setIntNum( const QString& path, int value)
    {ArnM::instance().setValue( path, value);}

    //// "static" help functions
    bool  isFolderPath( const QString& path)    {return Arn::isFolderPath( path);}
    bool  isProviderPath( const QString& path)  {return Arn::isProviderPath( path);}
    QString  itemName( const QString& path)     {return Arn::itemName( path);}
    QString  twinPath( const QString& path)     {return Arn::twinPath( path);}

    QString  changeBasePath( const QString& oldBasePath, const QString& newBasePath, const QString& path)
    {return Arn::changeBasePath( oldBasePath, newBasePath, path);}

    QString  childPath( const QString &parentPath, const QString &posterityPath)
    {return Arn::childPath( parentPath, posterityPath);}

    QString  makePath( const QString &parentPath, const QString &itemName)
    {return Arn::makePath( parentPath, itemName);}

    QString  providerPath( const QString& path, bool giveProviderPath = true)
    {return  Arn::providerPath( path, giveProviderPath);}
};
//! \endcond

#endif // ARNINTERFACE_HPP
