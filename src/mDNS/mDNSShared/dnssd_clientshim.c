/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2003 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * This file defines a simple shim layer between a client calling the "/usr/include/dns_sd.h" APIs
 * and an implementation of mDNSCore ("mDNSEmbeddedAPI.h" APIs) in the same address space.
 * When the client calls a dns_sd.h function, the shim calls the corresponding mDNSEmbeddedAPI.h
 * function, and when mDNSCore calls the shim's callback, we call through to the client's callback.
 * The shim is responsible for two main things:
 * - converting string parameters between C string format and native DNS format,
 * - and for allocating and freeing memory.
 */

// Modified 2014-2016 by Michael Wiklund.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
//

#include "dns_sd.h"				// Defines the interface to the client layer above
#include "mDNSEmbeddedAPI.h"	// The interface we're building on top of
#include "mDNS/mDNSQt/mDNSQt.h"

extern mDNS mDNSStorage;		// We need to pass the address of this storage to the lower-layer functions

#if MDNS_BUILDINGSHAREDLIBRARY || MDNS_BUILDINGSTUBLIBRARY
#pragma export on
#endif

//********************************************************************************************
//  Supporting Functions

#define mDNSIsDigit(X)     ((X) >= '0' && (X) <= '9')

// DomainEndsInDot returns 1 if name ends with a dot, 0 otherwise
// (DNSServiceConstructFullName depends this returning 1 for true, rather than any non-zero value meaning true)

static int DomainEndsInDot(const char *dom)
    {
    while (dom[0] && dom[1])
        {
        if (dom[0] == '\\') // advance past escaped byte sequence
            {
            if (mDNSIsDigit(dom[1]) && mDNSIsDigit(dom[2]) && mDNSIsDigit(dom[3]))
                dom += 4;			// If "\ddd"    then skip four
            else dom += 2;			// else if "\x" then skip two
            }
        else dom++;					// else goto next character
        }
    return (dom[0] == '.');
    }


// If there's a comma followed by another character,
// FindFirstSubType overwrites the comma with a nul and returns the pointer to the next character.
// Otherwise, it returns a pointer to the final nul at the end of the string
mDNSlocal char *FindFirstSubType(char *p)
    {
    while (*p)
        {
        if (p[0] == '\\' && p[1]) p += 2;
        else if (p[0] == ',' && p[1]) { *p++ = 0; return(p); }
        else p++;
        }
    return(p);
    }

// If there's a comma followed by another character,
// FindNextSubType overwrites the comma with a nul and returns the pointer to the next character.
// If it finds an illegal unescaped dot in the subtype name, it returns mDNSNULL
// Otherwise, it returns a pointer to the final nul at the end of the string
mDNSlocal char *FindNextSubType(char *p)
    {
    while (*p)
        {
        if (p[0] == '\\' && p[1])		// If escape character
            p += 2;						// ignore following character
        else if (p[0] == ',')			// If we found a comma
            {
            if (p[1]) *p++ = 0;
            return(p);
            }
        else if (p[0] == '.')
            return(mDNSNULL);
        else p++;
        }
    return(p);
    }

// Returns -1 if illegal subtype found
mDNSlocal mDNSs32 ChopSubTypes(char *regtype)
    {
    mDNSs32 NumSubTypes = 0;
    char *stp = FindFirstSubType(regtype);
    while (stp && *stp)					// If we found a comma...
        {
        if (*stp == ',') return(-1);
        NumSubTypes++;
        stp = FindNextSubType(stp);
        }
    if (!stp) return(-1);
    return(NumSubTypes);
    }


mDNSexport AuthRecord *AllocateSubTypes(mDNSs32 NumSubTypes, char *p)
    {
    AuthRecord *st = mDNSNULL;
    if (NumSubTypes)
        {
        mDNSs32 i;
        st = mDNSPlatformMemAllocate(NumSubTypes * sizeof(AuthRecord));
        if (!st) return(mDNSNULL);
        for (i = 0; i < NumSubTypes; i++)
            {
            mDNS_SetupResourceRecord(&st[i], mDNSNULL, mDNSInterface_Any, kDNSQType_ANY, kStandardTTL, 0, AuthRecordAny, mDNSNULL, mDNSNULL);
            while (*p) p++;
            p++;
            if (!MakeDomainNameFromDNSNameString(&st[i].namestorage, p))
                { mDNSPlatformMemFree(st); return(mDNSNULL); }
            }
        }
    return(st);
    }


static mDNSBool strCaseEqual(const char *str1, const char *str2, int len)
    {
    int i;
    for (i = 0; i < len; ++i)
        {
        if ((*str1 == '\0') && (*str2 == '\0'))  return mDNStrue;
        if ((*str1 == '\0') || (*str2 == '\0'))  return mDNSfalse;
        if (*str1 == *str2)                      continue;
        if ((*str1 < 'A') || (*str2 < 'A'))      return mDNSfalse;
        if ((*str1 > 'z') || (*str2 > 'z'))      return mDNSfalse;
        if ((*str1 > 'Z') && (*str1 < 'a'))      return mDNSfalse;
        if ((*str2 > 'Z') && (*str2 < 'a'))      return mDNSfalse;
        if ((*str1 & 0xdf) != (*str2 & 0xdf))    return mDNSfalse;
        }
    return mDNStrue;
    }


//*************************************************************************************************************
// General Utility Functions
//
// Note: Need to make sure we don't write more than kDNSServiceMaxDomainName (1009) bytes to fullName
// In earlier builds this constant was defined to be 1005, so to avoid buffer overruns on clients
// compiled with that constant we'll actually limit the output to 1005 bytes.

DNSServiceErrorType DNSSD_API DNSServiceConstructFullName
    (
    char       *const fullName,
    const char *const service,      // May be NULL
    const char *const regtype,
    const char *const domain
    )
    {
    const int    len = !regtype ? 0 : mDNSPlatformStrLen(regtype) - DomainEndsInDot(regtype);
    char       *fn   = fullName;
    char *const lim  = fullName + 1005;
    const char *s    = service;
    const char *r    = regtype;
    const char *d    = domain;

    // regtype must be at least "x._udp" or "x._tcp"
    if (len < 6 || !domain || !domain[0]) return kDNSServiceErr_BadParam;
    if (!strCaseEqual((regtype + len - 4), "_tcp", 4) && !strCaseEqual((regtype + len - 4), "_udp", 4))
        return kDNSServiceErr_BadParam;

    if (service && *service)
        {
        while (*s)
            {
            unsigned char c = *s++;				// Needs to be unsigned, or values like 0xFF will be interpreted as < 32
            if (c <= ' ')						// Escape non-printable characters
                {
                if (fn+4 >= lim) goto fail;
                *fn++ = '\\';
                *fn++ = '0' + (c / 100);
                *fn++ = '0' + (c /  10) % 10;
                c     = '0' + (c      ) % 10;
                }
            else if (c == '.' || (c == '\\'))	// Escape dot and backslash literals
                {
                if (fn+2 >= lim) goto fail;
                *fn++ = '\\';
                }
            else
                if (fn+1 >= lim) goto fail;
            *fn++ = (char)c;
            }
        *fn++ = '.';
        }

    while (*r) if (fn+1 >= lim) goto fail; else *fn++ = *r++;
    if (!DomainEndsInDot(regtype)) { if (fn+1 >= lim) goto fail; else *fn++ = '.'; }

    while (*d) if (fn+1 >= lim) goto fail; else *fn++ = *d++;
    if (!DomainEndsInDot(domain)) { if (fn+1 >= lim) goto fail; else *fn++ = '.'; }

    *fn = '\0';
    return kDNSServiceErr_NoError;

fail:
    *fn = '\0';
    return kDNSServiceErr_BadParam;
    }


// All mDNS_DirectOP structures start with the pointer to the type-specific disposal function.
// Optional type-specific data follows these three fields
// When the client starts an operation, we return the address of the corresponding mDNS_DirectOP
// as the DNSServiceRef for the operation
// We stash the value in core context fields so we can get it back to recover our state in our callbacks,
// and pass it though to the client for it to recover its state

typedef struct mDNS_DirectOP_struct mDNS_DirectOP;
typedef void mDNS_DirectOP_Dispose(mDNS_DirectOP *op);
struct mDNS_DirectOP_struct
	{
	mDNS_DirectOP_Dispose  *disposefn;
	};

typedef struct
	{
	mDNS_DirectOP_Dispose  *disposefn;
	DNSServiceRegisterReply callback;
	void                   *context;
	mDNSBool                autoname;		// Set if this name is tied to the Computer Name
	mDNSBool                autorename;		// Set if we just got a name conflict and now need to automatically pick a new name
	domainlabel             name;
	domainname              host;
	ServiceRecordSet        s;
	} mDNS_DirectOP_Register;

typedef struct
	{
	mDNS_DirectOP_Dispose  *disposefn;
	DNSServiceBrowseReply   callback;
	void                   *context;
	DNSQuestion             q;
	} mDNS_DirectOP_Browse;

typedef struct
    {
    mDNS_DirectOP_Dispose     *disposefn;
    DNSServiceRef              aQuery;
    DNSServiceGetAddrInfoReply callback;
    void                      *context;
    } mDNS_DirectOP_GetAddrInfo;

typedef struct
	{
	mDNS_DirectOP_Dispose  *disposefn;
	DNSServiceResolveReply  callback;
	void                   *context;
	const ResourceRecord   *SRV;
	const ResourceRecord   *TXT;
	DNSQuestion             qSRV;
	DNSQuestion             qTXT;
	} mDNS_DirectOP_Resolve;

typedef struct
	{
	mDNS_DirectOP_Dispose      *disposefn;
	DNSServiceQueryRecordReply  callback;
	void                       *context;
	DNSQuestion                 q;
	} mDNS_DirectOP_QueryRecord;

int DNSSD_API DNSServiceRefSockFD(DNSServiceRef sdRef)
	{
	(void)sdRef;	// Unused
	return(0);
	}

DNSServiceErrorType DNSSD_API DNSServiceProcessResult(DNSServiceRef sdRef)
	{
	(void)sdRef;	// Unused
	return(kDNSServiceErr_NoError);
	}

void DNSSD_API DNSServiceRefDeallocate(DNSServiceRef sdRef)
	{
	mDNS_DirectOP *op = (mDNS_DirectOP *)sdRef;

    //mDNS_DirectOP_Register *op2 = (mDNS_DirectOP_Register *)sdRef;
    // printf("DNSServiceRefDeallocate: name=%s\n", op2->name.c);
    //qDebug() << "DNSServiceRefDeallocate: name=" << op2->name.c;

    //LogMsg("DNSServiceRefDeallocate");
    op->disposefn(op);
	}

//*************************************************************************************************************
// Domain Enumeration

// Not yet implemented, so don't include in stub library
// We DO include it in the actual Extension, so that if a later client compiled to use this
// is run against this Extension, it will get a reasonable error code instead of just
// failing to launch (Strong Link) or calling an unresolved symbol and crashing (Weak Link)
#if !MDNS_BUILDINGSTUBLIBRARY
DNSServiceErrorType DNSSD_API DNSServiceEnumerateDomains
	(
	DNSServiceRef                       *sdRef,
	DNSServiceFlags                     flags,
	uint32_t                            interfaceIndex,
	DNSServiceDomainEnumReply           callback,
	void                                *context  /* may be NULL */
	)
	{
	(void)sdRef;			// Unused
	(void)flags;			// Unused
	(void)interfaceIndex;	// Unused
	(void)callback;			// Unused
	(void)context;			// Unused
	return(kDNSServiceErr_Unsupported);
	}
#endif

//*************************************************************************************************************
// Register Service

mDNSlocal void FreeDNSServiceRegistration(mDNS_DirectOP_Register *x)
	{
	while (x->s.Extras)
		{
		ExtraResourceRecord *extras = x->s.Extras;
		x->s.Extras = x->s.Extras->next;
		if (extras->r.resrec.rdata != &extras->r.rdatastorage)
			mDNSPlatformMemFree(extras->r.resrec.rdata);
		mDNSPlatformMemFree(extras);
		}

	if (x->s.RR_TXT.resrec.rdata != &x->s.RR_TXT.rdatastorage)
			mDNSPlatformMemFree(x->s.RR_TXT.resrec.rdata);

	if (x->s.SubTypes) mDNSPlatformMemFree(x->s.SubTypes);
	
	mDNSPlatformMemFree(x);
	}

static void DNSServiceRegisterDispose(mDNS_DirectOP *op)
	{
	mDNS_DirectOP_Register *x = (mDNS_DirectOP_Register*)op;
	x->autorename = mDNSfalse;
	// If mDNS_DeregisterService() returns mStatus_NoError, that means that the service was found in the list,
	// is sending its goodbye packet, and we'll get an mStatus_MemFree message when we can free the memory.
	// If mDNS_DeregisterService() returns an error, it means that the service had already been removed from
	// the list, so we should go ahead and free the memory right now
	if (mDNS_DeregisterService(&mDNSStorage, &x->s) != mStatus_NoError)
		FreeDNSServiceRegistration(x);
	}

mDNSlocal void RegCallback(mDNS *const m, ServiceRecordSet *const sr, mStatus result)
	{
	mDNS_DirectOP_Register *x = (mDNS_DirectOP_Register*)sr->ServiceContext;

    domainlabel name;
    domainname type, dom;
	char namestr[MAX_DOMAIN_LABEL+1];		// Unescaped name: up to 63 bytes plus C-string terminating NULL.
	char typestr[MAX_ESCAPED_DOMAIN_NAME];
	char domstr [MAX_ESCAPED_DOMAIN_NAME];
    if (!DeconstructServiceName(sr->RR_SRV.resrec.name, &name, &type, &dom)) return;
    if (!ConvertDomainLabelToCString_unescaped(&name, namestr)) return;
    if (!ConvertDomainNameToCString(&type, typestr)) return;
    if (!ConvertDomainNameToCString(&dom, domstr)) return;

	if (result == mStatus_NoError)
		{
		if (x->callback)
			x->callback((DNSServiceRef)x, 0, result, namestr, typestr, domstr, x->context);
		}
    else if (result == mStatus_NameConflict)
        {
        if (x->autorename)
            mDNS_RenameAndReregisterService(m, sr, mDNSNULL);
        else if (x->callback)
            x->callback((DNSServiceRef)x, 0, result, namestr, typestr, domstr, x->context);
        }
    else if (result == mStatus_MemFree)
		{
		if (x->autorename)
			{
			x->autorename = mDNSfalse;
			x->name = mDNSStorage.nicelabel;
			mDNS_RenameAndReregisterService(m, &x->s, &x->name);
			}
		else
			FreeDNSServiceRegistration(x);
		}
	}

DNSServiceErrorType DNSSD_API DNSServiceRegister
	(
	DNSServiceRef                       *sdRef,
	DNSServiceFlags                     flags,
	uint32_t                            interfaceIndex,
	const char                          *name,         /* may be NULL */
	const char                          *regtype,
	const char                          *domain,       /* may be NULL */
	const char                          *host,         /* may be NULL */
	uint16_t                            notAnIntPort,
	uint16_t                            txtLen,
	const void                          *txtRecord,    /* may be NULL */
	DNSServiceRegisterReply             callback,      /* may be NULL */
	void                                *context       /* may be NULL */
	)
	{
	mStatus err = mStatus_NoError;
	const char *errormsg = "Unknown";
	domainlabel n;
	domainname t, d, h, srv;
	mDNSIPPort port;
	unsigned int size = sizeof(RDataBody);
	AuthRecord *SubTypes = mDNSNULL;
    mDNSs32 NumSubTypes = 0;
    mDNS_DirectOP_Register *x;
    char regTypeBuf[MAX_ESCAPED_DOMAIN_NAME];
	(void)flags;			// Unused
	(void)interfaceIndex;	// Unused

    // Check parameters
	if (!name) name = "";
	if (!name[0]) n = mDNSStorage.nicelabel;
    else if (!MakeDomainLabelFromLiteralString(&n, name))                              { errormsg = "Bad Instance Name"; goto badparam; }
    if (!regtype || !*regtype)                                                         { goto badregtype; }
    mDNSPlatformStrCopy(regTypeBuf, regtype);
    NumSubTypes = ChopSubTypes(regTypeBuf);	// Note: Modifies regtype string to remove trailing subtypes
    if (NumSubTypes < 0)                                                               { goto badregtype; }
    if (!*regTypeBuf || !MakeDomainNameFromDNSNameString(&t, regTypeBuf))              { goto badregtype; }
    if (!MakeDomainNameFromDNSNameString(&d, (domain && *domain) ? domain : "local.")) { errormsg = "Bad Domain";        goto badparam; }
	if (!MakeDomainNameFromDNSNameString(&h, (host   && *host  ) ? host   : ""))       { errormsg = "Bad Target Host";   goto badparam; }
	if (!ConstructServiceName(&srv, &n, &t, &d))                                       { errormsg = "Bad Name";          goto badparam; }
	port.NotAnInteger = notAnIntPort;

    // Allocate memory, and handle failure
	if (size < txtLen)
		size = txtLen;
    x = (mDNS_DirectOP_Register *)mDNSPlatformMemAllocate(sizeof(*x) - sizeof(RDataBody) + size);
    if (!x) { err = mStatus_NoMemoryErr; errormsg = "No memory"; goto fail; }

	// Set up object
	x->disposefn = DNSServiceRegisterDispose;
	x->callback  = callback;
	x->context   = context;
	x->autoname = (!name[0]);
    x->autorename = (flags & kDNSServiceFlagsNoAutoRename) == 0;
	x->name = n;
	x->host = h;

    SubTypes = AllocateSubTypes(NumSubTypes, regTypeBuf);
    if (NumSubTypes && !SubTypes)
        { mDNSPlatformMemFree(x); err = mStatus_UnknownErr; errormsg = "Failed subtypes"; goto fail; }

	// Do the operation
	err = mDNS_RegisterService(&mDNSStorage, &x->s,
		&x->name, &t, &d,		// Name, type, domain
		&x->host, port,			// Host and port
		txtRecord, txtLen,		// TXT data, length
        SubTypes, NumSubTypes,	// Subtypes
		mDNSInterface_Any,		// Interface ID
		RegCallback, x, 0);		// Callback, context, flags
    if (err)
        {
        if (SubTypes)  mDNSPlatformMemFree(SubTypes);
        mDNSPlatformMemFree(x);
        errormsg = "mDNS_RegisterService"; goto fail;
        }

	// Succeeded: Wrap up and return
	*sdRef = (DNSServiceRef)x;
	return(mStatus_NoError);

badregtype:
    errormsg = "Bad Service Type";
badparam:
	err = mStatus_BadParamErr;
fail:
	LogMsg("DNSServiceBrowse(\"%s\", \"%s\") failed: %s (%ld)", regtype, domain, errormsg, err);
	return(err);
	}

//*************************************************************************************************************
// Add / Update / Remove records from existing Registration

// Not yet implemented, so don't include in stub library
// We DO include it in the actual Extension, so that if a later client compiled to use this
// is run against this Extension, it will get a reasonable error code instead of just
// failing to launch (Strong Link) or calling an unresolved symbol and crashing (Weak Link)
#if !MDNS_BUILDINGSTUBLIBRARY
DNSServiceErrorType DNSSD_API DNSServiceAddRecord
	(
	DNSServiceRef                       sdRef,
	DNSRecordRef                        *RecordRef,
	DNSServiceFlags                     flags,
	uint16_t                            rrtype,
	uint16_t                            rdlen,
	const void                          *rdata,
	uint32_t                            ttl
	)
	{
	(void)sdRef;		// Unused
	(void)RecordRef;	// Unused
	(void)flags;		// Unused
	(void)rrtype;		// Unused
	(void)rdlen;		// Unused
	(void)rdata;		// Unused
	(void)ttl;			// Unused
	return(kDNSServiceErr_Unsupported);
	}

DNSServiceErrorType DNSSD_API DNSServiceUpdateRecord
	(
	DNSServiceRef                       sdRef,
	DNSRecordRef                        RecordRef,     /* may be NULL */
	DNSServiceFlags                     flags,
	uint16_t                            rdlen,
	const void                          *rdata,
	uint32_t                            ttl
	)
	{
	(void)sdRef;		// Unused
	(void)RecordRef;	// Unused
	(void)flags;		// Unused
	(void)rdlen;		// Unused
	(void)rdata;		// Unused
	(void)ttl;			// Unused
	return(kDNSServiceErr_Unsupported);
	}

DNSServiceErrorType DNSSD_API DNSServiceRemoveRecord
	(
	DNSServiceRef                 sdRef,
	DNSRecordRef                  RecordRef,
	DNSServiceFlags               flags
	)
	{
	(void)sdRef;		// Unused
	(void)RecordRef;	// Unused
	(void)flags;		// Unused
	return(kDNSServiceErr_Unsupported);
	}
#endif

//*************************************************************************************************************
// Browse for services

static void DNSServiceBrowseDispose(mDNS_DirectOP *op)
	{
	mDNS_DirectOP_Browse *x = (mDNS_DirectOP_Browse*)op;
	//LogMsg("DNSServiceBrowseDispose");
	mDNS_StopBrowse(&mDNSStorage, &x->q);
	mDNSPlatformMemFree(x);
	}

mDNSlocal void FoundInstance(mDNS *const m, DNSQuestion *question, const ResourceRecord *const answer, QC_result AddRecord)
	{
	DNSServiceFlags flags = AddRecord ? kDNSServiceFlagsAdd : (DNSServiceFlags)0;
	domainlabel name;
	domainname type, domain;
	char cname[MAX_DOMAIN_LABEL+1];			// Unescaped name: up to 63 bytes plus C-string terminating NULL.
	char ctype[MAX_ESCAPED_DOMAIN_NAME];
	char cdom [MAX_ESCAPED_DOMAIN_NAME];
	mDNS_DirectOP_Browse *x = (mDNS_DirectOP_Browse*)question->QuestionContext;
	(void)m;		// Unused
	
	if (answer->rrtype != kDNSType_PTR)
		{ LogMsg("FoundInstance: Should not be called with rrtype %d (not a PTR record)", answer->rrtype); return; }
	
	if (!DeconstructServiceName(&answer->rdata->u.name, &name, &type, &domain))
		{
		LogMsg("FoundInstance: %##s PTR %##s received from network is not valid DNS-SD service pointer",
			answer->name->c, answer->rdata->u.name.c);
		return;
		}

	ConvertDomainLabelToCString_unescaped(&name, cname);
	ConvertDomainNameToCString(&type, ctype);
	ConvertDomainNameToCString(&domain, cdom);
	if (x->callback)
		x->callback((DNSServiceRef)x, flags, 0, 0, cname, ctype, cdom, x->context);
	}

DNSServiceErrorType DNSSD_API DNSServiceBrowse
	(
	DNSServiceRef                       *sdRef,
	DNSServiceFlags                     flags,
	uint32_t                            interfaceIndex,
	const char                          *regtype,
	const char                          *domain,    /* may be NULL */
	DNSServiceBrowseReply               callback,
	void                                *context    /* may be NULL */
	)
	{
	mStatus err = mStatus_NoError;
	const char *errormsg = "Unknown";
	domainname t, d;
	mDNS_DirectOP_Browse *x;
    mDNSs32 NumSubTypes;
    char regTypeBuf[MAX_ESCAPED_DOMAIN_NAME];
    (void)flags;			// Unused
	(void)interfaceIndex;	// Unused

    if (!regtype || !*regtype)                                         { goto badregtype; }
    mDNSPlatformStrCopy(regTypeBuf, regtype);

    // Check parameters
    t.c[0] = 0;
    NumSubTypes = ChopSubTypes(regTypeBuf);	// Note: Modifies regtype string to remove trailing subtypes
    if (NumSubTypes < 0 || NumSubTypes > 1)                            { goto badregtype; }
    if (NumSubTypes == 1 &&
       !AppendDNSNameString(&t, regTypeBuf + mDNSPlatformStrLen(regTypeBuf) + 1))
                                                                       { goto badregtype; }

    if (!regTypeBuf[0] || !AppendDNSNameString(&t, regTypeBuf))        { goto badregtype; }

    if (!MakeDomainNameFromDNSNameString(&d, (domain && *domain) ? domain : "local.")) { errormsg = "Illegal domain";  goto badparam; }

	// Allocate memory, and handle failure
	x = (mDNS_DirectOP_Browse *)mDNSPlatformMemAllocate(sizeof(*x));
	if (!x) { err = mStatus_NoMemoryErr; errormsg = "No memory"; goto fail; }

	// Set up object
	x->disposefn = DNSServiceBrowseDispose;
	x->callback  = callback;
	x->context   = context;
	x->q.QuestionContext = x;

	// Do the operation
	err = mDNS_StartBrowse(&mDNSStorage, &x->q, &t, &d, mDNSInterface_Any, (flags & kDNSServiceFlagsForceMulticast) != 0, FoundInstance, x);
	if (err) { mDNSPlatformMemFree(x); errormsg = "mDNS_StartBrowse"; goto fail; }

	// Succeeded: Wrap up and return
	*sdRef = (DNSServiceRef)x;
	return(mStatus_NoError);

badregtype:
    errormsg = "Illegal regtype";
badparam:
	err = mStatus_BadParamErr;
fail:
	LogMsg("DNSServiceBrowse(\"%s\", \"%s\") failed: %s (%ld)", regtype, domain, errormsg, err);
	return(err);
	}

//*************************************************************************************************************
// Resolve Service Info

static void DNSServiceResolveDispose(mDNS_DirectOP *op)
	{
	mDNS_DirectOP_Resolve *x = (mDNS_DirectOP_Resolve*)op;
	if (x->qSRV.ThisQInterval >= 0) mDNS_StopQuery(&mDNSStorage, &x->qSRV);
	if (x->qTXT.ThisQInterval >= 0) mDNS_StopQuery(&mDNSStorage, &x->qTXT);
	mDNSPlatformMemFree(x);
	}

mDNSlocal void FoundServiceInfo(mDNS *const m, DNSQuestion *question, const ResourceRecord *const answer, QC_result AddRecord)
	{
	mDNS_DirectOP_Resolve *x = (mDNS_DirectOP_Resolve*)question->QuestionContext;
	(void)m;	// Unused
	if (!AddRecord)
		{
		if (answer->rrtype == kDNSType_SRV && x->SRV == answer) x->SRV = mDNSNULL;
		if (answer->rrtype == kDNSType_TXT && x->TXT == answer) x->TXT = mDNSNULL;
		}
	else
		{
		if (answer->rrtype == kDNSType_SRV) x->SRV = answer;
		if (answer->rrtype == kDNSType_TXT) x->TXT = answer;
		if (x->SRV && x->TXT && x->callback)
			{
			char fullname[MAX_ESCAPED_DOMAIN_NAME], targethost[MAX_ESCAPED_DOMAIN_NAME];
		    ConvertDomainNameToCString(answer->name, fullname);
		    ConvertDomainNameToCString(&x->SRV->rdata->u.srv.target, targethost);
			x->callback((DNSServiceRef)x, 0, 0, kDNSServiceErr_NoError, fullname, targethost,
				x->SRV->rdata->u.srv.port.NotAnInteger, x->TXT->rdlength, (unsigned char*)x->TXT->rdata->u.txt.c, x->context);
			}
		}
	}

DNSServiceErrorType DNSSD_API DNSServiceResolve
	(
	DNSServiceRef                       *sdRef,
	DNSServiceFlags                     flags,
	uint32_t                            interfaceIndex,
	const char                          *name,
	const char                          *regtype,
	const char                          *domain,
	DNSServiceResolveReply              callback,
	void                                *context  /* may be NULL */
	)
	{
	mStatus err = mStatus_NoError;
	const char *errormsg = "Unknown";
	domainlabel n;
	domainname t, d, srv;
	mDNS_DirectOP_Resolve *x;

	(void)flags;			// Unused
	(void)interfaceIndex;	// Unused

	// Check parameters
	if (!name[0]    || !MakeDomainLabelFromLiteralString(&n, name  )) { errormsg = "Bad Instance Name"; goto badparam; }
	if (!regtype[0] || !MakeDomainNameFromDNSNameString(&t, regtype)) { errormsg = "Bad Service Type";  goto badparam; }
	if (!domain[0]  || !MakeDomainNameFromDNSNameString(&d, domain )) { errormsg = "Bad Domain";        goto badparam; }
	if (!ConstructServiceName(&srv, &n, &t, &d))                      { errormsg = "Bad Name";          goto badparam; }

	// Allocate memory, and handle failure
	x = (mDNS_DirectOP_Resolve *)mDNSPlatformMemAllocate(sizeof(*x));
	if (!x) { err = mStatus_NoMemoryErr; errormsg = "No memory"; goto fail; }

	// Set up object
	x->disposefn = DNSServiceResolveDispose;
	x->callback  = callback;
	x->context   = context;
	x->SRV       = mDNSNULL;
	x->TXT       = mDNSNULL;

	x->qSRV.ThisQInterval       = -1;		// So that DNSServiceResolveDispose() knows whether to cancel this question
	x->qSRV.InterfaceID         = mDNSInterface_Any;
	x->qSRV.Target              = zeroAddr;
	AssignDomainName(&x->qSRV.qname, &srv);
	x->qSRV.qtype               = kDNSType_SRV;
	x->qSRV.qclass              = kDNSClass_IN;
	x->qSRV.LongLived           = mDNSfalse;
	x->qSRV.ExpectUnique        = mDNStrue;
	x->qSRV.ForceMCast          = mDNSfalse;
	x->qSRV.ReturnIntermed      = mDNSfalse;
	x->qSRV.SuppressUnusable    = mDNSfalse;
	x->qSRV.SearchListIndex     = 0;
	x->qSRV.AppendSearchDomains = 0;
	x->qSRV.RetryWithSearchDomains = mDNSfalse;
	x->qSRV.TimeoutQuestion     = 0;
	x->qSRV.WakeOnResolve       = 0;
	x->qSRV.qnameOrig           = mDNSNULL;
	x->qSRV.QuestionCallback    = FoundServiceInfo;
	x->qSRV.QuestionContext     = x;

	x->qTXT.ThisQInterval       = -1;		// So that DNSServiceResolveDispose() knows whether to cancel this question
	x->qTXT.InterfaceID         = mDNSInterface_Any;
	x->qTXT.Target              = zeroAddr;
	AssignDomainName(&x->qTXT.qname, &srv);
	x->qTXT.qtype               = kDNSType_TXT;
	x->qTXT.qclass              = kDNSClass_IN;
	x->qTXT.LongLived           = mDNSfalse;
	x->qTXT.ExpectUnique        = mDNStrue;
	x->qTXT.ForceMCast          = mDNSfalse;
	x->qTXT.ReturnIntermed      = mDNSfalse;
	x->qTXT.SuppressUnusable    = mDNSfalse;
	x->qTXT.SearchListIndex     = 0;
	x->qTXT.AppendSearchDomains = 0;
	x->qTXT.RetryWithSearchDomains = mDNSfalse;
	x->qTXT.TimeoutQuestion     = 0;
	x->qTXT.WakeOnResolve       = 0;
	x->qTXT.qnameOrig           = mDNSNULL;
	x->qTXT.QuestionCallback    = FoundServiceInfo;
	x->qTXT.QuestionContext     = x;

	err = mDNS_StartQuery(&mDNSStorage, &x->qSRV);
	if (err) { DNSServiceResolveDispose((mDNS_DirectOP*)x); errormsg = "mDNS_StartQuery qSRV"; goto fail; }
	err = mDNS_StartQuery(&mDNSStorage, &x->qTXT);
	if (err) { DNSServiceResolveDispose((mDNS_DirectOP*)x); errormsg = "mDNS_StartQuery qTXT"; goto fail; }

	// Succeeded: Wrap up and return
	*sdRef = (DNSServiceRef)x;
	return(mStatus_NoError);

badparam:
	err = mStatus_BadParamErr;
fail:
	LogMsg("DNSServiceResolve(\"%s\", \"%s\", \"%s\") failed: %s (%ld)", name, regtype, domain, errormsg, err);
	return(err);
	}

//*************************************************************************************************************
// Connection-oriented calls

// Not yet implemented, so don't include in stub library
// We DO include it in the actual Extension, so that if a later client compiled to use this
// is run against this Extension, it will get a reasonable error code instead of just
// failing to launch (Strong Link) or calling an unresolved symbol and crashing (Weak Link)
#if !MDNS_BUILDINGSTUBLIBRARY
DNSServiceErrorType DNSSD_API DNSServiceCreateConnection(DNSServiceRef *sdRef)
	{
	(void)sdRef;	// Unused
	return(kDNSServiceErr_Unsupported);
	}

DNSServiceErrorType DNSSD_API DNSServiceRegisterRecord
	(
	DNSServiceRef                       sdRef,
	DNSRecordRef                        *RecordRef,
	DNSServiceFlags                     flags,
	uint32_t                            interfaceIndex,
	const char                          *fullname,
	uint16_t                            rrtype,
	uint16_t                            rrclass,
	uint16_t                            rdlen,
	const void                          *rdata,
	uint32_t                            ttl,
	DNSServiceRegisterRecordReply       callback,
	void                                *context    /* may be NULL */
	)
	{
	(void)sdRef;			// Unused
	(void)RecordRef;		// Unused
	(void)flags;			// Unused
	(void)interfaceIndex;	// Unused
	(void)fullname;			// Unused
	(void)rrtype;			// Unused
	(void)rrclass;			// Unused
	(void)rdlen;			// Unused
	(void)rdata;			// Unused
	(void)ttl;				// Unused
	(void)callback;			// Unused
	(void)context;			// Unused
	return(kDNSServiceErr_Unsupported);
	}
#endif

//*************************************************************************************************************
// DNSServiceQueryRecord

static void DNSServiceQueryRecordDispose(mDNS_DirectOP *op)
	{
	mDNS_DirectOP_QueryRecord *x = (mDNS_DirectOP_QueryRecord*)op;
	if (x->q.ThisQInterval >= 0) mDNS_StopQuery(&mDNSStorage, &x->q);
	mDNSPlatformMemFree(x);
	}

mDNSlocal void DNSServiceQueryRecordResponse(mDNS *const m, DNSQuestion *question, const ResourceRecord *const answer, QC_result AddRecord)
	{
	mDNS_DirectOP_QueryRecord *x = (mDNS_DirectOP_QueryRecord*)question->QuestionContext;
	char fullname[MAX_ESCAPED_DOMAIN_NAME];
	(void)m;	// Unused
	ConvertDomainNameToCString(answer->name, fullname);
	x->callback((DNSServiceRef)x, AddRecord ? kDNSServiceFlagsAdd : (DNSServiceFlags)0, 0, kDNSServiceErr_NoError,
		fullname, answer->rrtype, answer->rrclass, answer->rdlength, answer->rdata->u.data, answer->rroriginalttl, x->context);
	}

DNSServiceErrorType DNSSD_API DNSServiceQueryRecord
	(
	DNSServiceRef                       *sdRef,
	DNSServiceFlags                     flags,
	uint32_t                            interfaceIndex,
	const char                          *fullname,
	uint16_t                            rrtype,
	uint16_t                            rrclass,
	DNSServiceQueryRecordReply          callback,
	void                                *context  /* may be NULL */
	)
	{
	mStatus err = mStatus_NoError;
	const char *errormsg = "Unknown";
	mDNS_DirectOP_QueryRecord *x;

	(void)flags;			// Unused
	(void)interfaceIndex;	// Unused

	// Allocate memory, and handle failure
	x = (mDNS_DirectOP_QueryRecord *)mDNSPlatformMemAllocate(sizeof(*x));
	if (!x) { err = mStatus_NoMemoryErr; errormsg = "No memory"; goto fail; }

	// Set up object
	x->disposefn = DNSServiceQueryRecordDispose;
	x->callback  = callback;
	x->context   = context;

	x->q.ThisQInterval       = -1;		// So that DNSServiceResolveDispose() knows whether to cancel this question
	x->q.InterfaceID         = mDNSInterface_Any;
	x->q.Target              = zeroAddr;
	MakeDomainNameFromDNSNameString(&x->q.qname, fullname);
	x->q.qtype               = rrtype;
	x->q.qclass              = rrclass;
	x->q.LongLived           = (flags & kDNSServiceFlagsLongLivedQuery) != 0;
	x->q.ExpectUnique        = mDNSfalse;
	x->q.ForceMCast          = (flags & kDNSServiceFlagsForceMulticast) != 0;
	x->q.ReturnIntermed      = (flags & kDNSServiceFlagsReturnIntermediates) != 0;
    x->q.SuppressUnusable    = (flags & kDNSServiceFlagsSuppressUnusable) != 0;
	x->q.SearchListIndex     = 0;
	x->q.AppendSearchDomains = 0;
	x->q.RetryWithSearchDomains = mDNSfalse;
	x->q.WakeOnResolve       = 0;
	x->q.qnameOrig           = mDNSNULL;
	x->q.QuestionCallback    = DNSServiceQueryRecordResponse;
	x->q.QuestionContext     = x;

	err = mDNS_StartQuery(&mDNSStorage, &x->q);
	if (err) { DNSServiceResolveDispose((mDNS_DirectOP*)x); errormsg = "mDNS_StartQuery"; goto fail; }

	// Succeeded: Wrap up and return
	*sdRef = (DNSServiceRef)x;
	return(mStatus_NoError);

fail:
	LogMsg("DNSServiceQueryRecord(\"%s\", %d, %d) failed: %s (%ld)", fullname, rrtype, rrclass, errormsg, err);
	return(err);
	}

//*************************************************************************************************************
// DNSServiceGetAddrInfo

static void DNSServiceGetAddrInfoDispose(mDNS_DirectOP *op)
	{
	mDNS_DirectOP_GetAddrInfo *x = (mDNS_DirectOP_GetAddrInfo*)op;
	if (x->aQuery) DNSServiceRefDeallocate(x->aQuery);
	mDNSPlatformMemFree(x);
	}

static void DNSSD_API DNSServiceGetAddrInfoResponse(
	DNSServiceRef		inRef,
	DNSServiceFlags		inFlags,
	uint32_t			inInterfaceIndex,
	DNSServiceErrorType	inErrorCode,
	const char *		inFullName,
	uint16_t			inRRType,
	uint16_t			inRRClass,
	uint16_t			inRDLen,
	const void *		inRData,
	uint32_t			inTTL,
	void *				inContext )
	{
	mDNS_DirectOP_GetAddrInfo *x = (mDNS_DirectOP_GetAddrInfo*)inContext;
    struct sockaddr_in        sa4;
    (void)inRef;      // Unused
    (void)inRRClass;  // Unused
    (void)inRDLen;    // Unused

	
	mDNSPlatformMemZero(&sa4, sizeof(sa4));
	if (inErrorCode == kDNSServiceErr_NoError && inRRType == kDNSServiceType_A)
		{
		sa4.sin_family = AF_INET;
		mDNSPlatformMemCopy(&sa4.sin_addr.s_addr, inRData, 4);
		}
	
	x->callback((DNSServiceRef)x, inFlags, inInterfaceIndex, inErrorCode, inFullName, 
		(const struct sockaddr *) &sa4, inTTL, x->context);
	}

DNSServiceErrorType DNSSD_API DNSServiceGetAddrInfo(
	DNSServiceRef *				outRef,
	DNSServiceFlags				inFlags,
	uint32_t					inInterfaceIndex,
	DNSServiceProtocol			inProtocol,
	const char *				inHostName,
	DNSServiceGetAddrInfoReply	inCallback,
	void *						inContext )
	{
	const char *					errormsg = "Unknown";
	DNSServiceErrorType				err;
	mDNS_DirectOP_GetAddrInfo *		x;
	
	// Allocate memory, and handle failure
	x = (mDNS_DirectOP_GetAddrInfo *)mDNSPlatformMemAllocate(sizeof(*x));
	if (!x) { err = mStatus_NoMemoryErr; errormsg = "No memory"; goto fail; }
	
	// Set up object
	x->disposefn = DNSServiceGetAddrInfoDispose;
	x->callback  = inCallback;
	x->context   = inContext;
	x->aQuery    = mDNSNULL;
	
	// Start the query.
	// (It would probably be more efficient to code this using mDNS_StartQuery directly,
	// instead of wrapping DNSServiceQueryRecord, which then unnecessarily allocates
	// more memory and then just calls through to mDNS_StartQuery. -- SC June 2010)
	err = DNSServiceQueryRecord(&x->aQuery, inFlags, inInterfaceIndex, inHostName, kDNSServiceType_A, 
		kDNSServiceClass_IN, DNSServiceGetAddrInfoResponse, x);
	if (err) { DNSServiceGetAddrInfoDispose((mDNS_DirectOP*)x); errormsg = "DNSServiceQueryRecord"; goto fail; }
	
	*outRef = (DNSServiceRef)x;
	return(mStatus_NoError);
	
fail:
	LogMsg("DNSServiceGetAddrInfo(\"%s\", %d) failed: %s (%ld)", inHostName, inProtocol, errormsg, err);
	return(err);
	}

//*************************************************************************************************************
// DNSServiceReconfirmRecord

// Not yet implemented, so don't include in stub library
// We DO include it in the actual Extension, so that if a later client compiled to use this
// is run against this Extension, it will get a reasonable error code instead of just
// failing to launch (Strong Link) or calling an unresolved symbol and crashing (Weak Link)
#if !MDNS_BUILDINGSTUBLIBRARY
DNSServiceErrorType DNSSD_API DNSServiceReconfirmRecord
	(
	DNSServiceFlags                    flags,
	uint32_t                           interfaceIndex,
	const char                         *fullname,
	uint16_t                           rrtype,
	uint16_t                           rrclass,
	uint16_t                           rdlen,
	const void                         *rdata
	)
	{
	(void)flags;			// Unused
	(void)interfaceIndex;	// Unused
	(void)fullname;			// Unused
	(void)rrtype;			// Unused
	(void)rrclass;			// Unused
	(void)rdlen;			// Unused
	(void)rdata;			// Unused
	return(kDNSServiceErr_Unsupported);
	}
#endif
