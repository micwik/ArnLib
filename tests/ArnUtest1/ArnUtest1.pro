#-------------------------------------------------
#
# Project created by QtCreator 2015-12-03T23:05:17
#
#-------------------------------------------------

CONFIG += ArnLibCompile

# Usage of internal mDNS code (no external dependency)
# CONFIG += mDnsIntern

QT       -= gui
QT       += testlib
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat

TARGET = ArnUtest1
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-deprecated-declarations -Wno-deprecated-copy

greaterThan(QT_MAJOR_VERSION, 5) {
    ARNLIB = Arn6
} else {
    greaterThan(QT_MAJOR_VERSION, 4) {
        ARNLIB = Arn5
    } else {
        ARNLIB = Arn4
    }
}

ArnLibCompile {
    ARN += core
    ARN += client
    ARN += qml
    #ARN += discover
    include(../../src/ArnLib.pri)
    INCLUDEPATH += $$PWD/../../src
} else {
    win32: INCLUDEPATH += $$PWD/../../src

    win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../release/ -l$${ARNLIB}
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../debug/ -l$${ARNLIB}
    else:unix: LIBS += -L$$OUT_PWD/../../ -l$${ARNLIB}
}

!mDnsIntern {
    win32:CONFIG(release, debug|release): LIBS +=  -ldns_sd
    else:win32:CONFIG(debug, debug|release): LIBS +=  -ldns_sd
    else:unix: LIBS += -ldns_sd
}


SOURCES += ArnUtest1.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += \
    TestMQFlags.hpp
