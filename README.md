    Copyright (C) 2010-2013 Michael Wiklund.
    All rights reserved.
    Contact: arnlib@wiklunden.se

# ArnLib - Active Registry Network.

This Qt based library makes it easy to distribute changing data objects. It also gives a
central place to find all your systems current data. By using the ArnBrowser, all data
objects are real time presented in a tree view.

### Comparison to similar concepts

* **Data mart:** Statistical data gathered from different systems. This makes it possible
to run cross system analysis.

* **Windows Registry & AD:** Centralized configuration data. All in one place easily shared.

* **ArnLib:** Hot changing data from different systems. Enables easy cross system data
exchange, debugging, etc.
<Br><Br>


## Installation and usage

Read [doc/Install.md](\ref ins_build) how to build, install and use.

ArnLib could be beneficial in a lot of projects.
It should be well suited to the following conditions:

* _A lot of configurations and changing values._  <Br>
ArnLib helps giving out-of-the-box diagnostics and ability to change values not yet
available in the custom application user interface.

* _Hardware with a lot of sensors and controls._  <Br>
Arnlib helps giving a common interface and diagnostic.

* _Distributed systems._  <Br>
ArnLib helps giving an out-of-the-box data sharing system that replicates Arn objects.

* _Networked services by RPC (remote procedure call)._  <Br>
Will be quite the same as setting up slots for local calls. Also calling a remote slot
can be done via a signal or a direct call. You can find an easy example in the ArnLib
package, showing a simple chat Client and Server.

* _Customization with scripts._  <Br>
Helps giving integration of Java Script to C++ and objects stored in the Arn Registry.
<Br><Br>


## Main features

* Based on QT, multiple platform and OS support.
* QT based Arn browser available. Allows you to access all data objects in a tree view (see ArnBrowser).
* Web based Arn browser available, allowing you to use a standard web browser (see WebArnBrowser).

#### Arn Data Objects

* Hierarchical storage of "hot" changing data objects.

* _Arn Data objects_ can be: integers, floats, strings, byte arrays and variants
(most QT data types, e.g. QImage).

* Data objects can typically be: measures, settings, data streams, documents, scripts (js), ...

* _Arn Data objects_ are thread-safe.

* Native support for data validation and double direction pipes.

#### Sharing

* Data objects can be shared in a single program, among threads or between programs at
different computers. This division of program modules can be changed and is transparent
to usage of ArnLib.

* Support for temporary session data objects.
Optional auto-delete of objects when tcp/ip closes and unique uuid names.

* Dependency system with custom offered services and getting signals when all needed services
are available.

* Monitoring of newly created data objects and any mode change.

#### Persistent storage

* Optional persistent storage of object in SQLight or in a file.

* Support for version control (VCS) of objects stored in files.

#### Java Script

* Native support in JavaScript for: _Arn Data Objects_, Dependency system and
Monitoring of changed objects.

* Java Script jobstack with preemptive and cooperative scripts running at different priorities.

* Hot swap of changed Java Script in jobstack.

#### Data streams and _Remote Procedure Call_

* All data streams (pipes) can easily be monitored and manual test data can be inserted
(see ArnBrowser).

* Service Api, for calling routines anywhere in connected Arn.
_Remote Procedure call_ (RPC) made simple as "remote signal slots".

* Service Api has an automatically generated help for giving syntax when doing debug manual
typed calls to a RPC service.
