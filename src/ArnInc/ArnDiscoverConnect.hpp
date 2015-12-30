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

#ifndef ARNDISCOVERCONNECT_HPP
#define ARNDISCOVERCONNECT_HPP

#include "ArnDiscover.hpp"
#include "ArnItem.hpp"

class ArnDiscoverConnectorPrivate;
class ArnClient;
class QTimer;
class QTime;


//! An automatic client discover connector.
/*!
[About Arn Discover Remote](\ref gen_discoverRemote)

This connector class manages client connections. Both as a list of possible _direct host_
addresses and using a service name for reolving into a _discover host_. The two methods can
coexist and as standard the _discover host_ has lowest priority number, i.e. tried first.

An _id_ is assigned to every connector. The _id_ should be chosen to describe the client
target or its purpose. It's not a host address or necessarily a specific host, as there
can be many possible addresses assigned to the ArnDiscoverConnector.

The _id_ will appear as an _Arn folder_, e.g. when _id_ is "WeatherData-XYZ" the
_connector folder path_ will be "Sys/Discover/Connect/WeatherData-XYZ/".

<b>Example usage</b> \n \code
    // In class declare
    ArnDiscoverConnector*  _connector
    ArnClient  _arnClient;

    // In class code
    _arnClient.addMountPoint("//");
    _arnClient.setAutoConnect(true);

    _connector = new ArnDiscoverConnector( _arnClient, "MyConnectionId");
    _connector->setResolver( new ArnDiscoverResolver());
    _connector->setService("My Service");
    _connector->addToDirectHosts("localhost");
    _connector->start();
\endcode
*/
class ArnDiscoverConnector : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnDiscoverConnector)

public:
    ArnDiscoverConnector( ArnClient& client, const QString& id);
    ~ArnDiscoverConnector();

    //! Clear the _direct host_ connection list
    /*! Typically used to start making a new connection list.
     *  \see addToDirectHosts()
     *  \see ArnClient
     */
    void  clearDirectHosts();

    //! Add an _Arn Server_ to the _direct host_ connection list
    /*! \param[in] arnHost is host name or ip address, e.g. "192.168.1.1".
     *  \param[in] port is the host port, 0 gives Arn::defaultTcpPort.
     *  \see clearDirectHosts()
     *  \see ArnClient
     */
    void  addToDirectHosts( const QString& arnHost, quint16 port = 0);

    //! Set the ArnDiscoverResolver to be used
    /*! The resolver handles resolving a known service name into a host name.
     *
     *  Ownership is taken of this resolver.
     *  Any previos set resolver will be deleted.
     *  \param[in] resolver is the used ArnDiscoverResolver. Use 0 (null) to set none.
     */
    void  setResolver( ArnDiscoverResolver* resolver);

    //! Start connector
    /*! \see addToDirectHosts()
     *  \see setResolver()
     */
    void  start();

    //! Return the identifier for this connector
    /*! \return the identifier, e.g "WeatherData-XYZ"
     *  \see ArnDiscoverRemote::newConnector()
     */
    QString  id()  const;

    //! Returns the service name for this connection
    /*! \return service name, e.g. "My House Registry"
     *  \see setService()
     */
    QString  service()  const;

    //! Return the priority for _direct hosts_
    /*! \return direct host priority
     *  \see setDirectHostPrio()
     */
    int  directHostPrio()  const;

    //! Set the priority for _direct hosts_
    /*! This priority controls order between _direct hosts_ and _discover host_.
     *  Low priority number give earlier try for its hosts.
     *  \param[in] directHostPrio is the priority.
     *  \note The priority for _direct hosts_ and _discover hosts_ must be different.
     *  \see directHostPrio()
     */
    void  setDirectHostPrio( int directHostPrio);

    //! Return the priority for _discovered hosts_
    /*! \return discoverHostPrio is the priority.
     *  \see setDiscoverHostPrio()
     */
    int  discoverHostPrio()  const;

    //! Set the priority for _discovered hosts_
    /*! This priority controls order between _direct hosts_ and _discover host_.
     *  Low priority number give earlier try for its hosts.
     *  \param[in] discoverHostPrio is the priority.
     *  \note The priority for _direct hosts_ and _discover hosts_ must be different.
     *  \see discoverHostPrio()
     */
    void  setDiscoverHostPrio( int discoverHostPrio);

    //! Return the resolv refresh period
    /*! \return _resolve refresh timeout_ in seconds.
     *  \see setResolveRefreshTimeout()
     */
    int  resolveRefreshTimeout()  const;

    //! Set the resolv refresh period
    /*! The refresh period is used when there is a failure to connect to a _discover host_.
     *
     *  The rationale is that the current resolv might be outdated as there is an error
     *  when connecting to the resolved host. A refreshed resolv will be done at an
     *  intervall of _resolveRefreshTimeout_ until connection to resolved host is successful.
     *  \param[in] resolveRefreshTimeout is the period in seconds.
     *  \see resolveRefreshTimeout()
     */
    void  setResolveRefreshTimeout( int resolveRefreshTimeout);

    //! Return the _external client connect_ mode
    /*! \return true when active.
     *  \see setExternalClientConnect()
     */
    bool  externalClientConnect()  const;

    //! Set the _external client connect_ mode
    /*! This mode is used when there is a need to do special processing when connecting a
     *  client. Then QObject::connect() should be used for the signal clientReadyToConnect()
     *  and a _receiver_ doing the special processing.
     *
     *  It's the responsibility of the _receiver_ to do the actual client connect by
     *  ArnClient::connectToArnList().
     *  \param[in] externalClientConnect true to activate.
     *  \see externalClientConnect()
     */
    void  setExternalClientConnect( bool externalClientConnect);

public slots:
    //! Set the service name for the connection
    /*! This is only functional if using ArnDiscoverResolver, see setResolver().
     *
     *  Will update connection service name if the resolver has been setup, otherwise the
     *  service name is only stored for future use.
     *
     *  For remote control the service name is also available as an _Arn Data Object_ at
     *  [local path](\ref gen_localPath): _connector folder path_ + "Service/value",
     *  e.g. "Sys/Discover/Connect/WeatherData-XYZ/Service/value".
     *  \param[in] service is the requested connection service name e.g. "My House Registry"
     *  \see ArnDiscoverAdvertise::setService()
     */
    void  setService( const QString& service);

signals:
    //! Signal for external client connection
    /*! When activated external client connection by the method setExternalClientConnect(),
     *  this signal will be emitted when the client has been prepared to connect.
     *
     *  It's the responsibility of the receiver to do the actual client connect by
     *  ArnClient::connectToArnList().
     *  \param[in] arnClient being ready for connection
     *  \param[in] id is the identifier used in ArnDiscoverRemote::newConnector(),
     *                e.g "WeatherData-XYZ"
     *  \see ArnDiscoverRemote::newConnector()
     *  \see setExternalClientConnect()
     */
    void  clientReadyToConnect( ArnClient* arnClient, const QString& id);

    //! \cond ADV
protected:
    ArnDiscoverConnector( ArnDiscoverConnectorPrivate& dd, QObject* parent,
                          ArnClient& client, const QString& id);
    ArnDiscoverConnectorPrivate* const  d_ptr;
    //! \endcond

private slots:
    void  doClientConnectChanged( int stat, int curPrio);
    void  doClientConnectRequest( int reqCode);
    //// Handle Client directHosts
    void  postSetupClient();
    void  doClientConnectChange( const QString& arnHost, quint16 port);
    void  doClientDirHostChanged();
    //// Handle Client resolvHost
    void  postSetupResolver();
    void  doClientServicetChanged();
    void  doClientResolvChanged( int index, ArnDiscoverInfo::State state);

private:
    void  doClientReadyToConnect( ArnClient* arnClient, const QString& id);
};


#endif // ARNDISCOVERCONNECT_HPP
