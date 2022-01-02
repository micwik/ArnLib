// Copyright (C) 2010-2022 Michael Wiklund.
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

#ifndef ARNDISCOVERREMOTE_HPP
#define ARNDISCOVERREMOTE_HPP

#include "ArnDiscover.hpp"
#include "ArnDiscoverConnect.hpp"
#include "ArnItem.hpp"

class ArnDiscoverRemotePrivate;
class ArnServer;
class QTimer;
class QTime;



//! Discover with remote setting.
/*!
[About Arn Discover Remote](\ref gen_discoverRemote)

This class is the main class for handling discover with remote setting.

Following rules apply:

* * If service is set before start using server, this service will be used.
* * If no persist is active or it gives an empty service name, timeout-processing is done.
* * Timeout-processing can wait upto initialServiceTimeout(), after that defaultService()
    will be used as service.
* * If service is set by any method before timeout-processing has finnished, that service
    is used. Timeout-processing is then also aborted.
* * After initial advertise of the service, it can be changed by any method and the changed
    service will be used.
* * The used service will also be saved if using persist.
* * Methods to change service are ArnDiscoverRemote::setService() and corresponding
    _Arn Data Objects_ which can be changed locally or remote.

For a complete example of advertisng a server, see the project ArnServer in ServerMain.hpp
and ServerMain.cpp files.

<b>Example usage</b> \n \code
    // In class declare
    ArnDiscoverRemote*  _discoverRemote;
    ArnClient*  _client;

    // In class code
    _client = new ArnClient;
    _client->addMountPoint("//");
    _client->setAutoConnect( true);

    _discoverRemote = new ArnDiscoverRemote( this);
    _discoverRemote->setDefaultService("My default service");
    _discoverRemote->addGroup("myId/myProduct");
    _discoverRemote->addCustomProperty("MyProtoVer", "1.0");
    _discoverRemote->startUseNewServer( ArnDiscover::Type::Client, 0);  // Dynamic server

    ArnDiscoverConnector*  connector = _discoverRemote->newConnector( *_client, "House");
    connector->setResolver( new ArnDiscoverResolver());
    connector->start();

    ArnPersist*  persist = new ArnPersist( this);
    persist->setupDataBase();
    persist->setMountPoint( Arn::pathLocal);
\endcode
*/
class ArnDiscoverRemote : public ArnDiscoverAdvertise
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnDiscoverRemote)

public:
    explicit ArnDiscoverRemote( QObject *parent = 0);
    ~ArnDiscoverRemote();

    //! Return the default service name
    /*! \return default service name, e.g. "Arn Default Service"
     *  \see setDefaultService()
     */
    QString  defaultService()  const;

    //! Set the default service name
    /*! This default service name will be used when no service has been set before timeout.
     *  If calling with _defaultService_ empty, it's ignored.
     *  \param[in] defaultService e.g. "My Default Service"
     *  \see defaultService()
     */
    void  setDefaultService( const QString& defaultService);

    //! Return the time for initial timeout processing
    /*! \return time in seconds
     *  \see setInitialServiceTimeout()
     */
    int  initialServiceTimeout()  const;

    //! Set the time for initial timeout processing
    /*! Initial timeout-processing can wait upto this time, after that defaultService()
     *  will be used as service.
     *  \param[in] initialServiceTimeout in seconds
     *  \see initialServiceTimeout()
     */
    void  setInitialServiceTimeout( int initialServiceTimeout);

    //! Start advertising the ArnServer as a service
    /*! Handle advertising of an existing ArnServer as a service on the local network.
     *  Everything is fully automatic, including remote setting service name and support
     *  for persistent storage of the name. Status can be accessed via _Arn Data Objects_.
     *  \param[in] arnServer is the ArnServer to be advertised
     *  \param[in] discoverType is used for discover filtering
     *  \see setService()
     *  \see setDefaultService()
     *  \see startUseNewServer()
     */
    void  startUseServer( ArnServer* arnServer, ArnDiscover::Type discoverType = ArnDiscover::Type::Server);

    //! Start a new ArnServer and advertise as a service
    /*! Handle advertising an internally created ArnServer as a service on the local
     *  network.
     *
     *  This method is typically used when there is no need to access the ArnServer class,
     *  which usually is the case in an client application. The ArnServer is then merely
     *  used to make the discover functionality remote controlled.
     *
     *  All the functionaly from startUseServer() do apply.
     *  \param[in] discoverType is used for discover filtering
     *  \param[in] port is the port of the service,
     *                  -1 gives Arn::defaultTcpPort, 0 gives [dynamic port](\ref gen_dynamicPort)
     *  \see setService()
     *  \see setDefaultService()
     *  \see startUseServer()
     */
    void  startUseNewServer( ArnDiscover::Type discoverType, int port = -1);

    //! Create and return an ArnDiscoverConnector for handling remote client
    /*! The ArnDiscoverConnector is internally connected to this ArnDiscoverRemote.
     *
     *  The _id_ should be chosen to describe the client target or its purpose. It's not
     *  a host address or necessarily a specific host, as there can be many possible
     *  addresses assigned to the ArnDiscoverConnector.
     *
     *  The _id_ will appear as an _Arn folder_, e.g. when _id_ is "WeatherData-XYZ" the
     *  folder path will be "Sys/Discover/Connect/WeatherData-XYZ/".
     *  \param[in] client
     *  \param[in] id identifies the target of the client connection, e.g "WeatherData-XYZ"
     *  \return The ArnDiscoverConnector
     */
    ArnDiscoverConnector*  newConnector( ArnClient& client, const QString& id);

signals:
    //! Central signal for external client connection
    /*! When activated external client connection by the connector method
     *  ArnDiscoverConnector::setExternalClientConnect(), this signal will be emitted
     *  when the client has been prepared to connect.
     *
     *  It's the responsibility of the receiver to do the actual client connect by
     *  ArnClient::connectToArnList().
     *  \param[in] arnClient being ready for connection
     *  \param[in] id is the identifier used in newConnector(), e.g "WeatherData-XYZ"
     *  \see newConnector()
     *  \see ArnDiscoverConnector::setExternalClientConnect()
     */
    void  clientReadyToConnect( ArnClient* arnClient, const QString& id);

public slots:
    //! Set the service name
    /*! Will update current advertised service name if this advertiser has been setup,
     *  otherwise the service name is stored for future use.
     *
     *  For remote control the service name is also available as an _Arn Data Object_ at
     *  [local path](\ref gen_localPath) "Sys/Discover/This/Service/value".
     *
     *  All the functionaly from ArnDiscoverAdvertise::setService() apply.
     *  \param[in] service is the requested service name e.g. "My House Registry"
     *  \see ArnDiscoverAdvertise::setService()
     *  \see currentService()
     *  \see advertiseService()
     */
    virtual void  setService( const QString& service);

    //! \cond ADV
protected:
    //// Handle Service This
    virtual void  postSetupThis();
    virtual void  serviceRegistered( const QString& serviceName);

    ArnDiscoverRemote( ArnDiscoverRemotePrivate& dd, QObject* parent);
    //! \endcond

private slots:
    //// Handle Service This
    void  serviceTimeout();
    void  firstServiceSetup( const QString& serviceName, bool forceSetup = false);
    void  doServiceChanged( const QString& val);

private:
    //// Hide
    void  advertiseService( ArnDiscover::Type discoverType, const QString& serviceName,
                            int port = -1, const QString& hostName = QString());
};

#endif // ARNDISCOVERREMOTE_HPP
