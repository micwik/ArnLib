General Description
===================

[TOC]

This document describes the general concepts of the ArnLib.
<Br><Br>


Arn Data Objects    {#gen_arnobj}
----------------
All objects are stored in a tree hierarchy and the naming is similar to typical
file systems, e.g. "//Measure/Water/Temperature/value".

To get a handle to a folder, use a path ending with "/", e.g "//Measure/Water/".

Folder names can be empty. In the above example, the first level folder is empty and the
second level folder is "Measure".
The empty folder name can also be referred as "@". Again, the example can equally be written
"/@/Measure/Water/Temperature/value". This "@" is typically used when an empty name is
unacceptable, e.g. in the tree viewer of ArnBrowser.

A relative path is also called the [local path](#gen_localPath), e.g.
"Sys/Discover/This/Service/value".

Each part in a given path is dynamically added as needed, i.e. any path can be used without
explicitly creating each folder in advance.
<Br><Br>

### Modes ###    {#gen_arnobjModes}
_Mode_ change is a one direction process. Once a specific _mode_ is set, it can't be reset.

If the ArnItem is in a closed state when the _mode_ change is done, the added modes will
be stored and the real _mode_ change is done when the ArnItem is opened to an
_Arn Data Object_. This implies that ArnItems can benefit from _mode_ settings before
being opened.

If the _general mode_ change is done to a [shared](#gen_shareArnobj) object, the change
of _general mode_ is also done at the server and any connected clients.

The following _general modes_ are available:

* **BiDir** A two-way object, typically for validation or pipe.
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
* **AutoDestroy** The _Arn Data Object_ (at client side) is set up for auto destruction.
  When the client closes tcp/ip, the server side will destroy the _Arn Data Object_ and
  this will also be done at any connected clients.

Note: It's convenient to always set all the needed modes before an ArnItem is opened or
an ArnItem is used as a template. See ArnItem::setTemplate().
<Br><Br>

### Local path ###    {#gen_localPath}
A relative path is also called the _local path_, e.g. the
[Discover remote](#gen_discoverRemote) _service name_ at path
"Sys/Discover/This/Service/value". The _local path_ is mapped to the absolute path
"/Local/". The example is then equal to "/local/Sys/Discover/This/Service/value". The
_local path_ should not be [shared](#gen_shareArnobj) as it will contain specific data
for its running program.

The exception to not sharing _local path_ is for some kind of remote client that must
be able to change an _Arn Data Object_ in the _local path_ at the remoted target.
For example this is used to change the [Discover remote](#gen_discoverRemote) _service name_
for a target host.

Following must be observed at remote client when sharing _local path_:

* use ArnM::setSkipLocalSysLoading()
* Don't put anything in _local path_, that should't end up in the shared target host.
* Don't use ArnDiscoverRemote or ArnDiscoverConnector.
* This unwanted exception design will be addressed in a future version of ArnLib.
<Br><Br>

### Naming conventions ###    {#gen_naming}
These rules must not be obeyed, but are recommended, to get the most benefits of the
Arn echo system, like ArnBrowser.

* First level folder empty, e.g. "//MyGlobalFolder/Date/value", is a global path and is
  [shared](#gen_shareArnobj) to server and clients.
* First level folder not empty, e.g "/MyLocalFolder/Key/value", is a local path and is
  not [shared](#gen_shareArnobj).
* When a leaf is used as an attribute, the following names are reserved:
    + **value** the value of the above closest folder denotation, e.g. "Temperature".
    + **set** allowed values and conversion to a more descriptive form, e.g. "0=Off 1=On".
    + **property** like precision and unit, e.g. "prec=1 unit=°C".
    + **info** like tool tips, e.g. "<tt\>Standard UV radiation index</tt\>".
    + **help.**XXX like "help.xhtml" contains help in xhtml format.
<Br><Br>


Bidirectional Arn Data Objects    {#gen_bidirArnobj}
------------------------------
A bidirectional _Arn Data Object_ is actually a double object, a twin.
Each part has its own path but their life span is depending on each other.

One part is the normal "official" and the other part is _provider_.
The provider has an added "!" to the normal path, e.g. normal = "//Measure/Depth/value",
provider = "//Measure/Depth/value!".

Data written to one part ends up in the other. When a provider slot is connected to the
provider part (ArnItem), the slot will receive "request" data from the normal part.
The provider slot processes the request data and writes the result to the same provider part.
This way the result will end up in the normal "official" part.

This functionality can typically be used for data validation and limiting.
<Br><Br>


Pipe Arn Data Objects    {#gen_pipeArnobj}
---------------------
_Pipes_ also use the [bidirectional](#gen_bidirArnobj) functionality. The two (twin) parts
are then named _requester_ and _provider_.

All data put into a pipe are part of a stream and as such will be fully transfered
(syncronized) if they are [shared](#gen_shareArnobj) with a server and other clients.

ArnPipe is a specialized class for handling pipes. <Br>
It contains logic for handling [sequence check](#gen_pipeSeqCheck) and
[anti congest](#gen_pipeAntiCongest).
<Br><Br>

### Pipe sequence check ###    {#gen_pipeSeqCheck}
Sequence check is used to make sure everything is received and nothing is lost or comes twice.
This can happen when a tcp/ip connection goes up and down.

The sequence check uses a hidden sequence number not visible in the pipe stream.
The sequence number is increased for each assignment to the pipe. The sending and checking of
this sequence number is activated at each end of the pipe.

When checking is activated and the received sequence number is unexpected, a signal will be
generated.

See also ArnPipe::setSendSeq(), ArnPipe::setCheckSeq(), ArnPipe::outOfSequence().
<Br><Br>

### Pipe anti congest ###    {#gen_pipeAntiCongest}
When the pipe is a [shared oject](#gen_shareArnobj), all assignment to the pipe is queued up
in a send queue. If there is a disconnect in the tcp/ip, an ArnServer will drop the send queue.
But in an ArnClient, this send queue will grow out of control if assignments to the pipe keeps
coming. This problem can also arise with a fast rate of status messages on a slow network.

One possibility is to keep track of the connection status, but this involves knowing about
which ArnClient (if many) to get status from. It also doesn't handle the problem with a slow
network.

A probably better way is to use the _Pipe anti congest_ logic. <Br>
We identify _messages_ that can be sent any number of times and are used to check the data flow,
resending, status and alike. Typically this can be _Heart beat_, _ping_, _request update_,
_current time_ etc.
These _async messages_ are assigned using ArnPipe::setValueOverwrite().

A regular expression is needed to identify "equal" _async messages_, that can be overwritten
in the send queue. If _async messages_ are repeatedly assigned to a pipe by
ArnPipe::setValueOverwrite(), the send queue will then not grow.

All other _messages_ will be normally assigned to the pipe. But these _messages_ will only be
assigned when normal data flow is present. Typically there is some expected _feedback message_
from the receiving part to block uncontrolled assignment from one side of the pipe.
<Br><Br>


Persistent Arn Data Objects    {#gen_persistArnobj}
---------------------------
The _server_ must use ArnPersist to support the persistance service. As a standard
_persist storage_, _Arn Data Objects_ are stored in a SQLite database. It's also possible to
store each object as a file.

The _mount point_ (path) for collecting the persistent _Arn Data objects_ is set by
ArnPersist::setMountPoint(). For server applications this is typically set to "/", which
makes all _Arn Data Objects_ potential persistent. In client applications the _mount point_
is typically restricted to Arn::pathLocal, which only saves local _Arn Data Objects_ in the
local _persist storage_.

Any connected _client_ or the _server_ can make an _Arn Data Object_ persistent.
It's just to open an ArnItem to the object and change _mode_ to _Save_.
> ArnItem  arnMaxLevel; <Br>
> arnMaxLevel.addMode( Arn::ObjectMode::Save); <Br>
> arnMaxLevel.open("//config/Level/Max/value"); <Br>

When the _Arn Data Object_ is set to _Save_ mode, it's automatically loaded by the
ArnPersist. At the _server_ this is instantly done. A _client_ has to wait for the value
to get synced from the _server_. It's convenient to use ArnDepend to get a signal
when the value is loaded and ready to use.

When the _Arn Data Object_ is changed, it will be automatically saved by ArnPersist.
There is a delay from first change of the object until the saving is done,
see ArnItem::setDelay(). This allows for intensive updates of the object without choking
down the server with saving operations.

It's possible to mark an object in the SQLite data base as _mandatory_. In this way the
_Arn Data Object_ is set as _persistent_ and gets loaded at start of ArnPersist.
<Br><Br>

### Saving objects in files ###    {#gen_fileArnobj}
To use the _persistent_ storing of _Arn Data Objects_ in files, the _root_ directory is
set by: ArnPersist::setPersistDir(). This can also be combined with support of VCS
(version control system).See ArnPersist::setVcs(). Currently there is a support module
for _git_.

In the _root_ directory and below, all (VCS) persistent files are stored.
The _root_ directory corresponds to the _root_ in Arn tree.

> Example: _root_ directory is set to "/usr/local/arn_persist". There > is a file stored
  at "/usr/local/arn_persist/@/doc/help.html". This > file will be mapped to Arn at
  "//doc/help.html".

Any files stored in the _root_ directory and below, get loaded into their _Arn Data Object_
with _mode_ set as _persistent_ at start of ArnPersist.

The files get updated in a similar way to the data base update. 
<Br><Br>


Sharing Arn Data Objects    {#gen_shareArnobj}
------------------------
A fundamental aspect of _Arn_ is that _Arn Data Objects_ can be shared. This is
centralized to the _Arn Server_, which stores all shared objects. It's still a
distributed model as each client and server has their own set of _Arn Data Objects_ that
operate independent of any connection.

Each _Arn Client_ connects to the _Arn Server_ and decides which part of the
_Arn Data Object_ tree to be shared. <Br>
`ArnClient::setMountPoint("/share/")` will make the tree"/share/" shared. <Br>
This doesn't mean that everything in the shared tree at the server now will be available
at the client. The client has to create an _Arn Data Object_ in the shared tree.
The client can then decide the exact objects of interest. <Br>
`ArnItem::Open("/share/Test/value")` will open a shared object (in previous example).

Note: Normally "//" is used for global (shared). See [naming conventions](#gen_naming).
<Br><Br>

### Dynamic port ###    {#gen_dynamicPort}
An ArnServer can be created with _port_ set to 0. This will be handled as a _dynamic port_
and the system will assign a free _port number_ to the server. The _port number_ will be
taken from a range specified by IANA.

This can typically be used to skip configuring static port numbers and be able to have
multiple instanses of the ArnServer on the same machine. As an ArnClient must find its
ArnServer, this can be used together with ArnDiscoverRemote / ArnDiscover.
<Br><Br>


RPC and SAPI    {#gen_rpc}
------------
ArnRpc is the basic functionality of RPC (Remote Procedure Call). ArnSapi implements SAPI
(Service Application Programming Interface) and is using ArnRpc as its base.
It's recommended to use ArnSapi which has a higher level model.

The SAPI works by a model which can be described as RPC by _remote signal slots_.
The _provider_ is usually assumed to wait for a _requester_ to initiate the session
and then react to different remote calls from the _requester_.
However, this is full duplex, so any side can make a remote call at any time.

A good example of the usage of SAPI is the _Arn Demo Chat_, which is included in the
source package of the _ArnLib_.

ArnRpc uses [pipes](\ref gen_pipeArnobj) to communicate. The _pipes_ can be monitored 
and receive test stimuli from the _Arn Browser_ program. The used
[protocol](#gen_rpcformat) is XString based and quite easy to handtype when common data
types are used. "$help" will give the syntax for the actual custom SAPI.

A SAPI is setup by deriving the ArnSapi class to a new class that defines the
_custom SAPI_. This custom-declared class is included at both the _provider_ and
_requester_ ends.
The _custom SAPI_ class by itself doesn't implement any _services_. It's merely a hub for
connections to _external signals and slots_.
The base ArnSapi class automatically transfers all _custom signal_ (SAPI) calls to the
remote connected ends, which also have the ArnSapi derived class and that emits the
transfered signal. See example in ArnSapi Detailed Description.

The provider connects the signals from custom SAPI that are prefixed with "pv_"
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
signals_ are _public_ and can be called by non-derived classes.
This makes it optional to use both _signal to signal_ connections or direct _signal_ calls
(emit), when issuing a RPC to the remote side.

The _service_ slot can get the emitting _custom SAPI_ object by using normal
QObject::sender() functionality.
<Br><Br>

### RPC and SAPI communication format ###    {#gen_rpcformat}
The RPC calling has a basic format as XString (see Arn::XStringMap). The most generic form
is seen below. The type mark _T_ is "t" for writeable types and "tb" for binary
(non writeable) types.
> _funcname  T_ = _type1_  a. _label1_ = _arg1_  _T_ = _type2_  a. _label2_ = _arg2_ ... <Br>
> Example: put t=QString a.id=level t=int a.value=123 <Br>
> For calling: put( QString("level"), 123)

Commonly used _types_ have a shorter form. The _types_ are:
> _int_, _uint_, _bool_, _double_, _bytes_ (QByteArray), _date_ (QDate), _time_ (QTime),
> _datetime_ (QDateTime), _list_ (QStringList), and _string_ (QString) as default.

This can be used in previous example: 
> put string.id=level int.value=123

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

The built-in call "$help" will give an automatically generated list of the present SAPI
with the syntax for each available service.
<Br><Br>


ZeroConfig    {#gen_zeroconf}
----------
For getting a basic understanding of ZeroConfig and further references to relevant
documentation, see: http://zeroconf.org/

_Arn ZeroConfig_ is the lowest level support for advertising and discovering services on
an local network. The implementation has very few dependences to the rest of the ArnLib.

_Arn ZeroConfig_ can use a built in implementation of Apple (R) _mDns_ / _DNS_SD_ that has no
further dependences to external libraries. For _mDns_ the low end system abstraction layer
has been written to use Qt for portability. The higher level _DNS_SD_ has wrappers written
to give a good c++ / Qt API.

It's also possible to use an external _DNS_SD_ library, like _Avahi_. This gives better
performance when many applications uses ZeroConfig on the same machine, as they share
cashing etc with a common daemon. However you have to deal with this external dependency.

_Arn ZeroConfig_ implementation has two parts. The ArnZeroConfRegister can be used to
advertise any _service_ given a _host address_ and a _port number_. The other part is the
ArnZeroConfBrowser / ArnZeroConfResolve / ArnZeroConfLookup. The browser is used to get a
realtime list of available _services_ on the network. The resolver takes a given _service_
and resolves it into its _host name_ and _port number_. Finally ArnZeroConfLookup takes
a given _host name_ and makes a DNS (mDNS) lookup to get its ip-address. Each of these
classes are stand alone and has to be combined with glue logic for the complete process.

A service has a _service type_, that preferably should be registered at IANA. Examples of
_service types_ are "http", "ftp" and "arn". This type is mandatory when advertising a
_service_. Also the _service_ must have a _service name_.
<Br><Br>

### Service name ###    {#gen_zeroconfServiceName}
_Service names_ can be any human readable id. It should be easy to understand, without
any cryptic coding. There should not be any attempts to make the _service name_ unique
as this is taken care of by the ZeroConfig system. It's common that the _service name_
can be modified by the end user. The default starting name could be some system or product
name. Example of _service name_: "My House Registry".
<Br><Br>

### Sub types ###    {#gen_zeroconfSubTypes}
_Services_ can also have _sub types_. These are identifiers that can be used to filter out
some sub group from a specific _service type_. All _services_ having the same _service type_
must still have some common protocol even if they belong to different _sub types_.
A _service_ can be advertised with many _sub types_, but browsing can only be filtered
with one _sub type_ or with no filter.
<Br><Br>

### Text record ###    {#gen_zeroconfTextRecord}
It's possible to add a _text record_ to a _service_. The format of this record is specified
by IANA. The purpose is to store properties by a _key_ / _value_ -pair. For convenience
this can be done with ArnZeroConfRegister::setTxtRecordMap() using an Arn::XStringMap.
<Br><Br>


Discover    {#gen_discover}
--------
_Arn Discover_ is the mid level support for advertising and discovering services on
an local network. This implementation is only for the "arn" _service type_ and is heavily
dependent on the ArnLib. The "arn" _service type_ is approved and registered by IANA.

_Arn Discover_ implementation has two parts. The ArnDiscoverAdvertise can be used to
advertise an Arn _service_ given a _host address_ and a _port number_. The other part is
the ArnDiscoverBrowser / ArnDiscoverResolver. The browser is used to get a realtime list
of available Arn _services_ on the network. The resolver is for taking a manual resolve
when a _service name_ is known in advance.

_Arn Discover_ is designed to minimize external glue logic as these classes do all the
common processing. Internally _Arn ZeroConfig_ is used, but focus is on solving Arn
specific needs in a powerful, yet flexible manner.

An _Arn service_ needs an ArnDiscover::Type and a [service name](\ref gen_zeroconfServiceName).
The ArnDiscover::Type sets up a coarse division of the applications into the _groups_
"server" and "client". The "client" typically only offer the service of ArnDiscoverRemote.

_Arn services_ can also have _groups_. These are identifiers that can be used to filter out
some sub group. An _Arn service_ can be advertised with many _groups_, but browsing can only be
filtered with one _group_ or with no filter.

It's possible to add a _custom property_ to an _Arn service_. This can be done with
ArnDiscoverAdvertise::setCustomProperties() using an Arn::XStringMap. The propertie has a
_key_ / _value_ -pair. The custom property are advised to have a _key_ starting with a
capital letter to avoid name collision with the system.
The added _groups_ will be set as properties with naming as "group0", "group1" ...

ArnDiscoverBrowser collects found Arn _services_. Each of these _services_ can automatically
be further examined. This is chosen by calling ArnDiscoverBrowserB::setDefaultStopState(),
which e.g. tells examination to stop after _host name_ has been found. The _service_ can
then manually be ordered for further examination by ArnDiscoverBrowserB::goTowardState(),
e.g. examination should now stop after _host ip_ is found.

All the information about a _service_ is stored in ArnDiscoverInfo. Found _services_ can
be accessed by index, id or _service name_. Increasing index, starting at 0, gives a list
of _services_ alfabetically sorted by _service name_. The index is kind of volatile and
should be used instantly, not be stored. The id gives a unique number for each service and
can be stored. However the _service_ given by the id might dissapear.
<Br><Br>


Discover remote    {#gen_discoverRemote}
---------------
_Arn Discover Remote_ is the highest level support for advertising and discovering services
on an local network. Its implementation is based on _Arn Discover_. The added functionality
is to have a remote control for both advertising an ArnServer and multiple ArnClient
connections. The remote control is done via _Arn Data Objects_ in [local path](#gen_localPath)
"Sys/Discover/".

_Arn Discover Remote_ has one main class, ArnDiscoverRemote which act as a central point.
The ArnDiscoverRemote class also takes an ArnServer and advertises it as a _service_. For
remote control the _service name_ is available at [local path](#gen_localPath)
"Sys/Discover/This/Service/value".

ArnDiscoverRemote can make an internal ArnServer, when there is no need to access the
ArnServer class. This is usually the case in an client application. The ArnServer is then
merely used to make the discover functionality remote controlled.

Remote controlled client connections can be added. Each ArnClient is handled by an
ArnDiscoverConnector instance, which is made by ArnDiscoverRemote::newConnector().
Connections can be added to ArnDiscoverConnector, both as a _direct host_ list and a
_discover host_.

The _discover host_ is indirerctly set, by adding an ArnDiscoverResolver to
ArnDiscoverConnector. A _service name_ can then be resolved into the _discover host_.

The two connection methods can coexist and as standard the _discover host_ has lower
priority number than _direct host_, i.e. _discover host_ is tried first.

The ArnDiscoverConnector is associated with an _id_, which should be chosen to describe the
client target or its purpose. It's not a host address or necessarily a specific host, as
there can be many possible addresses assigned to the ArnDiscoverConnector.

The _id_ will appear as an _Arn folder_ in [local path](#gen_localPath), e.g. when _id_ is
"WeatherData-XYZ" the folder path will be "Sys/Discover/Connect/WeatherData-XYZ/". The
folder and its sub folders will contain _Arn Data Objects_ to remote control the ArnClient.
For a more comprehensive description of these objects, see
[help discover description](@ref helpDiscDiscover).

In the above example, a _discover host_ can be remote controlled by setting the
_service name_ in [local path](#gen_localPath)
"Sys/Discover/Connect/WeatherData-XYZ/DiscoverHost/Service/value", e.g. to
"Region Weather XYZ".

Also in the above example, the first _direct host_ can be remote controlled by setting the
_host name_ in [local path](#gen_localPath)
"Sys/Discover/Connect/WeatherData-XYZ/DirectHosts/Host-0/value", e.g. to "localhost".

Normally it's wanted that any remote set values in the [local path](#gen_localPath) remains
after power cycling. This is supported by the [Arn persist system](#gen_persistArnobj).

Connecting via resolver uses the logic:
* If connection fails for a _discover host_, resolving is forced to be refreshed for the
  target _service name_. The Host for the _service name_ might have changed since last
  resolved and doing a refresh can get the new _discover host_.

* If connection continues to fail for a _discover host_, refreshing the resolv will have a
  blocking time to avoid spamming the net. Typically this time is 30 seconds, but it can be
  changed by ArnDiscoverConnector::setResolveRefreshTimeout().
<Br><Br>


Application notations    {#gen_appnote}
---------------------
* If any graphics are used, Gui must be included.

* If only using QImage, Windowing system can be off, like:
  QApplication a(argc, argv, false);
