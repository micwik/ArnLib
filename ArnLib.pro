# -------------------------------------------------
# Project created by QtCreator 2010-08-10T13:51:16
# -------------------------------------------------

PROJECT = ArnLib
ARNLIBVER = 1.0.0

!win32:VERSION = $$ARNLIBVER

DEFINES += LIBVER=\\\"$${ARNLIBVER}\\\"

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
SOURCES += src/Arn.cpp \
    src/ArnClient.cpp \
    src/ArnItem.cpp \
    src/ArnLink.cpp \
    src/ArnServer.cpp \
    src/XStringMap.cpp \
    src/ArnItemNet.cpp \
    src/ArnScript.cpp \
    src/ArnScriptJobs.cpp \
    src/ArnPersist.cpp \
    src/ArnDepend.cpp \
    src/ArnRpc.cpp \
    src/ArnScriptJob.cpp \
    src/ArnSapi.cpp \
    src/ArnMonitor.cpp \
    src/ArnSync.cpp
HEADERS += src/ArnLib_global.hpp \
    src/Arn.hpp \
    src/ArnClient.hpp \
    src/ArnItem.hpp \
    src/ArnLink.hpp \
    src/ArnServer.hpp \
    src/XStringMap.hpp \
    src/ArnError.hpp \
    src/ArnItemNet.hpp \
    src/ArnScript.hpp \
    src/ArnScriptJobs.hpp \
    src/ArnPersist.hpp \
    src/ArnDepend.hpp \
    src/MQFlags.hpp \
    src/ArnLib.hpp \
    src/ArnRpc.hpp \
    src/ArnScriptJob.hpp \
    src/ArnSapi.hpp \
    src/ArnPersistSapi.hpp \
    src/ArnMonitor.hpp \
    src/ArnSync.hpp


### INSTALL ###

headers.path = /usr/include/ArnLib
headers.files += *.hpp
target.path = /usr/lib64
INSTALLS += target \
    headers

OTHER_FILES += \
    doc/description.txt \
    README.md
