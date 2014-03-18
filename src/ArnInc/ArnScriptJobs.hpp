// Copyright (C) 2010-2014 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these
// other libraries is subject to their respective license agreements.
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
#include <QObject>


//! \cond ADV
class ArnScriptJobThread : public QThread
{
Q_OBJECT
public:
    ArnScriptJobThread( ArnScriptJobControl* jobConfig, ArnScriptJobFactory* jobFactory);
    ~ArnScriptJobThread();

protected:
    void  run();

private:
    ArnScriptJobControl*  _jobConfig;
    ArnScriptJobFactory*  _jobFactory;
};


class ARNLIBSHARED_EXPORT ArnScriptJobSingle : public QObject
{
Q_OBJECT
public:
    ArnScriptJobSingle( ArnScriptJobControl* jobConfig, ArnScriptJobFactory* jobFactory);
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

    ArnScriptJob*  _job;
    int  _runningId;
    bool  _scriptChanged;
};
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
    explicit ArnScriptJobs( QObject* parent = 0);
    void  addJob( ArnScriptJobControl* jobConfig, int prio = 1);
    void  setFactory( ArnScriptJobFactory* jobFactory);
    void  start( Type type = Type::Cooperative);

signals:

private slots:
    void  doScheduleRequest( int callerId);
    void  setScriptChanged( int id);

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
            thread        = 0;
            jobConfig     = 0;
            job           = 0;
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
};

#endif // ARNSCRIPTJOBS_HPP
