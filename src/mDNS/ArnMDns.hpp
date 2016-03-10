// Copyright (C) 2010-2016 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. Usage of these other libraries is subject to their respective
// license agreements.
// This file must also be used in compliance with Apache License, Version 2.0 (see below).
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE_LGPL.txt included in the packaging of this file.
// In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General
// Public License version 3.0 as published by the Free Software Foundation
// and appearing in the file LICENSE_GPL.txt included in the packaging of this file.
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
// This file contain modified code, originating from:
// Copyright (c) 2004 Apple Computer, Inc. All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//

#ifndef ARNMDNS_HPP
#define ARNMDNS_HPP

#include "mDNS/mDNSCore/mDNSEmbeddedAPI.h"
#include "mDNS/mDNSQt/mDNSQt.h"
#include <QTimer>
#include <QSocketNotifier>
#include <QObject>
#include <QMap>

class QUdpSocket;
class QHostAddress;


class ArnMDnsSockInfo : public QObject
{
    Q_OBJECT
public:
    ArnMDnsSockInfo( QObject* parent = 0);

    QUdpSocket*  _udpSocket;
    PosixNetworkInterface*  _mDnsInfo;
    int  _interfaceIndex;
    bool  _isUnicast;
};


class ArnMDns : public QObject
{
    Q_OBJECT
public:
    static void  attach();
    static void  detach();

    static ArnMDnsSockInfo*  addSocket();
    static void  addSocketInfo( ArnMDnsSockInfo* mdi);
    static ArnMDnsSockInfo*  socketInfo( int sd);
    static void  bindMDnsInfo( mDNS*  mdns);

    static bool  toMDNSAddr( const QHostAddress& ipAdr, mDNSAddr& mdnsAdr);
    static void  toMDNSPort( quint16 ipPort, mDNSIPPort& mdnsPort);
    static QHostAddress  fromMDNSAddr( const mDNSAddr& mdnsAdr);
    static quint16  fromMDNSPort( const mDNSIPPort& mdnsPort);

signals:
    
private slots:
    void  poll();
    void  socketDataReady();

private:
    explicit ArnMDns( QObject* parent = 0);
    ~ArnMDns();
    int  setup();
    void  close();
    void  socketDataProc( mDNS *const m, PosixNetworkInterface *intf, ArnMDnsSockInfo* mdi);

    static ArnMDns*  _self;
    static int  _refCount;
    mDNS_PlatformSupport _platformSupport;

    QTimer  _pollTimer;
    QMap<int,ArnMDnsSockInfo*>  _sockInfoMap;
    int _nfds;
    bool  _pollInit;
    bool  _started;

    int _numRegisteredInterfaces;
    int _numPktsAccepted;
    int _numPktsRejected;
};

#endif // ARNMDNS_HPP
