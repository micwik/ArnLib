// Copyright (C) 2010-2014 Michael Wiklund.
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

#include "XStringMap.hpp"
#include "MQFlags.hpp"
#include <QHostAddress>
#include <QVariantMap>

class ArnZeroConfRegister;
class ArnZeroConfBrowser;
class QHostInfo;


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
    friend class ArnDiscoverBrowserB;
public:
    //! State of Arn discover browse data. Can be tested by relative order.
    struct State {
        enum E {
            //! Initialized null state
            Init,
            //! Got service name and domain (from browsing)
            ServiceName,
            //! Got error during resolving HostName, HostPort, type and properties
            HostInfoErr,
            //! Also got HostName, HostPort, type and properties (from resolving)
            HostInfo,
            //! Got error during DNS lookup HostIp
            HostIpErr,
            //! Also got HostIp (from DNS lookup)
            HostIp
        };
        MQ_DECLARE_ENUM( State)
    };

    ArnDiscoverInfo();

    bool  inProgress()  const;
    bool  isError()  const;
    State  state()  const;
    State  stopState()  const;
    ArnDiscover::Type type()  const;
    QString  group()  const;
    QString  serviceName()  const;
    QString  domain()  const;
    QString  hostName()  const;
    quint16  hostPort()  const;
    QHostAddress  hostIp()  const;
    Arn::XStringMap  properties()  const;
    QString  typeString()  const;
    QString  hostPortString()  const;
    QString  hostIpString()  const;

    //! Get the the _HostWithInfo_ string
    /*! ArnClient and alike accepts such _HostWithInfo_ strings for connection.
     *  \return The _HostWithInfo_ string, e.g. "192.168.1.1  [myhost.local]"
     *  \see Arn::makeHostWithInfo()
     */
    QString  hostWithInfo()  const;
    int  resolvCode()  const;

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
    Arn::XStringMap  _properties;
    int  _resolvCode;
};

/// Browse() and resolve() together, may never be used to the same instance.
class ArnDiscoverBrowserB : public QObject
{
    Q_OBJECT
public:
    explicit ArnDiscoverBrowserB( QObject *parent = 0);

    const ArnDiscoverInfo&  infoByIndex( int index);
    const ArnDiscoverInfo&  infoById( int id);
    const ArnDiscoverInfo&  infoByName( QString serviceName);
    int  indexToId( int index);
    int  IdToIndex( int id);
    int  serviceNameToId( const QString& name);

    ArnDiscoverInfo::State  defaultStopState()  const;
    void  setDefaultStopState( ArnDiscoverInfo::State defaultStopState);
    bool  goTowardState( int index, ArnDiscoverInfo::State state);

signals:
    void  serviceAdded( int index, QString name);
    void  serviceRemoved( int index);
    void  infoUpdated( int index, ArnDiscoverInfo::State state);

public slots:

protected:
    bool  isBrowsing()  const;
    void  setFilter( ArnDiscover::Type typeFilter);
    void  setFilter( QString group);

    void  browse( bool enable = true);
    void  stopBrowse();
    void  resolve( QString serviceName, bool forceUpdate = true);

private slots:
    void  onBrowseError( int code);
    void  onServiceAdded( int id, QString name, QString domain);
    void  onServiceRemoved( int id, QString name, QString domain);

    void  onResolveError( int id, int code);
    void  onResolved( int id, QByteArray escFullDomain);

    void  onLookupError( int id, int code);
    void  onLookuped( int id);

private:
    int  newServiceInfo( int id, QString name, QString domain);
    void  removeServiceInfo( int index);
    void  doNextState( ArnDiscoverInfo& info);

    ArnZeroConfBrowser*  _serviceBrowser;
    QList<int>  _activeServIds;
    QList<ArnDiscoverInfo>  _activeServInfos;
    QString  _filter;
    ArnDiscoverInfo::State  _defaultStopState;
};


class ArnDiscoverBrowser : public ArnDiscoverBrowserB
{
    Q_OBJECT
public:
    explicit ArnDiscoverBrowser( QObject *parent = 0);

    bool  isBrowsing()  const
    {return ArnDiscoverBrowserB::isBrowsing();}

    void  setFilter( ArnDiscover::Type typeFilter)
    {ArnDiscoverBrowserB::setFilter( typeFilter);}

    void  setFilter( QString group)
    {ArnDiscoverBrowserB::setFilter( group);}

public slots:
    void  browse( bool enable = true)
    {ArnDiscoverBrowserB::browse( enable);}

    void  stopBrowse()
    {ArnDiscoverBrowserB::stopBrowse();}
};


class ArnDiscoverResolver : public ArnDiscoverBrowserB
{
    Q_OBJECT
public:
    explicit ArnDiscoverResolver( QObject *parent = 0);

    QString  defaultService()  const;
    void  setDefaultService( const QString& defaultService);

public slots:
    void  resolve( QString serviceName, bool forceUpdate = true);

private:
    QString  _defaultService;
};


class ArnDiscoverAdvertise : public QObject
{
    Q_OBJECT
public:
    //! States of DiscoverAdvertise
    //// These values must be synced with: ArnZeroConf::State
    struct State {
        enum E {
            //! Inactive state.
            None             = 0x0000,
            //! Startup advertising in progress.
            StartupAdvertise = 0x0100,
            //! Is now advertising. Startup has finished sucessfully.
            Advertising      = 0x0001,
            //! isAny(): Startup advertising in progress or has finished sucessfully.
            Advertise        = 0x0101
        };
        MQ_DECLARE_FLAGS( State)
    };

    explicit ArnDiscoverAdvertise( QObject *parent = 0);

    QStringList groups() const;
    void setGroups( const QStringList& groups);
    void  addGroup( const QString& group);

    QString  service() const;
    State  state()  const;

    void  advertiseService( ArnDiscover::Type discoverType, QString serviceName,
                            int port = -1, const QString& hostName = QString());

    Arn::XStringMap  customProperties()  const;
    void  setCustomProperties( const Arn::XStringMap& customProperties);
    void  addCustomProperty( const QString& key, const QString& val);

signals:
    void  serviceChanged( QString serviceName);
    void  serviceChangeError( int code);

public slots:
    //! Set the service name
    /*! Will update current advertised service name if this advertiser has been setup,
     *  otherwise the service name is stored for future use.
     *  Note: This base member must be called from derived member.
     *  \param[in] service is the service name.
     *  \see advertiseService()
     */
    virtual void  setService( QString service);

protected:
    bool  hasSetupAdvertise()  const;

protected slots:
    //! Post setup routine called from base class
    /*! Can be derived for special setup.
     *  Note: This base method must be called from derived method.
     *  \param[in] service is the service name.
     */
    virtual void  postSetupThis();

    //! Service registration callback
    /*! Can be derived for special notifying.
     *  Note: This base method must be called from derived method.
     *  \param[in] serviceName is the service name registered.
     */
    virtual void  serviceRegistered( QString serviceName);

    //! Service registration error callback
    /*! Can be derived for special notifying.
     *  Note: This base method must be called from derived method.
     *  \param[in] code is the error code.
     */
    virtual void  serviceRegistrationError( int code);

private:
    ArnZeroConfRegister*  _arnZCReg;
    QString  _service;
    QStringList  _groups;
    Arn::XStringMap  _customProperties;
    bool  _hasSetupAdvertise;
    ArnDiscover::Type  _discoverType;
};

MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnDiscoverAdvertise::State)

#endif // ARNDISCOVER_HPP
