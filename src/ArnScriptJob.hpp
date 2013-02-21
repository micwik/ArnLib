// Copyright (C) 2010-2013 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt 4 and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these other
// libraries is subject to their respective license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE.LGPL included in the packaging of this file.
// In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General
// Public License version 3.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and
// conditions contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//

#ifndef ARNSCRIPTJOB_HPP
#define ARNSCRIPTJOB_HPP

#include "ArnScript.hpp"
#include "ArnLib_global.hpp"
#include <QScriptValue>
#include <QObject>
#include <QAtomicInt>
#include <QMutex>

class ArnScript;
class ArnScriptJobB;
class ArnScriptJobControl;
class ArnScriptJobFactory;
class QTimer;
class QEvent;


class ArnScriptJ : public ArnScript
{
Q_OBJECT
public:
    explicit  ArnScriptJ( ArnScriptJobB* parent);
    ~ArnScriptJ();

    // Reimplemented for accessing info from ArnJobs layer
    virtual ArnClient*  getClient( QString clientId);
};


/// Internal class for hiding details from Script
class ArnScriptJobB : public QObject
{
    Q_OBJECT
    friend class ArnScriptJobControl;
public:
    explicit  ArnScriptJobB( int id, QObject* parent = 0);
    bool  evaluateScript( QByteArray script, QString idName);
    bool  evaluateScriptFile( QString fileName);
    int  id()  const;
    QString  name()  const;
    bool  setupScript();
    void  enterScript();
    void  leaveScript();
    ArnScriptJobFactory*  jobFactory()  const;

    void  setSleepState( bool isSleepState = true);
    void  setWatchDogTime( int time);
    int  watchDogTime();
    void  setWatchDog( int time = -1, bool persist = true);
    void  setPollTime( int time);
    int  pollTime();
    bool  isSleepState()  const;
    bool  isRunable()  const;
    bool  isStopped()  const;
    void  errorLog( QString txt);

signals:
    void  scheduleRequest( int callerId);
    void  quitRequest( int callerId);
    void  timeoutAbort( int id);
    void  errorText( QString txt);

public slots:
    void  yield();
    void  quit();

protected slots:
    void  doTimeoutAbort();

protected:
    virtual void  customEvent( QEvent* ev);

    void  installInterface( QString id, QObject* obj);
    bool  installExtension( QString id, ArnScriptJobControl* jobControl);
    bool  setConfig( const char* name, const QVariant& value);
    void  addConfig( QObject* obj);
    void  setJobFactory( ArnScriptJobFactory* jobFactory);

    ArnScriptJ*  _arnScr;
    QObject*  _configObj;
    QTimer*  _abortTimer;

    QScriptValue  _jobInit;
    QScriptValue  _jobEnter;
    QScriptValue  _jobLeave;

private:
    int  _id;
    int  _watchDogTime;  // Longest time to run continous script code until it is aborted
    int  _pollTime;  // Longest time to run continous script code until events are processed. Don't exceed ...
    bool  _isSleepState;
    bool  _isStopped;
    bool  _quitInProgress;
    ArnScriptJobFactory*  _jobFactory;
};


/// Interface class to be normally used, is also Script Job interface
class ARNLIBSHARED_EXPORT ArnScriptJob : public ArnScriptJobB
{
    Q_OBJECT
    Q_PROPERTY( bool sleepState WRITE setSleepState READ isSleepState )
    Q_PROPERTY( int watchDog WRITE setWatchDog READ watchDogTime )
    Q_PROPERTY( int poll WRITE setPollTime READ pollTime )
    Q_PROPERTY( QString name  READ name )
public:
    explicit  ArnScriptJob( int id, QObject* parent = 0);

signals:
    void  sigQuit();

public slots:
    void  setWatchDogTime( int time)  {ArnScriptJobB::setWatchDogTime( time);}
    void  yield()  {ArnScriptJobB::yield();}
    void  quit()  {ArnScriptJobB::quit();}
    void  errorLog( QString txt)  {ArnScriptJobB::errorLog( txt);}
};


/// Must be thread-safe as subclassed
class ARNLIBSHARED_EXPORT ArnScriptJobFactory
{
public:
    explicit  ArnScriptJobFactory();
    virtual  ~ArnScriptJobFactory();
    virtual bool  installExtension( QString id, QScriptEngine& engine,
                                    const ArnScriptJobControl* jobControl = 0) = 0;
    virtual ArnClient*  getClient( QString id);

protected:
    static void  setupJsObj( const QString& id, const QScriptValue& jsObj, QScriptEngine& engine);
    static bool  setupInterface( const QString& id, QObject* interface, QScriptEngine& engine);
};


/// Is thread-safe (except doSetupJob)
class ARNLIBSHARED_EXPORT ArnScriptJobControl : public QObject
{
    Q_OBJECT
public:
    explicit  ArnScriptJobControl( QObject* parent = 0);
    int  id();
    QString  name()  const;
    void  setName( QString name);
    void  addInterface( QString id);
    void  addInterfaceList( QStringList interfaceList);
    QByteArray  script()  const;
    void  loadScriptFile( QString fileName);
    QVariant  config( const char* name)  const;
    bool  setConfig( const char* name, const QVariant& value);
    void  addConfig( QObject* obj);

    void  setThreaded( bool isThreaded)  {_isThreaded = isThreaded;}
    void  doSetupJob( ArnScriptJob* job, ArnScriptJobFactory* jobFactory);

signals:
    void  scriptChanged( int id);
    void  errorText( QString txt);

public slots:
    void  setScript( QByteArray script);

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