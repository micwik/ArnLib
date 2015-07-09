ArnLib Internals
================

[TOC]

This document describes internal processes that are relatively complex and by this needs some explanation.


ScriptJobs    {#int_scriptjobs}
----------
* Each jobstack ScriptJobs is setup with a ScriptJobFactory wich makes custom interfaces etc.

* ScriptJobControl is setup with: Sriptfile, Config (QObject) and InterfaceList.
  Scriptfile is also copied to a ArnItem.

* ScriptJobControl can be connected to update of script in Arn, to make reload possible.

* Error text from ScriptJobControl can be connected to a pipe in Arn for logging.

* ScriptJobControl together with jobpriority define the ScriptJob and is added to ScriptJobs.
  Error text from Script job is connected to ScriptJobControl.

* Starting ScriptJobs in cooperative mode:
    1. Every ScriptJob is created and setup by corresponding ScriptJobControl
    2. Every ScriptJob is connected to Scheduler (yield etc).
    3. Every ScriptJobControl is connected to ScriptJobs for signaling update of script.
    4. Scheduler is started.

* Setup ScriptJob by ScriptJobControl:
    1. set ScriptJobFactory and Config
    2. Make and add the jobs Interfaces
    3. Evaluate the script (in js engine)
    4. run script function jobInit()

* Updating Script in cooperative mode:
    1. ScriptJobControl gets updated by Arn (or other).
    2. ScriptJobControl sends signal to ScriptJobs, which sets an updated flag for the corresponding Script Job.
    3. When scheduling, every updated script will get its sigQuit signal invoked and then reloaded.
    4. Reloading includes creating a new ScriptJob and setting up with ScriptJobControl etc.

* Starting ScriptJobs in preemtive mode:
    1. Every ScriptJob gets its own thread which also is setup with ScriptJobControl and ScriptJobFactory.
    2. Thread is started and it create a ScriptJobSingle where followning steps are done.
    3. ScriptJob is created and setup by ScriptJobControl
    4. ScriptJob is connected to Scheduler (yield etc).
    5. ScriptJobControl is connected to ScriptJobSingle for signaling update of script.
    6. Scheduler is started in ScriptJobSingle (just one job).

* Updating Script in preemtive mode:
    1. ScriptJobControl gets updated by Arn (or other).
    2. ScriptJobControl sends signal to ScriptJobSingle, which sets an updated flag
       and both invokes sigQuit signal to script and calls quit in scriptJob. 
    3. ScriptJob aborts its js script engine and posts a custom Quit event with high prio.
    4. When ScriptJob get the Quit event, it will send a QuitRequest signal to ScriptJobSingle.
    5. ScriptJobSingle will get the signal amd detect update flag, which means reloading.
    6. Reloading includes creating a new ScriptJob and setting up with ScriptJobControl etc.


ArnMonitor    {#int_arnmonitor}
----------
* Monitor starts its actual connection job when monitorPath is set.

* Monitor (at client-side) creates an ItemNet with path to monitorPath.

* The ItemNet is also put in syncQueue (always main-thread).

* Monitor puts the arn-event "monitorStart" in event loop,
  which makes sure event is sent after Monitor (and its caller) has finished initiializing.

* When "monitorStart" is received on local (client) side, the ItemNet will change SyncMode to Monitor.
  This will resync ItemNet to a Monitor at any server restart.

* Now 2 possibilities depending on threading:
    1. The ItemNet was sent before syncMode Monitor was set. Then server will receive an ordinary
       Itemnet and do standard setup.
    2. The ItemNet was sent with syncMode Monitor set. The server will detect this and do
       MonitorSetup on the ItemNet.

* When arn-event "monitorStart" is received on server-side, if SyncMode is not already set to "Monitor",
  server will do MonitorSetup on the ItemNet.

* When doing MonitorSetup (at server-side), connections are made to send arn-events when new
  childs are created, and present childs are directly sent as arn-event.


Destroy    {#int_destroy}
-------
* Destruction can be locally initiated and affects one link. Destruction can be set as
  local or global.

* Destruction can also be initiated by the destroy command and arives with a netId.

* Corresponding ItemNet is disabled (set as defunct), which prohibit sending destroy command
  back to the originator of the command.

* The ItemNet is also destroyed in the same way as a locally initiated destruction and affects
  one link. Destruction is set to be global.

* The affected link:s tree is recursively traversed and all links are first marked as retired.
  Also global or local destroy is marked.

* As the last thing in this recursion each link is emitting a retired signal, ie the leaves are
  the first to emit.

* The retired signal is handled by each connected Item. Each Item is sending a linkDestroyed
  signal to be handled by application code.
  The Items is finally closed and by this the link ref counter is decremented.

* When the links ref counter is reaching zero, a zeroRef signal is sent.

* The signal is handled by doZerRefLink(), in Main thread. It will set the link ref counter
  to -1, to mark the link as fully de-referenced.
  The link and parent (and grand parants ...) are deleted if they don't have any children
  and ref = -1 and they are marked retired.

* When the ItemNet is sending the linkDestroyed signal, it will be deleted from sync map
  and all queues. Finally a command is sent with its netId.

* The sent command depends on global or local destroy. For local, a nosync command is used.
  For global, a destroy command is used to spread the destruction to server and other clients.
