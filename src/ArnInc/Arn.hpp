// Copyright (C) 2010-2014 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these
// other libraries is subject to their respective license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE_LGPL.txt included in the packaging of this
// file. In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General Public
// License version 3.0 as published by the Free Software Foundation and appearing
// in the file LICENSE_GPL.txt included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and conditions
// contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
//

#ifndef ARN_HPP
#define ARN_HPP

#include "MQFlags.hpp"
#include <QString>

#define DATASTREAM_VER  QDataStream::Qt_4_6

#if defined( ARNREAL_FLOAT)
#  define ARNREAL   float
#else
#  define ARNREAL   double
#endif


namespace Arn {

    const quint16  defaultTcpPort = 2022;

    extern const QString  pathLocal;
    extern const QString  pathLocalSys;
    extern const QString  pathDiscover;
    extern const QString  pathDiscoverThis;
    extern const QString  pathDiscoverConnect;

//! Action when assigning same value to an ArnItem
struct SameValue {
    enum E {
        //! Assigning same value generates an update of the _Arn Data Object_
        Accept = 0,
        //! Assigning same value is ignored
        Ignore = 1,
        //! Assigning same value gives default action set in ArnM or ArnItem
        DefaultAction = -1
    };
    MQ_DECLARE_ENUM( SameValue)
};

//! Data type of an _Arn Data Object_
struct DataType {
    enum E {
        Null       = 0,
        Int        = 1,
        Double     = 2,
        Real       = 2,
        ByteArray  = 3,
        String     = 4,
        Variant    = 5
        // 16 and above is reserved by ArnItemB::ExportCode
    };
    MQ_DECLARE_ENUM( DataType)
};

//! General global mode of an _Arn Data Object_
struct ObjectMode {
    enum E {
        //! A two way object, typically for validation or pipe
        BiDir = 0x01,
        //! Implies _BiDir_ and all data is preserved as a stream
        Pipe  = 0x02,
        //! Data is persistent and will be saved
        Save  = 0x04
    };
    MQ_DECLARE_FLAGS( ObjectMode)
};

//! The client session sync mode of an _Arn Data Object_
struct ObjectSyncMode {  // This mode is sent with sync-command
    enum E {
        //! default
        Normal      = 0x000,
        //! Monitor of server object for client
        Monitor     = 0x001,
        //! The client is default generator of data
        Master      = 0x100,
        //! Destroy this _Arn Data Object_ when client (tcp/ip) closes
        AutoDestroy = 0x200
    };
    MQ_DECLARE_FLAGS( ObjectSyncMode)
};

//! Link flags when accessing an _Arn Data Object_
struct LinkFlags {
    enum E {
        Folder        = 0x01,
        CreateAllowed = 0x02,
        SilentError   = 0x04,
        Threaded      = 0x08
    };
    MQ_DECLARE_FLAGS( LinkFlags)
};

struct NameF {
    //! Selects a format for path or item name
    enum E {
        //! Empty not ok,  Path: Absolute  Item: FolderMark
        Default      = 0x00,
        //! Only on discrete names, no effect on path. "test/" ==> "test"
        NoFolderMark = 0x01,
        //! Path: "/@/test" ==> "//test", Item: "@" ==> ""
        EmptyOk      = 0x02,
        //! Only on path, no effect on discrete names. "/test/value" ==> "test/value"
        Relative     = 0x04
    };
    MQ_DECLARE_FLAGS( NameF)
};

struct Coding {
    enum E {
        //! No special coding, can be anything
        Binary  = 0x0000,
        //! Text coding, can be any character set
        Text    = 0x1000
    };
    MQ_DECLARE_FLAGS( Coding)
};

//! Convert a name to a specific format
/*! Name is a sub part from a _path_.
 *  Example: _name_ = "value/", nameF = NoFolderMark
 *  ==> return = "value"
 *  \param[in] name
 *  \param[in] nameF is the path naming format
 *  \return The converted _name_
 */
QString  convertName( const QString& name, Arn::NameF nameF = Arn::NameF());

//! Convert a path to a full absolute path
/*! Example: _path_ = "Measure/depth/value"
 *  ==> return = "/Local/Measure/depth/value"
 *  \param[in] path
 *  \return The converted _path_ full path
 */
QString  fullPath( const QString& path);

//! Test if _path_ is a _folder path_
/*! \param[in] path
 *  \retval true if _path_ is a _folder path_, i.e. ends with a "/".
 */
bool  isFolderPath( const QString& path);

//! Test if _path_ is a _provider path_
/*! [About Bidirectional Arn Data Objects](\ref gen_bidirArnobj)
 *  \param[in] path
 *  \retval true if _path_ is a _provider path_, i.e. ends with a "!".
 */
bool  isProviderPath( const QString& path);

//! The last part of a _path_
/*! Example: path = "//Measure/depth/value" ==> return = "value"
 *  \param[in] path
 *  \return The itemName, i.e. the last part of the path after last "/"
 */
QString  itemName( const QString& path);

//! Get substring for child from a path (posterityPath)
/*! _parentPath_ don't have to end with a "/", if missing it's added.
 *
 *  If _posterityPath_ not starts with _parentPath_, QString() is returned.
 *  Otherwise given the _posterityPath_ the child to _parentPath_ is returned.
 *
 *  Example 1: _posterityPath_ = "//Measure/depth/value",
 *  _parentPath_ = "//Measure/" ==> return = "//Measure/depth/"
 *
 *  Example 2: _posterityPath_ = "//Measure/depth/value",
 *  _parentPath_ = "//Measure/depth/" ==> return = //Measure/depth/value"
 *  \param[in] parentPath
 *  \param[in] posterityPath
 *  \return The _child path_
 */
QString  childPath( const QString& parentPath, const QString& posterityPath);

//! Change the base (start) of a path
/*! _oldBasePath_ and _newBasePath_ don't have to end with a "/", if missing it's added.
 *  If _path_ not starts with _oldBasePath_, _path_ is returned.
 *  Otherwise the path is returned with its base changed from _oldBasePath_ to _newBasePath_.
 *
 *  Example: _path_ = "//Measure/depth/value", _oldBasePath_ = "//Measure/",
 *  _newBasePath_ = "/Measure/Tmp/" ==> return = "/Measure/Tmp/depth/value"
 *  \param[in] oldBasePath
 *  \param[in] newBasePath
 *  \param[in] path
 *  \return The changed path
 */
QString  changeBasePath( const QString& oldBasePath, const QString& newBasePath, const QString& path);

//! Make a path from a parent and an item name
/*! _parentPath_ don't have to end with a "/", if missing it's added.
 *  Empty folder _itemName_ is allowed on returned path.
 *
 *  Example: _parentPath_ = "//Measure/depth/", _itemName_ = "value"
 *  ==> return = "//Measure/depth/value"
 *  \param[in] parentPath
 *  \param[in] itemName
 *  \return The _path_
 */
QString  makePath( const QString& parentPath, const QString& itemName);

//! Make a path from a parent and an additional relative path
/*! _parentPath_ don't have to end with a "/", if missing it's added.
 *
 *  Example: _parentPath_ = "//Measure/", _childRelPath_ = "depth/value"
 *  ==> return = "//Measure/depth/value"
 *  \param[in] parentPath
 *  \param[in] childRelPath
 *  \param[in] nameF is the path naming format
 *  \return The _path_
 *  \see convertPath()
 */
QString  addPath( const QString& parentPath, const QString& childRelPath,
                         Arn::NameF nameF = Arn::NameF::EmptyOk);

//! Convert a path to a specific format
/*! Example: _path_ = "//Measure/depth/value", nameF = Relative
 *  ==> return = "@/Measure/depth/value"
 *  \param[in] path
 *  \param[in] nameF is the path naming format
 *  \return The converted _path_
 */
QString  convertPath( const QString& path,
                             Arn::NameF nameF = Arn::NameF::EmptyOk);

//! Get the parent to a given _path_
/*! Example: _path_ = "//Measure/depth/value!"
 *  ==> return = "//Measure/depth/"
 *  \param[in] path
 *  \return The parent _path_
 */
QString  parentPath( const QString& path);

//! Get the bidirectional twin to a given _path_
/*! Example: _path_ = "//Measure/depth/value!"
 *  ==> return = "//Measure/depth/value"
 *  \param[in] path
 *  \return The twin _path_
 *  \see \ref gen_bidirArnobj
 */
QString  twinPath( const QString& path);

//! Get _provider path_ or _requester path_
/*! [About Bidirectional Arn Data Objects](\ref gen_bidirArnobj)
 *  \param[in] path to be converted
 *  \param[in] giveProviderPath choses between provider and requester path.
 *             false = requester path, default is true = provider path.
 *  \retval is _provider path_ or _requester path_
 *  \see twinPath()
 *  \see isProviderPath()
 */
QString  providerPath( const QString& path, bool giveProviderPath = true);

//! Make a combined host and info string, i.e. _HostWithInfo_
/*! This is typically used to pass some extra information about the host,
 *  but still be used for connection to the host.
 * 
 *  ArnClient and alike accepts such _HostWithInfo_ strings for connection.
 *  Hosts discovered using e.g. ArnDiscoverBrowser will be using the ip-address as host
 *  and the host name as info.  
 *  Example: _host_ = "192.168.1.1", _info_ = "myhost.local"
 *  ==> return = "192.168.1.1  [myhost.local]"
 *  \param[in] host the name or address of the host
 *  \param[in] info is corresponding info for the host
 *  \return The _HostWithInfo_ string
 *  \see hostFromHostWithInfo()
 *  \note As the format of the _HostWithInfo_ string can be changed in the future,
 *        allways use makeHostWithInfo() and hostFromHostWithInfo() for coding and decoding.
 */
QString  makeHostWithInfo( const QString& host, const QString& info);

//! Get the host from the _HostWithInfo_ string
/*! This is typically used to extract only the host part without information,
 *  to be used in e.g. QTcpSocket for connection to the host.
 * 
 *  Example: _hostWithInfo_ = "192.168.1.1  [myhost.local]"
 *  ==> return = "192.168.1.1"
 *  \param[in] hostWithInfo The _HostWithInfo_ string
 *  \return The name or address of the host
 *  \see makeHostWithInfo()
 *  \note As the format of the _HostWithInfo_ string can be changed in the future,
 *        allways use makeHostWithInfo() and hostFromHostWithInfo() for coding and decoding.
 */
QString  hostFromHostWithInfo( const QString& hostWithInfo);

#ifndef DOXYGEN_SKIP
QString  convertBaseName( const QString& name, Arn::NameF nameF);
#endif

}  // Arn::

MQ_DECLARE_OPERATORS_FOR_FLAGS( Arn::LinkFlags)
MQ_DECLARE_OPERATORS_FOR_FLAGS( Arn::ObjectMode)
MQ_DECLARE_OPERATORS_FOR_FLAGS( Arn::ObjectSyncMode)
MQ_DECLARE_OPERATORS_FOR_FLAGS( Arn::NameF)
MQ_DECLARE_OPERATORS_FOR_FLAGS( Arn::Coding)

#endif // ARN_HPP
