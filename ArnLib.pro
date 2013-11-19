# -------------------------------------------------
# Project created by QtCreator 2010-08-10T13:51:16
# -------------------------------------------------

PROJECT = ArnLib

# ARN += core       Basic Arn functionality without any tcp and syncing
# ARN += client     Client TCP functionality with sync etc
# ARN += server     Server TCP functionality with persistence etc
ARN += server
QT -= gui

include(src/ArnLib.pri)


win32 {
} else {
DEFINES += DUMMY=\\\"$$system(rm tmp/Arn.o)\\\"
}

greaterThan(QT_MAJOR_VERSION, 4) {
    TARGET = Arn5
} else {
    TARGET = Arn
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
    ArnLib.pri


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
