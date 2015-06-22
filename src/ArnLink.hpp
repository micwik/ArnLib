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


//! \cond ADV
class ArnLink : public QObject
{
    Q_OBJECT
    friend class ArnM;

public:
    void  setValue( int value, int sendId = 0, bool forceKeep = 0);
    void  setValue( ARNREAL value, int sendId = 0, bool forceKeep = 0);
    void  setValue( const QString& value, int sendId = 0, bool forceKeep = 0,
                    const ArnLinkHandle& handleData = ArnLinkHandle());
    void  setValue( const QByteArray& value, int sendId = 0, bool forceKeep = 0,
                    const ArnLinkHandle& handleData = ArnLinkHandle());
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
    bool  isBiDirMode();
    void  setPipeMode( bool isPipeMode, bool alsoSetTwin = true);
    bool  isPipeMode();
    void  setSaveMode( bool isSaveMode);
    bool  isSaveMode();
    bool  isProvider()  const;
    bool  isThreaded()  const;
    bool  isRetired();
    ArnLink*  twinLink();
    ArnLink*  valueLink();
    ArnLink*  providerLink();
    ArnLink*  holderLink( bool forceKeep);
    QString  twinName();
    void  deref();
    ~ArnLink();

public slots:
    void  trfValue( const QByteArray& value, int sendId, bool forceKeep, ArnLinkHandle handleData);

signals:
    void  changed( uint sendId, const ArnLinkHandle& handleData);
    void  changed( uint sendId, const QByteArray& value, const ArnLinkHandle& handleData);
    void  modeChanged( const QString& path, uint linkId);
    void  modeChangedBelow( const QString& path, uint linkId);
    void  linkCreatedBelow( ArnLink* link);
    void  zeroRef();
    void  retired();

protected:
    ArnLink( ArnLink* parent, const QString& name, Arn::LinkFlags flags);
    void  setupEnd( Arn::ObjectSyncMode syncMode);
    ArnLink*  findLink( const QString& name);
    void  ref();
    int  refCount();
    void  setRefCount( int count);
    void  setRetired();

private:
    void  resetHave();
    void  emitChanged( int sendId, const ArnLinkHandle& handleData = ArnLinkHandle());

    // Source for unique id to all ArnLink ..
    static QAtomicInt  _idCount;

    uint  _id;
    bool  _isFolder;
    bool  _isProvider;
    bool  _isThreaded;
    QMutex  _mutex;
    QAtomicInt  _refCount;
    volatile bool  _isRetired;

    volatile int  _valueInt;
    volatile ARNREAL  _valueReal;
    QString  _valueString;
    QByteArray  _valueByteArray;
    QVariant  _valueVariant;

    volatile bool  _haveInt;
    volatile bool  _haveReal;
    volatile bool  _haveString;
    volatile bool  _haveByteArray;
    volatile bool  _haveVariant;

    volatile Arn::DataType  _type;

    ArnLink*  _twin;   // Used for bidirectional functionality
    volatile int   _syncMode;
    volatile bool  _isPipeMode;
    volatile bool  _isSaveMode;
    bool  _hasBeenSetup;
};
//! \endcond

#endif // ARNLINK_HPP
