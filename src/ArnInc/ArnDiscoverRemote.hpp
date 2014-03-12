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
#include "ArnDiscoverConnect.hpp"
#include "ArnItem.hpp"

class ArnServer;
class QTimer;
class QTime;



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
