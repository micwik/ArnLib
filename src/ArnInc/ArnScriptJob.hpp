// Copyright (C) 2010-2022 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. Usage of these other libraries is subject to their respective
// license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE_LGPL.txt included in the packaging of this
// file. In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General Public
// License version 3.0 as published by the Free Software Foundation and appearing
// in the file LICENSE_GPL.txt included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and conditions
// contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//

#ifndef ARNSCRIPTJOB_HPP
#define ARNSCRIPTJOB_HPP

#include "ArnLib_global.hpp"
#include "ArnScript.hpp"
#include <QObject>
#include <QAtomicInt>
#include <QMutex>
#include <QThread>

class ArnScript;
class ArnScriptJobB;
class ArnScriptJobControl;
class ArnScriptJobFactory;
class ArnScriptWatchdog;
class QTimer;
class QEvent;


//! \cond ADV

/// Internal class for hiding details from Script
class ArnScriptJobB : public QObject
{
    Q_OBJECT
    friend class ArnScriptJobControl;

public:
    explicit  ArnScriptJobB( int id, QObject* parent = arnNullptr);
    bool  evaluateScript( const QByteArray& script, const QString& idName);
    bool  evaluateScriptFile( const QString& fileName);
    int  id()  const;
    QString  name()  const;
    bool  setupScript();
    void  enterScript();
    void  leaveScript();
    ArnScriptJobFactory*  jobFactory()  const;
    ArnScriptWatchdog*  watchdog() const;

    void  setSleepState( bool isSleepState = true);
    void  setWatchDogTime( int time);
    int  watchDogTime();
    void  setWatchDog( int time = -1, bool persist = true);
    void  setPollTime( int time);
    int  pollTime();
    bool  isSleepState()  const;
    bool  isRunable()  const;
    bool  isStopped()  const;
    bool  isRunning()  const;
    void  errorLog( const QString& txt);

signals:
    void  scheduleRequest( int callerId);
    void  quitRequest( int callerId);
    void  timeoutAbort( int id);
    void  errorText( const QString& txt);

public slots:
    void  yield();
    void  quit();

protected slots:
    void  doTimeoutAbort();

protected:
    virtual void  customEvent( QEvent* ev);

    void  installInterface( const QString& id, QObject* obj);
    bool  installExtension( const QString& id, ArnScriptJobControl* jobControl);
    bool  setConfig( const char* name, const QVariant& value);
    void  addConfig( QObject* obj);
    void  setJobFactory( ArnScriptJobFactory* jobFactory);

    ArnScript*  _arnScr;
    QObject*  _configObj;

    ARN_JSVALUE  _jobInit;
    ARN_JSVALUE  _jobEnter;
    ARN_JSVALUE  _jobLeave;

private:
    int  _id;
    int  _watchDogTime;  // Longest time to run continous script code until it is aborted
    int  _pollTime;  // Longest time to run continous script code until events are processed. Don't exceed ...
    bool  _isSleepState;
    bool  _isStopped;
    bool  _isRunning;
    bool  _quitInProgress;
    ArnScriptJobFactory*  _jobFactory;
    ArnScriptWatchdog*  _watchdog;
};


class ArnScriptWatchdog : public QObject
{
Q_OBJECT
public:
    ArnScriptWatchdog( ARN_JSENGINE* jsEngine, QObject* parent = arnNullptr);
    ~ArnScriptWatchdog();

    void  setup();
    void  setJsEngine( ARN_JSENGINE* jsEngine );

    void  setTime( int timeMs);

signals:
    void  timeout();

private:
    void  setTimeNow( int timeMs);

    ARN_JSENGINE*  _jsEngine;
    QMutex  _mutex;
    QTimer*  _timer;
    int  _lastTimeMs;
    bool  _isSetup;
};
//! \endcond


/// Interface class to be normally used, is also Script Job interface
class ARNLIBSHARED_EXPORT ArnScriptJob : public ArnScriptJobB
{
    Q_OBJECT
    Q_PROPERTY( bool sleepState WRITE setSleepState READ isSleepState )
    Q_PROPERTY( bool running READ isRunning )
    Q_PROPERTY( int watchDog WRITE setWatchDog READ watchDogTime )
    Q_PROPERTY( int poll WRITE setPollTime READ pollTime )
    Q_PROPERTY( QString name  READ name )
public:
    explicit  ArnScriptJob( int id, QObject* parent = arnNullptr);

signals:
    void  sigQuit();

public slots:
    void  setWatchDogTime( int time)  {ArnScriptJobB::setWatchDogTime( time);}
    void  yield()  {ArnScriptJobB::yield();}
    void  quit()  {ArnScriptJobB::quit();}
    void  errorLog( const QString& txt)  {ArnScriptJobB::errorLog( txt);}
};


/// Must be thread-safe as subclassed
class ARNLIBSHARED_EXPORT ArnScriptJobFactory
{
public:
    explicit  ArnScriptJobFactory();
    virtual  ~ArnScriptJobFactory();
    virtual bool  installExtension( const QString& id, ARN_JSENGINE& engine,
                                    const ArnScriptJobControl* jobControl = arnNullptr) = 0;

protected:
    static void  setupJsObj( const QString& id, const ARN_JSVALUE& jsObj, ARN_JSENGINE& engine);
    static bool  setupInterface( const QString& id, QObject* interface, ARN_JSENGINE& engine);
};


/// Is thread-safe (except doSetupJob)
class ARNLIBSHARED_EXPORT ArnScriptJobControl : public QObject
{
    Q_OBJECT
public:
    explicit  ArnScriptJobControl( QObject* parent = arnNullptr);
    int  id();
    QString  name()  const;
    void  setName( const QString& name);
    void  addInterface( const QString& id);
    void  addInterfaceList( const QStringList& interfaceList);
    QByteArray  script()  const;
    void  loadScriptFile( const QString& fileName);
    QVariant  config( const char* name)  const;
    bool  setConfig( const char* name, const QVariant& value);
    void  addConfig( QObject* obj);

    void  setThreaded( bool isThreaded);
    void  doSetupJob( ArnScriptJob* job, ArnScriptJobFactory* jobFactory);

signals:
    void  scriptChanged( int id);
    void  errorText( const QString& txt);

public slots:
    void  setScript( const QByteArray& script);

private:
    // Source for unique id to all Jobs ..
    static QAtomicInt  _idCount;

    int  _id;
    QString  _name;
    QStringList  _interfaceList;
    QByteArray  _script;
    QObject*  _configObj;

    bool  _isThreaded;
    mutable QMutex  _mutex;
};


#endif // ARNSCRIPTJOB_HPP
