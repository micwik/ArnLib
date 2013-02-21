#-------------------------------------------------
#
# Project created by QtCreator 2013-01-13T23:27:31
#
#-------------------------------------------------

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

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../ArnLib/release/ -lArn
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../ArnLib/debug/ -lArn
else:unix: LIBS += -L$$OUT_PWD/../ArnLib/ -lArn

INCLUDEPATH += $$PWD/..
