ArnLib Todo
===========

Major
-----
* Script support for Sapi.
* Convert to d-pointer for making binary compatible library in the future.
* Distributed deletion of folders
* Unit tests
* General access system
* Add more examples

Minor
-----
* In Signal Slot use "const Type&".
* Add setDelay in ArnItemQml, rework changed().
* ArnItemQml::updateValue() don't handle param data.
* Optimize data transfer with minimal copying.
* Optimize memory consumption with pointers to different data in ArnLink.
* Simple access system for Server/Client
* Add tranfer classes for copying values.
* Add multiplex/demultiplex-classes for pipes used by Sapi.

Done in 2.3
-----------
* QML with "files" as ArnObject and other integration with Arn.
* QML support for Sapi
* ArnClient stored centraly with an id. Also accessible by the id.
* External engine can be assigned to ArnScript
