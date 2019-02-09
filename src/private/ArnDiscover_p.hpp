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

#ifndef ARNDISCOVER_P_HPP
#define ARNDISCOVER_P_HPP

#include "ArnInc/ArnDiscover.hpp"

class ArnZeroConfRegister;
class ArnZeroConfBrowser;
class QHostInfo;


class ArnDiscoverInfoPrivate
{
    friend class ArnDiscoverInfo;
    friend class ArnDiscoverBrowserB;
public:
    ArnDiscoverInfoPrivate();
    virtual  ~ArnDiscoverInfoPrivate();

private:
    int  _id;
    ArnDiscoverInfo::State  _state;
    ArnDiscoverInfo::State  _stopState;
    ArnDiscover::Type  _type;
    QString  _serviceName;
    QString  _domain;
    QString  _hostName;
    quint16  _hostPort;
    QHostAddress  _hostIp;
    Arn::XStringMap  _properties;
    int  _resolvCode;
};


class ArnDiscoverBrowserBPrivate
{
    friend class ArnDiscoverBrowserB;
public:
    ArnDiscoverBrowserBPrivate();
    virtual  ~ArnDiscoverBrowserBPrivate();

private:    
    ArnZeroConfBrowser*  _serviceBrowser;
    QList<int>  _activeServIds;
    QList<ArnDiscoverInfo>  _activeServInfos;
    QString  _filter;
    ArnDiscoverInfo::State  _defaultStopState;
};


class ArnDiscoverResolverPrivate : public ArnDiscoverBrowserBPrivate
{
    friend class ArnDiscoverResolver;
public:
    ArnDiscoverResolverPrivate();
    virtual  ~ArnDiscoverResolverPrivate();

private:
    QString  _defaultService;
};


class ArnDiscoverAdvertisePrivate
{
    friend class ArnDiscoverAdvertise;
public:
    ArnDiscoverAdvertisePrivate();
    virtual  ~ArnDiscoverAdvertisePrivate();

private:    
    ArnZeroConfRegister*  _arnZCReg;
    QString  _service;
    QStringList  _groups;
    Arn::XStringMap  _customProperties;
    bool  _hasSetupAdvertise;
    ArnDiscover::Type  _discoverType;
};

#endif // ARNDISCOVER_P_HPP
