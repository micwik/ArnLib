#-------------------------------------------------
#
# Project created by QtCreator 2013-01-14T23:59:30
#
#-------------------------------------------------

# CONFIG += ArnLibCompile

# Usage of internal mDNS code (no external dependency)
CONFIG += mDnsIntern

QT       += core
QT       += network

TARGET = ArnDemoChatServer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app
OBJECTS_DIR = tmp
MOC_DIR = tmp


SOURCES += main.cpp \
    ServerMain.cpp

HEADERS += \
    ServerMain.hpp \
    ChatSapi.hpp

greaterThan(QT_MAJOR_VERSION, 4) {
    ARNLIB = Arn5
} else {
    ARNLIB = Arn4
}

ArnLibCompile {
    ARN += server
    ARN += discover
    CONFIG += mDnsIntern
    include(../../src/ArnLib.pri)
} else {
    win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../release/ -l$${ARNLIB}
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../debug/ -l$${ARNLIB}
    else:unix: LIBS += -L$$OUT_PWD/../../ -l$${ARNLIB}
}

INCLUDEPATH += $$PWD/../../src
