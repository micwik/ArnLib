General Description
===================

[TOC]

This document describes the general concepts of the ArnLib.
<Br><Br>


Arn Data Objects    {#gen_arnobj}
----------------
All objects are stored in a tree hierarchy and the naming is similar to typical
filesystems e.g. "//Measure/Water/Temperature/value".

To get a handle to a folder give a path ending with "/", e.g "//Measure/Water/".

Folder names can be empty. In the above example, the first level folder is empty and the
second level folder is "Measure".
The empty folder name can also be refered as "@". Again the example can equally be written
"/@/Measure/Water/Temperature/value". This "@" is typically used when an empty name is
unacceptable, e.g. in the tree viewer of ArnBrowser.

Each part in a given path is dynamically added as needed, i.e. any path can be used without
explicitly creating each folder in advance.
<Br><Br>

### Modes ###    {#gen_arnobjModes}
_Mode_ change is a one direction process. Once a specific _mode_ is set, it can't be reseted. 

If the ArnItem is in a closed state when the _mode_ change is done, the added modes will
be stored and the real _mode_ change is done when the ArnItem is opened to an
_Arn Data Object_. This implies that ArnItems can benefit from _mode_ settings before
being opened.

If the _general mode_ change is done to a [shared](#gen_shareArnobj) object, the change
of _general mode_ is also done at the server and any connected clients.

The following _general modes_ are available:

* **BiDir** A two way object, typically for validation or pipe.
  See [bidirectional](#gen_bidirArnobj) objects.
* **Pipe** Implies _BiDir_ and all data is preserved as a stream during
  [sharing](#gen_shareArnobj). Without _Pipe mode_, [sharing](#gen_shareArnobj) is
  optimized to sync latest value and not all values in a stream.
* **Save** Sets the _Arn Data Object_ as persistent and any data assigned to it will be
  saved. The persistent service must be started at the server.
  See [persistent](#gen_persistArnobj) objects.

Additionally there are some _sync modes_. These modes are used by the local client session
and are not shared with others.
The _sync modes_ must be set before the ArnItem is opened to an _Arn Data Object_.

Following _sync_modes_ are available:

* **Master** The _Arn Data Object_ (at client side) is set as _default generator_ of data.
  Normally the server is the _default generator_ of data.
  This makes difference when client connects or reconnects to the server.
  The data from the _default generator_ is then used and synced.
* **AutoDestroy** The _Arn Data Object_ (at client side) is setup for auto destruction.
  When the client close tcp/ip, the server side will destroy the _Arn Data Object_ and
  this will also be done at any connected clients.

Note: It's convenient to allways set all the needed modes before an ArnItem is opened or
use an ArnItem as a template. See ArnItem::setTemplate().
<Br><Br>

### Naming conventions ###    {#gen_naming}
These rules must not be obeyed, but it's recommended, to get the most benefits of the
Arn echo system, like ArnBrowser.

* First level folder not empty, e.g "/MyLocalFolder/Key/value", is a local path and is
  not [shared](#gen_shareArnobj).
* First level folder empty, e.g. "//MyGlobalFolder/Date/value", is a global path and is
  [shared](#gen_shareArnobj) to server and clients.
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
Each part has it's own path but their life span is depending on each other.

One part is the normal "official" and the other part is _provider_.
The provider has an added "!" to the normal path e.g. normal = "//Measure/Depth/value",
provider = "//Measure/Depth/value!".

Data written to one part ends up in the other. When a provider slot is connected to the
provider part (ArnItem), the slot will receive "request" data from the normal part.
The provider slot process the request data and write the result to the same provider part.
This way the result will end up in the normal "official" part.

This functionality can typically be used for data validation and limiting.
<Br><Br>

### Pipes ###    {#gen_pipeArnobj}
_Pipes_ also use the [bidirectional](#gen_bidirArnobj) functionality. The two (twin) parts are then named _requester_ and _provider_.

All data put into a pipe is part of a stream and as such will be fully transfered (syncronized) if it's [shared](#gen_shareArnobj) with a server and other clients.
<Br><Br>


Persistent Arn Data Objects    {#gen_persistArnobj}
------------------------------
The _server_ must use ArnPersist to support the persistance service. As standard,
objects are stored in a SQLite database. It's also possible to store each object as a file.

Any connected _client_ or the _server_ can make an _Arn Data Object_ persistent.
It's just having an ArnItem opened to the object and changing _mode_ to _Save_.
> ArnItem  arnMaxLevel; <Br>
> arnMaxLevel.addMode( ArnItem::Mode::Save); <Br>
> arnmaxLevel.open("//config/Level/Max/value"); <Br>

When the _Arn Data Object_ is set to _Save_ mode, it's automatically loaded by the
ArnPersist. At the _server_ this is instantly done. A _client_ has to wait for the value
to get synced from the _server_. It's convenient to use ArnDepend for getting a signal
when the value is loaded and ready to use.

When the _Arn Data Object_ is changed, it will be automatically saved by ArnPersist.
There is a delay from first change of the object to the saving is done,
see ArnItem::setDelay(). This allows for intensive updates of the object without choking
down the server with saving operations.

It's possible to mark an object in the SQLite data base as _mandatory_, this way the
_Arn Data Object_ is set as _persistent_ and gets loaded at start of ArnPersist.
<Br><Br>

### Saving objects in files ###    {#gen_fileArnobj}
To use the _persistent_ storing of _Arn Data Objects_ in files, the _root_ directory is
set by: ArnPersist::setPersistDir(). This can also be combined with support of VCS
(version control system), see ArnPersist::setVcs(). Currently there is a support module
for _git_.

In the _root_ directory and below, all (VCS) persistent files are stored.
The _root_ directory correspond to the _root_ in Arn tree.

> Example: _root_ directory is set to "/usr/local/arn_persist". There > is a file stored
  at "/usr/local/arn_persist/@/doc/help.html". This > file will be mapped to Arn at
  "//doc/help.html".

Any files stored In the _root_ directory and below, gets loaded into its _Arn Data Object_
with _mode_ set as _persistent_ at start of ArnPersist.

The files get updated in a similar way to the data base update. 
<Br><Br>


Sharing Arn Data Objects    {#gen_shareArnobj}
------------------------
A fundamental aspect of _Arn_ is that _Arn Data Objects_ can be shared. This is
centralized to the _Arn Server_, which stores all shared objects. It's still a
distributed model as each client and server has it's own set of _Arn Data Objects_ that
operates independent of any connection.

Each _Arn Client_ connects to the _Arn Server_ and decides which part of the
_Arn Data Object_ tree to be shared. <Br>
`ArnClient::setMountPoint("/share/")` will make the tree"/share/" shared. <Br>
This doesn't mean that everything in the shared tree at the server now will be available
at the client. The client has to create an _Arn Data Objects_ in the shared tree.
The client can then decide exactly the objects of interrest. <Br>
`ArnItem::Open("/share/Test/value")` will open a shared object (in previous example).

Note: Normally "//" is used for global (shared), see [naming conventions](#gen_naming).
<Br><Br>


RPC and SAPI    {#gen_rpc}
------------
ArnRpc is the basic funtionality of RPC (Remote Procedure Call). ArnSapi implements SAPI
(Service Application Programming Interface) and is using ArnRpc as its base.
It's recommended to use ArnSapi which uses a higher level model.

The SAPI works by a model that could be described as RPC by _remote signal slots_.
The _provider_ is usually assumed to wait for a _requester_ to initiate the session
and then react to different remote calls from the _requester_.
However this is full duplex, so any side can make a remote call at any time.

A good example of the usage of SAPI is the _Arn Demo Chat_, which is included in the
source package of the _ArnLib_.

ArnRpc uses [pipes](\ref gen_pipeArnobj) to communicate. The _pipes_ can be monitored 
and get injected test stimuli from the _Arn Browser_ program. The used
[protocoll](#gen_rpcformat) is XString based and quite easy to handtype when common data
types are used. "$help" will give the syntax for the actual custom SAPI.

A SAPI is setup by deriving the ArnSapi class to a new class that defines the
_custom SAPI_. This custom declared class is included at both the _provider_ and
_requester_ ends.
The _custem SAPI_ class by itself doesn't implement any _services_. It's merely a hub for
connections to _external signals and slots_.
The base ArnSapi class automatically transfer all _custom signal_ (SAPI) calls to the
remote connected ends, which also has the ArnSapi derived class and that emits the
transfered signal.

The provider connects the signals from custom SAPI that is prefixed with "pv_"
(as default) to each external slot that implements the services.
In the same way the _requester_ connects the signals prefixed with "rq_" to its external
"service" slots.

When there is a naming pattern between the _SAPI services_ and the
_external signals and slots_, it's a great convenience to use ArnRpc::BatchConnect().
This saves a lot of QObject::connect() calls.
Also newly added services in the SAPI, that obey the naming scheme, will automaically be
connected to the newly matching _external signals and slots_ for implementation of the
_service_.

An extended feature comparing to normal _signals_ is that the _SAPI
signals_ are _public_ and can be called by non derived classes.
This makes it optional to use both _signal to signal_ connections or direct _signal_ calls
when issuing a RPC to the remote side.

The _service_ slot can get the emiting _custom SAPI_ object by using normal
QObject::sender() functionality.
<Br><Br>

### RPC and SAPI communication format ###    {#gen_rpcformat}
The RPC calling has a basic format as XString (see XStringMap). The most generic form
is seen below. The type mark _T_ is "t" for writeable types and "tb" for binary
(non writeable) types.
> _funcname_ _T_=_type1_ a._label1_=_arg1_ _T_=_type2_ a._label2_=_arg2_ ... <Br> 
> Example: put t=QString a.id=level t=int a.value=123 <Br>
> For calling: put( QString("level"), 123)

Commonly used _types_ have a shorter form. The _types_ are:
> _int_, _uint_, _bool_, _ba (QByteArray), list (QStringList) and default is QString

This can be used in previous example: 
> put a.id=level int.value=123

Or even shorter, skipping labels, when typed by hand:
> put level int=123

List (QStringList) can be used. The _le_ is _list element_. All examples below will get
same resulting call.
> For a function: void test( QStringList lst, int num) <Br>
> test list=red green blu int=3 <Br>
> test list.lst=red green blu int.num=3 <Br>
> test list= le=red le=green le=blu int=3 <Br>
> test list=red le=green blu int=3 <Br>

For special cases, like empty elements, the _le_ (list element) is needed. The example
below has a first empty element followed by "green".
> test list= le= green blue int=2

The builtin call "$help" will give an automatically generated list of the present SAPI with the syntax for each available service.
<Br><Br>


Application notations    {#gen_appnote}
---------------------
* If any graphics is used, Gui must be included.

* If only using QImage, Windowing system can be off like:
  QApplication a(argc, argv, false);
