# include guard against multiple inclusion
isEmpty(ARNLIB_PRI_INCLUDED) {
ARNLIB_PRI_INCLUDED = 1

# Don't forget to change in Doxygen config
ARNLIBVER = 3.1.1

DEFINES += ARNLIBVER=\\\"$${ARNLIBVER}\\\"

win32 {
    DEFINES += ARNBUILDDATE=\\\"$$system('echo %date%')\\\"
    DEFINES += ARNBUILDTIME=\\\"00:00\\\"
    # DEFINES += ARNBUILDTIME=\\\"$$system('echo %time%')\\\"
} else {
    DEFINES += ARNBUILDTIME=\\\"$$system(date '+%H:%M')\\\"
    DEFINES += ARNBUILDDATE=\\\"$$system(date '+%y-%m-%d')\\\"
}

ArnLibCompile {
    DEFINES += ARNLIB_COMPILE
    warning("Warning: LGPL is not valid!!! ArnLib is statically linked into application")
}

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/ArnXStringMap.cpp \
    $$PWD/Arn.cpp \
    $$PWD/ArnLib.cpp \
    $$PWD/MQFlags.cpp

HEADERS += \
    $$PWD/ArnInc/ArnLib_global.hpp \
    $$PWD/ArnInc/XStringMap.hpp \
    $$PWD/ArnInc/ArnError.hpp \
    $$PWD/ArnInc/Arn.hpp \
    $$PWD/ArnInc/ArnLib.hpp \
    $$PWD/ArnInc/MQFlags.hpp

RESOURCES += \
    $$PWD/../resource/ArnLib.qrc \
    $$PWD/../legal/ArnLicenses.qrc



contains(ARN, server) {
    ARN += client
    ARN += script
    QT += sql
    SOURCES += \
        $$PWD/ArnServer.cpp \
        $$PWD/ArnServerRemote.cpp \
        $$PWD/ArnPersist.cpp

    HEADERS += \
        $$PWD/ArnInc/ArnServer.hpp \
        $$PWD/ArnInc/ArnServerRemote.hpp \
        $$PWD/ArnInc/ArnPersist.hpp \
        $$PWD/private/ArnServer_p.hpp \
        $$PWD/private/ArnServerRemote_p.hpp \
        $$PWD/private/ArnPersist_p.hpp
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
        $$PWD/ArnSync.cpp \
        $$PWD/ArnSyncLogin.cpp

    HEADERS += \
        $$PWD/ArnInc/ArnClient.hpp \
        $$PWD/ArnItemNet.hpp \
        $$PWD/ArnInc/ArnDepend.hpp \
        $$PWD/ArnInc/ArnRpc.hpp \
        $$PWD/ArnInc/ArnSapi.hpp \
        $$PWD/ArnInc/ArnPersistSapi.hpp \
        $$PWD/ArnInc/ArnMonitor.hpp \
        $$PWD/ArnInc/ArnMonEvent.hpp \
        $$PWD/ArnSync.hpp \
        $$PWD/ArnSyncLogin.hpp \
        $$PWD/private/ArnClient_p.hpp \
        $$PWD/private/ArnDepend_p.hpp \
        $$PWD/private/ArnRpc_p.hpp \
        $$PWD/private/ArnSapi_p.hpp \
        $$PWD/private/ArnMonitor_p.hpp
}


contains(ARN, script) {
    ARN += scriptcommon
    QT += script
}


contains(ARN, scriptjs) {
  lessThan(QT_MAJOR_VERSION, 5) {
    error("scriptjs not available before QT5, use script")
  }

    ARN += scriptcommon
    QT += qml
    DEFINES += ARNUSE_SCRIPTJS
}


contains(ARN, scriptcommon) {
    ARN += core
    ARN += script_qml
    SOURCES += \
        $$PWD/ArnScript.cpp \
        $$PWD/ArnScriptJobs.cpp \
        $$PWD/ArnScriptJob.cpp

    HEADERS += \
        $$PWD/ArnInc/ArnScript.hpp \
        $$PWD/ArnInc/ArnScriptJobs.hpp \
        $$PWD/ArnInc/ArnScriptJob.hpp
}


contains(ARN, qml) {
    ARN += core
    ARN += script_qml
    greaterThan(QT_MAJOR_VERSION, 4) {
        QT += qml quick
    } else {
        QT += declarative
    }
    SOURCES += \
        $$PWD/ArnQml.cpp \
        $$PWD/ArnQmlMSystem.cpp \
        $$PWD/ArnQmlMQt.cpp

    HEADERS += \
        $$PWD/ArnInc/ArnQml.hpp \
        $$PWD/ArnInc/ArnQmlMSystem.hpp \
        $$PWD/ArnInc/ArnQmlMQt.hpp
}


contains(ARN, script_qml) {
    HEADERS += \
        $$PWD/ArnInc/ArnInterface.hpp
}


contains(ARN, core) {
    SOURCES += \
        $$PWD/ArnM.cpp \
        $$PWD/ArnBasicItem.cpp \
        $$PWD/ArnAdaptItem.cpp \
        $$PWD/ArnItem.cpp \
        $$PWD/ArnItemValve.cpp \
        $$PWD/ArnLinkHandle.cpp \
        $$PWD/ArnLink.cpp \
        $$PWD/ArnEvent.cpp \
        $$PWD/ArnPipe.cpp \
        $$PWD/ArnCoreItem.cpp \
        $$PWD/ArnItemB.cpp

    HEADERS += \
        $$PWD/ArnInc/ArnM.hpp \
        $$PWD/ArnInc/ArnBasicItem.hpp \
        $$PWD/ArnInc/ArnAdaptItem.hpp \
        $$PWD/ArnInc/ArnItem.hpp \
        $$PWD/ArnInc/ArnItemValve.hpp \
        $$PWD/ArnInc/ArnPipe.hpp \
        $$PWD/ArnInc/ArnCoreItem.hpp \
        $$PWD/ArnInc/ArnItemB.hpp \
        $$PWD/ArnInc/ArnLinkHandle.hpp \
        $$PWD/ArnInc/ArnEvent.hpp \
        $$PWD/ArnLink.hpp \
        $$PWD/private/ArnBasicItem_p.hpp \
        $$PWD/private/ArnAdaptItem_p.hpp \
        $$PWD/private/ArnItemB_p.hpp \
        $$PWD/private/ArnItem_p.hpp \
        $$PWD/private/ArnItemValve_p.hpp \
        $$PWD/private/ArnPipe_p.hpp
}


contains(ARN, discover) {
    ARN += zeroconf
    SOURCES += $$PWD/ArnDiscover.cpp
    HEADERS += \
        $$PWD/ArnInc/ArnDiscover.hpp \
        $$PWD/private/ArnDiscover_p.hpp

    contains(ARN, server) {
        SOURCES += $$PWD/ArnDiscoverRemote.cpp
        HEADERS += \
            $$PWD/ArnInc/ArnDiscoverRemote.hpp \
            $$PWD/private/ArnDiscoverRemote_p.hpp
    }

    contains(ARN, client) {
        SOURCES += $$PWD/ArnDiscoverConnect.cpp
        HEADERS += \
            $$PWD/ArnInc/ArnDiscoverConnect.hpp \
            $$PWD/private/ArnDiscoverConnect_p.hpp
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
    }
}

}  # ARNLIB_PRI_INCLUDED
