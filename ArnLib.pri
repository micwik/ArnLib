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

INCLUDEPATH += $$PWD/src

contains(ARN, server) {
    ARN += client
    SOURCES += $$PWD/src/ArnServer.cpp \
        $$PWD/src/ArnScript.cpp \
        $$PWD/src/ArnScriptJobs.cpp \
        $$PWD/src/ArnPersist.cpp \
        $$PWD/src/ArnScriptJob.cpp

    HEADERS +=$$PWD/src/ArnServer.hpp \
        $$PWD/src/ArnScript.hpp \
        $$PWD/src/ArnScriptJobs.hpp \
        $$PWD/src/ArnPersist.hpp \
        $$PWD/src/ArnScriptJob.hpp
}

contains(ARN, client) {
    ARN += core
    SOURCES += $$PWD/src/ArnClient.cpp \
        $$PWD/src/ArnItemNet.cpp \
        $$PWD/src/ArnDepend.cpp \
        $$PWD/src/ArnRpc.cpp \
        $$PWD/src/ArnSapi.cpp \
        $$PWD/src/ArnMonitor.cpp \
        $$PWD/src/ArnSync.cpp

    HEADERS += $$PWD/src/ArnClient.hpp \
        $$PWD/src/ArnItemNet.hpp \
        $$PWD/src/ArnDepend.hpp \
        $$PWD/src/ArnRpc.hpp \
        $$PWD/src/ArnSapi.hpp \
        $$PWD/src/ArnPersistSapi.hpp \
        $$PWD/src/ArnMonitor.hpp \
        $$PWD/src/ArnSync.hpp
}

contains(ARN, core) {
    SOURCES += $$PWD/src/Arn.cpp \
        $$PWD/src/ArnItem.cpp \
        $$PWD/src/ArnLink.cpp \
        $$PWD/src/XStringMap.cpp \
        $$PWD/src/ArnPipe.cpp \
        $$PWD/src/ArnItemB.cpp

    HEADERS += $$PWD/src/ArnLib_global.hpp \
        $$PWD/src/Arn.hpp \
        $$PWD/src/ArnItem.hpp \
        $$PWD/src/ArnLink.hpp \
        $$PWD/src/XStringMap.hpp \
        $$PWD/src/ArnError.hpp \
        $$PWD/src/MQFlags.hpp \
        $$PWD/src/ArnLib.hpp \
        $$PWD/src/ArnPipe.hpp \
        $$PWD/src/ArnDefs.hpp \
        $$PWD/src/ArnItemB.hpp
}

}  # ARNLIB_PRI_INCLUDED
