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

DEFINES += ARNLIB_COMPILE

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/XStringMap.cpp \
    $$PWD/Arn.cpp \
    $$PWD/ArnLib.cpp

HEADERS += \
    $$PWD/ArnInc/ArnLib_global.hpp \
    $$PWD/ArnInc/XStringMap.hpp \
    $$PWD/ArnInc/ArnError.hpp \
    $$PWD/ArnInc/Arn.hpp \
    $$PWD/ArnInc/ArnLib.hpp \
    $$PWD/ArnInc/MQFlags.hpp

RESOURCES += \
    $$PWD/../resource/ArnLib.qrc \
    $$PWD/../legal/ArnLicences.qrc


contains(ARN, server) {
    ARN += client
    QT += script sql
    SOURCES += \
        $$PWD/ArnServer.cpp \
        $$PWD/ArnScript.cpp \
        $$PWD/ArnScriptJobs.cpp \
        $$PWD/ArnPersist.cpp \
        $$PWD/ArnScriptJob.cpp

    HEADERS += \
        $$PWD/ArnInc/ArnServer.hpp \
        $$PWD/ArnInc/ArnScript.hpp \
        $$PWD/ArnInc/ArnScriptJobs.hpp \
        $$PWD/ArnInc/ArnPersist.hpp \
        $$PWD/ArnInc/ArnScriptJob.hpp
}


contains(ARN, client) {
    ARN += core
    QT += network
    SOURCES += \
        $$PWD/ArnClient.cpp \
        $$PWD/ArnItemNet.cpp \
        $$PWD/ArnDepend.cpp \
        $$PWD/ArnRpc.cpp \
        $$PWD/ArnSapi.cpp \
        $$PWD/ArnMonitor.cpp \
        $$PWD/ArnSync.cpp

    HEADERS += \
        $$PWD/ArnInc/ArnClient.hpp \
        $$PWD/ArnInc/ArnDepend.hpp \
        $$PWD/ArnInc/ArnRpc.hpp \
        $$PWD/ArnInc/ArnSapi.hpp \
        $$PWD/ArnInc/ArnPersistSapi.hpp \
        $$PWD/ArnInc/ArnMonitor.hpp \
        $$PWD/ArnItemNet.hpp \
        $$PWD/ArnSync.hpp
}


contains(ARN, core) {
    SOURCES += \
        $$PWD/ArnM.cpp \
        $$PWD/ArnItem.cpp \
        $$PWD/ArnLinkHandle.cpp \
        $$PWD/ArnLink.cpp \
        $$PWD/ArnPipe.cpp \
        $$PWD/ArnItemB.cpp

    HEADERS += \
        $$PWD/ArnInc/ArnM.hpp \
        $$PWD/ArnInc/ArnItem.hpp \
        $$PWD/ArnInc/ArnPipe.hpp \
        $$PWD/ArnInc/ArnItemB.hpp \
        $$PWD/ArnInc/ArnLinkHandle.hpp \
        $$PWD/ArnLink.hpp
}


contains(ARN, discover) {
    ARN += zeroconf
    SOURCES += $$PWD/ArnDiscover.cpp
    HEADERS += $$PWD/ArnInc/ArnDiscover.hpp

    contains(ARN, server) {
        SOURCES += $$PWD/ArnDiscoverRemote.cpp
        HEADERS += $$PWD/ArnInc/ArnDiscoverRemote.hpp
    }

    contains(ARN, client) {
        SOURCES += $$PWD/ArnDiscoverConnect.cpp
        HEADERS += $$PWD/ArnInc/ArnDiscoverConnect.hpp
    }
}


contains(ARN, zeroconf) {
    SOURCES += $$PWD/ArnZeroConf.cpp
    HEADERS += \
        $$PWD/ArnInc/ArnZeroConf.hpp \
        $$PWD/mDNS/mDNSShared/dns_sd.h

    mDnsIntern {
        DEFINES += MDNS_INTERN
        DEFINES += MDNS_HAVE_LOOKUP

        DEFINES += _GNU_SOURCE
        DEFINES += HAVE_IPV6
        DEFINES += NOT_HAVE_SA_LEN
        DEFINES += USES_NETLINK
        DEFINES += HAVE_LINUX
        DEFINES += TARGET_OS_LINUX
        DEFINES += _PLATFORM_HAS_STRONG_PRNG_

        SOURCES += \
            $$PWD/mDNS/mDNSCore/mDNS.c \
            $$PWD/mDNS/mDNSCore/uDNS.c \
            $$PWD/mDNS/mDNSCore/DNSDigest.c \
            $$PWD/mDNS/mDNSCore/DNSCommon.c \
            $$PWD/mDNS/mDNSShared/dnssd_clientshim.c \
            $$PWD/mDNS/mDNSShared/mDNSDebug.c \
            $$PWD/mDNS/mDNSShared/PlatformCommon.c \
            $$PWD/mDNS/mDNSQt/mDNSQt.cpp \
            $$PWD/mDNS/ArnMDns.cpp

        HEADERS += \
            $$PWD/mDNS/mDNSQt/mDNSQt.h \
            $$PWD/mDNS/mDNSCore/DNSCommon.h \
            $$PWD/mDNS/mDNSCore/uDNS.h \
            $$PWD/mDNS/mDNSCore/mDNSEmbeddedAPI.h \
            $$PWD/mDNS/mDNSCore/mDNSDebug.h \
            $$PWD/mDNS/mDNSShared/PlatformCommon.h \
            $$PWD/mDNS/ArnMDns.hpp

        INCLUDEPATH += $$PWD/mDNS/mDNSCore $$PWD/mDNS/mDNSShared
    }
}

}  # ARNLIB_PRI_INCLUDED
