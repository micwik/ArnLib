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

#include "mDNSDebug.h"
#include "mDNSEmbeddedAPI.h"

mDNSexport int mDNS_LoggingEnabled = 1;
mDNSexport int mDNS_PacketLoggingEnabled = 0;

#if MDNS_DEBUGMSGS
mDNSexport int mDNS_DebugMode = mDNStrue;  // Not used
#else
mDNSexport int mDNS_DebugMode = mDNSfalse;  // Not used
#endif

// Note, this uses mDNS_vsnprintf instead of standard "vsnprintf", because mDNS_vsnprintf knows
// how to print special data types like IP addresses and length-prefixed domain names
#if MDNS_DEBUGMSGS > 1
mDNSexport void verbosedebugf_(const char *format, ...)
	{
	char buffer[512];
	va_list ptr;
	va_start(ptr,format);
	buffer[mDNS_vsnprintf(buffer, sizeof(buffer), format, ptr)] = 0;
	va_end(ptr);
	mDNSPlatformWriteDebugMsg(buffer);
	}
#endif

// Log message with default "mDNSResponder" ident string at the start
mDNSlocal void LogMsgWithLevelv(mDNSLogLevel_t logLevel, const char *format, va_list ptr)
	{
	char buffer[512];
	buffer[mDNS_vsnprintf((char *)buffer, sizeof(buffer), format, ptr)] = 0;
    mDNSPlatformWriteLogMsg(ProgramName, buffer, logLevel);
	}

#define LOG_HELPER_BODY(L) \
	{ \
	va_list ptr; \
	va_start(ptr,format); \
	LogMsgWithLevelv(L, format, ptr); \
	va_end(ptr); \
	}

// see mDNSDebug.h
#if !MDNS_HAS_VA_ARG_MACROS
void LogMsg_(const char *format, ...)       LOG_HELPER_BODY(MDNS_LOG_MSG)
void LogOperation_(const char *format, ...) LOG_HELPER_BODY(MDNS_LOG_OPERATION)
void LogSPS_(const char *format, ...)       LOG_HELPER_BODY(MDNS_LOG_SPS)
void LogInfo_(const char *format, ...)      LOG_HELPER_BODY(MDNS_LOG_INFO)
#endif

#if MDNS_DEBUGMSGS
void debugf_(const char *format, ...)       LOG_HELPER_BODY(MDNS_LOG_DEBUG)
#endif

// Log message with default "mDNSResponder" ident string at the start
mDNSexport void LogMsgWithLevel(mDNSLogLevel_t logLevel, const char *format, ...)
	LOG_HELPER_BODY(logLevel)
