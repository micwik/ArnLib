// Copyright (C) 2010-2014 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt 4 and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these other
// libraries is subject to their respective license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE.LGPL included in the packaging of this file.
// In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General
// Public License version 3.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and
// conditions contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//

#ifndef ARN_HPP
#define ARN_HPP

#include "MQFlags.hpp"

#define DATASTREAM_VER  QDataStream::Qt_4_6


namespace Arn {

    const quint16  defaultTcpPort = 2022;

    extern const QString  pathLocal;
    extern const QString  pathLocalSys;
    extern const QString  pathDiscoverThis;
    extern const QString  pathDiscoverConnect;

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

struct DataType {
    enum E {
        Null       = 0,
        Int        = 1,
        Double     = 2,
        ByteArray  = 3,
        String     = 4,
        Variant    = 5
        // 16 and above is reserved by ArnItemB::ExportCode
    };
    MQ_DECLARE_ENUM( DataType)
};

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
        //! Only on discrete names, no effect on path. "test/" ==> "test"
        NoFolderMark = 0x01,
        //! Path: "/@/test" ==> "//test", Item: "@" ==> ""
        EmptyOk      = 0x02,
        //! Only on path, no effect on discrete names. "/test/value" ==> "test/value"
        Relative     = 0x04
    };
    MQ_DECLARE_FLAGS( NameF)
};

QString  convertName( const QString& name, Arn::NameF nameF = Arn::NameF());
QString  convertBaseName( const QString& name, Arn::NameF nameF);
QString  fullPath( const QString& path);

//! Test if _path_ is a _folder path_
/*! \param[in] path.
 *  \retval true if _path_ is a _folder path_, i.e. ends with a "/".
 */
bool  isFolderPath( const QString& path);

//! Test if _path_ is a _provider path_
/*! [About Bidirectional Arn Data Objects](\ref gen_bidirArnobj)
 *  \param[in] path.
 *  \retval true if _path_ is a _provider path_, i.e. ends with a "!".
 */
bool  isProviderPath( const QString& path);

/*! \return The itemName, i.e. the last part of the path after last "/"
 */
QString  itemName( const QString& path);

//! Get substring for child from a path
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
//! Get the bidirectional twin to a given _path_
/*! Example: _path_ = "//Measure/depth/value!"
 *  ==> return = "//Measure/depth/value"
 *  \param[in] path
 *  \return The twin _path_
 *  \see \ref gen_bidirArnobj
 */
QString  twinPath( const QString& path);

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

}  // Arn::

MQ_DECLARE_OPERATORS_FOR_FLAGS( Arn::LinkFlags)
MQ_DECLARE_OPERATORS_FOR_FLAGS( Arn::NameF)

#endif // ARN_HPP
