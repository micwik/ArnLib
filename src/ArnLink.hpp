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

#ifndef ARNLINK_HPP
#define ARNLINK_HPP

#include "ArnInc/ArnLib_global.hpp"
#include "ArnInc/ArnLinkHandle.hpp"
#include "ArnInc/Arn.hpp"
#include "ArnInc/ArnBasicItem.hpp"
#include "ArnInc/MQFlags.hpp"
#include <QObject>
#include <QString>
#include <QVariant>
#include <QAtomicInt>
#include <QMutex>

struct ArnLinkValue;
class ArnEvent;
class ArnLink;

typedef QList<ArnLink*>  ArnLinkList;
typedef QList<ArnBasicItem*>  ArnBasicItemList;


//! \cond ADV
class ArnLink
{
    friend class ArnM;

public:
    struct RetireType {
        enum E {  // Warning limited number (ArnLink stores in bitfield)
            None,
            LeafLocal,
            LeafGlobal,
            Tree
        };
        MQ_DECLARE_ENUM( RetireType)
    };

    void  setValue( int value, int sendId = 0, bool forceKeep = 0);
    void  setValue( ARNREAL value, int sendId = 0, bool forceKeep = 0);
    void  setValue( const QString& value, int sendId = 0, bool forceKeep = 0,
                    const ArnLinkHandle& handleData = ArnLinkHandle::null());
    void  setValue( const QByteArray& value, int sendId = 0, bool forceKeep = 0,
                    const ArnLinkHandle& handleData = ArnLinkHandle::null());
    void  setValue( const QVariant& value, int sendId = 0, bool forceKeep = 0);

    int  toInt( bool* isOk = 0);
    ARNREAL  toReal( bool* isOk = 0);
    QString  toString( bool* isOk = 0);
    QByteArray  toByteArray( bool* isOk = 0);
    QVariant  toVariant( bool* isOk = 0);

    Arn::DataType  type();

    QString  linkPath( Arn::NameF nameF = Arn::NameF::EmptyOk);
    QString  linkName( Arn::NameF nameF = Arn::NameF());
    uint  linkId()  const;
    bool  isFolder();

    void  sendArnEvent( ArnEvent* ev);
    void  addSyncMode( Arn::ObjectSyncMode syncMode);
    Arn::ObjectSyncMode  syncMode();
    Arn::ObjectMode  getMode();
    bool  isBiDirMode();
    void  setPipeMode( bool isPipeMode, bool alsoSetTwin = true);
    bool  isPipeMode();
    void  setSaveMode( bool isSaveMode);
    bool  isSaveMode();
    bool  isProvider()  const;
    bool  isThreaded()  const;
    bool  isRetired();
    uint  retireType();
    ArnLink*  twinLink();
    ArnLink*  valueLink();
    ArnLink*  providerLink();
    ArnLink*  holderLink( bool forceKeep);
    QString  twinName();
    bool  subscribe( ArnBasicItem* subscriber);
    bool  unsubscribe( ArnBasicItem* subscriber);
    void  deref();
    int  refCount();
    ~ArnLink();

    QString  objectName()  const;
    ArnLink*  parent()  const;
    const ArnLinkList&  children()  const;

    QMutex*  getMutex()  const;

protected:
    //// Will never be inherited, this section is separated for use by friend ArnM
    ArnLink( ArnLink* parent, const QString& name, Arn::LinkFlags flags);
    void  setupEnd( const QString& path, Arn::ObjectSyncMode syncMode, Arn::LinkFlags flags);
    void  setParent( ArnLink* parent);
    void  doModeChanged();
    ArnLink*  findLink( const QString& name);
    void  ref();
    void  setRefCount( int count);
    void  decZeroRefs();
    bool  isLastZeroRef();
    void  setRetired( RetireType retireType);
    void  doRetired( ArnLink* startLink, bool isGlobal);
    void  setThreaded();
    void  lock();
    void  unlock();
    static QObject*  arnM( QObject* inArnM = 0);

    ArnLink*  _twin;   // Used for bidirectional functionality

private:
    void  resetHave();
    void  doValueChanged( int sendId, const QByteArray* valueData = 0,
                          const ArnLinkHandle& handleData = ArnLinkHandle::null());
    void  sendEventsInThread( ArnEvent* ev, const ArnBasicItemList& recipients);
    void  sendEventsDirRoot( ArnEvent* ev, ArnLink* startLink);
    void  sendEventArnM( ArnEvent* ev);

    // Source for unique id to all ArnLink ..
    static QAtomicInt  _idCount;

    QMutex*  _mutex;
    ArnLinkValue*  _val;
    ArnBasicItemList*  _subscribeTab;
    ArnLink*  _parent;
    QString  _objectName;
    ArnLinkList*  _children;

    quint32  _id;
    volatile qint32  _refCount;

    volatile qint16   _syncMode;

    volatile Arn::DataType  _type;
    volatile quint8  _zeroRefCount;

    bool  _hasBeenSetup : 1;
    bool  _isFolder : 1;
    bool  _isProvider : 1;

    volatile bool  _isPipeMode : 1;
    volatile bool  _isSaveMode : 1;

    volatile bool  _isRetired : 1;
    volatile uint  _retireType : 2;

    volatile bool  _haveInt : 1;
    volatile bool  _haveReal : 1;
    volatile bool  _haveString : 1;
    volatile bool  _haveByteArray : 1;
    volatile bool  _haveVariant : 1;
};
//! \endcond

#endif // ARNLINK_HPP
