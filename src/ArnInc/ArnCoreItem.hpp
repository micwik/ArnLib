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

#ifndef ARNCOREITEM_HPP
#define ARNCOREITEM_HPP

#include "ArnLib_global.hpp"
#include "Arn.hpp"
#include "MQFlags.hpp"

class ArnBasicItemPrivate;
class ArnEvent;
class QThread;


//! Core base class for the inherited ArnItem classes.
/*!
[About ArnItem access](\ref gen_arnItem)

See ArnItem.

ArnCoreItem is just a base class for ArnBasicItem and its inherited classes.
Its purpose is to have a core API for meta handling ArnItems without having
many virtual functions that icrease the memory footprint for especially ArnBasicItem.

It is the only real base class for all kinds of ArnItems.
*/
class ARNLIBSHARED_EXPORT ArnCoreItem
{
    Q_DECLARE_PRIVATE(ArnBasicItem)
    friend class ArnBasicItemEventHandler;

public:
    struct Heritage {
        //! The heritage track of this item
        enum E {
            BasicItem    = 0x01,
            ItemB        = 0x02,
            AdaptItem    = 0x04,

            None         = 0x00
        };
        MQ_DECLARE_FLAGS( Heritage)
    };

    //! Standard constructor of a closed handle
    /*!
     */
    ArnCoreItem();

    virtual  ~ArnCoreItem();

    //! Get the thread affinity of this ArnCoreItem
    /*! The definition of affinity is different for different ArnItem classes.
     *  The returned value should still indicate for the caller if the item is in
     *  another thread and then the caller should treat the item with isAlienThread=true.
     *
     *  \return the thread affinity
     *  \see setEventHandler()
     */
    QThread*  thread()  const;

    //! \cond ADV
    void  sendArnEventItem( ArnEvent* ev, bool isAlienThread, bool isLocked = false);

protected:
    virtual void  arnEvent( QEvent* ev, bool isAlienThread) = 0;

    //// Methods not to be public
    void addHeritage( ArnCoreItem::Heritage heritage);

    ArnCoreItem( ArnBasicItemPrivate& dd);
    ArnBasicItemPrivate* const  d_ptr;
    //! \endcond

private:
    void  init();
};

MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnCoreItem::Heritage)

#endif // ARNCOREITEM_HPP
