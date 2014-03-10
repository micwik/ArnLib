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

#ifndef ARNDISCOVERREMOTE_HPP
#define ARNDISCOVERREMOTE_HPP

#include "ArnDiscover.hpp"
#include "ArnItem.hpp"

class ArnServer;
class ArnClient;
class QTimer;
class QTime;


//! An Automatic discover connector.
/*!
[About Discover Remote](\ref gen_discoverRemote)

tbd

<b>Example usage</b> \n \code
tbd
\endcode
*/
class ArnDiscoverConnector : public QObject
{
    Q_OBJECT
public:
    ArnDiscoverConnector( ArnClient& client, const QString& id);

    //! Clear the DirectHost connection list
    /*! Typically used to start making a new connection list.
     *  \see addToDirectHosts()
     */
    void  clearDirectHosts();

    //! Add an _Arn Server_ to the DirectHost connection list
    /*! \param[in] arnHost is host name or ip address, e.g. "192.168.1.1".
     *  \param[in] port is the port number (default 2022).
     *  \see clearDirectHosts()
     */
    void  addToDirectHosts( const QString& arnHost, quint16 port = 0);

    //! Set the ArnDiscoverResolver to be used
    /*! Ownership is taken of this resolver.
     *  Any previos set resolver will be deleted.
     *  \param[in] resolver is the used ArnDiscoverResolver. Use 0 (null) to set none.
     */
    void  setResolver( ArnDiscoverResolver* resolver);
    void  start();

    QString  id()  const;
    QString  service()  const;

    int  directHostPrio()  const;
    void  setDirectHostPrio( int directHostPrio);

    int  resolvHostPrio()  const;
    void  setResolvHostPrio( int resolvHostPrio);

    int  resolveRefreshTimeout()  const;
    void  setResolveRefreshTimeout( int resolveRefreshTimeout);

    bool  externalClientConnect()  const;
    void  setExternalClientConnect( bool externalClientConnect);

public slots:
    void  setService( QString service);

signals:
    void  clientReadyToConnect( ArnClient* arnClient, const QString& id);

private slots:
    void  doClientConnectChanged( int stat, int curPrio);
    void  doClientConnectRequest( int reqCode);
    //// Handle Client directHosts
    void  postSetupClient();
    void  doClientConnectChange( QString arnHost, quint16 port);
    void  doClientDirHostChanged();
    //// Handle Client resolvHost
    void  postSetupResolver();
    void  doClientServicetChanged();
    void  doClientResolvChanged( int index, ArnDiscoverInfo::State state);

private:
    void  doClientReadyToConnect( ArnClient* arnClient, const QString& id);

    ArnClient*  _client;
    ArnDiscoverResolver*  _resolver;
    QString  _id;
    QString  _service;
    int  _directHostPrio;
    int  _discoverHostPrio;
    int  _resolveRefreshTimeout;
    QObject*  _directHosts;
    QTime*  _resolveRefreshTime;
    bool  _resolveRefreshBlocked;
    bool  _isResolved;
    bool  _externalClientConnect;
    bool  _hasBeenSetupClient;

    ArnItem*  _arnDisHostService;
    ArnItem*  _arnDisHostServicePv;
    ArnItem*  _arnDisHostAddress;
    ArnItem*  _arnDisHostPort;
    ArnItem*  _arnDisHostStatus;
};


//! Discover with remote setting.
/*!
This class handles is the main class for handling discover with remote setting.

Following is met:

* If service is set before start using server, this service will be used.
* If no persist is active or it gives an empty service name, timeout-processing is done.
* Timeout-processing can wait upto initialServiceTimeout(), after that defaultService()
  will be used as service.
* If service is set by any method before timeout-processing has finnished that service
  is used. Timeout-processing is then also aborted.
* After initial advertise of the service, it can be changed by any method and the changed
  changed service will be used.
* The used service will also be saved if using persist.
* Methods to change service are ArnDiscoverAdvertise::setService() and corresponding
  _arnObject_ which can be changed locally or remote.

<b>Example usage</b> \n \code
\endcode
*/
class ArnDiscoverRemote : public ArnDiscoverAdvertise
{
    Q_OBJECT
public:
    explicit ArnDiscoverRemote( QObject *parent = 0);

    QString  defaultService()  const;
    void  setDefaultService( const QString& defaultService);

    int  initialServiceTimeout()  const;
    void  setInitialServiceTimeout( int initialServiceTimeout);

    void  startUseServer( ArnServer* arnServer, ArnDiscover::Type discoverType = ArnDiscover::Type::Server);
    void  startUseNewServer( ArnDiscover::Type discoverType, int port = -1);
    ArnDiscoverConnector*  newConnector( ArnClient& client, const QString& id);

signals:
    void  clientReadyToConnect( ArnClient* arnClient, const QString& id);

public slots:
    virtual void  setService( QString service);

protected:
    //// Handle Service This
    virtual void  postSetupThis();
    virtual void  serviceRegistered( QString serviceName);

private slots:
    //// Handle Service This
    void  serviceTimeout();
    void  firstServiceSetup( QString serviceName, bool forceSetup = false);
    void  doServiceChanged( QString val);

private:
    ArnServer*  _arnInternalServer;
    ArnDiscoverResolver*  _arnDResolver;
    ArnItem  _arnServicePv;
    ArnItem  _arnService;
    QTimer*  _servTimer;
    QString  _defaultService;
    int  _initialServiceTimeout;
};

#endif // ARNDISCOVERREMOTE_HPP
