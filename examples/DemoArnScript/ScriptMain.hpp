#ifndef SCRIPTMAIN_HPP
#define SCRIPTMAIN_HPP

#include <ArnInc/ArnClient.hpp>
#include <ArnInc/ArnScript.hpp>
#include <QTimer>


class ScriptMain : public QObject
{
    Q_OBJECT
public:
    explicit ScriptMain( QObject* parent = arnNullptr);

private slots:
    void onTimeout();

private:
    ArnScript _arnScript;
    ArnClient  _arnClient;
    ArnItem  _arnClkPulse;
    QTimer _timer;
};

#endif // SCRIPTMAIN_HPP
