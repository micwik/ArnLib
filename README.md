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

* **Windows Registry:** Centralized configuration data. All in one place easily shared.

* **ArnLib:** Hot changing data from different systems. Enables easy cross system data
exchange, debugging, etc.
<Br><Br>


## Main features

* Based on QT, multiple platform and OS support.

#### Arn Data Objects

* Hierarchical storage of "hot" changing data objects.

* _Arn Data objects_ can be: integers, floats, strings, bytearrays and variants
(most QT data types, e.g. QImage).

* Data objects can typically be: measures, settings, datastreams, documents, scripts (js), ...

* _Arn Data objects_ are thread-safe.

* Native support for data validation and double direction pipes.

#### Sharing

* Data objects can be shared in a single program, among threads or between programs at
different computers. This division of program modules can be changed and are transparent
to usage of ArnLib.

* Support for temporary session data objects.
Optional auto-delete of objects when tcp/ip close and unique uuid names.

* Dependency system with custom offered services and getting signals when all needed services
are available.

* Monitoring of newly created data objects and any mode change.

#### Persistent storage

* Optional persistent storage of object in SQLight or in a file.

* Support for version control (VCS) of objects stored in files.

#### Java Script

* Native support in JavaScript for: _Arn Data Objects_, Dependency system and
Monitoring of changed objects.

* Java Script jobstack with preemtive and cooperative scripts running at different priorities.

* Hot swap of changed Java Script in jobstack.

#### Data streams and _Remote Procedure Call_

* All data streams (pipes) can easily be monitored and manual testdata can be inserted
(see ArnBrowser).

* Service Api, for calling routines anywhere in connected Arn.
_Remote Procedure call_ (RPC) made simple as "remote signal slots".

* Service Api has an automaticly generated help for giving syntax when doing debug manual
typed calls to a RPC service.
