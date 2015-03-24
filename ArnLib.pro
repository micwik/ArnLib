# -------------------------------------------------
# Project created by QtCreator 2010-08-10T13:51:16
# -------------------------------------------------

PROJECT = ArnLib

# ARN += core       # Level 1: Basic Arn functionality without any tcp and syncing
# ARN += client     # Level 2: Client TCP functionality with sync etc
# ARN += server     # Level 3: Server TCP functionality with persistence etc
# ARN += script     # Java script support
# ARN += qml        # QML support
# ARN += zeroconf   # Using part of Bonjour (R), Apple's (R) implementation of zero-configuration networking.
# ARN += discover   # High level service discovery using <zeroconf> and optionally <server> for remote config
ARN += server
ARN += discover
ARN += qml
QT -= gui

# Usage of internal mDNS code (no external dependency).
CONFIG += mDnsIntern

# Usage of float as real type, default is double. Must be same in application pro-file.
# DEFINES += ARNREAL_FLOAT

include(src/ArnLib.pri)

!mDnsIntern {
    win32:CONFIG(release, debug|release): LIBS +=  -ldns_sd
    else:win32:CONFIG(debug, debug|release): LIBS +=  -ldns_sd
    else:unix: LIBS += -ldns_sd
}

win32 {
} else {
DEFINES += DUMMY=\\\"$$system(rm tmp/ArnM.o)\\\"
}

greaterThan(QT_MAJOR_VERSION, 4) {
    TARGET = Arn5
} else {
    TARGET = Arn4
}
TEMPLATE = lib

!win32:VERSION = $$ARNLIBVER

# CONFIG += staticlib
DEFINES += ARNLIB_LIBRARY
OBJECTS_DIR = tmp
MOC_DIR = tmp

OTHER_FILES += \
    doc/Internals.md \
    README.md \
    doc/Description.md \
    examples/Examples.txt \
    ArnLib.pri \
    doc/Install.md \
    doc/Todo.md


### Install
win32 {
headers.path = $$OUT_PWD/../include/ArnInc
target.path = $$OUT_PWD/../lib
} else {
headers.path = /usr/include/ArnInc
target.path = /usr/lib64
}
headers.files += src/ArnInc/*
INSTALLS += target \
    headers


### Custom target 'doc' in *.pro file

doc.commands = doxygen Doxyfile
doc.depends = FORCE
QMAKE_EXTRA_TARGETS += doc
QMAKE_DISTCLEAN += doc/doxy_html/* doc/doxy_html/search/*
