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


//! Class for holding current discover info of one service.
/*!
[About Arn Discover](\ref gen_discover)

This class holds the service info and its discover state.
*/
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

    //! Is discover in progress for this service
    /*! \retval true if discover is in progress
     *  \see state()
     */
    bool  inProgress()  const;

    //! Is in an error state for this service
    /*! \retval true if in error state
     *  \see state()
     */
    bool  isError()  const;

    //! Return the state for this service
    /*! \return state
     *  \see State
     */
    State  state()  const;

    //! Return the stop state for this service
    /*! The discover logic will stop when reaching the stop state for a service.
     *  \return stop state
     *  \see ArnDiscoverBrowserB::setDefaultStopState()
     *  \see ArnDiscoverBrowserB::goTowardState()
     *  \see State
     */
    State  stopState()  const;

    //! Return the discover type for this service
    /*! \return discover type
     *  \see Type
     *  \see ArnDiscoverAdvertise::advertiseService()
     */
    ArnDiscover::Type type()  const;

    //! Return the groups for this service
    /*! Groups are used for filtering discovered services. They will also be availabe
     *  as properties with naming as "group0", "group1" ...
     *  \return groups, e.g. ("mydomain.se", "mydomain.se/House", "Any Group ID")
     *  \see ArnDiscoverAdvertise::setGroups()
     */
    QStringList  groups()  const;

    //! Return the service name for this service
    /*! \return service name, e.g. "My House Registry"
     *  \see ArnDiscoverAdvertise::advertiseService()
     *  \see ArnDiscoverAdvertise::setService()
     */
    QString  serviceName()  const;

    //! Return the domain for this service
    /*! \return domain, e.g. "local."
     */
    QString  domain()  const;

    //! Return the host name for this service
    /*! \return host name, e.g. "myHost.local"
     *  \see ArnDiscoverAdvertise::advertiseService()
     */
    QString  hostName()  const;

    //! Return the port for this service
    /*! \return port
     *  \see ArnDiscoverAdvertise::advertiseService()
     */
    quint16  hostPort()  const;

    //! Return the host ip-address for this service
    /*! \return host ip-address
     */
    QHostAddress  hostIp()  const;

    //! Return the properties for this service
    /*! \return properties
     *  \see ArnDiscoverAdvertise::setCustomProperties()
     */
    Arn::XStringMap  properties()  const;

    //! Return the printable type for this service
    /*! \return type, e.g. "Client"
     */
    QString  typeString()  const;

    //! Return the printable host port for this service
    /*! Will return empty string if no valid port available
     *  \return host port, e.g. "2022", "" etc
     */
    QString  hostPortString()  const;

    //! Return the printable host ip-address for this service
    /*! Will return empty string if no valid ip available
     *  \return host ip-address, e.g. "192.168.1.1", "" etc
     */
    QString  hostIpString()  const;

    //! Get the the _HostWithInfo_ string
    /*! ArnClient and alike accepts such _HostWithInfo_ strings for connection.
     *  \return The _HostWithInfo_ string, e.g. "192.168.1.1  [myhost.local]"
     *  \see Arn::makeHostWithInfo()
     */
    QString  hostWithInfo()  const;

    //! Return the latest resolv error code for this service
    /*! This code can come from booth resolving a service and lookup ip-address.
     *  \return error code
     *  \see ArnZeroConf::Error
     */
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

    //! Return the number of active discover services
    /*! \return number of services
     */
    int serviceCount()  const;

    //! Return the discover service info by its index
    /*! The index for a service info is only valid valid for a given moment, it can change
     *  as services are added and removed. If given an invalid index, a Null discover info
     *  will be returned.
     *  \param[in] index
     *  \return selected service discover info
     *  \see infoById()
     *  \see infoByName()
     *  \see indexToId()
     */
    const ArnDiscoverInfo&  infoByIndex( int index);

    //! Return the discover service info by its id
    /*! The id for a service info is unique and stays same over time, but the service
     *  can have been removed. If given a non existent service id, a Null discover info
     *  will be returned.
     *  \param[in] id
     *  \return selected service discover info
     *  \see infoByIndex()
     */
    const ArnDiscoverInfo&  infoById( int id);

    //! Return the discover service info by its name
    /*! The service name is unique for a given moment, but the service can be removed
     *  and then reappear with a different service name. Also non used service names
     *  can be reused for a different service. If given a non existent service name,
     *  a Null discover info will be returned.
     *  \param[in] serviceName
     *  \return selected service discover info
     *  \see serviceNameToId()
     */
    const ArnDiscoverInfo&  infoByName( QString serviceName);

    //! Return the discover service id by its index
    /*! The index for a service info is only valid valid for a given moment, it can change
     *  as services are added and removed. If given an invalid index, -1 will be returned.
     *  \param[in] index
     *  \return selected service discover id
     *  \see IdToIndex()
     *  \see infoById()
     */
    int  indexToId( int index);

    //! Return the discover service index by its id
    /*! The index for a service info is only valid valid for a given moment, it can change
     *  as services are added and removed. If given a non existent id, -1 will be returned.
     *  \param[in] id
     *  \return selected service discover index
     *  \see indexToId()
     *  \see infoByIndex()
     */
    int  IdToIndex( int id);

    //! Return the discover service id by its name
    /*! The service name is unique for a given moment. If given a non existent service name,
     *  -1 will be returned.
     *  \param[in] name
     *  \return selected service discover id
     *  \see IdToIndex()
     *  \see infoByName()
     */
    int  serviceNameToId( const QString& name);

    //! Return the default stop state for this service discover browser
    /*! This default stop state will be used for all services discovered by this browser.
     *  \return default stop state
     *  \see setDefaultStopState()
     *  \see goTowardState()
     *  \see ArnDiscoverInfo::stopState()
     *  \see State
     */
    ArnDiscoverInfo::State  defaultStopState()  const;

    //! Set the default stop state for this service discover browser
    /*! This default stop state will be used for all services discovered by this browser.
     *  \param[in] defaultStopState
     *  \see defaultStopState()
     *  \see goTowardState()
     *  \see ArnDiscoverInfo::stopState()
     *  \see State
     */
    void  setDefaultStopState( ArnDiscoverInfo::State defaultStopState);

    //! Command a service to go towards a stop state
    /*! The service is specified by its index. The wanted final state must be forward,
     *  otherwise it is ignored.
     *  \param[in] index for the service
     *  \param[in] state is the wanted final state
     *  \see defaultStopState()
     *  \see infoUpdated()
     *  \see ArnDiscoverInfo::stopState()
     *  \see State
     */
    bool  goTowardState( int index, ArnDiscoverInfo::State state);

signals:
    //! Indicate service has been added (discovered)
    /*! The service has been added to a list sorted by ascending service names.
     *  The index is a reference to this sorted list.
     *  \param[in] index for the service
     *  \param[in] name is the service name e.g. "My House Registry"
     *  \see serviceRemoved()
     *  \see infoUpdated()
     */
    void  serviceAdded( int index, QString name);

    //! Indicate service has been removed
    /*! \param[in] index for the service
     *  \see serviceAdded()
     */
    void  serviceRemoved( int index);

    //! Indicate service has been updated
    /*! \param[in] index for the service
     *  \param[in] state is the current state of the service info
     *  \see goTowardState()
     *  \see serviceAdded()
     */
    void  infoUpdated( int index, ArnDiscoverInfo::State state);

    //! \cond ADV
protected:
    //! Return the status of the browsing
    /*! \retval true if browsing is started
     *  \see browse()
     */
    bool  isBrowsing()  const;

    //! Set service discover filter using predefined types
    /*! When filter is enabled, only services that have the same type
     *  is discovered.
     *  \param[in] typeFilter
     *  \see ArnDiscoverAdvertise::advertiseService()
     */
    void  setFilter( ArnDiscover::Type typeFilter);

    //! Set service discover filter using group name
    /*! If passing empy group, this is taken as subtype (filter) disabled.
     *  When subtype (filter) is enabled, only services that have the same group
     *  is discovered.
     *  \param[in] group the filter group name
     *  \see ArnDiscoverAdvertise::setGroups()
     */
    void  setFilter( QString group);

    //! Change state of browsing
    /*! When browsing is started, services will be discovered.
     *  \param[in] enable if true browsing is started, otherwise it is stopped
     *  \see stopBrowse()
     *  \see serviceAdded()
     */
    void  browse( bool enable = true);

    //! Stop browsing
    /*! \see browse()
     */
    void  stopBrowse();

    //! Resolve a specific service name
    /*! Only the specified service will be resolved, but there can be many ongoing resolves
     *  by calling this method multiple times with different service names. The infoUpdated()
     *  signal will always be emitted when calling this method. The signal can also be emitted
     *  multiple times later regarding the same service.
     *  \param[in] serviceName is the service to be resolved
     *  \param[in] forceUpdate when true, a new resolve is always done, otherwise
     *                         a service name that already is resolved will not be resolved again.
     *  \see infoUpdated()
     */
    void  resolve( QString serviceName, bool forceUpdate = true);
    //! \endcond

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

    //! Return the status of the browsing
    /*! \retval true if browsing is started
     *  \see browse()
     */
    bool  isBrowsing()  const
    {return ArnDiscoverBrowserB::isBrowsing();}

    //! Set service discover filter using predefined types
    /*! When filter is enabled, only services that have the same type
     *  is discovered.
     *  \param[in] typeFilter
     *  \see ArnDiscoverAdvertise::advertiseService()
     */
    void  setFilter( ArnDiscover::Type typeFilter)
    {ArnDiscoverBrowserB::setFilter( typeFilter);}

    //! Set service discover filter using group name
    /*! If passing empy group, this is taken as subtype (filter) disabled.
     *  When subtype (filter) is enabled, only services that have the same group
     *  is discovered.
     *  \param[in] group the filter group name, e.g. "myGroup1"
     *  \see ArnDiscoverAdvertise::setGroups()
     */
    void  setFilter( QString group)
    {ArnDiscoverBrowserB::setFilter( group);}

public slots:
    //! Change state of browsing
    /*! When browsing is started, services will be discovered.
     *  \param[in] enable if true browsing is started, otherwise it is stopped
     *  \see stopBrowse()
     *  \see serviceAdded()
     */
    void  browse( bool enable = true)
    {ArnDiscoverBrowserB::browse( enable);}

    //! Stop browsing
    /*! \see browse()
     */
    void  stopBrowse()
    {ArnDiscoverBrowserB::stopBrowse();}
};


class ArnDiscoverResolver : public ArnDiscoverBrowserB
{
    Q_OBJECT
public:
    explicit ArnDiscoverResolver( QObject *parent = 0);

    //! Return the default service name
    /*! This default service name will be used when resolve() is called with empty
     *  service name.
     *  \return default service name, e.g. "Arn Default Service"
     *  \see setDefaultService()
     *  \see resolve()
     */
    QString  defaultService()  const;

    //! Set the default service name
    /*! This default service name will be used when resolve() is called with empty
     *  service name. If calling with _defaultService_ empty, it is ignored.
     *  \param[in] defaultService e.g. "My Default Service"
     *  \see defaultService()
     *  \see resolve()
     */
    void  setDefaultService( const QString& defaultService);

public slots:
    //! Resolve a specific service name
    /*! Only the specified service will be resolved, but there can be many ongoing resolves
     *  by calling this method multiple times with different service names. The infoUpdated()
     *  signal will always be emitted when calling this method. The signal can also be emitted
     *  multiple times later regarding the same service.
     *  \param[in] serviceName is the service to be resolved
     *  \param[in] forceUpdate when true, a new resolve is always done, otherwise
     *                         a service name that already is resolved will not be resolved again.
     *  \see infoUpdated()
     */
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

    //! \cond ADV
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
    //! \endcond

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
