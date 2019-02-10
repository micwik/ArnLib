// Copyright (C) 2010-2019 Michael Wiklund.
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

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"

#include "mDNS/ArnMDns.hpp"
#include "../../ArnInc/ArnLib.hpp"
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QDateTime>
#include <QByteArray>
#include <QtGlobal>
#include <QDebug>

extern "C" {

#include "../mDNSCore/mDNSEmbeddedAPI.h"           // Defines the interface provided to the client layer above
#include "../mDNSCore/DNSCommon.h"
#include "mDNSQt.h"				 // Defines the specific types needed to run mDNS on this platform

#include <unistd.h>
//android? #include <sys/socket.h>
//android?#include <netinet/in.h>
//android?#include <arpa/inet.h>

#ifdef ANDROID
    #include <android/log.h>
#endif


// ***************************************************************************
// Structures

// ***************************************************************************
// Globals (for debugging)

static int num_registered_interfaces = 0;
static int num_pkts_accepted = 0;
static int num_pkts_rejected = 0;

// ***************************************************************************
// Functions

int gMDNSPlatformPosixVerboseLevel = 1;

#define PosixErrorToStatus(errNum) ((errNum) == 0 ? mStatus_NoError : mStatus_UnknownErr)


// mDNS core calls this routine when it needs to send a packet.
mDNSexport mStatus mDNSPlatformSendUDP(const mDNS *const m, const void *const msg,
                                       const mDNSu8 *const end, mDNSInterfaceID InterfaceID,
                                       UDPSocket *src, const mDNSAddr *dst, mDNSIPPort dstPort)
{
    int  err = 0;
    PosixNetworkInterface*  thisIntf = (PosixNetworkInterface *)(InterfaceID);
    int  sendingsocket = -1;

    (void)src;	// Will need to use this parameter once we implement mDNSPlatformUDPSocket/mDNSPlatformUDPClose

    Q_ASSERT(m);
    Q_ASSERT(msg);
    Q_ASSERT(end);
    Q_ASSERT((((const char*) end) - ((const char*) msg)) > 0);

    if (dstPort.NotAnInteger == 0) {
        if (Arn::warningMDNS)
            qWarning() << "mDNSPlatformSendUDP: Invalid argument -dstPort is set to 0";
        return mStatus_UnknownErr;
    }

    if (dst->type == mDNSAddrType_IPv4) {
        sendingsocket = thisIntf ? thisIntf->multicastSocket4 : m->p->unicastSocket4;
    }

#if HAVE_IPV6
    else if (dst->type == mDNSAddrType_IPv6) {
        sendingsocket = thisIntf ? thisIntf->multicastSocket6 : m->p->unicastSocket6;
    }
#endif

    ArnMDnsSockInfo*  mdi = 0;
    if (sendingsocket >= 0) {
        mdi = ArnMDns::socketInfo( sendingsocket);
        if (mdi) {
            QHostAddress  ipDstAdr = ArnMDns::fromMDNSAddr( *dst);
            quint16  ipDstPort = ArnMDns::fromMDNSPort( dstPort);
            qint64  msgLen = qint64((const char*)end - (const char*)msg);
            err = int(mdi->_udpSocket->writeDatagram( (const char*) msg, msgLen, ipDstAdr, ipDstPort));
        }
    }

    if ((err >= 0) || !mdi)
        return mStatus_NoError;

    static int MessageCount = 0;
    // Don't report EHOSTDOWN (i.e. ARP failure), ENETDOWN, or no route to host for unicast destinations
    if (!mDNSAddressIsAllDNSLinkGroup(dst)) {
        QAbstractSocket::SocketError err = mdi->_udpSocket->error();
        if ((err = QAbstractSocket::NetworkError) || (err = QAbstractSocket::HostNotFoundError))
            return mStatus_TransientErr;
        //if (errno == EHOSTDOWN || errno == ENETDOWN || errno == EHOSTUNREACH || errno == ENETUNREACH)
        //    return(mStatus_TransientErr);
    }

    if (MessageCount < 1000) {
        MessageCount++;
        // if (thisIntf)
        //     LogMsg("mDNSPlatformSendUDP got error %d (%s) sending packet to %#a on interface %#a/%s/%d",
        //                   errno, strerror(errno), dst, &thisIntf->coreIntf.ip, thisIntf->intfName, thisIntf->index);
        // else
        //     LogMsg("mDNSPlatformSendUDP got error %d (%s) sending packet to %#a", errno, strerror(errno), dst);
    }

    return mStatus_UnknownErr;
}


mDNSexport TCPSocket *mDNSPlatformTCPSocket(mDNS * const m, TCPSocketFlags flags, mDNSIPPort * port)
{
    (void)m;			// Unused
    (void)flags;		// Unused
    (void)port;			// Unused
    return NULL;
}


mDNSexport TCPSocket *mDNSPlatformTCPAccept(TCPSocketFlags flags, int sd)
{
    (void)flags;		// Unused
    (void)sd;			// Unused
    return NULL;
}


mDNSexport int mDNSPlatformTCPGetFD(TCPSocket *sock)
{
    (void)sock;			// Unused
    return -1;
}


mDNSexport mStatus mDNSPlatformTCPConnect(TCPSocket *sock, const mDNSAddr *dst, mDNSOpaque16 dstport, domainname *hostname, mDNSInterfaceID InterfaceID,
                                          TCPConnectionCallback callback, void *context)
{
    (void)sock;			// Unused
    (void)dst;			// Unused
    (void)dstport;		// Unused
    (void)hostname;     // Unused
    (void)InterfaceID;	// Unused
    (void)callback;		// Unused
    (void)context;		// Unused
    return(mStatus_UnsupportedErr);
}


mDNSexport void mDNSPlatformTCPCloseConnection(TCPSocket *sock)
{
    (void)sock;			// Unused
}


mDNSexport long mDNSPlatformReadTCP(TCPSocket *sock, void *buf, unsigned long buflen, mDNSBool * closed)
{
    (void)sock;			// Unused
    (void)buf;			// Unused
    (void)buflen;		// Unused
    (void)closed;		// Unused
    return 0;
}


mDNSexport long mDNSPlatformWriteTCP(TCPSocket *sock, const char *msg, unsigned long len)
{
    (void)sock;			// Unused
    (void)msg;			// Unused
    (void)len;			// Unused
    return 0;
}


mDNSexport UDPSocket *mDNSPlatformUDPSocket(mDNS * const m, mDNSIPPort port)
{
    (void)m;			// Unused
    (void)port;			// Unused
    return NULL;
}


mDNSexport void           mDNSPlatformUDPClose(UDPSocket *sock)
{
    (void)sock;			// Unused
}


mDNSexport void mDNSPlatformUpdateProxyList(mDNS *const m, const mDNSInterfaceID InterfaceID)
{
    (void)m;			// Unused
    (void)InterfaceID;			// Unused
}


mDNSexport void mDNSPlatformSendRawPacket(const void *const msg, const mDNSu8 *const end, mDNSInterfaceID InterfaceID)
{
    (void)msg;			// Unused
    (void)end;			// Unused
    (void)InterfaceID;			// Unused
}


mDNSexport void mDNSPlatformSetLocalAddressCacheEntry(mDNS *const m, const mDNSAddr *const tpa, const mDNSEthAddr *const tha, mDNSInterfaceID InterfaceID)
{
    (void)m;			// Unused
    (void)tpa;			// Unused
    (void)tha;			// Unused
    (void)InterfaceID;			// Unused
}


mDNSexport mStatus mDNSPlatformTLSSetupCerts(void)
{
    return(mStatus_UnsupportedErr);
}


mDNSexport void mDNSPlatformTLSTearDownCerts(void)
{
}


mDNSexport void mDNSPlatformSetAllowSleep(mDNS *const m, mDNSBool allowSleep, const char *reason)
{
    (void) m;
    (void) allowSleep;
    (void) reason;
}


mDNSexport void FreeEtcHosts(mDNS *const m, AuthRecord *const rr, mStatus result)
{
    (void)m;  // unused
    (void)rr;
    (void)result;
}


mDNSexport void mDNSPlatformSetDNSConfig(mDNS *const m, mDNSBool setservers, mDNSBool setsearch, domainname *const fqdn, DNameListElem **RegDomains, DNameListElem **BrowseDomains)
{
    (void) m;
    (void) setservers;
    (void) fqdn;
    (void) setsearch;
    (void) RegDomains;
    (void) BrowseDomains;
}


mDNSexport mStatus mDNSPlatformGetPrimaryInterface(mDNS * const m, mDNSAddr * v4, mDNSAddr * v6, mDNSAddr * router)
{
    (void) m;
    (void) v4;
    (void) v6;
    (void) router;

    return mStatus_UnsupportedErr;
}


mDNSexport void mDNSPlatformDynDNSHostNameStatusChanged(const domainname *const dname, const mStatus status)
{
    (void) dname;
    (void) status;
}


// This gets the current hostname, truncating it at the first dot if necessary
mDNSlocal void GetUserSpecifiedRFC1034ComputerName(domainlabel *const namelabel)
{
    int len = 0;
#ifndef ANDROID
    qstrncpy((char *) &namelabel->c[1],
             QHostInfo::localHostName().toUtf8().constData(),
             MAX_DOMAIN_LABEL);
#else
    // use an appropriate default label rather than the linux default of 'localhost'
    strncpy((char *) &namelabel->c[1], "Android", MAX_DOMAIN_LABEL);
#endif
    while (len < MAX_DOMAIN_LABEL && namelabel->c[len+1] && namelabel->c[len+1] != '.') len++;
    namelabel->c[0] = mDNSu8(len);
}


// On OS X this gets the text of the field labelled "Computer Name" in the Sharing Prefs Control Panel
// Other platforms can either get the information from the appropriate place,
// or they can alternatively just require all registering services to provide an explicit name
mDNSlocal void GetUserSpecifiedFriendlyComputerName(domainlabel *const namelabel)
{
    // On Unix we have no better name than the host name, so we just use that.
    GetUserSpecifiedRFC1034ComputerName(namelabel);
}

mDNSexport int ParseDNSServers(mDNS *m, const char *filePath)
{
    Q_UNUSED(m);
    Q_UNUSED(filePath);
/*
    char line[256];
    char nameserver[16];
    char keyword[10];
    int  numOfServers = 0;
    FILE *fp = fopen(filePath, "r");
    if (fp == NULL) return -1;
    while (fgets(line,sizeof(line),fp))
        {
        struct in_addr ina;
        line[255]='\0';		// just to be safe
        if (sscanf(line,"%10s %15s", keyword, nameserver) != 2) continue;	// it will skip whitespaces
        if (strncasecmp(keyword,"nameserver",10)) continue;
        if (inet_aton(nameserver, (struct in_addr *)&ina) != 0)
            {
            mDNSAddr DNSAddr;
            DNSAddr.type = mDNSAddrType_IPv4;
            DNSAddr.ip.v4.NotAnInteger = ina.s_addr;
            mDNS_AddDNSServer(m, NULL, mDNSInterface_Any, &DNSAddr, UnicastDNSPort, mDNSfalse, 0);
            numOfServers++;
            }
        }
    return (numOfServers > 0) ? 0 : -1;
*/
    return 0;  // Not implemented
}


// Frees the specified PosixNetworkInterface structure. The underlying
// interface must have already been deregistered with the mDNS core.
mDNSlocal void FreePosixNetworkInterface(PosixNetworkInterface *intf)
{
    Q_ASSERT(intf);
    if (intf->intfName != NULL)
        free((void*) intf->intfName);
    // The sockets intf->multicastSocket4 and intf->multicastSocket6 are closed by QUdpSocket in ArnMDns
    mDNSPlatformMemFree( intf);
}


// Grab the first interface, deregister it, free it, and repeat until done.
mDNSlocal void ClearInterfaceList( mDNS* const m)
{
    Q_ASSERT(m);

    while (m->HostInterfaces) {
        PosixNetworkInterface*  intf = (PosixNetworkInterface*) (m->HostInterfaces);
        mDNS_DeregisterInterface( m, &intf->coreIntf, mDNSfalse);
        if (Arn::debugMDNS)
            qDebug("Deregistered interface %s\n", intf->intfName);
        FreePosixNetworkInterface( intf);
    }
    num_registered_interfaces = 0;
    num_pkts_accepted = 0;
    num_pkts_rejected = 0;
}


// Sets up a send/receive socket.
// If mDNSIPPort port is non-zero, then it's a multicast socket on the specified interface
// If mDNSIPPort port is zero, then it's a randomly assigned port number, used for sending unicast queries
int SetupSocket(int addrFam, mDNSIPPort port, int interfaceIndex, int *sktPtr)
{
    // static const int kOn = 1;
    // static const int kIntTwoFiveFive = 255;

    Q_ASSERT(sktPtr);
    Q_ASSERT(*sktPtr == -1);

    bool  stat = true;  // Default ok
    bool  joinMulticastGroup = (port.NotAnInteger != 0);

    ArnMDnsSockInfo*  mdi = ArnMDns::addSocket();
    QUdpSocket*  udpSock  = mdi->_udpSocket;
    mdi->_interfaceIndex  = interfaceIndex;
    mdi->_isUnicast       = !joinMulticastGroup;

    QUdpSocket::BindMode  bindMode;
    if (joinMulticastGroup)
        bindMode |= QUdpSocket::ReuseAddressHint | QUdpSocket::ShareAddress;

    QHostAddress  anyAddress;
    QHostAddress  mcgAddress;
    if (addrFam == mDNSAddrType_IPv4) {
        mcgAddress = ArnMDns::fromMDNSAddr( AllDNSLinkGroup_v4);
#if QT_VERSION >= 0x050000
        anyAddress = QHostAddress::AnyIPv4;
#else
        anyAddress = QHostAddress::Any;
#endif
    }
    else if (addrFam == mDNSAddrType_IPv6) {
        mcgAddress = ArnMDns::fromMDNSAddr( AllDNSLinkGroup_v6);
        anyAddress = QHostAddress::AnyIPv6;
    }
    else {
        if (Arn::warningMDNS)  qWarning() << "ArnMDns SetupSocket: unknown addrFam=" << addrFam;
        return 0;  // Internal error
    }

    stat = stat && udpSock->bind( anyAddress, ArnMDns::fromMDNSPort( port), bindMode);
    if (Arn::debugMDNS)  qDebug() << "SetupSocket Bind: orgPort=" << ArnMDns::fromMDNSPort( port)
                                  << " port=" << udpSock->localPort()
                                  << " interfaceIndex=" << interfaceIndex;

    if (joinMulticastGroup) {
        QNetworkInterface  nif = QNetworkInterface::interfaceFromIndex( interfaceIndex);
        if (Arn::debugMDNS)  qDebug() << "SetupSocket found: nif=" << nif.humanReadableName()
                                      << "  Join MCG: mcgAdr=" << mcgAddress.toString();
        stat = stat && udpSock->joinMulticastGroup( mcgAddress, nif);
        udpSock->setMulticastInterface( nif);
    }
    // Some options are not supported by Qt, will be ignored ...
    // IPV4
    // Should be IP_PKTINFO, (IP_RECVTTL)
    // err = setsockopt(fd, IPPROTO_IP, IP_PKTINFO, &kOn, sizeof(kOn));
    // err = setsockopt(fd, IPPROTO_IP, IP_RECVTTL, &kOn, sizeof(kOn));
    // Should be IP_TTL = 255
    // err = setsockopt(fd, IPPROTO_IP, IP_TTL, &kIntTwoFiveFive, sizeof(kIntTwoFiveFive));
    //
    // IPV6
    // Should be IPV6_2292_PKTINFO (for multiple homed systems)
    // err = setsockopt(fd, IPPROTO_IPV6, IPV6_2292_PKTINFO, &kOn, sizeof(kOn));
    // Should be IPV6_V6ONLY
    // err = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &kOn, sizeof(kOn));
    // Should be IPV6_UNICAST_HOPS = 255
    // err = setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &kIntTwoFiveFive, sizeof(kIntTwoFiveFive));
    udpSock->setSocketOption( QAbstractSocket::MulticastTtlOption, int(255));

    if (stat) {
        int  fd = int(udpSock->socketDescriptor());
        *sktPtr = fd;
        ArnMDns::addSocketInfo( mdi);
        return 0;  // Success
    }
    else {
        if (Arn::warningMDNS)
            qWarning() << "SetupSocket error: code=" << udpSock->error()
                       << " txt=" << udpSock->errorString();
        if (mdi)
            delete mdi;
        return 1;  // Fail
    }
}


int  arnSetupOneInterface( mDNS* const m, const QNetworkInterface& interface,
                           const QNetworkAddressEntry& entry, PosixNetworkInterface **intfpp)
{
    int err = 0;
    PosixNetworkInterface *intf;
    PosixNetworkInterface *alias = NULL;

    Q_ASSERT(m);

    // Allocate the interface structure itself.
    intf = (PosixNetworkInterface*) mDNSPlatformMemAllocate( sizeof(*intf));
    if (intf == NULL) { Q_ASSERT(0); err = mStatus_NoMemoryErr; }

    // And make a copy of the intfName.
    if (err == 0) {
        intf->intfName = strdup( interface.name().toLatin1().constData());
        if (intf->intfName == NULL) { Q_ASSERT(0); err = mStatus_NoMemoryErr; }
    }

    if (err == 0) {
        // Set up the fields required by the mDNS core.
        ArnMDns::toMDNSAddr( entry.ip(), intf->coreIntf.ip);
        ArnMDns::toMDNSAddr( entry.netmask(), intf->coreIntf.mask);
        if (Arn::debugMDNS)  qDebug() << "Setup1Intf: ip=" << entry.ip().toString()
                                      << " mask=" << entry.netmask().toString();

        strncpy(intf->coreIntf.ifname, intf->intfName, sizeof(intf->coreIntf.ifname));
        intf->coreIntf.ifname[sizeof(intf->coreIntf.ifname)-1] = 0;
        intf->coreIntf.Advertise = mDNSu8(m->AdvertiseLocalAddresses);
        intf->coreIntf.McastTxRx = mDNStrue;

        // Set up the extra fields in PosixNetworkInterface.
        Q_ASSERT(intf->intfName);         // intf->intfName already set up above
        intf->index                = interface.index();
        intf->multicastSocket4     = -1;
#if HAVE_IPV6
        intf->multicastSocket6     = -1;
#endif
        alias                       = *intfpp;
        if (alias == NULL)  alias   = intf;
        intf->coreIntf.InterfaceID = (mDNSInterfaceID)alias;
    }

    // Set up the multicast socket
    if (err == 0) {
        if ((alias->multicastSocket4 == -1)
        && (entry.ip().protocol() == QAbstractSocket::IPv4Protocol))
            err = SetupSocket(mDNSAddrType_IPv4, MulticastDNSPort, intf->index, &alias->multicastSocket4);
#if HAVE_IPV6
        else if ((alias->multicastSocket6 == -1)
        && (entry.ip().protocol() == QAbstractSocket::IPv6Protocol))
            err = SetupSocket(mDNSAddrType_IPv6, MulticastDNSPort, intf->index, &alias->multicastSocket6);
#endif
    }

    // The interface is all ready to go, let's register it with the mDNS core.
    if (err == 0)
        err = mDNS_RegisterInterface(m, &intf->coreIntf, mDNSfalse);

    // Clean up.
    if (err == 0) {
        // num_registered_interfaces++;
        debugf("SetupOneInterface: nif=%s ip=%#a Registered",
               interface.humanReadableName().toUtf8().constData(), &intf->coreIntf.ip);
        if (Arn::debugMDNS)
            qDebug("Registered interface %s\n", intf->intfName);
    }
    else {
        // Use intfName instead of intf->intfName in the next line to avoid dereferencing NULL.
        // debugf("SetupOneInterface: %s %#a failed to register %d", intfName, &intf->coreIntf.ip, err);
        if (intf) { FreePosixNetworkInterface(intf); intf = NULL; }
    }

    // bombs with android: assert((err == 0) == (intf != NULL));

    *intfpp = (err == 0) ? intf : NULL;
    return err;
}


int  arnSetupInterfaceList( mDNS *const m)
{
    bool  foundIntV4 = false;
    QNetworkInterface  firstLoopbackInterface;
    QNetworkAddressEntry  firstLoopbackEntry;

    QList<QNetworkInterface>  intfList = QNetworkInterface::allInterfaces();
    int  intfListLen = intfList.size();
    for (int i = 0; i < intfListLen; ++i) {
        QNetworkInterface  interface = intfList.at(i);
        QNetworkInterface::InterfaceFlags  flags = interface.flags();
        if (!flags.testFlag( QNetworkInterface::IsUp)
        || flags.testFlag( QNetworkInterface::IsPointToPoint))
            continue;

        PosixNetworkInterface*  firstMDnsIntf = 0;
        QList<QNetworkAddressEntry>  entries = interface.addressEntries();
        int  entriesLen = entries.size();
        for (int j = 0; j < entriesLen; ++j) {
            QNetworkAddressEntry  entry = entries.at(j);
            QAbstractSocket::NetworkLayerProtocol  prot = entry.ip().protocol();
            if ((prot != QAbstractSocket::IPv4Protocol) && (prot != QAbstractSocket::IPv6Protocol))
                continue;
            if (entry.prefixLength() == 0)  continue;

            if (Arn::debugMDNS)  qDebug() << "SetupIntfList found: nif="
                                          << interface.humanReadableName() + " ip=" + entry.ip().toString();
            if (entry.prefixLength() < 0) {
                // MW: This is a bug in Qt for android, windows ...  (Not linux)
                entry.setPrefixLength( flags.testFlag( QNetworkInterface::IsLoopBack) ? 8 : 24);
                if (Arn::warningMDNS)  qWarning() << "Bad netmask: nif=" << interface.humanReadableName()
                                                  << ", asume prefixLen=" << entry.prefixLength();
            }
            if (flags.testFlag( QNetworkInterface::IsLoopBack)) {
                if (!firstLoopbackInterface.isValid()) {
                    firstLoopbackInterface = interface;
                    firstLoopbackEntry     = entry;
                }
            }
            else {
                arnSetupOneInterface( m, interface, entry, &firstMDnsIntf);
                if (prot == QAbstractSocket::IPv4Protocol)
                    foundIntV4 = true;
            }
        }
    }

    // If we found no normal interfaces but we did find a loopback interface, register the
    // loopback interface.  This allows self-discovery if no interfaces are configured.
    // Temporary workaround: Multicast loopback on IPv6 interfaces appears not to work.
    // In the interim, we skip loopback interface only if we found at least one v4 interface to use
    // if ((m->HostInterfaces == NULL) && (firstLoopback != NULL))
    if (!foundIntV4 && firstLoopbackInterface.isValid()) {
        PosixNetworkInterface*  mDnsIntf = 0;
        arnSetupOneInterface( m, firstLoopbackInterface, firstLoopbackEntry, &mDnsIntf);
    }

    return mStatus_NoError;
}


// Register with either a Routing Socket or RtNetLink to listen for interface changes.
mDNSlocal mStatus  WatchForInterfaceChange( mDNS* const m)
{
    Q_UNUSED(m);
/*
    mStatus		err;
    IfChangeRec	*pChgRec;

    pChgRec = (IfChangeRec*) mDNSPlatformMemAllocate(sizeof *pChgRec);
    if (pChgRec == NULL)
        return mStatus_NoMemoryErr;

    pChgRec->mDNS = m;
    err = OpenIfNotifySocket(&pChgRec->NotifySD);
    if (err == 0)
        err = mDNSPosixAddFDToEventLoop(pChgRec->NotifySD, InterfaceChangeCallback, pChgRec);

    return err;
*/
    return mStatus_NoError;  // Interface change not implemented
}


// Test to see if we're the first client running on UDP port 5353, by trying to bind to 5353 without using SO_REUSEPORT.
// If we fail, someone else got here first. That's not a big problem; we can share the port for multicast responses --
// we just need to be aware that we shouldn't expect to successfully receive unicast UDP responses.
mDNSlocal mDNSBool mDNSPlatformInit_CanReceiveUnicast(void)
{
/*
    int err;
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in s5353;
    s5353.sin_family      = AF_INET;
    s5353.sin_port        = MulticastDNSPort.NotAnInteger;
    s5353.sin_addr.s_addr = 0;
    err = bind(s, (struct sockaddr *)&s5353, sizeof(s5353));
    close(s);
    if (err) debugf("No unicast UDP responses");
    else     debugf("Unicast UDP responses okay");
    return(err == 0);
*/
    return mDNSfalse;  // Not implemented
}


// mDNS core calls this routine to initialise the platform-specific data.
mDNSexport mStatus  mDNSPlatformInit( mDNS *const m)
{
    int err = 0;
    Q_ASSERT(m);

    qsrand( uint(QDateTime::currentMSecsSinceEpoch()));

    if (mDNSPlatformInit_CanReceiveUnicast())
        m->CanReceiveUnicastOn5353 = mDNStrue;

    // Tell mDNS core the names of this machine.

    // Set up the nice label
    m->nicelabel.c[0] = 0;
    GetUserSpecifiedFriendlyComputerName( &m->nicelabel);
    if (m->nicelabel.c[0] == 0)
        MakeDomainLabelFromLiteralString( &m->nicelabel, "Computer");

    // Set up the RFC 1034-compliant label
    m->hostlabel.c[0] = 0;
    GetUserSpecifiedRFC1034ComputerName( &m->hostlabel);
    if (m->hostlabel.c[0] == 0)
        MakeDomainLabelFromLiteralString( &m->hostlabel, "Computer");

    mDNS_SetFQDN(m);

    m->p->unicastSocket4 = -1;
    if (err == mStatus_NoError)  err = SetupSocket(mDNSAddrType_IPv4, zeroIPPort, 0, &m->p->unicastSocket4);
#if HAVE_IPV6
    m->p->unicastSocket6 = -1;
    if (err == mStatus_NoError)  err = SetupSocket(mDNSAddrType_IPv6, zeroIPPort, 0, &m->p->unicastSocket6);
#endif

    // Tell mDNS core about the network interfaces on this machine.
    // if (err == mStatus_NoError)  err = SetupInterfaceList(m);
    if (err == mStatus_NoError)  err = arnSetupInterfaceList( m);
    if (err == mStatus_NoError)  ArnMDns::bindMDnsInfo( m);

    // Tell mDNS core about DNS Servers
    mDNS_Lock(m);
    if (err == mStatus_NoError)  ParseDNSServers( m, uDNS_SERVERS_FILE);
    mDNS_Unlock(m);

    if (err == mStatus_NoError) {
        err = WatchForInterfaceChange( m);
        // Failure to observe interface changes is non-fatal.
        if (err != mStatus_NoError) {
            if (Arn::debugMDNS)
                qDebug("mDNS(%d) WARNING: Unable to detect interface changes (%d).\n", getpid(), err);
            err = mStatus_NoError;
        }
    }

    // We don't do asynchronous initialization on the Posix platform, so by the time
    // we get here the setup will already have succeeded or failed.  If it succeeded,
    // we should just call mDNSCoreInitComplete() immediately.
    if (err == mStatus_NoError)
        mDNSCoreInitComplete( m, mStatus_NoError);

    return PosixErrorToStatus( err);
}


// mDNS core calls this routine to clean up the platform-specific data.
// In our case all we need to do is to tear down every network interface.
mDNSexport void  mDNSPlatformClose( mDNS* const m)
{
    Q_ASSERT(m);
    ClearInterfaceList( m);
    // The sockets m->p->unicastSocket4 and m->p->unicastSocket6 are closed by QUdpSocket in ArnMDns
}


// On the Posix platform, locking is a no-op because we only ever enter
// mDNS core on the main thread.

// mDNS core calls this routine when it wants to prevent
// the platform from reentering mDNS core code.
mDNSexport void  mDNSPlatformLock( const mDNS* const m)
{
    (void) m;	// Unused
}


// mDNS core calls this routine when it release the lock taken by
// mDNSPlatformLock and allow the platform to reenter mDNS core code.
mDNSexport void  mDNSPlatformUnlock( const mDNS* const m)
{
    (void) m;	// Unused
}


// mDNS core calls this routine to copy C strings.
mDNSexport void  mDNSPlatformStrCopy( void* dst, const void* src)
{
    qstrcpy((char*) dst, (const char*) src);
}


// mDNS core calls this routine to get the length of a C string.
mDNSexport mDNSu32  mDNSPlatformStrLen( const void* src)
{
    return qstrlen((const char*) src);
}


// mDNS core calls this routine to copy memory.
// On the Posix platform this maps directly to the ANSI C memcpy.
mDNSexport void  mDNSPlatformMemCopy( void* dst, const void* src, mDNSu32 len)
{
    //qMemCopy( dst, src, len);
    memcpy( dst, src, len);
}


// mDNS core calls this routine to test whether blocks of memory are byte-for-byte
// identical. On the Posix platform this is a simple wrapper around ANSI C memcmp.
mDNSexport mDNSBool  mDNSPlatformMemSame( const void* dst, const void* src, mDNSu32 len)
{
    return memcmp( dst, src, len) == 0;
}


// mDNS core calls this routine to clear blocks of memory.
// On the Posix platform this is a simple wrapper around ANSI C memset.
mDNSexport void  mDNSPlatformMemZero( void* dst, mDNSu32 len)
{
    memset( dst, 0, len);
}


mDNSexport void*  mDNSPlatformMemAllocate( mDNSu32 len)
{
    return new char[len];
}


mDNSexport void  mDNSPlatformMemFree( void* mem)
{
    delete [] (char*) mem;
}


mDNSexport mDNSu32  mDNSPlatformRandomSeed(void)
{
    return uint( QDateTime::currentMSecsSinceEpoch());
}


mDNSexport mDNSs32  mDNSPlatformOneSecond = 1024;

mDNSexport mStatus  mDNSPlatformTimeInit(void)
{
    // No special setup is required on Posix -- we just use gettimeofday();
    // This is not really safe, because gettimeofday can go backwards if the user manually changes the date or time
    // We should find a better way to do this
    return(mStatus_NoError);
}


mDNSexport mDNSs32  mDNSPlatformRawTime()
{
    qint64  ms = QDateTime::currentMSecsSinceEpoch();
    return mDNSs32((uint(ms / 1000) << 10) | (uint(ms % 1000) * 16000 / 15610));

    //struct timeval tv;
    //gettimeofday(&tv, NULL);
    // tv.tv_sec is seconds since 1st January 1970 (GMT, with no adjustment for daylight savings time)
    // tv.tv_usec is microseconds since the start of this second (i.e. values 0 to 999999)
    // We use the lower 22 bits of tv.tv_sec for the top 22 bits of our result
    // and we multiply tv.tv_usec by 16 / 15625 to get a value in the range 0-1023 to go in the bottom 10 bits.
    // This gives us a proper modular (cyclic) counter that has a resolution of roughly 1ms (actually 1/1024 second)
    // and correctly cycles every 2^22 seconds (4194304 seconds = approx 48 days).
    //return((tv.tv_sec << 10) | (tv.tv_usec * 16 / 15625));
}


mDNSexport mDNSs32  mDNSPlatformUTC(void)
{
    return mDNSs32(QDateTime::currentMSecsSinceEpoch() / 1000);
}


mDNSexport void  mDNSPlatformSendWakeupPacket( mDNS *const m, mDNSInterfaceID InterfaceID, char *EthAddr, char *IPAddr, int iteration)
{
    (void) m;
    (void) InterfaceID;
    (void) EthAddr;
    (void) IPAddr;
    (void) iteration;
}


mDNSexport mDNSBool  mDNSPlatformValidRecordForInterface( AuthRecord *rr, const NetworkInterfaceInfo *intf)
{
    (void) rr;
    (void) intf;

    return 1;
}


#ifdef MDNS_DEBUGMSGS
mDNSexport void  mDNSPlatformWriteDebugMsg( const char *msg)
{
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_DEBUG, "bonjour", "%s", msg);
#else
    if (Arn::debugMDNS)  qDebug("%s\n", msg);
#endif
}
#endif


mDNSexport void mDNSPlatformWriteLogMsg(const char *ident, const char *buffer, mDNSLogLevel_t loglevel)
{
    Q_UNUSED(ident);

#if 0
    if (mDNS_DebugMode)	{
        qDebug("%s\n", buffer);
        return;
    }
#endif

#ifdef ANDROID
    //__android_log_print(ANDROID_LOG_DEBUG, "mDNS", "%s", buffer);
    if (Arn::debugMDNS)  qDebug("Android: %s\n", buffer);

    int  syslog_level = 0;
    switch (loglevel) {
    case MDNS_LOG_DEBUG:     syslog_level = ANDROID_LOG_DEBUG;  break;
#if MDNS_DEBUGMSGS > 0
    case MDNS_LOG_OPERATION: syslog_level = ANDROID_LOG_WARN;   break;
    case MDNS_LOG_SPS:       syslog_level = ANDROID_LOG_DEBUG;  break;
    case MDNS_LOG_INFO:      syslog_level = ANDROID_LOG_INFO;   break;
    default:                 syslog_level = ANDROID_LOG_ERROR;  break;
#else
    default:  return;
#endif
    }
    __android_log_print(syslog_level, "mDNS", "%s", buffer);
#else
    switch (int(loglevel)) {
    case MDNS_LOG_MSG:
        // qCritical("[Error] %s\n", buffer);
        // break;
    case MDNS_LOG_OPERATION:
        if (Arn::warningMDNS)  qWarning("[Warn1] %s\n", buffer);
        break;
    case MDNS_LOG_SPS:
    case MDNS_LOG_INFO:
    case MDNS_LOG_DEBUG:
        if (Arn::debugMDNS)  qDebug("[Debug] %s\n", buffer);
        break;
    default:
        if (Arn::warningMDNS)  qWarning("[Warn2] mDNS Unknown loglevel %d: %s\n", loglevel, buffer);
    }
#endif
}


mDNSexport mDNSu32  mDNSPlatformRandomNumber(void)
{
    return mDNSu32(qrand());
}

}  // extern "C"
