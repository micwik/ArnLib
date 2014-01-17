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
#include "XStringMap.hpp"
#include <QHostAddress>

class ArnServer;
class ArnClient;
class ArnZeroConfRegister;
class ArnZeroConfBrowser;
class QHostInfo;
class QTimer;


namespace ArnDiscover
{
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
}


class ArnDiscoverInfo
{
    friend class ArnDiscoverBrowser;
public:
    //! State of Arn discover browse data
    struct State {
        enum E {
            //! Initialized null state
            Init,
            //! Got service name and domain (from browsing)
            ServiceName,
            //! Also got HostName, HostPort, type and properties (from resolving)
            HostInfo,
            //! Also got HostIp (from DNS lookup)
            HostIp
        };
        MQ_DECLARE_ENUM( State)
    };

    ArnDiscoverInfo();

    State  state()  const;
    State  stopState()  const;
    ArnDiscover::Type type()  const;
    QString  serviceName()  const;
    QString  domain()  const;
    QString  hostName()  const;
    quint16  hostPort()  const;
    QHostAddress  hostIp()  const;
    XStringMap  properties()  const;
    QString  typeString()  const;
    QString  hostPortString()  const;
    QString  hostIpString()  const;

private:
    int  _id;
    State  _state;
    State  _stopState;
    ArnDiscover::Type  _type;
    QString  _serviceName;
    QString  _domain;
    QString  _hostName;
    quint16  _hostPort;
    QHostAddress  _hostIp;
    XStringMap  _properties;
};


class ArnDiscoverBrowser : public QObject
{
    Q_OBJECT
public:
    explicit ArnDiscoverBrowser( QObject *parent = 0);

    const ArnDiscoverInfo&  infoByIndex( int index);
    const ArnDiscoverInfo&  infoById( int id);
    const ArnDiscoverInfo&  infoByName( QString serviceName);
    int  indexToId( int index);
    int  IdToIndex( int id);

    bool  isBrowsing()  const;
    void  setFilter( ArnDiscover::Type typeFilter);

    ArnDiscoverInfo::State  defaultStopState()  const;
    void  setDefaultStopState( ArnDiscoverInfo::State defaultStopState);

signals:
    void  serviceAdded( int index, QString name);
    void  serviceRemoved( int index);
    void  infoUpdated( int index, ArnDiscoverInfo::State state);

public slots:
    void  browse( bool enable = true);
    void  stopBrowse();

private slots:
    void  onBrowseError( int code);
    void  onServiceAdded( int id, QString name, QString domain);
    void  onServiceRemoved( int id, QString name, QString domain);

    void  onResolveError( int code);
    void  onResolved( int id, QByteArray escFullDomain);

    void  onIpLookup( const QHostInfo& host);

private:
    void  doNextState( const ArnDiscoverInfo& info);

    ArnZeroConfBrowser*  _serviceBrowser;
    QList<int>  _activeServIds;
    QList<ArnDiscoverInfo>  _activeServInfos;
    QMap<int,int>  _ipLookupIds;
    QString  _filter;
    ArnDiscoverInfo::State  _defaultStopState;
};


class ArnDiscoverAdvertise : public QObject
{
    Q_OBJECT
public:
    explicit ArnDiscoverAdvertise( QObject *parent = 0);

    QString  defaultService()  const;
    void  setDefaultService( const QString& defaultService);

    QString  service() const;

    void  setArnServer( ArnServer* arnServer, ArnDiscover::Type discoverType = ArnDiscover::Type::Server);
    void  startNewArnServer( ArnDiscover::Type discoverType, int port = -1);
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
    ArnDiscover::Type  _discoverType;
};

#endif // ARNDISCOVER_HPP
