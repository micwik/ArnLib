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

#include "ArnInc/ArnPersist.hpp"
#include "ArnInc/ArnPersistSapi.hpp"
#include "ArnInc/ArnDepend.hpp"
#include "ArnInc/XStringMap.hpp"
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

using Arn::XStringMap;

const int  arnDbSaveVer = 200;


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
    _db            = new QSqlDatabase;
    _archiveDir    = new QDir("archive");
    _persistDir    = new QDir("persist");
    _xsm           = new XStringMap;
    _sapiCommon    = new ArnPersistSapi( this);
    _vcs           = new ArnVcs( this);
    _arnMountPoint = 0;
    _query         = 0;
    _depOffer      = 0;

    setupSapi( _sapiCommon, "//.sys/Persist/Pipes/CommonPipe!");
}


ArnPersist::~ArnPersist()
{
    delete _db;
    delete _archiveDir;
    delete _persistDir;
    delete _xsm;
    if (_arnMountPoint)  delete _arnMountPoint;
    if (_query)  delete _query;
    if (_depOffer)  delete _depOffer;
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
    if (_vcs) {
        delete _vcs;
        _vcs = 0;
    }
    if (!vcs)  return;  // No use of VCS

    _vcs = vcs;
    _vcs->setParent( this);

    connect( _vcs, SIGNAL(checkoutR()), this, SLOT(vcsCheckoutR()));
    ArnRpc::Mode  mode;
    _sapiCommon->batchConnect( _vcs, QRegExp("(.+)"), "rq_vcs\\1");
    _sapiCommon->batchConnect( QRegExp("^pv_vcs(.+)"), _vcs, "\\1");
}


ArnItemPersist*  ArnPersist::getPersistItem( const QString& path)
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


ArnItemPersist*  ArnPersist::setupMandatory( const QString& path, bool isMandatory)
{
    ArnItemPersist::StoreType  st;
    ArnItemPersist*  item = 0;

    if (isMandatory) {
        item = getPersistItem( path);
        item->setMandatory(true);
        if ((item->storeType() == st.DataBase) && !_pathPersistMap.contains( path)) {
            _pathPersistMap.insert( path, item->linkId());
            item->addMode( Arn::ObjectMode::Save);
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
            if (!item->getMode().is( Arn::ObjectMode::Save)) {
                _itemPersistMap.remove( linkId);
                delete item;
            }
        }
    }
    return item;
}


void  ArnPersist::removeFilePersistItem( const QString& path)
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

    // qDebug() << "Persist destroyArn: path=" << item->path();
    _itemPersistMap.remove( item->linkId());
    _pathPersistMap.remove( item->path());
    item->deleteLater();
}


void  ArnPersist::doArnModeChanged( const QString& path, uint linkId, Arn::ObjectMode mode)
{
    // qDebug() << "Persist modeChanged: path=" << path << " mode=" << mode.f;
    if (!mode.is( mode.Save))  return;  // Not save-mode
    if (Arn::isProviderPath( path))  return;  // Only value path is used (non provider)
    if (_itemPersistMap.contains( linkId))  return;  // Already in map
    // qDebug() << "Persist modeChanged New: path=" << path << " mode=" << mode.f;

    ArnItemPersist*  item = getPersistItem( path);
    QByteArray  data;
    int  storeId;
    if (getDbValue( path, data, storeId)) {  // path exist in db, load data
        // qDebug() << "Persist modeChanged dbRead: path=" << path;
        item->arnImport( data, false);
        item->setStoreId( storeId);
    }
    else {  // path is new in db, create it with cuurent value in item  // null data
        // qDebug() << "Persist modeChanged dbInsert: path=" << path;
        //insertDbValue( path, QByteArray());
        QByteArray  data = item->arnExport();
        insertDbValue( path, data);
        if (getDbId( path, storeId)) {
            item->setStoreId( storeId);
        }
        item->arnImport( data, false);  // Do an update, to signal update done
        //item->setValue( QByteArray(), false);  // Do a null update, to signal update done
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
        QString  relPath = item->path( Arn::NameF::Relative);
        // qDebug() << "Persist arnUpdate: Save to relPath=" << relPath;
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
        connect( _arnMountPoint, SIGNAL(arnModeChanged(QString,uint,Arn::ObjectMode)),
                 this, SLOT(doArnModeChanged(QString,uint,Arn::ObjectMode)));

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


bool  ArnPersist::setupDataBase( const QString& dbName)
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

    int  curArnDbVer = 100;  // Default for db with no meta table
    // qDebug() << "Persist-Db tables:" << _db->tables();
    if (_db->tables().contains("meta"))
        curArnDbVer = metaDbValue("ver", "101").toInt();
    bool  hasStoreTable = _db->tables().contains("store");

    //// Legacy conversion of db
    if (curArnDbVer <= 100) {
        _query->exec("CREATE TABLE meta ("
                     "attr TEXT PRIMARY KEY,"
                     "value TEXT)");
        ArnM::errorLog("Creating Persist meta-table", ArnError::Info);
    }
    if (curArnDbVer <= 101)
        setMetaDbValue("ver", "102");

    if ((curArnDbVer < 200) && hasStoreTable) {
        _query->exec("ALTER TABLE store RENAME TO store_save");
        ArnM::errorLog("Saving old Persist data to table 'store_save'", ArnError::Info);
    }
    if ((curArnDbVer < 200) || !hasStoreTable) {
        _query->exec("CREATE TABLE store ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                     "meta TEXT,"
                     "path TEXT,"
                     "valueTxt TEXT,"
                     "value BLOB,"
                     "isMandatory INTEGER NOT NULL DEFAULT(0),"
                     "isUsed INTEGER NOT NULL DEFAULT(1))");
        setMetaDbValue("ver", "200");
        ArnM::errorLog("Creating new Persist data table", ArnError::Info);
    }
    if ((curArnDbVer < 200) && hasStoreTable) {
        _query->exec("INSERT INTO store (path, value, isUsed, isMandatory) "
                     "SELECT path, value, isUsed, isMandatory FROM store_save");
        _query->exec("DROP TABLE store_save");
        ArnM::errorLog("Converting Persist data to ArnDB 2.0", ArnError::Info);
    }

    return true;
}


QString  ArnPersist::metaDbValue( const QString& attr, const QString& def)
{
    QString  retVal = def;

    _query->prepare("SELECT value FROM meta WHERE attr = :attr");
    _query->bindValue(":attr", attr);
    _query->exec();
    if (_query->next()) {
        retVal = _query->value(0).toString();
        if (retVal.isNull())  // Successful query must not be null
            retVal = "";
    }
    _query->finish();

    return retVal;
}


bool  ArnPersist::setMetaDbValue( const QString& attr, const QString& value)
{
    QString  curVal = metaDbValue( attr);
    if (value == curVal)  return true;  // Already set to value

    if (curVal.isNull())  // Meta attr not present, insert new
        _query->prepare("INSERT INTO meta (attr, value) VALUES (:attr, :value)");
    else  // Meta attr present, update it
        _query->prepare("UPDATE meta SET value = :value WHERE attr = :attr");

    _query->bindValue(":attr", attr);
    _query->bindValue(":value", value);
    bool  retVal = _query->exec();
    _query->finish();

    return retVal;
}


bool  ArnPersist::getDbId( const QString& path, int& storeId)
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


/*! meta, valueTxt and value comes from database and vill be converted to a new
 *  value to be used in ArnItemB::arnImport()
 */
void  ArnPersist::dbSetupReadValue( const QString& meta, const QString& valueTxt,
                                    QByteArray& value)
{
    if (!value.isEmpty())  return;  // Non textual is used

    if (!meta.isEmpty()) {
        _xsm->fromXString( meta.toLatin1());
        QByteArray  variantType = _xsm->value("V");
        if (!variantType.isEmpty()) {  // Variant is stored
            value = char( ArnItemB::ExportCode::VariantTxt)
                  + variantType + ":" + valueTxt.toUtf8();
            return;
        }
    }
    value = valueTxt.toUtf8();
    if (!value.isEmpty()) {
        if (value.at(0) < 32) {  // Starting char conflicting with Export-code
            value.insert( 0, char( ArnItemB::ExportCode::String));  // Stuff String-code
        }
    }
}


/*! value comes from ArnItemB::arnExport() and vill be converted to new
 *  meta, valueTxt and value to be used in database
 */
void ArnPersist::dbSetupWriteValue( QString& meta, QString& valueTxt, QByteArray& value)
{
    valueTxt = "";
    meta = "";
    if (value.isEmpty())  return;

    uchar  c = value.at(0);
    if (c == ArnItemB::ExportCode::VariantTxt) {
        int  sepPos = value.indexOf(':', 1);
        Q_ASSERT(sepPos > 0);

        QByteArray  variantType( value.constData() + 1, sepPos - 1);
        _xsm->clear();
        _xsm->add("V", variantType);
        meta = QString::fromLatin1( _xsm->toXString().constData());
        valueTxt = QString::fromUtf8( value.constData() + sepPos + 1, value.size() - sepPos - 1);
        value = QByteArray();
    }
    else if (c == ArnItemB::ExportCode::String) {
        valueTxt = QString::fromUtf8( value.constData() + 1, value.size() - 1);
        value = QByteArray();
    }
    else if (c >= 32) {  // Normal printable
        valueTxt = QString::fromUtf8( value.constData(), value.size());
        value = QByteArray();
    }
}


bool  ArnPersist::getDbValue(int storeId, QString &path, QByteArray &value)
{
    bool  retVal = false;
    int   isUsed = 1;
    QByteArray  meta;
    QString  valueTxt;
    _query->prepare("SELECT meta, path, valueTxt, value, isUsed FROM store WHERE id = :id");
    _query->bindValue(":id", storeId);
    _query->exec();
    if (_query->next()) {
        meta     = _query->value(0).toByteArray();
        path     = _query->value(1).toString();
        valueTxt = _query->value(2).toString();
        value    = _query->value(3).toByteArray();
        isUsed   = _query->value(4).toInt();
        retVal   = true;
        dbSetupReadValue( meta, valueTxt, value);
    }
    _query->finish();

    if (!isUsed)  updateDbUsed( storeId, 1);
    return retVal;
}


bool  ArnPersist::getDbValue( const QString& path, QByteArray& value, int& storeId)
{
    bool  retVal = false;
    int   isUsed = 1;
    QByteArray  meta;
    QString  valueTxt;
    _query->prepare("SELECT id, meta, valueTxt, value, isUsed FROM store WHERE path = :path");
    _query->bindValue(":path", path);
    _query->exec();
    if (_query->next()) {
        storeId  = _query->value(0).toInt();
        meta     = _query->value(1).toByteArray();
        valueTxt = _query->value(2).toString();
        value    = _query->value(3).toByteArray();
        isUsed   = _query->value(4).toInt();
        retVal   = true;
        dbSetupReadValue( meta, valueTxt, value);
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


bool  ArnPersist::insertDbValue( const QString& path, const QByteArray& value)
{
    bool  retVal = false;

    QString  meta;
    QString  valueTxt;
    QByteArray  value_ = value;
    dbSetupWriteValue( meta, valueTxt, value_);

    _query->prepare("INSERT INTO store (meta, path, valueTxt, value) "
                   "VALUES (:meta, :path, :valueTxt, :value)");
    _query->bindValue(":meta", meta);
    _query->bindValue(":path", path);
    _query->bindValue(":valueTxt", valueTxt);
    _query->bindValue(":value", value_);
    retVal = _query->exec();
    _query->finish();

    return retVal;
}


bool  ArnPersist::updateDbValue( int storeId, const QByteArray& value)
{
    // qDebug() << "Persist updateDb: id=" << storeId << " value=" << value;
    bool  retVal = false;

    QString  meta;
    QString  valueTxt;
    QByteArray  value_ = value;
    dbSetupWriteValue( meta, valueTxt, value_);

    _query->prepare("UPDATE store SET meta = :meta, valueTxt = :valueTxt, value = :value "
                    "WHERE id = :id");
    _query->bindValue(":id", storeId);
    _query->bindValue(":meta", meta);
    _query->bindValue(":valueTxt", valueTxt);
    _query->bindValue(":value", value_);
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
    // qDebug() << "UpdateDb: id=" << storeId << " isMandatory=" << isMandatory;
    bool  retVal = false;

    _query->prepare("UPDATE store SET isMandatory = :isMandatory WHERE id = :id");
    _query->bindValue(":id", storeId);
    _query->bindValue(":isMandatory", isMandatory);
    retVal = _query->exec();
    _query->finish();

    return retVal;
}


bool  ArnPersist::doArchive( const QString& name)
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

    // qDebug() << "Persist Archive: src=" << dbFileName << " dst=" << arFileName;
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
    getFileList( flist, *_persistDir);
    foreach (const QString& relPath, flist) {
        loadFile( relPath);
    }
}


void  ArnPersist::loadFile( const QString& relPath)
{
    QString  arnPath = Arn::convertPath( relPath, Arn::NameF::EmptyOk);
    // qDebug() << "Persist loadFile: relPath=" << relPath;

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
    item->setValue( data, Arn::SameValue::Accept);
}


void  ArnPersist::getFileList(QStringList& flist, const QDir& dir, const QDir* baseDir)
{
    if (!baseDir)
        baseDir = &dir;
    QString  path = dir.absolutePath();
    // qDebug() << "fileList: dir=" << path;

    foreach( QFileInfo finfo, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files)) {
        if (finfo.isFile()) {
            flist += baseDir->relativeFilePath( finfo.absoluteFilePath());
        }
        else if (finfo.isDir()) {
            QString  fname = finfo.fileName();
            getFileList( flist, QDir(finfo.absoluteFilePath()), baseDir);
        }
    }
}


void  ArnPersist::setupSapi( ArnPersistSapi* sapi, const QString& pipePath)
{
    typedef ArnRpc::Mode  Mode;
    sapi->open( pipePath, Mode::Provider);
    sapi->batchConnect( QRegExp("^pv_(.+)"), this, "sapi\\1", Mode());
}


void  ArnPersist::destroyRpc()
{
    ArnRpc*  rpc = qobject_cast<ArnRpc*>( sender());
    if (!rpc)  return;  // No valid sender

    // qDebug() << "Persist destroyRpc: pipePath=" << rpc->pipePath();
    rpc->deleteLater();
}


void  ArnPersist::convertFileList( QStringList& files, Arn::NameF nameF)
{
    for (int i = 0; i < files.size(); ++i) {
        files[i] = Arn::convertPath( files.at(i), nameF);
    }
}


void  ArnPersist::vcsCheckoutR()
{
    doLoadFiles();
    // emit rps_vcsCheckoutR();
}


void  ArnPersist::sapiTest( const QString& str, int i)
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


void  ArnPersist::sapiLs( const QString& path)
{
    QStringList  flist;
    getFileList( flist, *_persistDir);
    convertFileList( flist, Arn::NameF::EmptyOk);

    if (path.isEmpty())
        emit _sapiCommon->rq_lsR( flist);
    else  // Filter only files beginning with path
        emit _sapiCommon->rq_lsR( flist.filter( QRegExp("^" + QRegExp::escape( path))));
}


void  ArnPersist::sapiRm( const QString& path)
{
    QString  relPath = Arn::convertPath( path, Arn::NameF::Relative);
    bool  isOk = true;
    if (Arn::isFolderPath( relPath)) {
        QStringList  flist;
        getFileList( flist, *_persistDir);
        foreach (const QString& delPath, flist) {
            if (delPath.startsWith( relPath)) {
                isOk &= _persistDir->remove( delPath);
                removeFilePersistItem( Arn::convertPath( delPath, Arn::NameF::EmptyOk));
                if (isOk) {
                    _persistDir->rmpath( Arn::parentPath( delPath));  // Remove empty paths if possible
                }
            }
        }
    }
    else {
        isOk = _persistDir->remove( relPath);
        removeFilePersistItem( path);
        if (isOk) {
            _persistDir->rmpath( Arn::parentPath( relPath));  // Remove empty paths if possible
        }
    }

    emit _sapiCommon->rq_rmR( isOk);
}


void  ArnPersist::sapiTouch( const QString& path)
{
    if (path.endsWith('/'))  return;  // Don't touch a dir

    bool  isOk = true;
    QString  relPath = Arn::convertPath( path, Arn::NameF::Relative);
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


void  ArnPersist::sapiDbMandatory( const QString& path, bool isMandatory)
{
    bool isOk = true;

    if (Arn::isFolderPath( path)) {
        QList<int>  storeIdList;
        if (!getDbList( true, storeIdList)) {
            emit _sapiCommon->rq_dbMandatoryR( isOk);
            return;
        }

        QString  pathDb;
        QByteArray  value;
        foreach (int storeId, storeIdList) {
            if (!getDbValue( storeId, pathDb, value))  continue;
            if (!pathDb.startsWith( path))  continue;

            if (updateDbMandatory( storeId, isMandatory)) {
                setupMandatory( pathDb, isMandatory);
            }
            else
                isOk = false;
        }
    }
    else {
        int storeId;
        if (getDbId( path, storeId)) {
            if (updateDbMandatory( storeId, isMandatory)) {
                setupMandatory( path, isMandatory);
            }
            else
                isOk = false;
        }
    }
    emit _sapiCommon->rq_dbMandatoryR( isOk);
}


void  ArnPersist::sapiDbMandatoryLs( const QString& path)
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


void  ArnPersist::sapiDbLs( const QString& path, bool isUsed)
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


void  ArnPersist::sapiDbMarkUnused( const QString& path)
{
    QList<int>  storeIdList;
    if (!getDbList( true, storeIdList)) {
        emit _sapiCommon->rq_dbMarkUnusedR(false);  // Error
        return;
    }

    QString  pathDb;
    QByteArray  value;
    foreach (int storeId, storeIdList) {
        if (!getDbValue( storeId, pathDb, value))  continue;
        if (!pathDb.startsWith( path))  continue;

        if (!updateDbUsed( storeId, false)) {
            emit _sapiCommon->rq_dbMarkUnusedR(false);  // Error
            return;
        }
    }
    emit _sapiCommon->rq_dbMarkUnusedR(true);  // Success
}


void  ArnPersist::sapiInfo()
{
    emit _sapiCommon->rq_infoR("Arn Persist", "1.0");
}
