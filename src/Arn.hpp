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

#ifndef ARN_HPP
#define ARN_HPP

#include "ArnLib.hpp"
#include "ArnLib_global.hpp"
#include "ArnError.hpp"
#include "ArnLink.hpp"
#include "ArnItem.hpp"
#include <QStringList>
#include <QVector>
#include <QObject>
#include <QMutex>
#include <QWaitCondition>

class ArnThreadComStorage;


class ArnThreadCom
{
    friend class ArnThreadComCaller;
    friend class ArnThreadComProxyLock;
public:
    ArnThreadCom() {
        _retObj = 0;
    }

    //// Payload of "return value" from proxy to caller
    QObject*  _retObj;
    QStringList  _retStringList;

private:
    static ArnThreadCom*  getThreadCom();

    static ArnThreadComStorage* _threadStorage;
    QWaitCondition  _commandEnd;
    QMutex  _mutex;
};

Q_DECLARE_METATYPE(ArnThreadCom*)


class ArnThreadComCaller
{
    ArnThreadCom*  _p;
public:
    ArnThreadComCaller();
    ~ArnThreadComCaller();
    void  waitCommandEnd();
    ArnThreadCom*  p()  {return _p;}
};


class ArnThreadComProxyLock
{
    ArnThreadCom*  _p;
public:
    ArnThreadComProxyLock( ArnThreadCom* threadCom);
    ~ArnThreadComProxyLock();
};



class ARNLIBSHARED_EXPORT Arn : public QObject
{
Q_OBJECT
    friend class ArnItem;

public:
    static Arn&  instance();
    static Arn&  getInstance()  {return instance();}  // For compatibility ... (do not use)
    static void  setConsoleError( bool isConsoleError);
    static void  setDefaultIgnoreSameValue( bool isIgnore = true);
    static bool  defaultIgnoreSameValue();
    static bool  isMainThread();
    static bool  isThreadedApp();
    static bool  isProviderPath( const QString& path);
    static QString  itemName( const QString& path);
    static QString  childPath( const QString& parentPath, const QString& posterityPath);
    static QString  makePath( const QString& parentPath, const QString& itemName);
    static QString  addPath( const QString& parentPath, const QString& childRelPath,
                             ArnLink::NameF nameF = ArnLink::NameF::EmptyOk);
    static QString  convertPath( const QString& path,
                                 ArnLink::NameF nameF = ArnLink::NameF::EmptyOk);
    static QString  twinPath( const QString& path);

    static int   valueInt( const QString& path);
    static double   valueDouble( const QString& path);
    static QString  valueString( const QString& path);
    static QByteArray  valueByteArray( const QString& path);
    static QVariant  valueVariant( const QString& path);

    static QStringList  items( const QString& path);
    static bool  exist(const QString& path);
    static bool  isFolder( const QString& path);
    static bool  isLeaf( const QString& path);

    static void  setValue( const QString& path, int value);
    static void  setValue( const QString& path, double value);
    static void  setValue( const QString& path, const QString& value);
    static void  setValue( const QString& path, const QByteArray& value);
    static void  setValue( const QString& path, const QVariant& value);

    static void  errorLog( QString errText, ArnError err = ArnError::Undef, void* reference = 0);
    static QString  errorSysName();
    static QByteArray  info();

public slots:
    static void  destroyLink( const QString& path);
    static void  setupErrorlog( QObject* errLog);

signals:
    void  errorLogSig( QString errText, uint errCode, void* reference);

protected:
    static ArnLink*  root();
    static ArnLink*  link( const QString& path, ArnLink::Flags flags,
                           ArnItem::SyncMode syncMode = ArnItem::SyncMode());
    static ArnLink*  link( ArnLink *parent, const QString& name, ArnLink::Flags flags,
                           ArnItem::SyncMode syncMode = ArnItem::SyncMode());
    static ArnLink*  addTwin( ArnLink* child, ArnItem::SyncMode syncMode = ArnItem::SyncMode(),
                              ArnLink::Flags flags = ArnLink::Flags());
    static void  destroyLink( ArnLink* link);
    static void  destroyLinkMain( ArnLink* link);

private slots:
    static void  linkProxy( ArnThreadCom* threadCom, const QString& path,
                            int flagValue, int syncMode = 0);
    static void  itemsProxy( ArnThreadCom* threadCom, const QString& path);
    static void  doZeroRefLink( QObject* obj = 0);

private:
    /// Private constructor/destructor to keep this class singleton
    Arn();
    Arn( const Arn&);
    ~Arn();
    Arn&  operator=( const Arn&);

    static ArnLink*  linkMain( const QString& path, ArnLink::Flags flags,
                               ArnItem::SyncMode syncMode = ArnItem::SyncMode());
    static ArnLink*  linkThread( const QString& path, ArnLink::Flags flags,
                                 ArnItem::SyncMode syncMode = ArnItem::SyncMode());
    static ArnLink*  linkMain( ArnLink *parent, const QString& name, ArnLink::Flags flags,
                               ArnItem::SyncMode syncMode = ArnItem::SyncMode());
    static ArnLink*  addTwinMain( ArnLink* child, ArnItem::SyncMode syncMode = ArnItem::SyncMode(),
                                  ArnLink::Flags flags = ArnLink::Flags::F(0));
    static ArnLink*  getRawLink( ArnLink *parent, const QString& name, ArnLink::Flags flags);
    static QStringList  itemsMain( const ArnLink *parent);
    static QStringList  itemsMain( const QString& path);

    // The root object of all other arn data
    ArnLink*  _root;

    QVector<QString>  _errTextTab;
    bool  _consoleError;
    bool  _defaultIgnoreSameValue;
    QObject*  _errorLogger;

    volatile bool  _isThreadedApp;
    QThread* _mainThread;
};

#endif // ARN_HPP

