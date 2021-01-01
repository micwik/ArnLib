// Copyright (C) 2010-2020 Michael Wiklund.
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

#include "ArnInc/ArnScriptJob.hpp"
#include <QFileInfo>
#include <QTimer>
#include <QEvent>
#include <QCoreApplication>
#include <QDebug>

#ifdef ARNUSE_SCRIPTJS
  #include <QJSEngine>
  #include <QMetaObject>
#else
  #include <QScriptable>
  #include <QtScript>
  #include <QScriptEngine>
#endif

const QEvent::Type  EventQuit = QEvent::Type( QEvent::User + 0);


ArnScriptJobB::ArnScriptJobB( int id, QObject* parent) :
        QObject( parent)
{
    _id = id;
    _isSleepState   = false;
    _isStopped      = false;
    _isRunning      = false;
    _quitInProgress = false;
    _watchDogTime   = 2000;  // ms
    _pollTime       = 2000;  // ms

    _jobFactory = 0;
    _configObj  = new QObject( this);
    _arnScr     = new ArnScript( this);
    setPollTime( _pollTime);

    _watchdog = new ArnScriptWatchdog( &_arnScr->engine(), this);

    connect( _watchdog, SIGNAL(timeout()), this, SLOT(doTimeoutAbort()));
    connect( _arnScr, SIGNAL(errorText(QString)), this, SIGNAL(errorText(QString)));

#ifdef ARNUSE_SCRIPTJS
    _arnScr->setInterruptedText( "Error: Job run timeout");
#endif
}


void  ArnScriptJobB::setWatchDogTime( int time)
{
    _watchDogTime = time;  // ms
}


int  ArnScriptJobB::watchDogTime()
{
    return _watchDogTime;
}


void  ArnScriptJobB::setWatchDog( int time, bool persist)
{
    int  wt = time;
    if (wt < 0)
        wt = _watchDogTime;

    if (persist)
        _watchDogTime = wt;

#if ARNUSE_SCRIPTJS
    _watchdog->setTime( wt);
#else
    _watchdog->setTime( wt);
    if (wt > 0) {
        _arnScr->engine().setProcessEventsInterval( wt);
    }
    else {
        _arnScr->engine().setProcessEventsInterval( _watchDogTime > 0 ? _watchDogTime : _pollTime);
    }
#endif
}


void  ArnScriptJobB::setPollTime( int time)
{
    _pollTime = time;

#if ARNUSE_SCRIPTJS
    // No support for this in QJSEngine
#else
    _arnScr->engine().setProcessEventsInterval( _pollTime);
#endif
}


int  ArnScriptJobB::pollTime()
{
    return _pollTime;
}


void  ArnScriptJobB::installInterface( const QString& id, QObject* obj)
{
    if ((id.isEmpty()) || (obj == 0))  return;

    if (obj != this)  obj->setParent( this);  // Reparent interface to this Job
    //MW: fix parrenting and delete failed install object

    _arnScr->addObject( id, obj);
}


bool  ArnScriptJobB::installExtension( const QString& id, ArnScriptJobControl *jobControl)
{
    if (!_jobFactory || !_arnScr)  return false;

    return _jobFactory->installExtension( id, _arnScr->engine(), jobControl);
}


bool  ArnScriptJobB::evaluateScript( const QByteArray& script, const QString& idName)
{
    setWatchDog();
    bool stat = true;
#ifdef ARN_JSENGINE
    //// QJSEngine lacks support for QObject dynamic property, this is a workaround
    QByteArray conf = "config = {";
    QByteArray sep;
    QList<QByteArray>  nameList = _configObj->dynamicPropertyNames();
    foreach (QByteArray name, nameList) {
        conf += sep;
        name.replace(".", "_");
        conf += name + ": \"" + _configObj->property( name.constData()).toString().toUtf8() + "\"" ;
        sep = ", ";
    }
    conf += "};";
    stat = _arnScr->evaluate( conf, idName, "Config");
    if (!stat)
        errorLog( "Config-script: " + conf );
#endif
    if (stat) {
        stat = _arnScr->evaluate( script, idName);
    }
    setWatchDog( 0, false);
    return stat;
}


bool  ArnScriptJobB::evaluateScriptFile( const QString& fileName)
{
    setWatchDog();
    bool stat = _arnScr->evaluateFile( fileName);
    setWatchDog( 0, false);
    return stat;
}


int  ArnScriptJobB::id()  const
{
    return _id;
}


QString  ArnScriptJobB::name()  const
{
    return _arnScr->idName();
}


bool  ArnScriptJobB::setConfig( const char* name, const QVariant& value)
{
    if (!name)  return false;

    return _configObj->setProperty( name, value);
}


void  ArnScriptJobB::addConfig( QObject* obj)
{
    if (!obj)  return;

    QList<QByteArray>  nameList = obj->dynamicPropertyNames();
    foreach (QByteArray name, nameList) {
        setConfig( name.constData(), obj->property( name.constData()));
    }
}


void  ArnScriptJobB::setJobFactory( ArnScriptJobFactory* jobFactory)
{
    _jobFactory = jobFactory;
}


ArnScriptJobFactory*  ArnScriptJobB::jobFactory()  const
{
    return _jobFactory;
}


ArnScriptWatchdog*  ArnScriptJobB::watchdog()  const
{
    return _watchdog;
}


bool  ArnScriptJobB::setupScript()
{
    _jobInit  = _arnScr->globalProperty("jobInit");
    _jobEnter = _arnScr->globalProperty("jobEnter");
    _jobLeave = _arnScr->globalProperty("jobLeave");

    setWatchDog();
    ARN_JSVALUE  result = _arnScr->callFunc( _jobInit, ARN_JSVALUE(), ARN_JSVALUE_LIST());
    if (result.isError()) {
        _isStopped = true;
        _isRunning = false;
    }
    setWatchDog( 0, false);

    return true;
}


void  ArnScriptJobB::enterScript()
{
    if (isStopped())  return;  // Don't execute Enter if Job is stopped (error ...)

    _isRunning = true;
    setWatchDog();
    ARN_JSVALUE  result = _arnScr->callFunc( _jobEnter, ARN_JSVALUE(), ARN_JSVALUE_LIST());
    if (result.isError()) {
        setWatchDog( 0, false);
        _isStopped = true;
        _isRunning = false;
        //qDebug() << "Enter Script timeout: isEval=" << _arnScr->engine().isEvaluating();
        emit scheduleRequest( _id);
    }
}


void  ArnScriptJobB::leaveScript()
{
    if (isStopped())  return;  // Don't execute Leave if Job is stopped (error ...)

    ARN_JSVALUE  result = _arnScr->callFunc( _jobLeave, ARN_JSVALUE(), ARN_JSVALUE_LIST());
    if (result.isError()) {
        _isStopped = true;
        //qDebug() << "Leave Script timeout: isEval=" << _arnScr->engine().isEvaluating();
    }
    _isRunning = false;
    setWatchDog( 0, false);
}


void  ArnScriptJobB::yield()
{
    emit scheduleRequest( _id);
}


void  ArnScriptJobB::quit()
{
    if (_quitInProgress)  return;  // Quit is already in progress
    _quitInProgress = true;

    ARN_JSENGINE& engine = _arnScr->engine();
#if ARNUSE_SCRIPTJS
    Q_UNUSED(engine)
    engine.setInterrupted( true);
#else
    engine.abortEvaluation( engine.currentContext()->throwError("Job quit"));
#endif

    QEvent*  ev = new QEvent( EventQuit);
    QCoreApplication::postEvent( this, ev, Qt::HighEventPriority);
}


void  ArnScriptJobB::customEvent(QEvent *ev)
{
    switch (ev->type()) {
    case EventQuit:
        _quitInProgress = false;
        emit quitRequest( _id);
        break;
    default:;
    }
}


void  ArnScriptJobB::doTimeoutAbort()
{
#if ARNUSE_SCRIPTJS
    // qDebug() << "Script timeout:";
    if (!_isStopped) {
        ARN_JSENGINE& engine = _arnScr->engine();
        engine.setInterrupted( false);
        _isStopped = true;
        _isRunning = false;
        errorLog( "Error: Job run timeout sig");

        emit scheduleRequest( _id);
    }
#else
    // qDebug() << "Script timeout: isEval=" << _arnScr->engine().isEvaluating();
    _isStopped = true;
    _isRunning = false;
    errorLog( "Error: Watchdog timeout");  // Extra error as throwError() not allways works ...
    ARN_JSENGINE& engine = _arnScr->engine();
    engine.abortEvaluation( engine.currentContext()->throwError("Job run timeout"));

    emit scheduleRequest( _id);
#endif

    emit timeoutAbort( _id);
}


void  ArnScriptJobB::setSleepState( bool isSleepState)
{
    _isSleepState = isSleepState;
    //qDebug() << "ArnScriptJob: sleepState=" << isSleepState << " fileName=" << name();
    emit scheduleRequest( _id);
}


bool  ArnScriptJobB::isSleepState()  const
{
    return _isSleepState;
}


bool  ArnScriptJobB::isRunable()  const
{
    return !_isSleepState && !_isStopped;
}


bool  ArnScriptJobB::isStopped()  const
{
    return _isStopped;
}


bool  ArnScriptJobB::isRunning()  const
{
    return _isRunning;
}



void  ArnScriptJobB::errorLog( const QString& txt)
{
    ArnM::errorLog( txt + " name=" + name(), ArnError::ScriptError);
    emit errorText( txt);
}


ArnScriptJobFactory::ArnScriptJobFactory()
{
}


ArnScriptJobFactory::~ArnScriptJobFactory()
{
}


void  ArnScriptJobFactory::setupJsObj( const QString& id, const ARN_JSVALUE& jsObj, ARN_JSENGINE& engine)
{
    engine.globalObject().setProperty( id, jsObj);
}


bool  ArnScriptJobFactory::setupInterface( const QString& id, QObject* interface, ARN_JSENGINE& engine)
{
    if (id.isEmpty() || (interface == 0))  return false;

#if ARNUSE_SCRIPTJS
    ARN_JSVALUE  objScr = engine.newQObject( interface);
#else
    QScriptValue  objScr = engine.newQObject( interface, QScriptEngine::QtOwnership,
                                              QScriptEngine::ExcludeSuperClassContents);
#endif
    if (objScr.isNull())  return false;

    setupJsObj( id, objScr, engine);

    if (!interface->parent())
        interface->setParent( engine.parent());  // Reparent interface to same parent as engine

    return true;
    //MW: fix parrenting and delete failed install object
}


/////////////

ArnScriptWatchdog::ArnScriptWatchdog( ARN_JSENGINE* jsEngine, QObject* parent)
    : QObject( parent)
{
    _jsEngine   = jsEngine;
    _lastTimeMs = 0;
    _isSetup    = false;
    _timer      = new QTimer( this);
    _timer->setSingleShot( true);
}


ArnScriptWatchdog::~ArnScriptWatchdog()
{
}


void ArnScriptWatchdog::setup()
{
#ifdef ARNUSE_SCRIPTJS
    connect( _timer, &QTimer::timeout,
             [this]() {
        _mutex.lock();
        if (_jsEngine)
            _jsEngine->setInterrupted( true);
        _mutex.unlock();
        emit timeout();
    });
#else
    connect( _timer, SIGNAL(timeout()), this, SIGNAL(timeout()));
#endif
    setTimeNow( _lastTimeMs );
    _isSetup = true;
}


// Threadsafe
void ArnScriptWatchdog::setJsEngine( ARN_JSENGINE* jsEngine)
{
    _mutex.lock();
    _jsEngine = jsEngine;
    _mutex.unlock();
}


// Threadsafe when using ARNUSE_SCRIPTJS
void ArnScriptWatchdog::setTime( int timeMs)
{
    _lastTimeMs = timeMs;
    if (_isSetup) {
#ifdef ARNUSE_SCRIPTJS
        QMetaObject::invokeMethod( this, [timeMs, this]() { this->setTimeNow( timeMs); }, Qt::QueuedConnection);
#else
        setTimeNow( timeMs);
#endif
    }
}


void ArnScriptWatchdog::setTimeNow( int timeMs)
{
    if (timeMs > 0)
        _timer->start( timeMs);
    else
        _timer->stop();
}


////////////////

ArnScriptJob::ArnScriptJob( int id, QObject* parent) :
        ArnScriptJobB( id, parent)
{
    installInterface( "job", this);
#ifndef ARN_JSENGINE
    installInterface( "config", _configObj);
#endif
}


QAtomicInt ArnScriptJobControl::_idCount(1);


ArnScriptJobControl::ArnScriptJobControl( QObject* parent) :
    QObject( parent)
{
    _id = _idCount.fetchAndAddRelaxed(1);
    _configObj = new QObject( this);
    _isThreaded = true;  // Default safe
}


void  ArnScriptJobControl::setName( const QString& name)
{
    if (_isThreaded)  _mutex.lock();
    _name = name;
    if (_isThreaded)  _mutex.unlock();
}


int  ArnScriptJobControl::id()
{
    if (_isThreaded)  _mutex.lock();
    int  retVal = _id;
    if (_isThreaded)  _mutex.unlock();

    return retVal;
}


QString  ArnScriptJobControl::name()  const
{
    if (_isThreaded)  _mutex.lock();
    QString  retVal = _name;
    if (_isThreaded)  _mutex.unlock();

    return retVal;
}


void  ArnScriptJobControl::addInterface( const QString& id)
{
    if (_isThreaded)  _mutex.lock();
    if (!id.isEmpty())  _interfaceList += id;
    _interfaceList.removeDuplicates();
    if (_isThreaded)  _mutex.unlock();
}


void  ArnScriptJobControl::addInterfaceList( const QStringList& interfaceList)
{
    if (_isThreaded)  _mutex.lock();
    _interfaceList += interfaceList;
    _interfaceList.removeDuplicates();
    if (_isThreaded)  _mutex.unlock();
}


void  ArnScriptJobControl::setScript( const QByteArray& script)
{
    if (_isThreaded)  _mutex.lock();
    _script = script;
    if (_isThreaded)  _mutex.unlock();

    emit scriptChanged( _id);
}


QByteArray  ArnScriptJobControl::script()  const
{
    if (_isThreaded)  _mutex.lock();
    QByteArray  retVal = _script;
    if (_isThreaded)  _mutex.unlock();

    return retVal;
}


void  ArnScriptJobControl::loadScriptFile( const QString& fileName)
{
    if (fileName.isEmpty())  return;

    QFile  file( fileName);
    file.open( QIODevice::ReadOnly);

    setName( QFileInfo( file).baseName());
    setScript( file.readAll());
}


bool  ArnScriptJobControl::setConfig( const char* name, const QVariant& value)
{
    if (!name)  return false;

    if (_isThreaded)  _mutex.lock();
    bool  retVal = _configObj->setProperty( name, value);
    if (_isThreaded)  _mutex.unlock();

    return retVal;
}


void  ArnScriptJobControl::addConfig( QObject* obj)
{
    if (!obj)  return;

    QList<QByteArray>  nameList = obj->dynamicPropertyNames();
    foreach (QByteArray name, nameList) {
        setConfig( name.constData(), obj->property( name.constData()));
    }
}


void  ArnScriptJobControl::setThreaded( bool isThreaded)
{
    _isThreaded = isThreaded;
}


/// Not threadsafe, only run in same thread as script
void  ArnScriptJobControl::doSetupJob( ArnScriptJob* job, ArnScriptJobFactory* jobFactory)
{
    if (!job)  return;
    if (!jobFactory)  return;

    // qDebug() << "ScrJobConfig: setup job=" << _name;
    job->setJobFactory( jobFactory);
    foreach (QString id, _interfaceList) {
        job->installExtension( id, this);  // Depends on jobFactory
    }
    job->addConfig( _configObj);
    job->evaluateScript( _script, _name);
    job->setupScript();
}


QVariant  ArnScriptJobControl::config( const char* name)  const
{
    if (!name)  return QVariant();

    if (_isThreaded)  _mutex.lock();
    QVariant  retVal = _configObj->property( name);
    if (_isThreaded)  _mutex.unlock();

    return retVal;
}
