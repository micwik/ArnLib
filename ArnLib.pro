# -------------------------------------------------
# Project created by QtCreator 2010-08-10T13:51:16
# -------------------------------------------------

PROJECT = ArnLib
ARNLIBVER = 1.0.2

!win32:VERSION = $$ARNLIBVER

DEFINES += LIBVER=\\\"$${ARNLIBVER}\\\"

win32 {
DEFINES += BUILDDATE=\\\"$$system('echo %date%')\\\"
DEFINES += BUILDTIME=\\\"00:00\\\"
# DEFINES += BUILDTIME=\\\"$$system('echo %time%')\\\"
} else {
DEFINES += BUILDTIME=\\\"$$system(date '+%H:%M')\\\"
DEFINES += BUILDDATE=\\\"$$system(date '+%y-%m-%d')\\\"
DEFINES += DUMMY=\\\"$$system(rm tmp/Arn.o)\\\"
}

QT += network
QT += script
QT += sql
QT -= gui

greaterThan(QT_MAJOR_VERSION, 4) {
    TARGET = Arn5
} else {
    TARGET = Arn
}
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

OTHER_FILES += \
    doc/Internals.md \
    README.md \
    doc/Description.md \
    examples/Examples.txt


### Install
win32 {
headers.path = $$OUT_PWD/../include/ArnLib
target.path = $$OUT_PWD/../lib
} else {
headers.path = /usr/include/ArnLib
target.path = /usr/lib64
}
headers.files += src/*.hpp
INSTALLS += target \
    headers


### Custom target 'doc' in *.pro file

doc.commands = doxygen Doxyfile
doc.depends = FORCE
QMAKE_EXTRA_TARGETS += doc
QMAKE_DISTCLEAN += doc/doxy_html/* doc/doxy_html/search/*
