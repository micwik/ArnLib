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

#include "mDNSEmbeddedAPI.h"	// Defines the interface provided to the client layer above
#include "DNSCommon.h"
#include "PlatformCommon.h"


// Bind a UDP socket to find the source address to a destination
mDNSexport void mDNSPlatformSourceAddrForDest(mDNSAddr *const src, const mDNSAddr *const dst)
	{
    (void) dst;

    src->type = mDNSAddrType_None;
    return;  //MW: Not implemented

/*
    union { struct sockaddr s; struct sockaddr_in a4; struct sockaddr_in6 a6; } addr;
	socklen_t len = sizeof(addr);
	socklen_t inner_len = 0;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	src->type = mDNSAddrType_None;
	if (sock == -1) return;
	if (dst->type == mDNSAddrType_IPv4)
		{
		inner_len = sizeof(addr.a4);
		#ifndef NOT_HAVE_SA_LEN
		addr.a4.sin_len         = inner_len;
		#endif
		addr.a4.sin_family      = AF_INET;
		addr.a4.sin_port        = 1;	// Not important, any port will do
		addr.a4.sin_addr.s_addr = dst->ip.v4.NotAnInteger;
		}
	else if (dst->type == mDNSAddrType_IPv6)
		{
		inner_len = sizeof(addr.a6);
		#ifndef NOT_HAVE_SA_LEN
		addr.a6.sin6_len      = inner_len;
		#endif
		addr.a6.sin6_family   = AF_INET6;
		addr.a6.sin6_flowinfo = 0;
		addr.a6.sin6_port     = 1;	// Not important, any port will do
		addr.a6.sin6_addr     = *(struct in6_addr*)&dst->ip.v6;
		addr.a6.sin6_scope_id = 0;
		}
	else return;

	if ((connect(sock, &addr.s, inner_len)) < 0)
		{ LogMsg("mDNSPlatformSourceAddrForDest: connect %#a failed errno %d (%s)", dst, errno, strerror(errno)); goto exit; }

	if ((getsockname(sock, &addr.s, &len)) < 0)
		{ LogMsg("mDNSPlatformSourceAddrForDest: getsockname failed errno %d (%s)", errno, strerror(errno)); goto exit; }

	src->type = dst->type;
	if (dst->type == mDNSAddrType_IPv4) src->ip.v4.NotAnInteger = addr.a4.sin_addr.s_addr;
	else                                src->ip.v6 = *(mDNSv6Addr*)&addr.a6.sin6_addr;
exit:
	close(sock);
*/
	}


/*
// dst must be at least MAX_ESCAPED_DOMAIN_NAME bytes, and option must be less than 32 bytes in length
mDNSlocal mDNSBool GetConfigOption(char *dst, const char *option, FILE *f)
	{
	char buf[32+1+MAX_ESCAPED_DOMAIN_NAME];	// Option name, one space, option value
	unsigned int len = strlen(option);
	if (len + 1 + MAX_ESCAPED_DOMAIN_NAME > sizeof(buf)-1) { LogMsg("GetConfigOption: option %s too long", option); return mDNSfalse; }
	fseek(f, 0, SEEK_SET);  // set position to beginning of stream
	while (fgets(buf, sizeof(buf), f))		// Read at most sizeof(buf)-1 bytes from file, and append '\0' C-string terminator
		{
		if (!strncmp(buf, option, len))
			{
			strncpy(dst, buf + len + 1, MAX_ESCAPED_DOMAIN_NAME-1);
			if (dst[MAX_ESCAPED_DOMAIN_NAME-1]) dst[MAX_ESCAPED_DOMAIN_NAME-1] = '\0';
			len = strlen(dst);
			if (len && dst[len-1] == '\n') dst[len-1] = '\0';  // chop newline
			return mDNStrue;
			}
		}
	debugf("Option %s not set", option);
	return mDNSfalse;
	}

mDNSexport void ReadDDNSSettingsFromConfFile(mDNS *const m, const char *const filename, domainname *const hostname, domainname *const domain, mDNSBool *DomainDiscoveryDisabled)
	{
	char buf[MAX_ESCAPED_DOMAIN_NAME] = "";
	mStatus err;
	FILE *f = fopen(filename, "r");

    if (hostname)                 hostname->c[0] = 0;
    if (domain)                   domain->c[0] = 0;
	if (DomainDiscoveryDisabled) *DomainDiscoveryDisabled = mDNSfalse;

	if (f)
		{
		if (DomainDiscoveryDisabled && GetConfigOption(buf, "DomainDiscoveryDisabled", f) && !strcasecmp(buf, "true")) *DomainDiscoveryDisabled = mDNStrue;
		if (hostname && GetConfigOption(buf, "hostname", f) && !MakeDomainNameFromDNSNameString(hostname, buf)) goto badf;
		if (domain && GetConfigOption(buf, "zone", f) && !MakeDomainNameFromDNSNameString(domain, buf)) goto badf;
		buf[0] = 0;
		GetConfigOption(buf, "secret-64", f);  // failure means no authentication
		fclose(f);
		f = NULL;
		}
	else
		{
		if (errno != ENOENT) LogMsg("ERROR: Config file exists, but cannot be opened.");
		return;
		}

	if (domain && domain->c[0] && buf[0])
		{
		DomainAuthInfo *info = (DomainAuthInfo*)mDNSPlatformMemAllocate(sizeof(*info));
		// for now we assume keyname = service reg domain and we use same key for service and hostname registration
		err = mDNS_SetSecretForDomain(m, info, domain, domain, buf, NULL, 0, NULL);
		if (err) LogMsg("ERROR: mDNS_SetSecretForDomain returned %d for domain %##s", err, domain->c);
		}

	return;

	badf:
	LogMsg("ERROR: malformatted config file");
	if (f) fclose(f);
	}
*/
