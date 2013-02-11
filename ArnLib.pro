# -------------------------------------------------
# Project created by QtCreator 2010-08-10T13:51:16
# -------------------------------------------------
PROJECT = ArnLib

!win32:VERSION = 1.0.0

win32 {
DEFINES += BUILDTIME=\\\"$$system('echo %time%')\\\"
DEFINES += BUILDDATE=\\\"$$system('echo %date%')\\\"
} else {
DEFINES += BUILDTIME=\\\"$$system(date '+%H:%M')\\\"
DEFINES += BUILDDATE=\\\"$$system(date '+%y-%m-%d')\\\"
DEFINES += DUMMY=\\\"$$system(rm tmp/Arn.o)\\\"
}

QT += network
QT += script
QT += sql
QT -= gui

TARGET = Arn
TEMPLATE = lib

# CONFIG += staticlib
DEFINES += ARNLIB_LIBRARY
OBJECTS_DIR = tmp
MOC_DIR = tmp
SOURCES += Arn.cpp \
    ArnClient.cpp \
    ArnItem.cpp \
    ArnLink.cpp \
    ArnServer.cpp \
    XStringMap.cpp \
    ArnItemNet.cpp \
    ArnScript.cpp \
    ArnScriptJobs.cpp \
    ArnPersist.cpp \
    ArnDepend.cpp \
    ArnRpc.cpp \
    ArnScriptJob.cpp \
    ArnSapi.cpp \
    ArnMonitor.cpp \
    ArnSync.cpp
HEADERS += ArnLib_global.hpp \
    Arn.hpp \
    ArnClient.hpp \
    ArnItem.hpp \
    ArnLink.hpp \
    ArnServer.hpp \
    XStringMap.hpp \
    ArnError.hpp \
    ArnItemNet.hpp \
    ArnScript.hpp \
    ArnScriptJobs.hpp \
    ArnPersist.hpp \
    ArnDepend.hpp \
    MQFlags.hpp \
    ArnLib.hpp \
    ArnRpc.hpp \
    ArnScriptJob.hpp \
    ArnSapi.hpp \
    ArnPersistSapi.hpp \
    ArnMonitor.hpp \
    ArnSync.hpp

# ########
# INSTALL#
# ########
headers.path = /usr/include/ArnLib
headers.files += *.hpp
target.path = /usr/lib64
INSTALLS += target \
    headers

OTHER_FILES += \
    doc/description.txt \
    doc/README










