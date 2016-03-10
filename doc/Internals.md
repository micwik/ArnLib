ArnLib Internals    {#int_page}
================

[TOC]

This document describes internal processes that are relatively complex and by this needs some explanation.


ScriptJobs    {#int_scriptjobs}
==========
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
==========
* Monitor starts its actual connection job when its start method is called.

* Monitor (at client-side) results in creates an ItemNet with path to monitorPath.

* The ItemNet is also put in syncQueue (always main-thread).

* Monitor puts the arn-event "monitorStart" in event loop,
  which makes sure event is sent after Monitor (and its caller) has finished initiializing.

* When "monitorStart" is received on local (client) side, the ItemNet will change SyncMode to Monitor.
  This will resync ItemNet to a Monitor at any server restart.

* Now 2 possibilities depending on threading:
    1. The ItemNet was sent before syncMode Monitor was set. Then server will receive an ordinary
       ItemNet and do standard setup.
    2. The ItemNet was sent with syncMode Monitor set. The server will detect this and do
       MonitorSetup on the ItemNet.

* When arn-event "monitorStart" is received on server-side, if SyncMode is not already set to "Monitor",
  server will do MonitorSetup on the ItemNet.

* When doing MonitorSetup (at server-side), logic are made to send arn-events when new
  childs are created, and present childs are directly sent as arn-event.


Destroy    {#int_destroy}
=======
* Destruction can be locally initiated and affects one link. Destruction can be set as
  local or global.

* Destruction can also be initiated for leaves by the destroy command and arives with a netId.
  Or it can be with the delete command for a folder (tree).

* For leaf, corresponding ItemNet is disabled (set as defunct), which prohibit sending destroy
  command back to the originator of the command.

* The ItemNet is also destroyed in the same way as a locally initiated destruction and affects
  one link. Destruction is set to be global.

* The affected link:s tree is recursively traversed and all links are first marked as retired.
  Also the retire type is set as LeafLocal, LeafGlobal or Tree.

* As the last thing in this recursion each link is sending a Retired ArnEvent, ie the leaves are
  the first to send. The event is sent to the subscriptions (ArnBasicItems or derived) of each
  link.

* If it's a destroy of a tree (folder), a Retired ArnEvent is also sent to the tree:s parent
  and all the way up to the root. The event is sent to the subscriptions (ArnBasicItems) of each
  link. These events have a marking telling destroy is below.

* The Retired ArnEvent is handled by each subscribing Item. For ArnBasicItem this is done by
  its eventhandler, which by default is an internal handler. For ArnItem this is done by
  sending a linkDestroyed signal to be handled by application code.
  The Items is finally closed and by this the link ref counter is decremented.

* When the links ref counter is reaching zero, a ZeroRef ArnEvent is sent. Also a ZeroRef pending
  counter is increased.

* The event is handled by ArnM::doZerRefLink(), in Main thread. First the ZeroRef pending
  counter is decreased. Next both ref counter and ZeroRef pending counter is checked to be zero,
  which indicates that this is the final ZeroRef for this link.
  This is to prohibit a scenario where the link has been reused during ZeroRef ArnEvent delivery.
  Also this reuse might have been followed by a dropped usage resulting in a second ZeroRef
  ArnEvent.

* In ArnM::doZerRefLink() if this is the final ZeroRef, it will set the link ref counter
  to -1, to mark the link as fully de-referenced.
  The link and parent (and grand parants ...) are deleted if they don't have any children
  and ref = -1 and they are marked retired.

* When the ArnSync, which is eventHandler for ItemNet, is handling the Retired ArnEvent, it
  will delete the corresponding ItemNet from sync map and all queues. Finally a command can
  be sent with its netId.

* The sent command depends on retire type. For Leaflocal, a nosync command is used.
  For LeafGlobal, a delete command is used to spread the destruction to server and other
  clients. The Tree type doesn't send a command at item level.

* For tree destroy, ArnClient is using a monitoring ArnItemNetEar at each mount point to catch
  the Retire ArnEvent for a tree below. Such an event is resulting in a delete or noSync
  command is sent, depending on global or local destroy. The command is sent with the path
  to the destroyed tree.

* For tree destroy, ArnServer is using a monitoring ArnItemNetEar at root to catch the Retire
  ArnEvent  for a tree below. Such an event is resulting in a delete command is sent.
  The command is sent with the path to the destroyed tree.

* When a delete command is echoed back to the originator, it will stop with this only echo
  as the affected tree is already marked for retire and this will terminate the command.
