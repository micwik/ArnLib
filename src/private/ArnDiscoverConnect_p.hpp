// Copyright (C) 2010-2016 Michael Wiklund.
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

#ifndef ARNDISCOVERCONNECT_P_HPP
#define ARNDISCOVERCONNECT_P_HPP

#include "ArnInc/ArnDiscoverConnect.hpp"

class ArnClient;
class QTimer;
class QTime;


class ArnDiscoverConnectorPrivate
{
    friend class ArnDiscoverConnector;
public:
    ArnDiscoverConnectorPrivate();
    virtual  ~ArnDiscoverConnectorPrivate();

private:
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

#endif // ARNDISCOVER_P_HPP
