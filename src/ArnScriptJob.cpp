// Copyright (C) 2010-2016 Michael Wiklund.
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
#include <QScriptable>
#include <QtScript>
#include <QScriptEngine>
#include <QFileInfo>
#include <QTimer>
#include <QEvent>
#include <QDebug>


const QEvent::Type  EventQuit = QEvent::Type( QEvent::User + 0);


ArnScriptJobB::ArnScriptJobB( int id, QObject* parent) :
        QObject( parent)
{
    _id = id;
    _isSleepState   = false;
    _isStopped      = false;
    _quitInProgress = false;
    _watchDogTime   = 2000;  // ms
    _pollTime       = 2000;  // ms

    _jobFactory = 0;
    _configObj  = new QObject( this);
    _abortTimer = new QTimer( this);
    _arnScr     = new ArnScript( this);
    setPollTime( _pollTime);

    connect( _abortTimer, SIGNAL(timeout()), this, SLOT(doTimeoutAbort()));
    connect( _arnScr, SIGNAL(errorText(QString)), this, SIGNAL(errorText(QString)));
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

    if (wt > 0) {
        _abortTimer->start( wt);
        _arnScr->engine().setProcessEventsInterval( wt);
    }
    else {
        _abortTimer->stop();
        _arnScr->engine().setProcessEventsInterval( _watchDogTime > 0 ? _watchDogTime : _pollTime);
    }
}


void  ArnScriptJobB::setPollTime( int time)
{
    _pollTime = time;
    _arnScr->engine().setProcessEventsInterval( _pollTime);
}


int  ArnScriptJobB::pollTime()
{
    return _pollTime;
}


void  ArnScriptJobB::installInterface( const QString& id, QObject* obj)
{
    if ((id.isEmpty()) || (obj == 0))  return;

    QScriptValue objScr = _arnScr->engine().newQObject( obj, QScriptEngine::QtOwnership,
                                                        QScriptEngine::ExcludeSuperClassContents);
    _arnScr->engine().globalObject().setProperty( id, objScr);

    if (obj != this)  obj->setParent( this);  // Reparent interface to this Job
    //MW: fix parrenting and delete failed install object
}


bool  ArnScriptJobB::installExtension( const QString& id, ArnScriptJobControl *jobControl)
{
    if (!_jobFactory || !_arnScr)  return false;

    return _jobFactory->installExtension( id, _arnScr->engine(), jobControl);
}


bool  ArnScriptJobB::evaluateScript( const QByteArray& script, const QString& idName)
{
    setWatchDog();
    bool stat = _arnScr->evaluate( script, idName);
    setWatchDog(0, false);
    return stat;
}


bool  ArnScriptJobB::evaluateScriptFile( const QString& fileName)
{
    setWatchDog();
    bool stat = _arnScr->evaluateFile( fileName);
    setWatchDog(0, false);
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


bool  ArnScriptJobB::setupScript()
{
    _jobInit  = _arnScr->engine().globalObject().property("jobInit");
    _jobEnter = _arnScr->engine().globalObject().property("jobEnter");
    _jobLeave = _arnScr->engine().globalObject().property("jobLeave");

    setWatchDog();
    QScriptValue  result = _jobInit.call(QScriptValue(), QScriptValueList());
    if (_arnScr->logUncaughtError( result)) {
        _isStopped = true;
        //qDebug() << "Setup Script timeout: isEval=" << _arnScr->engine().isEvaluating();
    }
    setWatchDog(0, false);

    return true;
}


void  ArnScriptJobB::enterScript()
{
    if (isStopped())  return;  // Don't execute Enter if Job is stopped (error ...)

    setWatchDog();
    QScriptValue  result = _jobEnter.call(QScriptValue(), QScriptValueList());
    if (_arnScr->logUncaughtError( result)) {
        setWatchDog(0, false);
        _isStopped = true;
        //qDebug() << "Enter Script timeout: isEval=" << _arnScr->engine().isEvaluating();
        emit scheduleRequest( _id);
    }
}


void  ArnScriptJobB::leaveScript()
{
    if (isStopped())  return;  // Don't execute Leave if Job is stopped (error ...)

    QScriptValue  result = _jobLeave.call(QScriptValue(), QScriptValueList());
    _arnScr->logUncaughtError( result);
    setWatchDog(0, false);
}


void  ArnScriptJobB::yield()
{
    emit scheduleRequest( _id);
}


void  ArnScriptJobB::quit()
{
    if (_quitInProgress)  return;  // Quit is already in progress
    _quitInProgress = true;

    QScriptEngine& engine = _arnScr->engine();
    engine.abortEvaluation( engine.currentContext()->throwError("Job quit"));

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
    _abortTimer->stop();
    _isStopped = true;

    //qDebug() << "Script timeout: isEval=" << _arnScr->engine().isEvaluating();
    QScriptEngine& engine = _arnScr->engine();
    engine.abortEvaluation( engine.currentContext()->throwError("Job run timeout"));

    emit timeoutAbort( _id);
    emit scheduleRequest( _id);
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


void  ArnScriptJobFactory::setupJsObj( const QString& id, const QScriptValue& jsObj, QScriptEngine& engine)
{
    engine.globalObject().setProperty( id, jsObj);
}


bool  ArnScriptJobFactory::setupInterface(const QString& id, QObject* interface, QScriptEngine& engine)
{
    if (id.isEmpty() || (interface == 0))  return false;

    QScriptValue  objScr = engine.newQObject( interface, QScriptEngine::QtOwnership,
                                              QScriptEngine::ExcludeSuperClassContents);
    if (objScr.isNull())  return false;

    setupJsObj( id, objScr, engine);

    if (!interface->parent())
        interface->setParent( engine.parent());  // Reparent interface to same parent as engine

    return true;
    //MW: fix parrenting and delete failed install object
}


ArnScriptJob::ArnScriptJob( int id, QObject* parent) :
        ArnScriptJobB( id, parent)
{
    installInterface("job", this);
    installInterface("config", _configObj);
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
