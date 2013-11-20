// Copyright (C) 2010-2013 Michael Wiklund.
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
// Copyright (c) 2002-2004 Apple Computer, Inc. All rights reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//

#ifndef __mDNSPlatformPosix_h
#define __mDNSPlatformPosix_h

#ifdef  __cplusplus
    extern "C" {
#endif

struct sockaddr;

// PosixNetworkInterface is a record extension of the core NetworkInterfaceInfo
// type that supports extra fields needed by the Posix platform.
//
// IMPORTANT: coreIntf must be the first field in the structure because
// we cast between pointers to the two different types regularly.

typedef struct PosixNetworkInterface PosixNetworkInterface;

struct PosixNetworkInterface
{
	NetworkInterfaceInfo    coreIntf;
	const char *            intfName;
	PosixNetworkInterface * aliasIntf;
	int                     index;
	int                     multicastSocket4;
#if HAVE_IPV6
	int                     multicastSocket6;
#endif
};

// This is a global because debugf_() needs to be able to check its value
extern int gMDNSPlatformPosixVerboseLevel;


struct mDNS_PlatformSupport_struct
{
	int unicastSocket4;
#if HAVE_IPV6
	int unicastSocket6;
#endif
};


#define uDNS_SERVERS_FILE "/etc/resolv.conf"
extern int ParseDNSServers(mDNS *m, const char *filePath);


#ifdef  __cplusplus
}
#endif

#endif
