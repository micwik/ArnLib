#include "ScriptMain.hpp"


ScriptMain::ScriptMain( QObject* parent)
    : QObject( parent)
{
    ArnM::setConsoleError( false);
    ArnM::setDefaultIgnoreSameValue( true);

    //// Select Arn tree to sync (//) and auto reconnect
    _arnClient.addMountPoint("//");
    _arnClient.setAutoConnect(true);

    //// Connect to the Server
    _arnClient.connectToArn( "localhost", 2022);

    _commonPath = "//DemoCommon/";
    _arnClkPulse.open( _commonPath + "ClkPulse/value");
    connect( &_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    _timer.start( 1000);

    _scriptJobFactory = new ScriptJobFactory;
    _jobs.setFactory( _scriptJobFactory);
    QString scriptFileName = "demo.js";
    QString scriptName     = "Demo";
    for (int i = 1; i <= 2; ++i) {
        QString jobName = QString("Demo-%1").arg(i);
        addJobb( scriptFileName, scriptName, jobName);
    }

    _jobs.start( ArnScriptJobs::Type::Cooperative);
    // _jobs.start( ArnScriptJobs::Type::Preemptive);
}


void ScriptMain::addJobb( const QString& scriptFileName, const QString& scriptName, const QString& jobName)
{
    ArnScriptJobControl* jobControl = new ArnScriptJobControl( this);
    jobControl->setName( scriptFileName);  // name?
    jobControl->addInterfaceList( QStringList() << "mq");
    jobControl->setConfig( "basePath",   QString("//Demojobs/%1/").arg( jobName));
    jobControl->setConfig( "commonPath", _commonPath);

    //// Connect to Arn Js & logPipes
    QString  scrPath = QString("//Script/%1/%2").arg( scriptName).arg( scriptFileName);
    ArnItem*  arnScript = new ArnItem( jobControl);
    arnScript->open( scrPath);
    ArnM::loadFromFile( arnScript->path(), scriptFileName, Arn::Coding::Text);
    jobControl->setScript( arnScript->toByteArray());
    connect( arnScript, SIGNAL(changed(QByteArray)), jobControl, SLOT(setScript(QByteArray)));
    QString  logPath = QString("//Script/%1/logPipes/%2!").arg( scriptName).arg( jobName);
    ArnItem*  arnLog = new ArnItem( logPath, jobControl);
    arnLog->setPipeMode();
    connect( jobControl, SIGNAL(errorText(QString)), arnLog, SLOT(setValue(QString)));
    _jobControlTab += jobControl;
    _jobs.addJob( jobControl, 10);
}


void ScriptMain::onTimeout()
{
    _arnClkPulse = !_arnClkPulse.toBool();
}


////////////////

ScriptJobFactory::ScriptJobFactory()
{
}


/// Must be threadsafe
bool  ScriptJobFactory::installExtension( const QString& id, ARN_JSENGINE& engine, const ArnScriptJobControl* jobControl)
{
    Q_UNUSED(jobControl)

    //// Interfaces
    if (id == "mq") {
        ScriptMQ*  mq = new ScriptMQ;
        return setupInterface( id, mq, engine);
    }
    return false;  // Fail
}


//////////////////////

ScriptMQ::ScriptMQ( QObject* parent) :
    QObject( parent)
{
}


QString  ScriptMQ::dateToString( QDateTime dateTime, QString format)
{
    return dateTime.toString( format);
}
