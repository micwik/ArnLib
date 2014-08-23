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

#include "ArnInc/ArnQmlMSystem.hpp"
#include <QFile>
#include <QUrl>
#include <QTextStream>
#include <QDebug>


namespace Arn {

QmlMFileIO::QmlMFileIO( QObject* parent)
    : QObject( parent)
{
}


QString  QmlMFileIO::read()
{
    QUrl  url = QUrl( _path);
    if (_path.isEmpty() || !url.isLocalFile()){
        emit error("Invalid fileName=" + _path);
        return QString();
    }
    QString  path = url.path();

    QFile  file( path);
    QString  fileContent;
    if (file.open(QIODevice::ReadOnly)) {
        QString  line;
        QTextStream  ts( &file);
        do {
            line = ts.readLine();
            fileContent += ((fileContent.size() > 0) && !line.isNull()) ? "\n" : "";
            fileContent += line;
         } while (!line.isNull());

        file.close();
    }
    else {
        emit error("Can't' open file, path=" + path);
        return QString();
    }
    return fileContent;
}


bool  QmlMFileIO::write( const QString& data)
{
    if (_path.isEmpty())
        return false;

    QFile  file( _path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    QTextStream out( &file);
    out << data;

    file.close();

    return true;
}


QByteArray  QmlMFileIO::readBytes()
{
    QUrl  url = QUrl( _path);
    if (_path.isEmpty() || !url.isLocalFile()){
        emit error("Invalid fileName=" + _path);
        return QByteArray();
    }
    QString  path = url.path();

    QFile  file( path);
    QByteArray  fileContent;
    if (file.open( QIODevice::ReadOnly) ) {
        fileContent = file.readAll();
        file.close();
    }
    else {
        emit error("Can't' open file, path=" + path);
        return QByteArray();
    }
    return fileContent;
}


bool  QmlMFileIO::writeBytes( const QByteArray& data)
{
    if (_path.isEmpty())
        return false;

    QFile file(_path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    file.write( data);

    file.close();

    return true;
}


QString  QmlMFileIO::path()
{
    return _path;
}


void  QmlMFileIO::setPath( const QString& path)
{
    _path = path;
    emit pathChanged( path);
}

}
