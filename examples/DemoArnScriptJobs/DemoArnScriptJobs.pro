CONFIG += ArnLibCompile

# Usage of internal mDNS code (no external dependency)
# CONFIG += mDnsIntern

QT += network
QT -= gui
TARGET = DemoArnScriptJobs
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
OBJECTS_DIR = tmp
MOC_DIR = tmp

SOURCES += \
        ScriptMain.cpp \
        main.cpp

HEADERS += \
    ScriptMain.hpp

DISTFILES += \
    demo.js

# OTHER_FILES += \

# INCLUDEPATH += src $$PWD/../include

greaterThan(QT_MAJOR_VERSION, 4) {
    ARNLIB = Arn5
} else {
    ARNLIB = Arn4
}

ArnLibCompile {
    ARN += client
    ARN += scriptjs
    include(../../src/ArnLib.pri)
    INCLUDEPATH += $$PWD/../ArnLib/src
} else {
    win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../ArnLib/release/ -l$${ARNLIB}
    else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../ArnLib/debug/ -l$${ARNLIB}
    else:unix: LIBS += -L$$OUT_PWD/../ArnLib/ -l$${ARNLIB}
}

!mDnsIntern {
    win32:CONFIG(release, debug|release): LIBS +=  -ldns_sd
    else:win32:CONFIG(debug, debug|release): LIBS +=  -ldns_sd
    else:unix: LIBS += -ldns_sd
}
