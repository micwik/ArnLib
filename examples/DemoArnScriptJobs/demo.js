// Demo  V1.0  Show integration of javascript to Arn application


// Templates for ArnItem modes
var tplStd = new ArnItem;
tplStd.smTemplate  = true;  // Activate template mode

var tplMaster = new ArnItem;
tplMaster.smTemplate  = true;
tplMaster.smMaster    = true;

var tplSave = new ArnItem;
tplSave.smTemplate = true;
tplSave.saveMode   = true;
//


function jobInit()
{
    print("Init DemoScript");

    basePath        = config.basePath;
    commonPath      = config.commonPath;
    settingsPath    = "//Settings/Demo/";
    demoRun         = false;
    print( "basePath=" + basePath);

    arnClkIn        = new ArnItem( commonPath + "ClkPulse/value");
    arnD1Num        = new ArnItem( basePath + "D1Num/value");
    arnD2Num        = new ArnItem( basePath + "D2Num/value");
    arnPeriodTimer  = new ArnItem( basePath + "PeriodTimer/value");
    arnClkOut       = new ArnItem( tplMaster, basePath + "ClockOut");
    arnPeriodTimo   = new ArnItem( tplSave,   settingsPath + "PeriodTimeout/value");

    arn.setString( settingsPath  + "PeriodTimeout/property", "prec=0 unit=sec");

    // Handle dependency wait
    job.sleepState = true;
    depend = new ArnDepend;
    depend.setMonitorName("DemoScript");
    depend.add("$Persist");
    depend.startMonitor();
    depend.completed.connect(
        function() {
            job.sleepState = false;
            print("DemoScript depend Ok");
            postInit();
        }
    );

    // Handle script Quit (reload)
    job.sigQuit.connect(
        function() {
            print("sigQuit");
        }
    );
}


// Run after dependeny is completed
function postInit()
{
    arnClkIn.changedVoid.connect( onClkChanged); // Gives tick every second
    arnD1Num.changedNum.connect(
        function( value) {
            arnD2Num.num = value + 10;
        }
    );

    if (arnPeriodTimo.num <= 5)
        arnPeriodTimo.num = 5;
    arnPeriodTimer.num = arnPeriodTimo.num;
}


function onClkChanged()
{
    if (!demoRun)  return;

    arnPeriodTimer.num = arnPeriodTimer.num - 1;
    if (arnPeriodTimer.num <= 0) {
        if (arnPeriodTimo.num <= 5)
            arnPeriodTimo.num = 5;
        arnPeriodTimer.num = arnPeriodTimo.num;
        var d = new Date();
        print("Periodic timeout ... " + mq.dateToString( d, "yyyy-MM-dd_hh:mm:ss"));
        job.yield();
    }

    arnClkOut.num = (arnClkOut.num + 1) % 100;
}


function jobEnter()
{
    print("Enter DemoScript");
    demoRun = true;
    job.watchDog = 0;  // Turn off watchdog for this run slot (until jobLeave)
}


function jobLeave()
{
    print("Leave DemoScript");
    demoRun = false;
}
