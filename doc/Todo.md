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
* Add more Unit tests.

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
