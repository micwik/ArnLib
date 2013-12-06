# include guard against multiple inclusion
isEmpty(ARNL_PRI_INCLUDED) {
ARNLIB_PRI_INCLUDED = 1

ARNLIBVER = 2.0.0

DEFINES += ARNLIBVER=\\\"$${ARNLIBVER}\\\"

win32 {
DEFINES += ARNBUILDDATE=\\\"$$system('echo %date%')\\\"
DEFINES += ARNBUILDTIME=\\\"00:00\\\"
# DEFINES += ARNBUILDTIME=\\\"$$system('echo %time%')\\\"
} else {
DEFINES += ARNBUILDTIME=\\\"$$system(date '+%H:%M')\\\"
DEFINES += ARNBUILDDATE=\\\"$$system(date '+%y-%m-%d')\\\"
}

INCLUDEPATH += $$PWD

SOURCES += $$PWD/XStringMap.cpp

HEADERS += $$PWD/ArnLib_global.hpp \
    $$PWD/XStringMap.hpp \
    $$PWD/ArnError.hpp \
    $$PWD/ArnDefs.hpp \
    $$PWD/MQFlags.hpp


contains(ARN, server) {
    ARN += client
    QT += script sql
    SOURCES += $$PWD/ArnServer.cpp \
        $$PWD/ArnScript.cpp \
        $$PWD/ArnScriptJobs.cpp \
        $$PWD/ArnPersist.cpp \
        $$PWD/ArnScriptJob.cpp

    HEADERS +=$$PWD/ArnServer.hpp \
        $$PWD/ArnScript.hpp \
        $$PWD/ArnScriptJobs.hpp \
        $$PWD/ArnPersist.hpp \
        $$PWD/ArnScriptJob.hpp
}


contains(ARN, client) {
    ARN += core
    QT += network
    SOURCES += $$PWD/ArnClient.cpp \
        $$PWD/ArnItemNet.cpp \
        $$PWD/ArnDepend.cpp \
        $$PWD/ArnRpc.cpp \
        $$PWD/ArnSapi.cpp \
        $$PWD/ArnMonitor.cpp \
        $$PWD/ArnSync.cpp

    HEADERS += $$PWD/ArnClient.hpp \
        $$PWD/ArnItemNet.hpp \
        $$PWD/ArnDepend.hpp \
        $$PWD/ArnRpc.hpp \
        $$PWD/ArnSapi.hpp \
        $$PWD/ArnPersistSapi.hpp \
        $$PWD/ArnMonitor.hpp \
        $$PWD/ArnSync.hpp
}


contains(ARN, core) {
    SOURCES += $$PWD/Arn.cpp \
        $$PWD/ArnItem.cpp \
        $$PWD/ArnLink.cpp \
        $$PWD/ArnPipe.cpp \
        $$PWD/ArnItemB.cpp

    HEADERS += $$PWD/Arn.hpp \
        $$PWD/ArnItem.hpp \
        $$PWD/ArnLink.hpp \
        $$PWD/ArnLib.hpp \
        $$PWD/ArnPipe.hpp \
        $$PWD/ArnItemB.hpp
}


contains(ARN, zeroconf) {
    SOURCES += $$PWD/ArnZeroConf.cpp
    HEADERS += $$PWD/ArnZeroConf.hpp \
        $$PWD/mDNS/mDNSShared/dns_sd.h

    contains(ARN, server) {
        SOURCES += $$PWD/ArnDiscover.cpp
        HEADERS += $$PWD/ArnDiscover.hpp
    }

    mDnsIntern {
        DEFINES += MDNS_INTERN

        DEFINES += _GNU_SOURCE
        DEFINES += HAVE_IPV6
        DEFINES += NOT_HAVE_SA_LEN
        DEFINES += USES_NETLINK
        DEFINES += HAVE_LINUX
        DEFINES += TARGET_OS_LINUX

        SOURCES += $$PWD/mDNS/mDNSCore/mDNS.c \
            $$PWD/mDNS/mDNSCore/uDNS.c \
            $$PWD/mDNS/mDNSCore/DNSDigest.c \
            $$PWD/mDNS/mDNSCore/DNSCommon.c \
            $$PWD/mDNS/mDNSShared/dnssd_clientshim.c \
            $$PWD/mDNS/mDNSShared/mDNSDebug.c \
            $$PWD/mDNS/mDNSShared/PlatformCommon.c \
            $$PWD/mDNS/mDNSQt/mDNSQt.cpp \
            $$PWD/mDNS/ArnMDns.cpp

        HEADERS += $$PWD/mDNS/mDNSQt/mDNSQt.h
        HEADERS += $$PWD/mDNS/mDNSCore/DNSCommon.h \
            $$PWD/mDNS/mDNSCore/uDNS.h \
            $$PWD/mDNS/mDNSCore/mDNSEmbeddedAPI.h \
            $$PWD/mDNS/mDNSCore/mDNSDebug.h \
            $$PWD/mDNS/mDNSShared/PlatformCommon.h \
            $$PWD/mDNS/ArnMDns.hpp

        INCLUDEPATH += $$PWD/mDNS/mDNSCore $$PWD/mDNS/mDNSShared
    }
}

}  # ARNLIB_PRI_INCLUDED
