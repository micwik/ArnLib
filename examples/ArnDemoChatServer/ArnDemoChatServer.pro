#-------------------------------------------------
#
# Project created by QtCreator 2013-01-14T23:59:30
#
#-------------------------------------------------

QT       += core
QT       += network

//QT       -= gui

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

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../ArnLib/release/ -lArn
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../ArnLib/debug/ -lArn
else:unix: LIBS += -L$$OUT_PWD/../ArnLib/ -lArn

INCLUDEPATH += $$PWD/..
