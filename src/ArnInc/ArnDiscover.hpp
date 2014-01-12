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

#ifndef ARNDISCOVER_HPP
#define ARNDISCOVER_HPP

#include "ArnItem.hpp"

class ArnServer;
class ArnClient;
class ArnZeroConfRegister;
class QTimer;


class ArnDiscoverAdvertise : public QObject
{
    Q_OBJECT
public:
    //! Types of Arn discover advertise
    struct Type {
        enum E {
            //! Undefined Arn discover
            None,
            //! Server Arn discover
            Server,
            //! Client Arn discover
            Client
        };
        MQ_DECLARE_ENUM( Type)
    };

    explicit ArnDiscoverAdvertise( QObject *parent = 0);

    QString  defaultService()  const;
    void  setDefaultService( const QString& defaultService);

    QString  service() const;

    void  setArnServer( ArnServer* arnServer, Type discoverType = Type::Server);
    void  startNewArnServer( Type discoverType, int port = -1);
    void  addArnClient( ArnClient* arnClient, const QString& id);

signals:
    void  serviceChanged( QString serviceName);
    void  serviceChangeError( int code);
    void  clientReadyToConnect( ArnClient* arnClient);

public slots:
    void  setService( QString service);

private slots:
    //// Handle Service This
    void  postSetupThis();
    void  serviceTimeout();
    void  firstServiceSetup( QString serviceName);
    void  doServiceChanged( QString val);
    void  serviceRegistered( QString serviceName);
    void  serviceRegistrationError( int code);
    //// Handle Client
    void  postSetupClient( QObject* arnClientObj);
    void  doClientConnected( QString arnHost, quint16 port);
    void  doClientDirHostChanged( QObject* dirHostsObj = 0);
    void  doClientConnectRequest( int reqCode);

private:
    ArnZeroConfRegister*  _arnZCReg;
    ArnServer*  _arnInternalServer;
    ArnItem  _arnServicePv;
    ArnItem  _arnService;
    QTimer*  _servTimer;
    QString  _defaultService;
    QString  _service;
    bool  _hasBeenSetup;
    Type  _discoverType;
};

#endif // ARNDISCOVER_HPP