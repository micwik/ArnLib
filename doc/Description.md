General Description
===================

[TOC]

This document describes the general concepts of the ArnLib.
<Br><Br>


Arn Data Objects    {#gen_arnobj}
----------------
All objects are stored in a tree hierarchy and the naming is similar to typical filesystems e.g. "//Measure/Water/Temperature/value". 

Folder names can be empty. In the above example, the first level folder is empty and the second level folder is "Measure".
The empty folder name can also be refered as "@". Again the example can equally be written "/@/Measure/Water/Temperature/value". This "@" is typically used when an empty name is unacceptable, e.g. in the tree viewer of ArnBrowser.

Each part in a given path is dynamically added as needed, i.e. any path can be used without explicitly creating each folder in advance.
<Br><Br>

### Modes ###    {#gen_arnobjModes}
_Mode_ change is a one direction process. Once a specific _mode_ is set, it can't be reseted. 

If the ArnItem is in a closed state when the _mode_ change is done, the added modes will be stored and the real _mode_
change is done when the ArnItem is opened to an _Arn Data Object_. This implies that ArnItems can benefit from _mode_ settings before being opened.

If the _general mode_ change is done to a [shared](#gen_shareArnobj) object, the change of _general mode_ is also done at the server and any connected clients.

The following _general modes_ are available:

* **BiDir** A two way object, typically for validation or pipe.
  See [bidirectional](#gen_bidirArnobj) objects.
* **Pipe** Implies _BiDir_ and all data is preserved as a stream during [sharing](#gen_shareArnobj).
  Without _Pipe mode_, [sharing](#gen_shareArnobj) is optimized to sync latest value and not all values in a stream.
* **Save** Sets the _Arn Data Object_ as persistent and any data assigned to it will be saved.
  The persistent service must be started at the server.
  See [persistent](#gen_persistArnobj) objects.

Additionally there are some _sync modes_. These modes are used by the local client session and are not shared with others.
The _sync modes_ must be set before the ArnItem is opened to an _Arn Data Object_.

Following _sync_modes_ are available:

* **Master** The _Arn Data Object_ (at client side) is set as _default generator_ of data.
  Normally the server is the _default generator_ of data.
  This makes difference when client connects or reconnects to the server.
  The data from the _default generator_ is then used and synced.
* **AutoDestroy** The _Arn Data Object_ (at client side) is setup for auto destruction.
  When the client close tcp/ip, the server side will destroy the _Arn Data Object_ and this will also be done at any connected clients.

Note: It's convenient to allways set all the needed modes before an ArnItem is opened or use an ArnItem as a template. See ArnItem::setTemplate().
<Br><Br>

### Naming conventions ###    {#gen_naming}
These rules must not be obeyed, but it's recommended, to get the most benefits of the Arn echo system, like ArnBrowser.

* First level folder not empty, e.g "/MyLocalFolder/Key/value", is a local path and is not [shared](#gen_shareArnobj).
* First level folder empty, e.g. "//MyGlobalFolder/Date/value", is a global path and is [shared](#gen_shareArnobj) to server and clients.
* When a leaf is used as a attribute, the following names are reserved:
    + **value** the value of the above closest folder denotation e.g. "Temperature".
    + **set** allowed values and conversion to a more desciptive form e.g. "0=Off 1=On".
    + **property** like precision and unit e.g. "prec=1 unit=°C".
    + **info** like tool tips e.g. "<tt\>Standard UV radiation index</tt\>".
    + **help.**XXX like "help.html" contains help in xhtml format.
<Br><Br>


Bidirectional Arn Data Objects    {#gen_bidirArnobj}
------------------------------
A bidirectional _Arn Data Object_ is actually a double object, a twin.
Each part has it's own path but there life span is depending on each other.

One part is the normal "official" and the other part is provider.
The provider has an added "!" to the normal path e.g. normal = "//Measure/Depth/value", provider = "//Measure/Depth/value!".

Data written to one part ends up in the other. When a provider slot is connected 
to the provider part (ArnItem), the slot will receive "request" data from the normal part.
The provider slot process the request data and write the
result to the same provider part. This way the result will end up in the
normal "official" part.

This functionality can typically be used for data validation and limiting.
<Br><Br>

### Pipes ###    {#gen_pipeArnobj}
_Pipes_ also use the [bidirectional](#gen_bidirArnobj) functionality. The two (twin) parts are then named _requester_ and _provider_.

All data put into a pipe is part of a stream and as such will be fully transfered (syncronized) if it's [shared](#gen_shareArnobj) with a server and other clients.
<Br><Br>


Persistent Arn Data Objects    {#gen_persistArnobj}
------------------------------
<Br><Br>


Sharing Arn Data Objects    {#gen_shareArnobj}
------------------------
A fundamental aspect of _Arn_ is that _Arn Data Objects_ can be shared. This is centralized to the _Arn Server_, which stores all shared objects. It's still a distributed model as each client and server has it's own set of _Arn Data Objects_ that operates independent of any connection.

Each _Arn Client_ connects to the _Arn Server_ and decides which part of the _Arn Data Object_ tree to be shared. <Br>
`ArnClient::setMountPoint("/share/")` will make the tree"/share/" shared. <Br>
This doesn't mean that everything in the shared tree at the server now will be available at the client. The client has to create an _Arn Data Objects_ in the shared tree. The client can then decide exactly objects of interrest. <Br>
`ArnItem::Open("/share/Test/value")` will open a shared object (in previous example).

Note: Normally "//" is used for global (shared), see [naming conventios](#gen_naming).
<Br><Br>


Application notations    {#gen_appnote}
---------------------
* If any graphics is used, Gui must be included.

* If only using QImage, Windowing system can be off like:
  QApplication a(argc, argv, false);
