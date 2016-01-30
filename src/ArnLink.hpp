// Copyright (C) 2010-2014 Michael Wiklund.
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

#ifndef ARNLINK_HPP
#define ARNLINK_HPP

#include "ArnInc/ArnLib_global.hpp"
#include "ArnInc/ArnLinkHandle.hpp"
#include "ArnInc/Arn.hpp"
#include "ArnInc/MQFlags.hpp"
#include <QObject>
#include <QString>
#include <QVariant>
#include <QAtomicInt>
#include <QMutex>

struct ArnLinkValue;
class ArnEvent;


//! \cond ADV
class ArnLink : public QObject
{
    Q_OBJECT
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

    int  toInt();
    ARNREAL toReal();
    QString  toString();
    QByteArray  toByteArray();
    QVariant  toVariant();

    Arn::DataType  type();

    QString  linkPath( Arn::NameF nameF = Arn::NameF::EmptyOk);
    QString  linkName( Arn::NameF nameF = Arn::NameF());
    uint  linkId()  const;
    bool  isFolder();

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
    bool  subscribe( QObject* subscriber);
    bool  unsubscribe( QObject* subscriber);
    void  deref( QObject* subscriber = 0);
    ~ArnLink();

protected:
    //// Will never be inherited, this section is separated for use by friend ArnM
    ArnLink( ArnLink* parent, const QString& name, Arn::LinkFlags flags);
    void  setupEnd( const QString& path, Arn::ObjectSyncMode syncMode, Arn::LinkFlags flags);
    void  doModeChanged();
    ArnLink*  findLink( const QString& name);
    void  ref();
    int  refCount();
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
    void  emitChanged( int sendId, const QByteArray* valueData = 0,
                       const ArnLinkHandle& handleData = ArnLinkHandle::null());
    void  sendEventsInThread( ArnEvent* ev, const QObjectList& recipients);
    void  sendEvents( ArnEvent* ev);
    void  sendEventsDirRoot( ArnEvent* ev, ArnLink* startLink);
    void  sendEventArnM( ArnEvent* ev);

    // Source for unique id to all ArnLink ..
    static QAtomicInt  _idCount;

    QMutex*  _mutex;
    ArnLinkValue*  _val;
    QObjectList*  _subscribeTab;

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
