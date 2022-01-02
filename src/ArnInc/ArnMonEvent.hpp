// Copyright (C) 2010-2022 Michael Wiklund.
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

#ifndef ARNMONEVENT_HPP
#define ARNMONEVENT_HPP

#include "ArnLib_global.hpp"
#include "MQFlags.hpp"


class ARNLIBSHARED_EXPORT ArnMonEventType {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        //! Invalid
        None = 0,
        //! Newly created Arn object
        ItemCreated,
        //! Found a present Arn object
        ItemFound,
        //! An Arn object was deleted
        ItemDeleted,
        //! An Arn object changed mode
        ItemModeChg,

        //! Internal: start the Monitor
        MonitorStart,
        //! Internal: restart the Monitor
        MonitorReStart
    };
    MQ_DECLARE_ENUMTXT( ArnMonEventType)

    enum NS {NsEnum, NsCom};
    MQ_DECLARE_ENUM_NSTXT(
        { NsCom, ItemCreated,    "itemCreated" },
        { NsCom, ItemFound,      "itemFound" },
        { NsCom, ItemDeleted,    "itemDeleted" },
        { NsCom, ItemModeChg,    "itemModeChg" },
        { NsCom, MonitorStart,   "monitorStart" },
        { NsCom, MonitorReStart, "monitorReStart" },
        { NsCom, MQ_NSTXT_FILL_MISSING_FROM( NsEnum) }
    )
};

#endif // ARNMONEVENT_HPP

