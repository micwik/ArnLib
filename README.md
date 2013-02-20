    Copyright (C) 2010-2013 Michael Wiklund.
    All rights reserved.
    Contact: arnlib@wiklunden.se

# ArnLib - Active Registry Network.

This Qt based library makes it easy to distribute changing data objects. It also gives a central place to find all your systems current data. By using the ArnBrowser, all data objects are real time presented in a tree view.

### Comparison to similar concepts

* **Data mart:** Statistical data gathered from different systems. This makes it possible to run cross system analysis.

* **Windows Registry:** Centralized configuration data. All in one place easily shared.

* **ArnLib:** Hot changing data from different systems. Enables easy cross system data exchange, debugging etc.


### Main features

* Based on QT, multiple plattform and OS support.

* Hierchical storage of "hot" changing data objects.

* Data objects can be: integers, floats, strings, bytearrays and variants (most QT types, ex picture).

* Data objects can be: measures, settings, datastreams, documents, scripts (js), ...

* Native support for data validation and double direction pipes.

* Optional persistent storage of object in SQLight or in a file.

* Support for version control of objects stored in files.

* Native support for JavaScript, jobstack of JS with preemtive and 
cooperative script running.

* Support for managing changed script restart in jobstack.

* Service Api, for calling routines over tcp/ip. Just as remote signal slots.

* All data streams can easily be monitored and manual testdata can be inserted (see ArnBrowser).

* ...
  To be Continued
