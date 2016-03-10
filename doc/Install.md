Installation and usage    {#ins_page}
======================

[TOC]

Introduction    {#ins_intro}
============

This software uses qmake to build all its components. 
qmake is part of a Qt distribution. 

qmake reads project files, that contain the options and rules how to 
build a certain project. A project file ends with the suffix "*.pro". 
Files that end with the suffix "*.pri" are included by the project 
files and contain definitions, that are common for several project files.

The first step is to edit the *.pri / *.pro files to adjust 
them to your needs. Take care to select your deployment directories.
<Br><Br>


Documentation    {#ins_doc}
=============

The documentation is built by:
> qmake <Br>
> make doc <Br>

ArnLib includes a class documentation, that is available in various formats:

* **Html files**
* **PDF document** <Br>
  refman.pdf is built by:
  > cd doc/latex <Br>
  > make <Br>
* **Qt Compressed Help** (*.qch ) for the Qt assistant or creator. <Br> 
  Load the doc/qthelp/arnlib.qch file into Qt Creator. Start Qt creator and go to 
  Tools > Options, open up Help and Documentation. Click Add and browse for the qch file
  that was just created, then Apply. 
  It's best to close Qt creator at this point, and restart it. 
<Br><Br>


Building ArnLib    {#ins_build}
===============

The software can be built both by command line and IDE (Qt Creator).
When using IDE, don't forget the "make install" step.


A) Unix    {#ins_buildUnix}
-------

> qmake <Br>
> make <Br>
> make install

The easiest way of installing this library, is to let it be placed in a standard location 
for librarys and includes, e.g. /usr/lib and /usr/include/ArnInc.
When using a shared library it's path has to be known to 
the run-time linker of your operating system. On Linux systems read
"man ldconfig" (or google for it). Another option is to use
the LD_LIBRARY_PATH (on some systems LIBPATH is used instead, on MacOSX
it is called DYLD_LIBRARY_PATH) environment variable.

If you only want to check the library examples without installing something,
you can set the LD_LIBRARY_PATH to the lib directory 
of your local build. 
it's also possible to compile the sources together by ArnLibCompile (see Using ArnLib below).

The examples is built this way:
> cd examples/ArnDemoChat <Br>
> qmake <Br>
> make


B) Win32/MSVC    {#ins_buildWin32Msvc}
-------------

Has not been tested yet ...

Check that your Qt version has been built with MSVC - not with MinGW !

Please read the qmake documentation how to convert 
your *.pro files into your development environment.

For example MSVC with nmake:
> qmake ArnLib.pro <Br>
> nmake <Br>
> nmake install

The examples is built this way:
> cd examples\\ArnDemoChat <Br>
> qmake ArnDemoChat.pro <Br>
> nmake

Windows doesn't like mixing of debug and release binaries.

In windows it's possible to install the dll files together with the application binary,
as the application directory always is included in the search path for dll.


C) Win32/MinGW    {#ins_buildWin32Mingw}
--------------

Using Qt Creator for windows, will give you the needed tools for building a Qt project.

Check that your Qt version has been built with MinGW - not with MSVC !

Start a Shell, where Qt is initialized. (e.g. with
"Programs->Qt by Trolltech ...->Qt 4.x.x Command Prompt" ).
Check if you can execute "make" or something like "mingw32-make".

> qmake ArnLib.pro <Br>
> make <Br>
> make install

The examples is built this way:
> cd examples\\ArnDemoChat <Br>
> qmake ArnDemoChat.pro <Br>
> make

Windows doesn't like mixing of debug and release binaries.

In windows it's possible to install the dll files together with the application binary,
as the application directory always is included in the search path for dll.


D) MacOSX    {#ins_buildMacosx}
---------

Has not been tested yet ...

Well, the Mac is only another Unix system. So read the instructions in A).

In the recent Qt4 releases the default target of qmake is to generate
XCode project files instead of makefiles. So you might need to do the
following:
> qmake -spec macx-g++


E) Qt Embedded    {#ins_buildEmbedded}
--------------

ArnLib has been built with Qt Embedded using a Raspberry Pi. To build was as simple as
for a regular Unix build.
<Br><Br>


Using ArnLib    {#ins_usage}
============
In the *.pro file of the application the below lines can be used.

This will give a starting point for the configuration. It works well when using the same
base directory for ArnLib as the application, e.g. basedir/ArnLib and basedir/myApp. In
Unix-alike systems it's also needed to install the library files in a path known by the system,
see a) Unix.

It's possible to include the ArnLib source in the application compiling by adding
ArnLibCompile to CONFIG. The included part of the source can be selected by addings to ARN,
e.g. ARN += server.

WARNING! Using source inclusion (static linking) excludes the right to use LGPL for ArnLib.
Options are then to use GPL for the whole application or have a written agreement with
Michael Wiklund for other terms using the ArnLib.


Internal mDNS (ZeroConfig) is selected by adding mDnsIntern to CONFIG.

    CONFIG += ArnLibCompile
    CONFIG += mDnsIntern

    greaterThan(QT_MAJOR_VERSION, 4) {
        ARNLIB = Arn5
    } else {
        ARNLIB = Arn4
    }

    ArnLibCompile {
        #ARN += client
        ARN += server
        ARN += discover
        include(../ArnLib/src/ArnLib.pri)
        INCLUDEPATH += $$PWD/../ArnLib/src
    } else {
        win32: INCLUDEPATH += $$PWD/../ArnLib/src
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../ArnLib/release/ -l$${ARNLIB}
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../ArnLib/debug/ -l$${ARNLIB}
        else:unix: LIBS += -L$$OUT_PWD/../ArnLib/ -l$${ARNLIB}
    }

    !mDnsIntern {
        win32:CONFIG(release, debug|release): LIBS +=  -ldns_sd
        else:win32:CONFIG(debug, debug|release): LIBS +=  -ldns_sd
        else:unix: LIBS += -ldns_sd
    }

If you don't use qmake you have to add the include path to find the ArnLib 
headers to your compiler flags and the ArnLib library to your linker list.
<Br><Br>


    This Install.md file is based on documentation in the Qwt project.
