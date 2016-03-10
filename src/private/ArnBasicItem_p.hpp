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

#ifndef ARNBASICITEM_P_HPP
#define ARNBASICITEM_P_HPP

// #define ARNITEMB_INCPATH

class QObject;
class ArnEvent;


class ArnBasicItemPrivate
{
    friend class ArnBasicItem;
public:
    ArnBasicItemPrivate();
    virtual ~ArnBasicItemPrivate();

private:
    /// Source for unique id to all ArnItem ..
    static QAtomicInt  _idCount;

    void*  _reference;
    QObject*  _eventHandler;
    ArnEvent*  _pendingEvChain;
#ifdef ARNITEMB_INCPATH
    QString _path;
#endif
    quint32  _id;
    Arn::ObjectSyncMode  _syncMode;
    Arn::ObjectMode  _mode;
    bool  _syncModeLinkShare : 1;
    bool  _useForceKeep : 1;
    bool  _ignoreSameValue : 1;
    bool  _isOnlyEcho : 1;
    bool  _isStdEvHandler : 1;
};

#endif // ARNBASICITEM_P_HPP

