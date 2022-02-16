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

#include "ArnInc/ArnScriptJobs.hpp"
#include <QTimer>
#include <QMutexLocker>
#include <QDebug>


ArnScriptJobThread::ArnScriptJobThread( ArnScriptJobControl* jobConfig, ArnScriptJobFactory* jobFactory,
                                        ArnScriptWatchdogThread* watchdogThread)
{
    _jobConfig      = jobConfig;
    _jobFactory     = jobFactory;
    _watchdogThread = watchdogThread;
}


ArnScriptJobThread::~ArnScriptJobThread()
{
    // qDebug() << "ArnScriptThread wait for destroy";
    wait();
    // qDebug() << "ArnScriptThread destroyed";
}


void  ArnScriptJobThread::run()
{
    // qDebug() << "ArnScriptThread start";
    ArnScriptJobSingle  jobPreem( _jobConfig, _jobFactory, _watchdogThread);

    exec();
    // qDebug() << "ArnScriptThread stop";
}


/////////////

ArnScriptJobSingle::ArnScriptJobSingle( ArnScriptJobControl* jobConfig, ArnScriptJobFactory* jobFactory,
                                        ArnScriptWatchdogThread* watchdogThread)
{
    _jobConfig      = jobConfig;
    _jobFactory     = jobFactory;
    _watchdogThread = watchdogThread;
    _scriptChanged  = false;
    _job            = arnNullptr;

    newScriptJob();
    doScheduleRequest(0);

    connect( _jobConfig, SIGNAL(scriptChanged(int)), this, SLOT(doScriptChanged(int)), Qt::QueuedConnection);
}


ArnScriptJobSingle::~ArnScriptJobSingle()
{
}


void  ArnScriptJobSingle::newScriptJob()
{
    _runningId = 0;  // Mark no running job

    if (_job) {
        _watchdogThread->removeWatchdog( _job->watchdog());
        delete _job;
    }
    _job = new ArnScriptJob( _jobConfig->id(), this);
    _job->setWatchDogTime(0);  // Default no abort for single (preemtive) job
    connect( _job, SIGNAL(errorText(QString)), _jobConfig, SIGNAL(errorText(QString)));

    _jobConfig->doSetupJob( _job, _jobFactory);
    connect( _job, SIGNAL(scheduleRequest(int)),
             this, SLOT(doScheduleRequest(int)), Qt::QueuedConnection);
    connect( _job, SIGNAL(quitRequest(int)),
             this, SLOT(doQuitRequest(int)));  // High priority posted event

    _watchdogThread->addWatchdog( _job->watchdog());
}


void  ArnScriptJobSingle::doScheduleRequest( int callerId)
{
    if ((_runningId != 0) && (callerId != 0) && (_runningId != callerId))  return;

    // Last running job
    if (_runningId != 0) {
        _job->leaveScript();
        _runningId = 0;  // Mark no running job
    }

    // New job to run
    if (_job->isRunable()) {
        _runningId = _job->id();
        _job->enterScript();
    }
}


void  ArnScriptJobSingle::doQuitRequest( int callerId)
{
    Q_UNUSED( callerId)

    if (_scriptChanged) {
        _scriptChanged = false;

        newScriptJob();
        doScheduleRequest(0);
    }
}


void  ArnScriptJobSingle::doScriptChanged( int id)
{
    Q_UNUSED( id)

    _scriptChanged = true;
    // qDebug() << "Script changed sending sigQuit";
    QMetaObject::invokeMethod( _job,
                               "sigQuit",
                               Qt::DirectConnection);
    _job->quit();
}


/////////////

#ifdef ARNUSE_SCRIPTJS
ArnScriptWatchdogRun::ArnScriptWatchdogRun( ArnScriptWatchdogThread& watchdogThread)
{
    _watchdogThread = &watchdogThread;
}


ArnScriptWatchdogRun::~ArnScriptWatchdogRun()
{
}


void  ArnScriptWatchdogRun::addWatchdog( ArnScriptWatchdog* watchdog)
{
    watchdog->setParent( this);
    watchdog->setup();
}


void  ArnScriptWatchdogRun::removeWatchdog( ArnScriptWatchdog* watchdog)
{
    delete watchdog;
}


ArnScriptWatchdogThread::ArnScriptWatchdogThread( QObject* parent)
    : QThread( parent)
{
}


ArnScriptWatchdogThread::~ArnScriptWatchdogThread()
{
    wait();
}


// Threadsafe
void  ArnScriptWatchdogThread::addWatchdog( ArnScriptWatchdog* watchdog)
{
    if (!watchdog)  return;

    QMutexLocker mutexLocker( &_mutex);

    if (_wdTab.contains( watchdog))  return;
    _wdTab += watchdog;

    if (!_isReady)  return;

    mutexLocker.unlock();

    addWatchdogNow( watchdog);
}


// Threadsafe
void  ArnScriptWatchdogThread::addWatchdogNow( ArnScriptWatchdog* watchdog)
{
    watchdog->setParent( arnNullptr);
    watchdog->moveToThread( _watchdogRun->thread());

    QMetaObject::invokeMethod( _watchdogRun, [watchdog, this]() {
        _watchdogRun->addWatchdog( watchdog);
    }, Qt::QueuedConnection);
}


// Threadsafe
void  ArnScriptWatchdogThread::removeWatchdog( ArnScriptWatchdog* watchdog)
{
    if (!watchdog)  return;

    QMutexLocker mutexLocker( &_mutex);

    if (!_wdTab.contains( watchdog))  return;
    _wdTab.removeAll( watchdog);

    if (!_isReady)  return;

    mutexLocker.unlock();

    watchdog->setJsEngine( arnNullptr);
    QMetaObject::invokeMethod( _watchdogRun, [watchdog, this]() {
        _watchdogRun->removeWatchdog( watchdog);
    }, Qt::QueuedConnection);
}


void  ArnScriptWatchdogThread::run()
{
    // qDebug() << "ArnScriptWatchdogThread start";
    _watchdogRun = new ArnScriptWatchdogRun( *this);

    QTimer::singleShot( 0, _watchdogRun, [this]() {
        QMetaObject::invokeMethod( this, [this]() {
            postInit();
        }, Qt::QueuedConnection);
    } );

    exec();
    delete _watchdogRun;
    _watchdogRun = arnNullptr;
}


void  ArnScriptWatchdogThread::postInit()
{
    _mutex.lock();

    foreach( ArnScriptWatchdog* watchdog, _wdTab) {
        addWatchdogNow( watchdog);
    }
    _isReady = true;

    _mutex.unlock();

    emit ready();
}


#else
ArnScriptWatchdogThread::ArnScriptWatchdogThread( QObject* parent)
    : QObject( parent)
{
    QTimer::singleShot( 0, this, SIGNAL(ready()));
}


void  ArnScriptWatchdogThread::start()
{
}


void  ArnScriptWatchdogThread::addWatchdog( ArnScriptWatchdog* watchdog)
{
    if (!watchdog)  return;

    watchdog->setup();
}


void  ArnScriptWatchdogThread::removeWatchdog( ArnScriptWatchdog* watchdog)
{
    Q_UNUSED( watchdog)
}
#endif


////////////////


ArnScriptJobs::ArnScriptJobs( QObject* parent) :
    QObject( parent)
{
    _runningId      = 0;
    _runningIndex   = 0;
    _type           = _type.Null;
    _jobFactory     = arnNullptr;
    _watchdogThread = arnNullptr;
}


void  ArnScriptJobs::addJob( ArnScriptJobControl *jobConfig, int prio)
{
    if (!jobConfig)  return;

    JobSlot  jobSlot;
    jobSlot.startPrio = prio;
    jobSlot.jobConfig = jobConfig;
    _idToSlot.insert( jobConfig->id(), _jobSlots.size());
    _jobSlots += jobSlot;
}


void  ArnScriptJobs::setFactory( ArnScriptJobFactory *jobFactory)
{
    _jobFactory = jobFactory;
}


void  ArnScriptJobs::start( Type type)
{
    _watchdogThread = new ArnScriptWatchdogThread( this);
    _watchdogThread->start();

    switch (type.e) {
    case Type::Cooperative:
        doCooperativeStart();
        break;
    case Type::Preemptive:
        doPreemtiveStart();
        break;
    default:;
    }
}


void  ArnScriptJobs::doCooperativeStart()
{
    // qDebug() << "ScrJobs: CooperativeStart";
    for (int i = 0; i < _jobSlots.size(); ++i) {
        JobSlot&  jobSlot = _jobSlots[i];
        newScriptJob( jobSlot);

        connect( jobSlot.jobConfig, SIGNAL(scriptChanged(int)), this, SLOT(setScriptChanged(int)));
    }
    doScheduleRequest(0);
}


void  ArnScriptJobs::doPreemtiveStart()
{
    connect( _watchdogThread, SIGNAL(ready()), this, SLOT(doPreemtiveStartNow()));
    // doPreemtiveStartNow();
}


void  ArnScriptJobs::doPreemtiveStartNow()
{
    for (int i = 0; i < _jobSlots.size(); ++i) {
        JobSlot&  jobSlot = _jobSlots[i];
        jobSlot.thread = new ArnScriptJobThread( jobSlot.jobConfig, _jobFactory, _watchdogThread);
        jobSlot.thread->start();  // MW: No priority set ...
    }
}


/// Only for cooperative jobs
void  ArnScriptJobs::doScheduleRequest( int callerId)
{
    // qDebug() << "rsnScheduleRequest callerId=" << callerId << " runningId=" << _runningId;
    if ((_runningId != 0) && (callerId != 0) && (_runningId != callerId))  return;

    //// Last running slot
    if (_runningId != 0) {
        // qDebug() << "Last run slot: index=" << _runningIndex;
        JobSlot&  jobSlot = _jobSlots[ _runningIndex];
        jobSlot.job->leaveScript();

        for (int i = 0; i < _jobSlots.size(); ++i) {  // All slots get "higher" prio
            if (_jobSlots.at(i).curPrio > -100)
                _jobSlots[i].curPrio -= 100 / _jobSlots[i].startPrio;  // Test
                //_jobSlots[i].curPrio--;
        }
        jobSlot.curPrio = 10000;  // Test
        //jobSlot.curPrio = jobSlot.startPrio;  // Last running slot get its start pri0

        _runningId = 0;  // Mark no running slot
    }

    //// Script restart
    for (int i = 0; i < _jobSlots.size(); ++i) {
        JobSlot&  jobSlot = _jobSlots[i];
        if (jobSlot.scriptChanged) {
            QMetaObject::invokeMethod( jobSlot.job,
                                       "sigQuit",
                                       Qt::DirectConnection);
            jobSlot.scriptChanged = false;
            newScriptJob( jobSlot);
        }
    }

    //// Find new slot to run
    int  hiPrio = 32767;  // Large num (low prio)
    int  hiPrioIndex = -1;  // Mark not valid index
    for (int i = 0; i < _jobSlots.size(); ++i) {
        JobSlot&  jobSlot = _jobSlots[i];
        if ((jobSlot.job->isRunable()) && (jobSlot.curPrio < hiPrio)) {
            hiPrio = jobSlot.curPrio;  // Highest prio (lowest number) so far
            hiPrioIndex = i;
        }
    }
    if (hiPrioIndex >= 0) {  // New slot to run found
        _runningIndex = hiPrioIndex;
        JobSlot&  jobSlot = _jobSlots[ _runningIndex];
        _runningId = jobSlot.job->id();

        // qDebug() << "Starting Job: runningId=" << _runningId << " prio=" << runSlot.curPrio;
        jobSlot.job->enterScript();
    }
}


/// Only for cooperative jobs
void  ArnScriptJobs::newScriptJob( JobSlot& jobSlot)
{       
    if (jobSlot.job) {
        _watchdogThread->removeWatchdog( jobSlot.job->watchdog());
        delete jobSlot.job;
    }
    jobSlot.job = new ArnScriptJob( jobSlot.jobConfig->id(), this);
    connect( jobSlot.job, SIGNAL(errorText(QString)), jobSlot.jobConfig, SIGNAL(errorText(QString)));

    jobSlot.jobConfig->doSetupJob( jobSlot.job, _jobFactory);
    connect( jobSlot.job, SIGNAL(scheduleRequest(int)),
             this, SLOT(doScheduleRequest(int)), Qt::QueuedConnection);

    _watchdogThread->addWatchdog( jobSlot.job->watchdog());
}


/// Only for cooperative jobs
void  ArnScriptJobs::setScriptChanged( int id)
{
    int i = _idToSlot.value( id, -1);
    if (i < 0)  return;  // Not a valid id

    _jobSlots[i].scriptChanged = true;

    if (_runningId == 0) {  // No active jobs
        doScheduleRequest( 0);
    }
}
