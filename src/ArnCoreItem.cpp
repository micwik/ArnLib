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

#include "ArnInc/ArnBasicItem.hpp"
#include "ArnInc/ArnAdaptItem.hpp"
#include "private/ArnBasicItem_p.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/ArnEvent.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnLink.hpp"
#include <QDataStream>
#include <QThreadStorage>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>


void  ArnCoreItem::init()
{
}


ArnCoreItem::ArnCoreItem()
    : d_ptr( new ArnBasicItemPrivate)
{
}


ArnCoreItem::ArnCoreItem( ArnBasicItemPrivate& dd)
    : d_ptr( &dd)
{
}


ArnCoreItem::~ArnCoreItem()
{
    delete d_ptr;
}


QThread*  ArnCoreItem::thread()  const
{
    Heritage heritage = d_ptr->heritage();
    if (heritage.is( Heritage::AdaptItem)) {
        return reinterpret_cast<const ArnAdaptItem*>(this)->thread();
    }
    else if (heritage.is( Heritage::BasicItem)) {
        return static_cast<const ArnBasicItem*>(this)->thread();
    }
    return 0;  // TODO:
}


void  ArnCoreItem::sendArnEventItem( ArnEvent* ev, bool isAlienThread, bool isLocked)
{
    Heritage heritage = d_ptr->heritage();
    if (heritage.is( Heritage::AdaptItem)) {
        return reinterpret_cast<ArnAdaptItem*>(this)->sendArnEventItem( ev, isAlienThread, isLocked);
    }
    else if (heritage.is( Heritage::BasicItem)) {
        return static_cast<ArnBasicItem*>(this)->sendArnEventItem( ev, isAlienThread, isLocked);
    }
}


void  ArnCoreItem::addHeritage( ArnCoreItem::Heritage heritage)
{
    d_ptr->addHeritage( heritage);
}
