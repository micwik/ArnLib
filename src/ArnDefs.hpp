// Copyright (C) 2010-2013 Michael Wiklund.
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

#ifndef ARNDEFS_HPP
#define ARNDEFS_HPP

#include "MQFlags.hpp"

#define DATASTREAM_VER  QDataStream::Qt_4_6


namespace Arn {
    const quint16  defaultTcpPort = 2022;

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
}

MQ_DECLARE_OPERATORS_FOR_FLAGS( Arn::NameF)

#endif // ARNDEFS_HPP
