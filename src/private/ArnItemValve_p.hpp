// Copyright (C) 2010-2015 Michael Wiklund.
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

#ifndef ARNITEMVALVE_P_HPP
#define ARNITEMVALVE_P_HPP

#include "ArnItemB_p.hpp"
#include "ArnInc/ArnItemValve.hpp"
#include <QPointer>


class ArnItemValvePrivate : public ArnItemBPrivate
{
    friend class ArnItemValve;
public:
    ArnItemValvePrivate();
    virtual ~ArnItemValvePrivate();

private:
    bool  _switchValue;
    ArnItemValve::SwitchMode  _switchMode;
    QPointer<ArnItemB>  _targetItem;
};

#endif // ARNITEMVALVE_P_HPP

