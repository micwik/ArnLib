// Copyright (C) 2010-2013 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnLib - Active Registry Network.
// Parts of ArnLib depend on Qt 4 and/or other libraries that have their own
// licenses. ArnLib is independent of these licenses; however, use of these other
// libraries is subject to their respective license agreements.
//
// GNU Lesser General Public License Usage
// This file may be used under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation and
// appearing in the file LICENSE.LGPL included in the packaging of this file.
// In addition, as a special exception, you may use the rights described
// in the Nokia Qt LGPL Exception version 1.1, included in the file
// LGPL_EXCEPTION.txt in this package.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU General
// Public License version 3.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL included in the packaging of this file.
//
// Other Usage
// Alternatively, this file may be used in accordance with the terms and
// conditions contained in a signed written agreement between you and Michael Wiklund.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//

#ifndef ARNITEM_HPP
#define ARNITEM_HPP

#include "ArnLib_global.hpp"
#include "ArnError.hpp"
#include "ArnLink.hpp"
#include "MQFlags.hpp"
#include <QTextStream>
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVariant>
#include <QAtomicInt>

class QTimer;


class ARNLIBSHARED_EXPORT ArnItem : public QObject
{
    Q_OBJECT
    friend class ArnClient;
    friend class ArnSync;

public:
    struct Mode{
        enum E {
            BiDir = 0x01,
            Pipe  = 0x02,
            Save  = 0x04
        };
        MQ_DECLARE_FLAGS( Mode)
    };
    struct SyncMode{     // This mode is sent with sync-command
        enum E {
            Normal      = 0x000,
            Monitor     = 0x001,
            Master      = 0x100,
            AutoDestroy = 0x200
        };
        MQ_DECLARE_FLAGS( SyncMode)
    };

    ArnItem( QObject* parent = 0);
    ArnItem( const QString& path, QObject* parent = 0);
    ArnItem( const ArnItem& folder_template, const QString& itemName_path, QObject* parent = 0);
    virtual  ~ArnItem();

    bool  open( const QString& path);
    bool  openUuidPipe( const QString& path);
    bool  openFolder( const QString& path);
    void  close();
    void  destroyLink();
    bool  isOpen()  const;
    bool  isFolder()  const;
    bool  isBiDir()  const;
    ArnLink::Type  type()  const;
    QString  path( ArnLink::NameF nameF = ArnLink::NameF::EmptyOk)  const;
    QString  name( ArnLink::NameF nameF)  const;

    bool  isOnlyEcho()  const {return _isOnlyEcho;}
    void  setBlockEcho( bool blockEcho)  {_blockEcho = blockEcho;}
    void  setIgnoreSameValue( bool isIgnore = true);
    bool  isIgnoreSameValue();

    void  setReference( void* reference)  {_reference = reference;}
    void*  reference()  const {return _reference;}
    uint  itemId()  const {return _id;}
    uint  linkId()  const;
    void  addMode( Mode mode);
    Mode  getMode()  const;
    SyncMode  syncMode()  const;

    ArnItem&  setTemplate( bool isTemplate = true);
    bool  isTemplate()  const;
    ArnItem&  setBiDirMode();
    bool  isBiDirMode()  const;
    ArnItem&  setPipeMode();
    bool  isPipeMode()  const;
    ArnItem&  setSaveMode();
    bool  isSaveMode()  const;
    ArnItem&  setMaster();
    bool  isMaster()  const;
    ArnItem&  setAutoDestroy();
    bool  isAutoDestroy()  const;

    void  setDelay( int delay);
    void  arnImport( const QByteArray& data, int ignoreSame = -1);
    QByteArray  arnExport()  const;

    int  toInt()  const;
    double  toDouble()  const;
    bool  toBool()  const;
    QString  toString()  const;
    QByteArray  toByteArray()  const;
    QVariant  toVariant()  const;

    ArnItem&  operator=( const ArnItem& other);
    ArnItem&  operator=( int other);
    ArnItem&  operator=( double other);
    ArnItem&  operator=( const QString& other);
    ArnItem&  operator=( const QByteArray& other);
    ArnItem&  operator=( const QVariant& other);
    ArnItem&  operator=( const char* other);

public slots:
    void  setValue( int value, int ignoreSame = -1);
    void  setValue( double value, int ignoreSame = -1);
    void  setValue( bool value, int ignoreSame = -1);
    void  setValue( const QString& value, int ignoreSame = -1);
    void  setValue( const QByteArray& value, int ignoreSame = -1);
    void  setValue( const QVariant& value, int ignoreSame = -1);
    void  setValue( const char* value, int ignoreSame = -1);
    void  toggleBool();

signals:
    ///  changed(...) is using connectNotify & disconnectNotify
    ///  Must be updated if new types are added
    void  changed();
    void  changed( int value);
    void  changed( double value);
    void  changed( bool value);
    void  changed( QString value);
    void  changed( QByteArray value);
    void  changed( QVariant value);

    void  arnItemCreated( QString path);
    void  arnModeChanged( QString path, uint linkId, ArnItem::Mode mode);
    void  arnLinkDestroyed();

protected slots:
    virtual void  modeUpdate( bool isSetup = false);

protected:
    bool  open( const ArnItem& folder, const QString& itemName);
    void  setForceKeep( bool fk = true)  {_useForceKeep = fk;}
    Mode  getMode( ArnLink* link)  const;
    void  addSyncMode( SyncMode syncMode, bool linkShare);
    void  resetOnlyEcho()  {_isOnlyEcho = true;}
    virtual void  itemUpdateStart() {}
    virtual void  itemUpdateEnd();
    QStringList  childItemsMain()  const;
    void  errorLog( QString errText, ArnError err = ArnError::Undef, void* reference = 0);

    ArnLink*  _link;

private slots:
    void  linkValueUpdated( uint sendId);
    void  linkValueUpdated( uint sendId, QByteArray value);
    void  doItemUpdate();
    void  arnLinkCreatedBelow( ArnLink* link);
    void  arnModeChangedBelow( QString path, uint linkId);
    void  doArnLinkDestroyed();

private:
    void  init();
    void  setupOpenItem( bool isFolder);
    bool  open( const QString& path, bool isFolder);
    bool  open( const ArnItem& folder, const QString& itemName, bool isFolder);
    void  connectNotify( const char* signal);
    void  disconnectNotify( const char* signal);
    void  trfValue( QByteArray value, int sendId, bool forceKeep);

    /// Source for unique id to all ArnItem ..
    static QAtomicInt  _idCount;

    QTimer*  _delayTimer;

    int  _emitChanged;
    int  _emitChangedInt;
    int  _emitChangedDouble;
    int  _emitChangedBool;
    int  _emitChangedString;
    int  _emitChangedByteArray;
    int  _emitChangedVariant;

    SyncMode  _syncMode;
    Mode  _mode;
    bool  _syncModeLinkShare;
    bool  _useForceKeep;
    bool  _blockEcho;
    bool  _ignoreSameValue;
    bool  _isTemplate;
    bool  _isOnlyEcho;
    uint  _id;
    void*  _reference;
};

QTextStream&  operator<<(QTextStream& out, const ArnItem& item);

MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnItem::Mode)
MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnItem::SyncMode)

#endif // ARNITEM_HPP
