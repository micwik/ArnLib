#include "ScriptMain.hpp"

ScriptMain::ScriptMain( QObject* parent)
    : QObject( parent)
{    
    //// Select Arn tree to sync (//) and auto reconnect
    _arnClient.addMountPoint("//");
    _arnClient.setAutoConnect(true);

    //// Connect to the Server
    _arnClient.connectToArn( "localhost", 2022);

    _arnClkPulse.open("//Demo/ClkPulse/value");
    _arnScript.evaluateFile( "demo.js");

    connect( &_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    _timer.start( 1000);

    ARN_JSVALUE initFunc = _arnScript.globalProperty( "jobInit");
    ARN_JSVALUE result   = _arnScript.callFunc( initFunc, ARN_JSVALUE(), ARN_JSVALUE_LIST());
}


void ScriptMain::onTimeout()
{
    _arnClkPulse = !_arnClkPulse.toBool();
}
