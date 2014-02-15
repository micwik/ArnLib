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

#include "ArnLib_global.hpp"
#include "ArnM.hpp"
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

namespace Arn {
class XStringMap;
}


//! \cond ADV
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
//! \endcond


//! Class for handling persistent _Arn Data object_.
/*!
[About Persistent Arn Data Object](\ref gen_persistArnobj)

This class is used at an _ArnServer_ to implemennt persistent objects.

<b>Example usage</b> \n \code
    // In class declare
    ArnPersist  *_persist;
    VcsGit  *_git;

    // In class code
    _persist = new ArnPersist( this);
    _persist->setupDataBase("persist.db");
    _persist->setArchiveDir("archive");  // Use this directory for backup
    _persist->setPersistDir("persist");  // use this directory for VCS persist files
    _persist->setMountPoint("/");
    _persist->setVcs( _git);
\endcode
*/
class ARNLIBSHARED_EXPORT ArnPersist : public QObject
{
    Q_OBJECT
    ArnVcs*  _vcs;

public:
    explicit ArnPersist( QObject* parent = 0);
    ~ArnPersist();
    //! Set the persistent enabled tree path
    /*! Mountpoint is a folder. When an _Arn Data Object_ change to _Save_ mode in this
     *  folder or anywhere below in the tree, it will be treated as a persistent object.
     *  \param[in] path is the persistent enabled tree.
     *  \retval false if error.
     *  \see \ref gen_persistArnobj
     */
    bool  setMountPoint( const QString& path);

    //! Set the VCS persistent file directory _root_
    /*! In this directory and below, all VCS persistent files are stored.
     *  The _path_ correspond to the _root_ in Arn.
     *
     *  Example: _path_ is set to "/usr/local/arn_persist". There is a file stored at
     *  "/usr/local/arn_persist/@/doc/help.html". This file will be mapped to Arn at
     *  "//doc/help.html".
     *  \param[in] path is the persistent file directory _root_.
     *  \see setVcs()
     *  \see \ref gen_persistArnobj
     */
    void  setPersistDir( const QString& path);

    //! Set the persistent database backup directory
    /*! In this directory, all backup files are stored.
     *  \param[in] path is the persistent file directory _root_.
     *  \see doArchive()
     *  \see \ref gen_persistArnobj
     */
    void  setArchiveDir( const QString& path);

    //! Set the _Version Control System_ to be used
    /*! The VCS is implemented in a class derived from ArnVcs.
     *  \param[in] vcs is the class implementing the VCS.
     *  \see setPersistDir()
     *  \see \ref gen_persistArnobj
     */
    void  setVcs( ArnVcs* vcs);

    //! Setup the persistent database
    /*! Starting a SQLite database to store persistent _Arn Data Object_ in.
     *  \param[in] dbName is the name (and path) of the SQLite database file.
     *  \see \ref gen_persistArnobj
     */
    bool  setupDataBase( QString dbName = "persist.db");

    //! \cond ADV
    QString  metaDbValue( const QString& attr, const QString& def = QString());
    bool  setMetaDbValue( const QString& attr, const QString& value);
    bool  getDbId( QString path, int& storeId);
    bool  getDbValue( int storeId, QString& path, QByteArray& value);
    bool  getDbValue( QString path, QByteArray& value, int& storeId);
    bool  getDbMandatoryList( QList<int>& storeIdList);
    bool  getDbList( bool isUsed, QList<int>& storeIdList);
    bool  insertDbValue( QString path, QByteArray value);
    bool  updateDbValue( int storeId, QByteArray value);
    bool  updateDbUsed( int storeId, int isUsed);
    bool  updateDbMandatory( int storeId, int isMandatory);
    //! \endcond

public slots:
    //! Do a persistent database backup
    /*! By default the backup file will be marked by date and clock. Optionally a
     *  custom name can be set for the backup file.
     *  \param[in] name is the file name of the backup. QString() is default name.
     *  \see setArchiveDir()
     */
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
    void  sapiDbMarkUnused( QString path);
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
    void  convertFileList( QStringList& files, Arn::NameF nameF);
    void  dbSetupReadValue( const QString& meta, const QString& valueTxt,
                            QByteArray& value);
    void  dbSetupWriteValue( QString& meta, QString& valueTxt, QByteArray& value);

    QDir*  _persistDir;
    QDir*  _archiveDir;
    ArnItem*  _arnMountPoint;
    ArnDependOffer* _depOffer;
    QMap<uint,ArnItemPersist*>  _itemPersistMap;
    QMap<QString,uint>  _pathPersistMap;
    QSqlDatabase*  _db;
    QSqlQuery*  _query;
    ArnPersistSapi*  _sapiCommon;
    Arn::XStringMap*  _xsm;
};

#endif // ARNPERSIST_HPP
