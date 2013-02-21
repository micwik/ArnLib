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

#ifndef ARNPERSIST_HPP
#define ARNPERSIST_HPP

#include "Arn.hpp"
#include "ArnLib_global.hpp"
#include <QMap>
#include <QList>
#include <QObject>

class ArnPersist;
class ArnPersistSapi;
class ArnDependOffer;
class QSqlDatabase;
class QSqlQuery;
class QDir;
class QStringList;


class ArnItemPersist : public ArnItem
{
    Q_OBJECT
public:
    struct StoreType {
        enum E {
            DataBase = 0,
            File
        };
        MQ_DECLARE_ENUM( StoreType)
    };
    explicit ArnItemPersist( ArnPersist* arnPersist);
    void  setStoreId( int id)  {_storeId = id;}
    int  storeId()  const {return _storeId;}
    void  setStoreType( StoreType type)  {_storeType = type;}
    StoreType  storeType()  const {return _storeType;}
    void  setMandatory( bool v)  {_isMandatory = v;}
    bool  isMandatory()  const {return _isMandatory;}

signals:

public slots:

private slots:

private:
    StoreType  _storeType;
    int  _storeId;
    bool  _isMandatory;
    ArnPersist*  _arnPersist;
};


class ARNLIBSHARED_EXPORT ArnVcs : public QObject
{
    Q_OBJECT
public:
    explicit ArnVcs( QObject* parent = 0);
    ~ArnVcs();

signals:
    void  notify( const QString msg);
    void  progress( int, QString msg=QString());

    void  userSettingsR( QString name, QString email);
    void  filesR( QStringList files);
    void  tagR();
    void  commitR();
    void  checkoutR();
    void  branchesR( QStringList branches);
    void  tagsR( QStringList tags);
    void  statusR( QString statusLines);
    void  diffR( QString diffLines);
    void  logRecordR( QString lines);
    void  logR( QStringList refs, QStringList msgs);

public slots:
    virtual void  log( int numLog);
    virtual void  files( QString ref);
    virtual void  commit( QString commitMsg, QStringList files, QString name, QString email);
    virtual void  tag( QString name, QString ref);
    virtual void  diff( QString ref, QStringList files);
    virtual void  logRecord( QString ref);
    virtual void  tags();
    virtual void  branches();
    virtual void  status();
    virtual void  userSettings();
    virtual void  setUserSettings( QString userName, QString userEmail);
    virtual void  checkout( QString ref, QStringList files);

private slots:

private:
};


class ARNLIBSHARED_EXPORT ArnPersist : public QObject
{
    Q_OBJECT
    ArnVcs*  _vcs;

public:
    explicit ArnPersist( QObject* parent = 0);
    ~ArnPersist();
    bool  setMountPoint( const QString& path);
    void  setPersistDir( const QString& path);
    void  setArchiveDir( const QString& path);
    void  setVcs( ArnVcs* vcs);
    bool  setupDataBase( QString dbName = "persist.db");
    bool  getDbId( QString path, int& storeId);
    bool  getDbValue( int storeId, QString& path, QByteArray& value);
    bool  getDbValue( QString path, QByteArray& value, int& storeId);
    bool  getDbMandatoryList( QList<int>& storeIdList);
    bool  getDbList( bool isUsed, QList<int>& storeIdList);
    bool  insertDbValue( QString path, QByteArray value);
    bool  updateDbValue( int storeId, QByteArray value);
    bool  updateDbUsed( int storeId, int isUsed);
    bool  updateDbMandatory( int storeId, int isMandatory);

public slots:
    bool  doArchive( QString name = QString());

private slots:
    void  sapiTest( QString str, int i=0);
    void  sapiLs( QString path);
    void  sapiLoad();
    void  sapiRm( QString path);
    void  sapiTouch( QString path);
    void  sapiDbMandatory( QString path, bool isMandatory);
    void  sapiDbMandatoryLs( QString path);
    void  sapiDbLs( QString path, bool isUsed = true);
    void  sapiInfo();

    void  vcsCheckoutR();

    void  doArnModeChanged( QString path, uint linkId, ArnItem::Mode mode);
    void  doArnUpdate();
    void  doArnDestroy();
    void  destroyRpc();

private:
    ArnItemPersist*  getPersistItem( QString path);
    ArnItemPersist*  setupMandatory( QString path, bool isMandatory);
    void  removeFilePersistItem( QString path);
    void  fileList( QStringList& flist, const QDir& dir, const QDir* baseDir = 0);
    void  loadFile( QString relPath);
    void  doLoadMandatory();
    void  doLoadFiles();
    void  setupSapi( ArnPersistSapi* sapi, QString pipePath);
    void  convertFileList( QStringList& files, ArnLink::NameF nameF);

    QDir*  _persistDir;
    QDir*  _archiveDir;
    ArnItem*  _arnMountPoint;
    ArnDependOffer* _depOffer;
    QMap<uint,ArnItemPersist*>  _itemPersistMap;
    QMap<QString,uint>  _pathPersistMap;
    QSqlDatabase*  _db;
    QSqlQuery*  _query;
    ArnPersistSapi*  _sapiCommon;
};

#endif // ARNPERSIST_HPP
