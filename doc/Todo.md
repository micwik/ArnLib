ArnLib Todo
===========

Major
-----
* Convert to d-pointer for making binary compatible library in the future.
* Script support for Sapi.
* ArnObject Link to other ArnObject (like in a filesystem).
* Add atomic operations in ArnItem for: "+=", "&=", "|=" and alike.
* General access system with privileges at ArnObject level.
* Add more examples.
* Add Unit/Function tests.
* API to Sync ArnObjects with other protocols (e.g. JSON-based).
* API to Sync ArnObjects over other media (e.g. CAN).
* Javascript based ArnLib for Web-applications over WebSocket.
* Put ArnServer client sessions in "/Local/..." to be viewed and controlled (e.g kill).

Minor
-----
* ArnItemQml::updateValue() don't handle param data.
* Optimize data transfer with minimal copying.
* Make ArnLink none QObject to save memory and independent on main-thread-create.
* Add tranfer classes for copying values.
* Add multiplex/demultiplex-classes for pipes used by Sapi.
* Script with "include".
* Converter classes for ArnPipes to other streams (e.g UART, TCP etc).
* Browsing and controlling connected clients.
* ArnItem none native data-types like: uint, int64 etc.
* Addition to login a system to "pair" ArnServer and ArnClient.
* A flush mechanism for ArnPersist to force saving.

Done in 3.0
-----------
* Delete ArnObject, but only local (remove any sync of it).
* ArnClient disconnect and close.
* Optimize memory consumption with pointers to different data in ArnLink.
* Minimized signal/slot:s in ArnLink by change to ArnEvent.
* Distributed deletion of folders.
* Distributed create of folder.
* ArnMonitor detects destructions of _Arn Objects_.
* Add setDelay in ArnItemQml, rework changed() and using timer events.
* Access system for Server/Client login with session level privilege.
* Allow read access to "freePaths" without login. Used to view for example licenses.

Done in 2.3
-----------
* In Signal Slot (and more) use "const Type&".
* QML with "files" as ArnObject and other integration with Arn.
* QML support for Sapi.
* ArnClient stored centraly with an id. Also accessible by the id.
* External engine can be assigned to ArnScript.
