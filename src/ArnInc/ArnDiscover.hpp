// Copyright (C) 2010-2019 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. Usage of these other libraries is subject to their respective
// license agreements.
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

class ArnDiscoverInfoPrivate;
class ArnDiscoverBrowserBPrivate;
class ArnDiscoverResolverPrivate;
class ArnDiscoverAdvertisePrivate;
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
    Q_DECLARE_PRIVATE(ArnDiscoverInfo)
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
    ArnDiscoverInfo( const ArnDiscoverInfo& other);
    ArnDiscoverInfo&  operator=( const ArnDiscoverInfo& other);
    ~ArnDiscoverInfo();

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
    /*! Will return booth Arn system properties and custom (application) properties.
     *  System properties will always have a key starting with a lower case letter
     *  e.g. "protovers".
     *  \return properties
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

    //! \cond ADV
protected:
    ArnDiscoverInfo( ArnDiscoverInfoPrivate& dd);
    ArnDiscoverInfoPrivate* const  d_ptr;
    //! \endcond

private:
};


/// Browse() and resolve() together, may never be used to the same instance.
class ArnDiscoverBrowserB : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnDiscoverBrowserB)

public:
    explicit ArnDiscoverBrowserB( QObject *parent = 0);
    ~ArnDiscoverBrowserB();

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
    const ArnDiscoverInfo&  infoByName( const QString& serviceName);

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
    void  serviceAdded( int index, const QString& name);

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
    void  setFilter( const QString& group);

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
     *  \return _index_ to service info
     *  \see indexToId()
     *  \see infoUpdated()
     */
    int  resolve( const QString& serviceName, bool forceUpdate = true);

    ArnDiscoverBrowserB( ArnDiscoverBrowserBPrivate& dd, QObject* parent);
    ArnDiscoverBrowserBPrivate* const  d_ptr;
    //! \endcond

private slots:
    void  onBrowseError( int code);
    void  onServiceAdded( int id, const QString& name, const QString& domain);
    void  onServiceRemoved( int id, const QString& name, const QString& domain);

    void  onResolveError( int id, int code);
    void  onResolved( int id, const QByteArray& escFullDomain);

    void  onLookupError( int id, int code);
    void  onLookuped( int id);

private:
    void  init();
    int  newServiceInfo( int id, const QString& name, const QString& domain);
    void  removeServiceInfo( int index);
    void  doNextState( ArnDiscoverInfo& info);
};


//! Browsing for Arn services.
/*!
[About Arn Discover](\ref gen_discover)

For a more complete example see the project ArnBrowser in DiscoverWindow.hpp and
DiscoverWindow.cpp files.

<b>Example usage</b> \n \code
    // In class declare
    ArnDiscoverBrowser*  _serviceBrowser;
    QListWidget*  _serviceTabView;
    QLabel*  _hostNameValue;

    // In class code
    _serviceBrowser = new ArnDiscoverBrowser( this);
    connect(_serviceBrowser, SIGNAL(serviceAdded(int,QString)),
            this, SLOT(onServiceAdded(int,QString)));
    connect(_serviceBrowser, SIGNAL(serviceRemoved(int)), this, SLOT(onServiceRemoved(int)));
    connect(_serviceBrowser, SIGNAL(infoUpdated(int,ArnDiscoverInfo::State)),
            this, SLOT(onInfoUpdated(int,ArnDiscoverInfo::State)));

void  XXX::onServiceAdded( int index, QString name)
{
    _serviceTabView->insertItem( index, name);
}

void  XXX::onServiceRemoved( int index)
{
    QListWidgetItem*  item = _serviceTabView->takeItem( index);
    if (item)
        delete item;
}

void  XXX::onInfoUpdated( int index, ArnDiscoverInfo::State state)
{
    int  curIndex = _serviceTabView->currentRow();
    if (index != curIndex)  return;  // The updated info is not for selected row

    const ArnDiscoverInfo&  info = _serviceBrowser->infoByIndex( curIndex);
    _hostNameValue->setText( info.hostName());
}
\endcode
*/
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
    void  setFilter( const QString& group)
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


//! Resolv an Arn service.
/*!
[About Arn Discover](\ref gen_discover)

<b>Example usage</b> \n \code
    // In class declare
    ArnDiscoverResolver*  _resolver;

    // In class code
    _resolver = new ArnDiscoverResolver( this);
    connect( _resolver, SIGNAL(infoUpdated(int,ArnDiscoverInfo::State)),
             this, SLOT(doClientResolvChanged(int,ArnDiscoverInfo::State)));
    _resolver->resolve("My service");

void  XXX::doClientResolvChanged( int index, ArnDiscoverInfo::State state)
{
    const ArnDiscoverInfo&  info = _resolver->infoByIndex( index);

    if (state == state.HostIp) {
        qDebug() << "Resolved service:" << info.serviceName()
                 << " into host:" << info.hostWithInfo();
    }
    else if (info.isError()) {
        qDebug() << "Error resolving service:" << info.serviceName()
                 << " code:" << info.resolvCode();
    }
}
\endcode
*/
class ArnDiscoverResolver : public ArnDiscoverBrowserB
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnDiscoverResolver)

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
     *  \return _index_ to service info
     *  \see indexToId()
     *  \see infoUpdated()
     */
    int  resolve( const QString& serviceName, bool forceUpdate = true);

    //! \cond ADV
protected:
    ArnDiscoverResolver( ArnDiscoverResolverPrivate& dd, QObject* parent);
    //! \endcond

private:
};


//! Advertise an Arn service.
/*!
[About Arn Discover](\ref gen_discover)

_Arn Discover_ is the mid level support for advertising services on an local network.
For higher level support, use ArnDiscoverRemote.

<b>Example usage</b> \n \code
    // In class declare
    ArnDiscoverAdvertise*  _serviceAdvertiser;
    ArnServer*  _server;

    // In class code
    _server = new ArnServer( ArnServer::Type::NetSync, this);
    _server->start(0);  // Start server on dynamic port
    int  serverPort = _server->port();

    _serviceAdvertiser = new ArnDiscoverAdvertise( this);
    _serviceAdvertiser->addGroup("myId/myProduct");
    _serviceAdvertiser->addCustomProperty("MyProtoVer", "1.0");
    _serviceAdvertiser->advertiseService( ArnDiscover::Type::Server, "My service", serverPort);
\endcode
*/
class ArnDiscoverAdvertise : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnDiscoverAdvertise)

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
    ~ArnDiscoverAdvertise();

    //! Return service discover groups used for filter browsing
    /*! \return groups e.g. ("mydomain.se", "mydomain.se/House", "Any Group ID")
     *  \see setGroups()
     */
    QStringList groups() const;

    //! Set service discover groups used for filter browsing
    /*! Groups are used for filtering discovered services. They will also be availabe
     *  as properties with naming as "group0", "group1" ...
     *  \param[in] groups e.g. ("mydomain.se", "mydomain.se/House", "Any Group ID")
     *  \note Groups must be set before calling advertiseService().
     *  \see groups()
     *  \see ArnDiscoverBrowser::setFilter()
     */
    void setGroups( const QStringList& groups);

    //! Add a service discover group
    /*! \param[in] group e.g. "Any Group ID"
     *  \note Groups must be set before calling advertiseService().
     *  \see setGroups()
     */
    void  addGroup( const QString& group);

    //! Returns the requested service name for this Advertise
    /*! This is always the requested service name, the realy used name comes with the
     *  serviceChanged() signal and currentService().
     *  \return requested service name, e.g. "My House Registry"
     *  \see setService()
     *  \see currentService()
     *  \see advertiseService()
     */
    QString  service() const;

    //! Returns the current service name for this Advertise
    /*! This is the realy advertised name when it's available otherwise it's the requested
     *  service name.
     *  \return service namen (se above) e.g. "My House Registry (2)"
     *  \see setService()
     *  \see service()
     *  \see advertiseService()
     */
    QString  currentService() const;

    //! Returns the state for this Advertise
    /*! \return current state
     *  \see State
     */
    State  state()  const;

    //! Start advertising the service
    /*! Tries to advertise the service on the local network.
     *  Result is indicated by serviceChanged() and serviceChangeError() signals.
     *
     *  Empty _serviceName_ will be ignored, no advertising until using setService() with
     *  non empty name.
     *  \param[in] discoverType is used for discover filtering
     *  \param[in] serviceName is requested name e.g. "My House Registry"
     *  \param[in] port is the port of the service, -1 gives default Arn port number
     *  \param[in] hostName is the host doing the service, empty gives this advertising host
     *  \see setService()
     *  \see serviceChanged()
     *  \see serviceChangeError()
     */
    void  advertiseService( ArnDiscover::Type discoverType, const QString& serviceName,
                            int port = -1, const QString& hostName = QString());

    //! Return service custom properties
    /*! This is only the customer (application) properties, as there also are some
     *  Arn system properties.
     *  \return custom properties
     *  \see setCustomProperties()
     */
    Arn::XStringMap  customProperties()  const;

    //! Set service custom properties
    /*! This is only the customer (application) properties, as there also are some
     *  Arn system properties.
     *
     *  These custom properties are advised to have a key starting
     *  with a capital letter to avoid name collision with the system.
     *  \param[in] customProperties e.g. Arn::XStringMap().add("MyProp", "my data")
     *  \note Properties must be set before calling advertiseService().
     *  \see customProperties()
     *  \see addCustomProperty()
     *  \see ArnDiscoverInfo::properties()
     */
    void  setCustomProperties( const Arn::XStringMap& customProperties);

    //! Add service custom property
    /*! The custom property are advised to have a _key_ starting with a capital letter to
     *  avoid name collision with the system.
     *  \param[in] key property key (Start with capital letter) e.g. "MyProp"
     *  \param[in] val property value kan be any text e.g. "my data"
     *  \note Properties must be set before calling advertiseService().
     *  \see setCustomProperties()
     */
    void  addCustomProperty( const QString& key, const QString& val);

signals:
    //! Indicate successfull advertise of service
    /*! \param[in] serviceName is the realy advertised name e.g. "My House Registry (2)"
     *  \see advertiseService()
     *  \see setService()
     */
    void  serviceChanged( const QString& serviceName);

    //! Indicate unsuccessfull advertise of service
    /*! \param[in] code error code.
     *  \see advertiseService()
     */
    void  serviceChangeError( int code);

public slots:
    //! Set the service name
    /*! Will update current advertised service name if this advertiser has been setup,
     *  otherwise the service name is stored for future use.
     *
     *  Service names can be any human readable id. It should be easy to understand,
     *  without any cryptic coding, and can usually be modified by the end user
     *
     *  Empty name is ignored. The requested service name is not guaranted to be used
     *  for advertise, as it has to be unique within this local network. The realy used
     *  name comes with the serviceChanged() signal and currentService().
     *  \param[in] service is the requested service name e.g. "My House Registry"
     *  \see service()
     *  \see currentService()
     *  \see advertiseService()
     *  \see serviceChanged()
     *  \see serviceChangeError()
     */
    virtual void  setService( const QString& service);

    //! \cond ADV
protected:
    bool  hasSetupAdvertise()  const;

    ArnDiscoverAdvertise( ArnDiscoverAdvertisePrivate& dd, QObject* parent);
    ArnDiscoverAdvertisePrivate* const  d_ptr;

protected slots:
    //! Post setup routine called from base class
    /*! Can be derived for special setup.
     *  \param[in] service is the service name.
     *  \note This base method must be called from derived method.
     */
    virtual void  postSetupThis();

    //! Service registration callback
    /*! Can be derived for special notifying.
     *  \param[in] serviceName is the service name registered.
     *  \note This base method must be called from derived method.
     */
    virtual void  serviceRegistered( const QString& serviceName);

    //! Service registration error callback
    /*! Can be derived for special notifying.
     *  \param[in] code is the error code.
     *  \note This base method must be called from derived method.
     */
    virtual void  serviceRegistrationError( int code);
    //! \endcond

private:
    void  init();
};

MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnDiscoverAdvertise::State)

#endif // ARNDISCOVER_HPP
