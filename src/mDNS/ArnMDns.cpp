// Copyright (C) 2010-2014 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these other
// libraries is subject to their respective license agreements.
// This file must also be used in compliance with Apache License, Version 2.0 (see below).
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
// This file contain modified code, originating from:
// Copyright (c) 2004 Apple Computer, Inc. All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//

#include "ArnMDns.hpp"
#include "../ArnInc/ArnLib.hpp"
#include <QEventLoop>
#include <QSocketNotifier>
#include <QUdpSocket>
#include <QNetworkAddressEntry>
#include <QMapIterator>
#include <QDebug>


ArnMDnsSockInfo::ArnMDnsSockInfo( QObject* parent)
    : QObject( parent)
{
    _udpSocket      = 0;
    _interfaceIndex = -1;
    _isUnicast      = false;
    _mDnsInfo       = 0;
}


ArnMDns*  ArnMDns::_self(0);
int  ArnMDns::_refCount(0);

mDNS mDNSStorage;
const char ProgramName[] = "ProgramName???";  // Not realy used

// Start off with a default cache of 16K (about 100 records)
#define RR_CACHE_SIZE ((16*1024) / sizeof(CacheRecord))
static CacheEntity rrcachestorage[RR_CACHE_SIZE];

/*  mw: TODO fix additional mem alloc
mDNSlocal void mDNS_StatusCallback(mDNS *const m, mStatus result)
{
    if (result == mStatus_GrowCache)
        {
        // Allocate another chunk of cache storage
        //CacheEntity *storage = OTAllocMem(sizeof(CacheEntity) * RR_CACHE_SIZE);
        CacheEntity *storage = (CacheEntity*) mDNSPlatformMemAllocate(sizeof(CacheEntity) * RR_CACHE_SIZE);
        if (storage) mDNS_GrowCache(m, storage, RR_CACHE_SIZE);
        }
}
*/


ArnMDns::ArnMDns(QObject *parent) :
    QObject(parent)
{
    _pollInit = true;
    _started  = false;

    _numRegisteredInterfaces = 0;
    _numPktsAccepted = 0;
    _numPktsRejected = 0;

    connect( &_pollTimer, SIGNAL(timeout()), this, SLOT(poll()));
}


ArnMDns::~ArnMDns()
{
    close();
}


int  ArnMDns::setup()
{
    mStatus  err;

    mDNSPlatformMemZero( &mDNSStorage, sizeof(mDNSStorage));
    mDNSPlatformMemZero( &_platformSupport, sizeof(_platformSupport));

    err = mDNS_Init(
        &mDNSStorage,
        &_platformSupport,
        rrcachestorage,
        RR_CACHE_SIZE,
        mDNS_Init_AdvertiseLocalAddresses,
        mDNS_Init_NoInitCallback,
        mDNS_Init_NoInitCallbackContext
    );
    if (err)
        return err;

    _pollTimer.start(100);
    _started = true;
    return 0;
}


void  ArnMDns::close()
{
    if (Arn::debugZeroConf)  qDebug() << "ArmDns close: refcount=" << _refCount
                                      << " Started=" << _started;
    if (_started) {
        mDNS_StartExit( &mDNSStorage);
        
        QEventLoop  loop;
        QTimer::singleShot( 500, &loop, SLOT(quit()));  // Min 100ms
        loop.exec( QEventLoop::ExcludeUserInputEvents);

        _pollTimer.stop();
        QMapIterator<int,ArnMDnsSockInfo*>  i(_sockInfoMap);
        while (i.hasNext()) {
            i.next();
            ArnMDnsSockInfo*  mdi = i.value();
            delete mdi;
        }
        //mDNS_Close( &mDNSStorage);
         mDNS_FinalExit( &mDNSStorage);
    }
    _started = false;
}


void ArnMDns::attach()
{
    if (_refCount == 0) {
        _self = new ArnMDns;
        _self->setup();
    }
    ++_refCount;
    if (Arn::debugZeroConf)  qDebug() << "ArmDns ref++: count=" << _refCount;
}


void ArnMDns::detach()
{
    --_refCount;
    if (Arn::debugZeroConf)  qDebug() << "ArmDns ref--: count=" << _refCount;
    if (_refCount == 0) {
        _self->close();
        _self->deleteLater();
        _self = 0;
    }
}


ArnMDnsSockInfo*  ArnMDns::addSocket()
{
    ArnMDnsSockInfo*  mdi = new ArnMDnsSockInfo( _self);
    QUdpSocket*  sock = new QUdpSocket( mdi);
    mdi->_udpSocket = sock;
    connect( sock, SIGNAL(readyRead()), _self, SLOT(socketDataReady()));

    return mdi;
}


void  ArnMDns::addSocketInfo( ArnMDnsSockInfo* mdi)
{
    if (!mdi)  return;

    int  sd = mdi->_udpSocket->socketDescriptor();
    if (sd < 0)  return;

    _self->_sockInfoMap[ sd] = mdi;
}


ArnMDnsSockInfo*  ArnMDns::socketInfo( int sd)
{
    return _self->_sockInfoMap.value( sd, 0);
}


void  ArnMDns::bindMDnsInfo( mDNS* mdns)
{
    PosixNetworkInterface* info = (PosixNetworkInterface*) (mdns->HostInterfaces);

    while (info) {
        if (info->multicastSocket4 != -1) {
            ArnMDnsSockInfo*  mdi = socketInfo( info->multicastSocket4);
            if (mdi)
                mdi->_mDnsInfo = info;
        }
#if HAVE_IPV6
        if (info->multicastSocket6 != -1) {
            ArnMDnsSockInfo*  mdi = socketInfo( info->multicastSocket6);
            if (mdi)
                mdi->_mDnsInfo = info;
        }
#endif
        info = (PosixNetworkInterface*) (info->coreIntf.next);
    }
}


bool  ArnMDns::toMDNSAddr( const QHostAddress& ipAdr, mDNSAddr& mdnsAdr)
{
    switch (ipAdr.protocol()) {
    case QAbstractSocket::IPv4Protocol: {
        mdnsAdr.type = mDNSAddrType_IPv4;
        quint32 ha = ipAdr.toIPv4Address();
        for (int i = 3; i >= 0; --i) {
            mdnsAdr.ip.v4.b[i] = ha & 0xff;
            ha >>= 8;
        }
        break;
    }
    case QAbstractSocket::IPv6Protocol: {
        mdnsAdr.type = mDNSAddrType_IPv6;
        Q_IPV6ADDR ha = ipAdr.toIPv6Address();
        for (int i = 0; i < 16; ++i) {
            mdnsAdr.ip.v6.b[i] = ha[i];
        }
        break;
    }
    default:
        qWarning() << "ArnMDns toMDNSAddr: unknown protocol=" << ipAdr.protocol();
        return false;
    }

    return true;
}


void  ArnMDns::toMDNSPort( quint16 ipPort, mDNSIPPort& mdnsPort)
{
    mdnsPort.b[0] = ipPort >> 8;
    mdnsPort.b[1] = ipPort & 0xff;
}


QHostAddress  ArnMDns::fromMDNSAddr(const mDNSAddr& mdnsAdr)
{
    QHostAddress  retVal;

    switch (mdnsAdr.type) {
    case mDNSAddrType_IPv4: {
        quint32 ha = 0;
        for (int i = 0; i < 4; ++i) {
            ha <<= 8;
            ha |= mdnsAdr.ip.v4.b[i] & 0xff;
        }
        retVal.setAddress( ha);
        break;
    }
    case mDNSAddrType_IPv6: {
        Q_IPV6ADDR ha;
        for (int i = 0; i < 16; ++i) {
            ha[i] = mdnsAdr.ip.v6.b[i];
        }
        retVal.setAddress( ha);
        break;
    }
    default:
        qWarning() << "ArnMDns fromMDNSAddr: unknown protocol=" << mdnsAdr.type;
        break;
    }

    return retVal;
}


quint16  ArnMDns::fromMDNSPort( const mDNSIPPort& mdnsPort)
{
    quint16  retVal;

    retVal  = (mdnsPort.b[0] & 0xff) << 8;
    retVal |= (mdnsPort.b[1] & 0xff);

    return retVal;
}


void  ArnMDns::poll()
{
    mDNSs32 nextevent = mDNS_Execute( &mDNSStorage);

    mDNSs32 ticks = nextevent - mDNS_TimeNow( &mDNSStorage);
    if (ticks < 1) ticks = 1;
    int  sec  = ticks >> 10;                    // The high 22 bits are seconds
    int  usec = ((ticks & 0x3FF) * 15625) / 16;	// The low 10 bits are 1024ths
    int  msec = qMin( sec * 1000 + usec / 1000, 200);
    _pollTimer.setInterval( msec);
}


/// This routine is called when the main loop detects that data is available on a socket.
void  ArnMDns::socketDataProc( mDNS *const m, PosixNetworkInterface *intf, ArnMDnsSockInfo* mdi)
    {
    mDNSAddr   senderAddr, destAddr;
    mDNSIPPort senderPort;
    ssize_t                 packetLen;
    DNSMessage              packet;
    //struct my_in_pktinfo    packetInfo;
    mDNSBool                reject;
    const mDNSInterfaceID InterfaceID = intf ? intf->coreIntf.InterfaceID : NULL;

    Q_ASSERT(m);

    // miwi: TODO packet info destAddr, interface
    // packetLen = recvfrom_flags(skt, &packet, sizeof(packet), &flags, (struct sockaddr *) &from, &fromLen, &packetInfo, &ttl);
    // SockAddrTomDNSAddr((struct sockaddr*)&packetInfo.ipi_addr, &destAddr, NULL);
    // int packetInterface = packetInfo.ipi_ifindex;
    QHostAddress  senderIpAdr;
    quint16 senderIpPort;
    packetLen = mdi->_udpSocket->readDatagram( (char*) &packet, sizeof(packet),
                                               &senderIpAdr, &senderIpPort);

    if (packetLen >= 0) {
        toMDNSAddr( senderIpAdr, senderAddr);
        toMDNSPort( senderIpPort, senderPort);
        int packetInterface = intf->index;

        // If this platform doesn't have IP_PKTINFO or IP_RECVDSTADDR, then we have
        // no way to tell the destination address or interface this packet arrived on,
        // so all we can do is just assume it's a multicast

        //#if HAVE_BROKEN_RECVDSTADDR || (!defined(IP_PKTINFO) && !defined(IP_RECVDSTADDR))
            //if ((destAddr.NotAnInteger == 0) && (flags & MSG_MCAST))
                {
                destAddr.type = senderAddr.type;
                if      (senderAddr.type == mDNSAddrType_IPv4) destAddr.ip.v4 = AllDNSLinkGroup_v4.ip.v4;
                else if (senderAddr.type == mDNSAddrType_IPv6) destAddr.ip.v6 = AllDNSLinkGroup_v6.ip.v6;
                }
        //#endif

        // We only accept the packet if the interface on which it came
        // in matches the interface associated with this socket.
        // We do this match by name or by index, depending on which
        // information is available.  recvfrom_flags sets the name
        // to "" if the name isn't available, or the index to -1
        // if the index is available.  This accomodates the various
        // different capabilities of our target platforms.

        reject = mDNSfalse;
        if (!intf) {
            // Ignore multicasts accidentally delivered to our unicast receiving socket
            if (mDNSAddrIsDNSMulticast(&destAddr))
                packetLen = -1;
        }
        else {
            // if      (packetInfo.ipi_ifname[0] != 0) reject = (strcmp(packetInfo.ipi_ifname, intf->intfName) != 0);
            // else if (packetInfo.ipi_ifindex != -1)  reject = (packetInfo.ipi_ifindex != intf->index);
            if (packetInterface != -1)
                reject = packetInterface != intf->index;  // Fake test

            if (reject) {
                // verbosedebugf("SocketDataReady ignored a packet from %#a to %#a on interface %s/%d expecting %#a/%s/%d/%d",
                //    &senderAddr, &destAddr, packetInfo.ipi_ifname, packetInfo.ipi_ifindex,
                //    &intf->coreIntf.ip, intf->intfName, intf->index, skt);
                packetLen = -1;
                _numPktsRejected++;
                if (_numPktsRejected > (_numPktsAccepted + 1) * (_numRegisteredInterfaces + 1) * 2) {
                    qWarning() << "*** WARNING: Received" << _numPktsAccepted + _numPktsRejected
                               << " packets; Accepted " << _numPktsAccepted
                               << " packets; Rejected " << _numPktsRejected
                               << " packets because of interface mismatch.";
                    _numPktsAccepted = 0;
                    _numPktsRejected = 0;
                }
            }
            else {
                // verbosedebugf("SocketDataReady got a packet from %#a to %#a on interface %#a/%s/%d/%d",
                //     &senderAddr, &destAddr, &intf->coreIntf.ip, intf->intfName, intf->index, skt);
                _numPktsAccepted++;
            }
        }
    }

    if (packetLen >= 0)
        mDNSCoreReceive(m, &packet, (mDNSu8 *)&packet + packetLen,
                        &senderAddr, senderPort, &destAddr, MulticastDNSPort, InterfaceID);
}


void ArnMDns::socketDataReady()
{
    QUdpSocket*  udpSocket = qobject_cast<QUdpSocket*>(sender());
    Q_ASSERT(udpSocket);
    ArnMDnsSockInfo*  mdi = qobject_cast<ArnMDnsSockInfo*>(udpSocket->parent());
    Q_ASSERT(mdi);

    int sd = udpSocket->socketDescriptor();

    mDNS*  m = &mDNSStorage;

    if (mdi->_isUnicast) {
        socketDataProc( m, NULL, mdi);
    }
    else if (mdi->_mDnsInfo) {
        socketDataProc( m, mdi->_mDnsInfo, mdi);
    }
    else {
        qWarning() << "ArnMDNS received data from unexpected socket: sd=" << sd;
    }
}
