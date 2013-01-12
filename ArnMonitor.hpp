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

#ifndef ARNMONITOR_HPP
#define ARNMONITOR_HPP


#include "ArnLib_global.hpp"
#include <QStringList>
#include <QObject>

class ArnClient;
class ArnItemNet;


class ARNLIBSHARED_EXPORT ArnMonitor : public QObject
{
Q_OBJECT

public:
    explicit  ArnMonitor( QObject* parent = 0);
    void  setClient( ArnClient* client, QString id = QString());
    QString  clientId()  const;

    void  setMonitorPath( QString path, ArnClient* client = 0);
    QString  monitorPath()  const {return _monitorPath;}
    void  reStart();

    void  setReference( void* reference)  {_reference = reference;}
    void*  reference()  const {return _reference;}

signals:
    void  arnItemCreated( QString path);
    void  arnChildFound( QString path);
    void  arnChildFoundFolder( QString path);
    void  arnChildFoundLeaf( QString path);

public slots:
    void  foundChildDeleted( QString path);

protected:
    ArnClient*  _arnClient;
    QString  _monitorPath;

private:
    QStringList  _foundChilds;
    ArnItemNet*  _itemNet;
    void*  _reference;

private slots:
    void  dispatchArnEvent( QByteArray type, QByteArray data, bool isLocal);
    void  emitArnEvent( QByteArray type, QByteArray data = QByteArray());
};


#endif // ARNMONITOR_HPP
