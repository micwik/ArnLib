#ifndef SCRIPTMAIN_HPP
#define SCRIPTMAIN_HPP

#include <ArnInc/ArnClient.hpp>
#include <ArnInc/ArnScriptJobs.hpp>
#include <QTimer>
#include <QDateTime>
#include <QList>


class ScriptMQ : public QObject
{
    Q_OBJECT
public:
    explicit ScriptMQ( QObject* parent = 0);

public slots:
    QString  dateToString( QDateTime dateTime, QString format);
};


/// Must be thread-safe
class ScriptJobFactory : public ArnScriptJobFactory
{
public:
    explicit  ScriptJobFactory();
    virtual bool  installExtension( const QString& id, ARN_JSENGINE& engine,
                                    const ArnScriptJobControl* jobControl = arnNullptr);

private:
};


class ScriptMain : public QObject
{
    Q_OBJECT
public:
    explicit ScriptMain( QObject* parent = arnNullptr);

private slots:
    void onTimeout();

private:
    void addJobb( const QString& scriptFileName, const QString& scriptName, const QString& jobName);

    ArnScriptJobs  _jobs;
    ScriptJobFactory*  _scriptJobFactory;
    QList<ArnScriptJobControl*>  _jobControlTab;

    ArnClient  _arnClient;
    ArnItem  _arnClkPulse;
    QTimer _timer;
    QString _commonPath;
};

#endif // SCRIPTMAIN_HPP
