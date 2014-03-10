#-------------------------------------------------
#
# Project created by QtCreator 2013-01-13T23:27:31
#
#-------------------------------------------------

# CONFIG += ArnLibCompile

# Usage of internal mDNS code (no external dependency)
CONFIG += mDnsIntern

QT       += core gui
QT       += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ArnDemoChat
TEMPLATE = app
OBJECTS_DIR = tmp
MOC_DIR = tmp
UI_DIR = tmp


SOURCES += main.cpp\
        MainWindow.cpp

HEADERS  += MainWindow.hpp \
    ../ArnDemoChatServer/ChatSapi.hpp

FORMS    += MainWindow.ui

greaterThan(QT_MAJOR_VERSION, 4) {
    ARNLIB = Arn5
} else {
    ARNLIB = Arn4
}

ArnLibCompile {
    ARN += client
    ARN += discover
    CONFIG += mDnsIntern
    include(../../src/ArnLib.pri)
} else {
    win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../release/ -l$${ARNLIB}
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../debug/ -l$${ARNLIB}
    else:unix: LIBS += -L$$OUT_PWD/../../ -l$${ARNLIB}
}

INCLUDEPATH += $$PWD/../../src
