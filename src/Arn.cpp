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

#include "ArnInc/Arn.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QRegExp>
#include <QStringList>


namespace Arn {

const QString  pathLocal           = "/Local/";  // Must be absolute (full) path
const QString  pathLocalSys        = "Sys/";
const QString  pathDiscoverThis    = "Sys/Discover/This/";
const QString  pathDiscoverConnect = "Sys/Discover/Connect/";

bool debugThreading = false;
bool debugLinkRef   = false;
bool debugRecInOut  = false;
bool debugMonitor   = false;


QString  convertName( const QString& name, NameF nameF)
{
    bool  isFolderMarked = name.endsWith('/');
    QString  baseName = name.left( name.size() - int( isFolderMarked));  // remove any foldermark

    QString  retVal = convertBaseName( baseName, nameF);
    if (isFolderMarked && !nameF.is(( nameF.NoFolderMark)))
        retVal += '/';  // Restore previous foldermark

    return retVal;
}


QString  convertBaseName( const QString& name, NameF nameF)
{
    QString  retVal("");

    if (name.isEmpty() && !nameF.is( nameF.EmptyOk))
        retVal = '@';
    else if ((name != "@") || !nameF.is( nameF.EmptyOk))
        retVal = name;

    return retVal;
}


QString  fullPath( const QString& path)
{
    if (path.startsWith('/'))  return path;

    return Arn::pathLocal + path;
}


QString  itemName( const QString &path)
{
    int  from = path.endsWith('/') ? -2 : -1;
    int  pos = path.lastIndexOf('/', from);

    if (pos < 0)  return path;
    return Arn::convertName( path.mid( pos + 1));
}


QString  childPath( const QString &parentPath, const QString &posterityPath)
{
    QString  parentPath_ = parentPath;
    if (!parentPath_.endsWith('/'))  parentPath_ += '/';
    if (!posterityPath.startsWith( parentPath_))  return QString();  // Null, posterity not belonging tp parent

    int  i = posterityPath.indexOf('/', parentPath_.size());
    if (i >= 0)  // The child part has folder(s)
        return posterityPath.left(i + 1);
    else
        return posterityPath;
}


QString  makePath( const QString &parentPath, const QString &itemName)
{
    QString  parentPath_ = parentPath;
    if (!parentPath_.endsWith('/'))  parentPath_ += '/';

    return parentPath_ + Arn::convertName( itemName, Arn::NameF::EmptyOk);
}


QString  addPath( const QString &parentPath, const QString &childRelPath, Arn::NameF nameF)
{
    QString  retPath = parentPath;
    if (!retPath.endsWith('/'))
        retPath += '/';
    retPath += childRelPath;

    return convertPath( retPath, nameF);
}


QString  convertPath(const QString &path, Arn::NameF nameF)
{
    nameF.set( nameF.NoFolderMark, false);  // Foldermark '/' must be ...
    if (nameF.is( nameF.Relative))
        nameF.set( nameF.EmptyOk, false);   // Relative implicates no emty links

    QString  retPath;
    if (!nameF.is( nameF.Relative))
        retPath += '/';  // Start of absolute path

    QString  pathNorm = path.trimmed();
    bool  isFolder = pathNorm.isEmpty() || pathNorm.endsWith("/");
    if (isFolder && !pathNorm.isEmpty())
        pathNorm.resize( pathNorm.size() - 1);  // Remove '/' at end  (Also root become "")

    QStringList  linkNames = pathNorm.split("/", QString::KeepEmptyParts);
    bool needSeparator = false;

    for (int i = 0; i < linkNames.size(); i++) {
        QString  linkName = linkNames.at(i);
        if (linkName.isEmpty()  &&  i == 0)  // If link is root, go for next link
            continue;
        if (needSeparator)
            retPath += '/';  // Add link separator

        retPath += Arn::convertName( linkName, nameF);
        needSeparator = true;
    }
    if (isFolder && !pathNorm.isEmpty())  // Folder that is not root
        retPath += '/';  // Add folder mark

    return retPath;
}


QString  twinPath( const QString& path)
{
    if (path.endsWith('/'))  return path;  // Can't return twin for a folder

    if (path.endsWith('!'))  return path.left( path.size() - 1);
    return path + '!';
}


bool  isFolderPath( const QString& path)
{
    return path.endsWith('/');
}


bool  isProviderPath( const QString& path)
{
    return path.endsWith('!');
}


QString  makeHostWithInfo( const QString &host, const QString &info)
{
    return host + (info.isEmpty() ? QString() 
                                  : ("  [" + info + "]"));
}


QString  hostFromHostWithInfo( const QString &hostWithInfo)
{
    QString  retVal = hostWithInfo;
    int  pos = retVal.indexOf( QRegExp("\\s*\\[.+\\]"));
    if (pos >= 0)
        retVal.resize( pos);
    
    return retVal;
}

}  // Arn::

