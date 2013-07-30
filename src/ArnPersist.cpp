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

#include "ArnPersist.hpp"
#include "ArnPersistSapi.hpp"
#include "ArnDepend.hpp"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QRegExp>
#include <QStringList>
#include <QDebug>

#include <QMetaObject>
#include <QMetaMethod>


ArnItemPersist::ArnItemPersist( ArnPersist* arnPersist) :
    ArnItem( arnPersist)
{
    setForceKeep();  // Only normal value will be used for Load/Save
    setBlockEcho( true);  // Don't save own updates
    setIgnoreSameValue( true);
    _arnPersist  = arnPersist;
    _storeId     = 0;
    _isMandatory = false;
    _storeType   = StoreType::DataBase;
}


ArnVcs::ArnVcs( QObject* parent) :
    QObject( parent)
{
}


ArnVcs::~ArnVcs()
{
}


void  ArnVcs::log( int numLog)
{
    Q_UNUSED(numLog)
}


void  ArnVcs::files( QString ref)
{
    Q_UNUSED(ref)
}


void  ArnVcs::commit( QString commitMsg, QStringList files, QString name, QString email)
{
    Q_UNUSED(commitMsg)
    Q_UNUSED(files)
    Q_UNUSED(name)
    Q_UNUSED(email)
}


void  ArnVcs::tag( QString name, QString ref)
{
    Q_UNUSED(name)
    Q_UNUSED(ref)
}


void  ArnVcs::diff( QString ref, QStringList files)
{
    Q_UNUSED(ref)
    Q_UNUSED(files)
}


void  ArnVcs::logRecord( QString ref)
{
    Q_UNUSED(ref)
}


void  ArnVcs::tags()
{
}


void  ArnVcs::branches()
{
}


void  ArnVcs::status()
{
}


void  ArnVcs::userSettings()
{
}


void  ArnVcs::setUserSettings( QString userName, QString userEmail)
{
    Q_UNUSED(userName)
    Q_UNUSED(userEmail)
}


void  ArnVcs::checkout( QString ref, QStringList files)
{
    Q_UNUSED(ref)
    Q_UNUSED(files)
}


ArnPersist::ArnPersist( QObject* parent) :
    QObject( parent)
{
    _arnMountPoint = 0;
    _db         = new QSqlDatabase;
    _archiveDir = new QDir("archive");
    _persistDir = new QDir("persist");
    _query      = 0;
    _depOffer   = 0;
    _sapiCommon = new ArnPersistSapi( this);
    _vcs        = new ArnVcs( this);

    setupSapi( _sapiCommon, "//.sys/Persist/Pipes/CommonPipe!");
}


ArnPersist::~ArnPersist()
{
    delete _db;
    if (_query)  delete _query;
}


void  ArnPersist::setPersistDir(const QString &path)
{
    _persistDir->setPath( path);
}


void  ArnPersist::setArchiveDir(const QString &path)
{
    _archiveDir->setPath( path);
}


void  ArnPersist::setVcs( ArnVcs* vcs)
{
    if (_vcs)
        delete _vcs;
    _vcs = vcs;
    _vcs->setParent( this);

    connect( _vcs, SIGNAL(checkoutR()), this, SLOT(vcsCheckoutR()));
    ArnRpc::Mode  mode;
    _sapiCommon->batchConnect( _vcs, QRegExp("(.+)"), "rq_vcs\\1");
    _sapiCommon->batchConnect( QRegExp("^pv_vcs(.+)"), _vcs, "\\1");
}


ArnItemPersist*  ArnPersist::getPersistItem( QString path)
{
    ArnItemPersist*  item = new ArnItemPersist( this);
    item->open( path);
    uint  linkId = item->linkId();

    if (_itemPersistMap.contains( linkId)) { // Already as persist
        qDebug() << "getPersistItem already persist: path=" << path << " link=" << linkId;
        delete item;
        item = _itemPersistMap.value( linkId);

        return item;
    }

    item->setIgnoreSameValue( false);
    item->setDelay( 5000);

    connect( item, SIGNAL(changed()), this, SLOT(doArnUpdate()));
    connect( item, SIGNAL(arnLinkDestroyed()), this, SLOT(doArnDestroy()));
    _itemPersistMap.insert( linkId, item);

    return item;
}


ArnItemPersist*  ArnPersist::setupMandatory( QString path, bool isMandatory)
{
    ArnItemPersist::StoreType  st;
    ArnItemPersist*  item = 0;

    if (isMandatory) {
        item = getPersistItem( path);
        item->setMandatory(true);
        if ((item->storeType() == st.DataBase) && !_pathPersistMap.contains( path)) {
            _pathPersistMap.insert( path, item->linkId());
            item->addMode( ArnItem::Mode::Save);
        }
    }
    else {
        uint  linkId = _pathPersistMap.value( path);
        if (!linkId)  return 0;

        item = _itemPersistMap.value( linkId);
        Q_ASSERT(item);
        item->setMandatory(false);
        if (item->storeType() != st.File) {  // Not persist file
            _pathPersistMap.remove( path);  // Map only needed for mandatory & file
            if (!item->getMode().is( ArnItem::Mode::Save)) {
                _itemPersistMap.remove( linkId);
                delete item;
            }
        }
    }
    return item;
}


void  ArnPersist::removeFilePersistItem( QString path)
{
    uint  linkId = _pathPersistMap.value( path);
    if (!linkId)  return;  // path not found

    ArnItemPersist*  item = _itemPersistMap.value( linkId);
    Q_ASSERT(item);
    ArnItemPersist::StoreType  st;
    if (item->storeType() != st.File)  return;  // Not persist file

    item->setStoreType( st.DataBase);
    setupMandatory( path, item->isMandatory());
}


void  ArnPersist::doArnDestroy()
{
    ArnItemPersist*  item = qobject_cast<ArnItemPersist*>( sender());
    if (!item)  return;  // No valid sender

    qDebug() << "Persist destroyArn: path=" << item->path();
    _itemPersistMap.remove( item->linkId());
    _pathPersistMap.remove( item->path());
    item->deleteLater();
}


void  ArnPersist::doArnModeChanged( QString path, uint linkId, ArnItem::Mode mode)
{
    qDebug() << "Persist modeChanged: path=" << path << " mode=" << mode.f;
    if (!mode.is( mode.Save))  return;  // Not save-mode
    if (_itemPersistMap.contains( linkId))  return;  // Already in map
    qDebug() << "Persist modeChanged New: path=" << path << " mode=" << mode.f;

    ArnItemPersist*  item = getPersistItem( path);
    QByteArray  data;
    int  storeId;
    if (getDbValue( path, data, storeId)) {  // path exist in db, load data
        qDebug() << "Persist modeChanged dbRead: path=" << path;
        item->arnImport( data, false);
        item->setStoreId( storeId);
    }
    else {  // path is new in db, create it with null data
        qDebug() << "Persist modeChanged dbInsert: path=" << path;
        insertDbValue( path, QByteArray());
        if (getDbId( path, storeId)) {
            item->setStoreId( storeId);
        }
        item->setValue( QByteArray(), false);  // Do a null update, to signal update done
    }
}


void  ArnPersist::doArnUpdate()
{
    ArnItemPersist*  item;
    item = qobject_cast<ArnItemPersist*>( sender());
    if (!item) {
        ArnM::errorLog( QString(tr("Can't get ArnItemPersist sender for doArnUpdate")),
                            ArnError::Undef);
        return;
    }

    switch (item->storeType()) {
    case ArnItemPersist::StoreType::DataBase:
    {
        int storeId = item->storeId();
        if (storeId) {
            updateDbValue( storeId, item->arnExport());
        }
        else {
            ArnM::errorLog( QString(tr("Can't get persist storeId for doArnUpdate: path=")) +
                                           item->path(), ArnError::Undef);
        }
        break;
    }
    case ArnItemPersist::StoreType::File:
    {
        QString  relPath = item->path( ArnLink::NameF::Relative);
        qDebug() << "Persist arnUpdate: Save to relPath=" << relPath;
        QFile  file( _persistDir->absoluteFilePath( relPath));
        file.open( QIODevice::WriteOnly);
        QByteArray  data = item->toByteArray();
        file.write( data);
        // qDebug() << "Persist arnUpdate: Save to file=" << file.fileName();
        break;
    }
    }
    // qDebug() << "Persist Update: id=" << storeId << " path=" << item->path() << " value=" << item->toByteArray();
}


bool  ArnPersist::setMountPoint( const QString& path)
{
    if (!_db->isOpen()) {
        ArnM::errorLog( QString(tr("DataBase required before mountPoint")),
                            ArnError::NotOpen);
        return false;
    }

    if (_arnMountPoint)  delete _arnMountPoint;

    _arnMountPoint = new ArnItem( this);
    bool  isOk = _arnMountPoint->openFolder( path);
    if (isOk) {
        connect( _arnMountPoint, SIGNAL(arnModeChanged(QString,uint,ArnItem::Mode)),
                 this, SLOT(doArnModeChanged(QString,uint,ArnItem::Mode)));

        // Load all persist files & mandatory into arn
        doLoadFiles();
        doLoadMandatory();

        if (_depOffer)  delete _depOffer;
        // Persist service is available
        _depOffer = new ArnDependOffer( this);
        _depOffer->advertise("$Persist");
    }

    return isOk;
}


bool  ArnPersist::setupDataBase( QString dbName)
{
    if (_query)  delete _query;

    *_db = QSqlDatabase::addDatabase("QSQLITE", "ArnPersist");
    //db.setHostName("bigblue");
    _db->setDatabaseName( dbName);
    //db.setUserName("acarlson");
    //db.setPassword("1uTbSbAs");
    if (!_db->open()) {
        ArnM::errorLog( QString(tr("DataBase open: ") + _db->lastError().text()),
                            ArnError::ConnectionError);
        return false;
    }
    _query = new QSqlQuery( *_db);
    return true;
}


bool  ArnPersist::getDbId( QString path, int& storeId)
{
    bool  retVal = false;
    int   isUsed = 1;

    _query->prepare("SELECT id, isUsed FROM store WHERE path = :path");
    _query->bindValue(":path", path);
    _query->exec();
    if (_query->next()) {
        storeId = _query->value(0).toInt();
        isUsed  = _query->value(1).toInt();
        retVal  = true;
    }
    _query->finish();

    if (!isUsed)  updateDbUsed( storeId, 1);
    return retVal;
}


bool  ArnPersist::getDbValue(int storeId, QString &path, QByteArray &value)
{
    bool  retVal = false;
    int   isUsed = 1;
    _query->prepare("SELECT path, value, isUsed FROM store WHERE id = :id");
    _query->bindValue(":id", storeId);
    _query->exec();
    if (_query->next()) {
        path    = _query->value(0).toString();
        value   = _query->value(1).toByteArray();
        isUsed  = _query->value(2).toInt();
        retVal  = true;
    }
    _query->finish();

    if (!isUsed)  updateDbUsed( storeId, 1);
    return retVal;
}


bool  ArnPersist::getDbValue( QString path, QByteArray& value, int& storeId)
{
    bool  retVal = false;
    int   isUsed = 1;
    _query->prepare("SELECT id, value, isUsed FROM store WHERE path = :path");
    _query->bindValue(":path", path);
    _query->exec();
    if (_query->next()) {
        storeId = _query->value(0).toInt();
        value   = _query->value(1).toByteArray();
        isUsed  = _query->value(2).toInt();
        retVal  = true;
    }
    _query->finish();

    if (!isUsed)  updateDbUsed( storeId, 1);
    return retVal;
}


bool  ArnPersist::getDbMandatoryList( QList<int>& storeIdList)
{
    bool  retVal = false;
    storeIdList.clear();
    _query->prepare("SELECT id FROM store WHERE isMandatory = 1");
    _query->exec();
    while (_query->next()) {
        storeIdList += _query->value(0).toInt();
        retVal  = true;
    }
    _query->finish();

    return retVal;
}


bool  ArnPersist::getDbList( bool isUsed, QList<int> &storeIdList)
{
    bool  retVal = false;
    storeIdList.clear();
    _query->prepare("SELECT id FROM store WHERE isUsed = :isUsed");
    _query->bindValue(":isUsed", int(isUsed));
    _query->exec();
    while (_query->next()) {
        storeIdList += _query->value(0).toInt();
        retVal  = true;
    }
    _query->finish();

    return retVal;
}


bool  ArnPersist::insertDbValue( QString path, QByteArray value)
{
    bool  retVal = false;

    _query->prepare("INSERT INTO store (path, value) "
                   "VALUES (:path, :value)");
    _query->bindValue(":path", path);
    _query->bindValue(":value", value);
    retVal = _query->exec();
    _query->finish();

    return retVal;
}


bool  ArnPersist::updateDbValue( int storeId, QByteArray value)
{
    //qDebug() << "Persist updateDb: id=" << storeId << " value=" << value;
    bool  retVal = false;

    _query->prepare("UPDATE store SET value = :value WHERE id = :id");
    _query->bindValue(":id", storeId);
    _query->bindValue(":value", value);
    retVal = _query->exec();
    _query->finish();

    return retVal;
}


bool  ArnPersist::updateDbUsed( int storeId, int isUsed)
{
    // qDebug() << "UpdateDb: id=" << storeId << " isUsed=" << isUsed;
    bool  retVal = false;

    _query->prepare("UPDATE store SET isUsed = :isUsed WHERE id = :id");
    _query->bindValue(":id", storeId);
    _query->bindValue(":isUsed", isUsed);
    retVal = _query->exec();
    _query->finish();

    return retVal;
}


bool  ArnPersist::updateDbMandatory( int storeId, int isMandatory)
{
    //qDebug() << "UpdateDb: id=" << storeId << " isMandatory=" << isMandatory;
    bool  retVal = false;

    _query->prepare("UPDATE store SET isMandatory = :isMandatory WHERE id = :id");
    _query->bindValue(":id", storeId);
    _query->bindValue(":isMandatory", isMandatory);
    retVal = _query->exec();
    _query->finish();

    return retVal;
}


bool  ArnPersist::doArchive(QString name)
{
    QString  arFileName;

    if (name.isNull()) {
        QDateTime  dateTime = QDateTime::currentDateTime();
        QString  nowStr = dateTime.toString("yyMMdd-hhmmss");
        arFileName = _archiveDir->absoluteFilePath( QString("persist_%1.db").arg( nowStr));
    }
    else {
        arFileName = _archiveDir->absoluteFilePath( name);
    }
    QString  dbFileName = _db->databaseName();

    //qDebug() << "Persist Archive: src=" << dbFileName << " dst=" << arFileName;
    return QFile::copy( dbFileName, arFileName);
}


void  ArnPersist::doLoadMandatory()
{
    QList<int>  mandatoryList;
    if (!getDbMandatoryList( mandatoryList))  return;  // Nothing to load

    QString  path;
    QByteArray  value;
    foreach (int storeId, mandatoryList) {
        if (!getDbValue( storeId, path, value))  continue;  // Not ok ...

        ArnItemPersist::StoreType  st;
        ArnItemPersist*  item = setupMandatory( path, true);
        Q_ASSERT(item);
        if (item->storeType() != st.DataBase)  continue;  // Not a database persist

        QByteArray  data;
        int  storeId2;
        if (getDbValue( path, data, storeId2)) {
            item->arnImport( data, false);
            item->setStoreId( storeId2);
        }
        else {
            item->setValue( QByteArray(), false);  // Do a null update, to signal update done
        }
    }
}


void  ArnPersist::doLoadFiles()
{
    QStringList  flist;
    fileList( flist, *_persistDir);
    foreach (const QString& relPath, flist) {
        loadFile( relPath);
    }
}


void  ArnPersist::loadFile( QString relPath)
{
    QString  arnPath = ArnM::convertPath( relPath, ArnLink::NameF::EmptyOk);
    qDebug() << "Persist loadFile: relPath=" << relPath;

    ArnItemPersist*  item;
    if (!_pathPersistMap.contains( arnPath)) {  // First time loaded
        item = getPersistItem( arnPath);
        item->setDelay( 500);  // MW: ok?
        _pathPersistMap.insert( arnPath, item->linkId());
    }
    else {
        uint  linkId = _pathPersistMap.value( arnPath);
        item = _itemPersistMap.value( linkId);
        Q_ASSERT(item);
    }
    item->setStoreType( ArnItemPersist::StoreType::File);

    QFile  file( _persistDir->absoluteFilePath( relPath));
    file.open( QIODevice::ReadOnly);
    QByteArray  data = file.readAll();
    // qDebug() << "Persist loadFile: absPath=" << file.fileName();
    item->setValue( data, true);
}


void  ArnPersist::fileList(QStringList& flist, const QDir& dir, const QDir* baseDir)
{
    if (!baseDir)
        baseDir = &dir;
    QString  path = dir.absolutePath();
    //qDebug() << "fileList: dir=" << path;

    foreach( QFileInfo finfo, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files)) {
        if (finfo.isFile()) {
            flist += baseDir->relativeFilePath( finfo.absoluteFilePath());
        }
        else if (finfo.isDir()) {
            QString  fname = finfo.fileName();
            fileList( flist, QDir(finfo.absoluteFilePath()), baseDir);
        }
    }
}


void  ArnPersist::setupSapi( ArnPersistSapi* sapi, QString pipePath)
{
    typedef ArnRpc::Mode  Mode;
    sapi->open( pipePath, Mode::Provider);
    sapi->batchConnect( QRegExp("^pv_(.+)"), this, "sapi\\1", Mode());
}


void  ArnPersist::destroyRpc()
{
    ArnRpc*  rpc = qobject_cast<ArnRpc*>( sender());
    if (!rpc)  return;  // No valid sender

    qDebug() << "Persist destroyRpc: pipePath=" << rpc->pipePath();
    rpc->deleteLater();
}


void  ArnPersist::convertFileList( QStringList &files, ArnLink::NameF nameF)
{
    for (int i = 0; i < files.size(); ++i) {
        files[i] = ArnM::convertPath( files.at(i), nameF);
    }
}


void  ArnPersist::vcsCheckoutR()
{
    doLoadFiles();
    // emit rps_vcsCheckoutR();
}


void  ArnPersist::sapiTest( QString str, int i)
{
    ArnPersistSapi*  sapiSender = qobject_cast<ArnPersistSapi*>( sender());
    if (sapiSender) {
        qDebug() << "sapiTest sender=" << sapiSender->pipePath();
        sapiSender->sendText("sapiTest: str=" + str + " int=" + QString::number(i));
    }
    qDebug() << "----- sapiTest: str=" << str << " int=" << i;
}


void  ArnPersist::sapiLoad()
{
    doLoadFiles();
}


void  ArnPersist::sapiLs( QString path)
{
    QStringList  flist;
    fileList( flist, *_persistDir);
    convertFileList( flist, ArnLink::NameF::EmptyOk);

    if (path.isEmpty())
        emit _sapiCommon->rq_lsR( flist);
    else  // Filter only files beginning with path
        emit _sapiCommon->rq_lsR( flist.filter( QRegExp("^" + QRegExp::escape( path))));
}


void  ArnPersist::sapiRm( QString path)
{
    QString  relPath = ArnM::convertPath( path, ArnLink::NameF::Relative);
    bool  isOk = _persistDir->remove( relPath);
    if (isOk) {
        QString  filePath = _persistDir->absoluteFilePath( relPath);
        _persistDir->rmpath( QFileInfo( filePath).dir().path());  // Remove empty paths
        removeFilePersistItem( path);
    }
    emit _sapiCommon->rq_rmR( isOk);
}


void  ArnPersist::sapiTouch( QString path)
{
    if (path.endsWith('/'))  return;  // Don't touch a dir

    bool  isOk = true;
    QString  relPath = ArnM::convertPath( path, ArnLink::NameF::Relative);
    QString  filePath = _persistDir->absoluteFilePath( relPath);
    // Make any needed directories
    isOk &= _persistDir->mkpath( QFileInfo( filePath).dir().path());

    //// Touch the file
    QFile  file( filePath);
    isOk &= file.open( QIODevice::ReadWrite);
    file.close();
    if (isOk)
        loadFile( relPath);

    emit _sapiCommon->rq_touchR( isOk);
}


void  ArnPersist::sapiDbMandatory( QString path, bool isMandatory)
{
    ArnItemPersist::StoreType  st;
    int storeId;
    if (getDbId( path, storeId)) {
        if (updateDbMandatory( storeId, isMandatory)) {
            setupMandatory( path, isMandatory);
            emit _sapiCommon->rq_dbMandatoryR(true);
        }
    }
    emit _sapiCommon->rq_dbMandatoryR(false);
}


void  ArnPersist::sapiDbMandatoryLs( QString path)
{
    QStringList  retList;
    QList<int>  storeIdList;
    if (!getDbMandatoryList( storeIdList)) {
        emit _sapiCommon->rq_dbMandatoryLsR( retList);
        return;
    }

    QString  pathDb;
    QByteArray  value;
    foreach (int storeId, storeIdList) {
        if (!getDbValue( storeId, pathDb, value))  continue;
        if (!pathDb.startsWith( path))  continue;

        retList += pathDb;
    }

    emit _sapiCommon->rq_dbMandatoryLsR( retList);
}


void  ArnPersist::sapiDbLs( QString path, bool isUsed)
{
    QStringList  retList;
    QList<int>  storeIdList;
    if (!getDbList( isUsed, storeIdList)) {
        emit _sapiCommon->rq_dbLsR( retList);
        return;
    }

    QString  pathDb;
    QByteArray  value;
    foreach (int storeId, storeIdList) {
        if (!getDbValue( storeId, pathDb, value))  continue;
        if (!pathDb.startsWith( path))  continue;

        retList += pathDb;
    }

    emit _sapiCommon->rq_dbLsR( retList);
}


void  ArnPersist::sapiInfo()
{
    emit _sapiCommon->rq_infoR("Arn Persist", "1.0");
}