ArnLib Todo
===========

Major
-----
* Convert to d-pointer for making binary compatible library in the future.
* Distributed deletion of folders.
* Script support for Sapi.
* ArnObject Link to other ArnObject (like in a filesystem).
* Add atomic operations in ArnItem for: "+=", "&=", "|=" and alike.
* General access system.
* Add more examples.
* Add Unit tests.
* API to Sync ArnObjects with other protocols (e.g. JSON-based).
* API to Sync ArnObjects over other media (e.g. CAN).
* Javascript based ArnLib for Web-applications over WebSocket.

Minor
-----
* Delete ArnObject, but only local (remove any sync of it).
* Add setDelay in ArnItemQml, rework changed().
* ArnItemQml::updateValue() don't handle param data.
* Optimize data transfer with minimal copying.
* Optimize memory consumption with pointers to different data in ArnLink.
* Simple access system for Server/Client.
* Add tranfer classes for copying values.
* Add multiplex/demultiplex-classes for pipes used by Sapi.
* Script with "include".
* Converter classes for ArnPipes to other streams (e.g UART, TCP etc).

Done in 3.0
-----------

Done in 2.3
-----------
* In Signal Slot (and more) use "const Type&".
* QML with "files" as ArnObject and other integration with Arn.
* QML support for Sapi.
* ArnClient stored centraly with an id. Also accessible by the id.
* External engine can be assigned to ArnScript.
