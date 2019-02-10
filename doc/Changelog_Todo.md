ArnLib Changelog / Todo
=======================

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

Minor
-----
* Optimize data transfer with minimal copying.
* Converter classes for ArnPipes to other streams (e.g UART, TCP etc).
* Addition to login a system to "pair" ArnServer and ArnClient.

Done in 3.1
-----------
* Added ArnAdaptItem. Can be used in threads without eventloop or even non Qt threads.
* Added ArnClient syncMode for different client sync methods.
* Now all Bidir Objects has no echo, this was true only for pipes before.
  The official value comes always from one provider. The requested value can be from many.
* Single objects has echo with better logic to avoid bad echoes that restores old values.
* Persistent values to client has more robust logic, especially for Master objects.
* Added ArnItem::setUncrossed(), will make it easier to build Arn Bridges etc.

Done in 3.0
-----------
* Delete ArnObject, but only local (remove any sync of it).
* ArnClient disconnect and close.
* Optimized memory consumption with pointers to different data in ArnLink.
* Minimized signal/slot:s in ArnLink by change to ArnEvent.
* Distributed deletion of folders.
* Distributed create of folder.
* ArnMonitor detects destructions of _Arn Objects_.
* Added setDelay in ArnItemQml, rework changed() and using timer events.
* Access system for Server/Client login with session level privilege.
* Allow read access to "freePaths" without login. Used to view for example licenses.
* Option for free nets, e.g. "localnet", that don't need login for full access.
* A flush mechanism for ArnPersist to force saving.
* Pimpl: Converted to d-pointer for making binary compatible library in the future.
* Started unit tests
* Optimized HandleData class with Null-state that can be this == 0.
* Made ArnObject (ArnLink) none QObject to save memory and independent on main-thread-create.
  New methods and data for parent() etc.
* Changed to ArnLink::toInt(bool* isOk = 0).
  To make ignoreSameValue work as expected for "" -> int=0 and similar. Same for all toXXX().
* Changed to ArnItem::toInt(bool* isOk = 0).
  To give application the possibility to detect data type conversion errors.
* ArnBasicItem with no QObject, only inherited to give ArnEvent (QEvent). Small footprint!
* ArnItemNet (Arn syncing item) inherited from ArnBasicItem for small footprint.
* ArnMonitor no dependendency to ArnItemNet, that can be in other thread.
* ArnItem none native data-types: uint, int64 & uint64.
* Put ArnServer client sessions in "/Local/..." to be viewed and controlled (e.g kill).
  Added ArnServerRemote class. Also chat between server (pipe in Arn) and client is supported.
* Browsing and controlling connected clients.
* Arn Registry metrics available in "/local/..."
* Added auto "humanize" logic to MQFlags text.
  This will convert e.g. enum value WriteDelay200Ms to "Write delay 200 ms".
* XStringMap improved, e.g. addNumber().

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
