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

#ifndef ARNMONEVENT_HPP
#define ARNMONEVENT_HPP

#include "ArnLib_global.hpp"
#include "MQFlags.hpp"
#include <QObject>


//! Arn Monitor Event
/*!
This is a singleton class.
 */
class ARNLIBSHARED_EXPORT ArnMonEvent : public QObject
{
    Q_OBJECT
public:
    //! Types of Arn monitor Events
    struct Type {
        enum E {
            //! Invalid
            None = 0,
            //! Newly created Arn object
            ItemCreated,
            //! Found a present Arn object
            ItemFound,
            //! Found a present Arn object
            ItemDeleted,

            //! Internal: start the Monitor
            MonitorStart,
            //! Internal: restart the Monitor
            MonitorReStart
        };
        MQ_DECLARE_ENUM( Type)
    };

    static ArnMonEvent&  instance();
    static Type  textToId( const QString& txt);
    static const char*  idToText( Type id);

private:
    struct TypeSlot {
        const char*  typeText;
        int  typeId;
    };

    /// Private constructor/destructor to keep this class singleton
    ArnMonEvent();
    ArnMonEvent( const ArnMonEvent&);
    ~ArnMonEvent();
    ArnMonEvent&  operator=( const ArnMonEvent&);

    static TypeSlot  _typeTab[];
};

#endif // ARNMONEVENT_HPP

