# include guard against multiple inclusion
isEmpty(ARNCORE_PRI_INCLUDED) {
ARNCORE_PRI_INCLUDED = 1

ARNLIBVER = 2.0.0

DEFINES += ARNLIBVER=\\\"$${ARNLIBVER}\\\"

win32 {
DEFINES += ARNBUILDDATE=\\\"$$system('echo %date%')\\\"
DEFINES += ARNBUILDTIME=\\\"00:00\\\"
# DEFINES += ARNBUILDTIME=\\\"$$system('echo %time%')\\\"
} else {
DEFINES += ARNBUILDTIME=\\\"$$system(date '+%H:%M')\\\"
DEFINES += ARNBUILDDATE=\\\"$$system(date '+%y-%m-%d')\\\"
}

INCLUDEPATH += $$PWD

SOURCES += $$PWD/Arn.cpp \
    $$PWD/ArnItem.cpp \
    $$PWD/ArnLink.cpp \
    $$PWD/XStringMap.cpp \
    $$PWD/ArnPipe.cpp \
    $$PWD/ArnItemB.cpp

HEADERS += $$PWD/ArnLib_global.hpp \
    $$PWD/Arn.hpp \
    $$PWD/ArnItem.hpp \
    $$PWD/ArnLink.hpp \
    $$PWD/XStringMap.hpp \
    $$PWD/ArnError.hpp \
    $$PWD/MQFlags.hpp \
    $$PWD/ArnLib.hpp \
    $$PWD/ArnPipe.hpp \
    $$PWD/ArnDefs.hpp \
    $$PWD/ArnItemB.hpp

}  # ARNCORE_PRI_INCLUDED
