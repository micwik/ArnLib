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
        $$PWD/XStringMap.cpp \
        $$PWD/ArnPipe.cpp \
        $$PWD/ArnItemB.cpp

    HEADERS += $$PWD/ArnLib_global.hpp \
        $$PWD/Arn.hpp \
        $$PWD/ArnItem.hpp \
        $$PWD/ArnLink.hpp \
        $$PWD/XStringMap.hpp \
        $$PWD/ArnError.hpp \
        $$PWD/MQFlags.hpp \
        $$PWD/ArnLib.hpp \
        $$PWD/ArnPipe.hpp \
        $$PWD/ArnDefs.hpp \
        $$PWD/ArnItemB.hpp
}

}  # ARNLIB_PRI_INCLUDED
