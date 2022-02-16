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

#ifndef ARNSCRIPTJOBS_HPP
#define ARNSCRIPTJOBS_HPP

#include "ArnLib_global.hpp"
#include "ArnScriptJob.hpp"
#include "MQFlags.hpp"
#include <QThread>
#include <QMutex>
#include <QObject>

class ArnScriptWatchdogThread;
class ArnScriptWatchdogRun;


//! \cond ADV
class ArnScriptJobThread : public QThread
{
Q_OBJECT
public:
    ArnScriptJobThread( ArnScriptJobControl* jobConfig, ArnScriptJobFactory* jobFactory,
                        ArnScriptWatchdogThread* watchdogThread);
    ~ArnScriptJobThread();

protected:
    void  run();

private:
    ArnScriptJobControl*  _jobConfig;
    ArnScriptJobFactory*  _jobFactory;
    ArnScriptWatchdogThread*  _watchdogThread;
};


class ARNLIBSHARED_EXPORT ArnScriptJobSingle : public QObject
{
Q_OBJECT
public:
    ArnScriptJobSingle( ArnScriptJobControl* jobConfig, ArnScriptJobFactory* jobFactory,
                        ArnScriptWatchdogThread* watchdogThread);
    ~ArnScriptJobSingle();

signals:

private slots:
    void  doScheduleRequest( int callerId);
    void  doQuitRequest( int callerId);
    void  doScriptChanged( int id);

private:
    void  newScriptJob();

    ArnScriptJobControl*  _jobConfig;
    ArnScriptJobFactory*  _jobFactory;
    ArnScriptWatchdogThread*  _watchdogThread;

    ArnScriptJob*  _job;
    int  _runningId;
    bool  _scriptChanged;
};


#ifdef ARNUSE_SCRIPTJS
class ArnScriptWatchdogRun : public QObject
{
Q_OBJECT
public:
    ArnScriptWatchdogRun( ArnScriptWatchdogThread& watchdogThread);
    ~ArnScriptWatchdogRun();

public slots:
    void  addWatchdog( ArnScriptWatchdog* watchdog);
    void  removeWatchdog( ArnScriptWatchdog* watchdog);

private slots:

private:
    ArnScriptWatchdogThread*  _watchdogThread = arnNullptr;
};


class ArnScriptWatchdogThread : public QThread
{
Q_OBJECT
public:
    ArnScriptWatchdogThread( QObject* parent = arnNullptr);
    ~ArnScriptWatchdogThread();

    void  addWatchdog( ArnScriptWatchdog* watchdog);
    void  removeWatchdog( ArnScriptWatchdog* watchdog);

signals:
    void  ready();

protected:
    void  run();

private:
    void  postInit();
    void  addWatchdogNow( ArnScriptWatchdog* watchdog);

    ArnScriptWatchdogRun*  _watchdogRun = arnNullptr;
    QList<ArnScriptWatchdog*>  _wdTab;
    bool  _isReady  = false;
    QMutex  _mutex;
};


#else
/// This is a none threaded direkt handler for watchdog
class ArnScriptWatchdogThread : public QObject
{
Q_OBJECT
public:
    ArnScriptWatchdogThread( QObject* parent = arnNullptr);

    static void  start();
    static void  addWatchdog( ArnScriptWatchdog* watchdog);
    static void  removeWatchdog( ArnScriptWatchdog* watchdog);

signals:
    void  ready();
};
#endif
//! \endcond


/*! TODO: Add destructor that deletes jobs in _jobSlots
 */
class ARNLIBSHARED_EXPORT ArnScriptJobs : public QObject
{
    Q_OBJECT
public:
    struct Type {
        enum E {
            Null,
            Cooperative,
            Preemptive,
        };
        MQ_DECLARE_ENUM( Type)
    };
    explicit ArnScriptJobs( QObject* parent = arnNullptr);
    void  addJob( ArnScriptJobControl* jobConfig, int prio = 1);
    void  setFactory( ArnScriptJobFactory* jobFactory);
    void  start( Type type = Type::Cooperative);

signals:

private slots:
    void  doScheduleRequest( int callerId);
    void  setScriptChanged( int id);
    void  doPreemtiveStartNow();

private:
    struct JobSlot {
        ArnScriptJobThread*  thread;
        ArnScriptJobControl*  jobConfig;
        ArnScriptJob*  job;
        int  startPrio;
        int  curPrio;
        bool  scriptChanged;

        JobSlot()
        {
            startPrio     = 1;
            curPrio       = 1;
            thread        = arnNullptr;
            jobConfig     = arnNullptr;
            job           = arnNullptr;
            scriptChanged = false;
        }
    };
    QList<JobSlot>  _jobSlots;
    QMap<int,int>  _idToSlot;

    void  doCooperativeStart();
    void  doPreemtiveStart();
    void  newScriptJob( JobSlot& jobSlot);

    Type  _type;
    int  _runningId;
    int  _runningIndex;
    ArnScriptJobFactory*  _jobFactory;
    ArnScriptWatchdogThread*  _watchdogThread;
};

#endif // ARNSCRIPTJOBS_HPP
