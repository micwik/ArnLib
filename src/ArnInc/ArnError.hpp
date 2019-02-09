// Copyright (C) 2010-2019 Michael Wiklund.
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

#ifndef ARNERROR_HPP
#define ARNERROR_HPP

#include "MQFlags.hpp"


class ArnError
{
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E  {
        Ok          = 0,
        Info        = 1,
        Warning     = 2,
        Undef       = 15,
        Err_Undef   = 15,  // MW: TODO To be removed
        CreateError = 16,
        Err_Custom  = 16,  // MW: TODO To be removed
        NotFound,
        NotOpen,
        AlreadyExist,
        AlreadyOpen,
        Retired,
        NotMainThread,
        FolderNotOpen,
        ItemNotOpen,
        ItemNotSet,
        ConnectionError,
        RecUnknown,
        ScriptError,
        RpcInvokeError,
        RpcReceiveError,
        LoginBad,
        RecNotExpected,
        OpNotAllowed,
        Err_N   // Last (number of error codes)
    };
    MQ_DECLARE_ENUMTXT( ArnError)

    struct StdCode  // MW: TODO To be removed ?
    {
        enum E  {
            Ok         = 0,
            Info       = 1,
            Warning    = 2,
            Err_Undef  = 15,
            Err_Custom = 16
        };
    };
};

#endif // ARNERROR_HPP
