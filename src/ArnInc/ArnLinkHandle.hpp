// Copyright (C) 2010-2016 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt and/or other libraries that have their own
// licenses. Usage of these other libraries is subject to their respective
// license agreements.
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

#ifndef ARNLINKHANDLE_HPP
#define ARNLINKHANDLE_HPP

#include "Arn.hpp"
#include "MQFlags.hpp"
#include <QMetaType>
#include <QString>
#include <QVariant>
#include <QMap>


/// WARNING !!!  This class can have a this-pointer that is zero (0), for optimization.

//! \cond ADV
class ArnLinkHandle
{
public:
    //! Select how to handle a data assignment
    enum Code {
        //! Normal handling procedure
        Normal = 0,
        //! For pipes. If any item in the sendqueue matches Regexp, the item is replaced by
        //! this assignment. Typically used to avoid queue filling during a disconnected tcp.
        QueueFindRegexp = 0x01,
        //! For pipes. Sequence number is used and available in HandleData.
        SeqNo           = 0x02
    };
    Q_DECLARE_FLAGS( Codes, Code)

    struct Flags {
        enum E {
            //! Transitional temporary flag to indicate utf8-coded bytearray.
            Text        = 0x01,
            //! Data originates from a remote.
            FromRemote  = 0x02,
            //! Data originates from persistent loading.
            FromPersist = 0x04
        };
        MQ_DECLARE_FLAGS( Flags)
    };

    ArnLinkHandle();
    ArnLinkHandle( const ArnLinkHandle& other);
    ArnLinkHandle( const Flags& flags);
    ~ArnLinkHandle();
    static const ArnLinkHandle&  null();

    ArnLinkHandle&  add( Code code, const QVariant& valueRef);
    bool  has( Code code)  const;
    bool  isNull()  const;
    const QVariant&  valueRef( Code code)  const;

    const Flags&  flags()  const;
    Flags&  flags();

private:
    ArnLinkHandle&  operator=( const ArnLinkHandle&);  // Protect from usage
    void  init();

    Flags  _flags;
    Codes  _codes;
    typedef QMap<int,QVariant>  HandleData;
    HandleData*  _data;
};
//! \endcond

Q_DECLARE_OPERATORS_FOR_FLAGS( ArnLinkHandle::Codes)
MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnLinkHandle::Flags)
Q_DECLARE_METATYPE( ArnLinkHandle)

#endif // ARNLINKHANDLE_HPP
