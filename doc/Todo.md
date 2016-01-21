ArnLib Todo
===========

Major
-----
* Script support for Sapi.
* ArnObject Link to other ArnObject (like in a filesystem).
* Add atomic operations in ArnItem for: "+=", "&=", "|=" and alike.
* General access system with privileges at ArnObject level.
* Add more examples.
* Add Function tests.
* Add more Unit tests.
* API to Sync ArnObjects with other protocols (e.g. JSON-based).
* API to Sync ArnObjects over other media (e.g. CAN).
* Javascript based ArnLib for Web-applications over WebSocket.
* Put ArnServer client sessions in "/Local/..." to be viewed and controlled (e.g kill).
* ArnBasicItem with no QObject, only inherited to give ArnEvent (QEvent). Small footprint!

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
* A flush mechanism for ArnPersist to force saving.
* Pimpl: Convert to d-pointer for making binary compatible library in the future.
* Started unit tests

Done in 2.3
-----------
* Added ArnReal to be either float or double.
* Fixed zero reference to be more robust when deleting Arn objects in threads.
* Changed ArnM::valueXXX to create none existent ArnObjects.
* In Signal Slot (and more) use "const Type&".
* QML with "files" as ArnObject and other integration with Arn.
* QML support for Sapi.
* ArnClient stored centraly with an id. Also accessible by the id.
* External engine can be assigned to ArnScript.
* ArnSapi default path, not needing path for the pipe.
* Persistent values can be flushed to storage on demand.
* Enums (and flags) using MQFlags can use toString and more.
* Unit test sub project with tests for enum text.
* ArnQmlMQt with MQtObject for non gui qml (like Item/QtObject).
