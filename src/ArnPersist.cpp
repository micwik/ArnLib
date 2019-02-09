// Copyright (C) 2010-2019 Michael Wiklund.
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

#include "ArnInc/ArnPersist.hpp"
#include "private/ArnPersist_p.hpp"
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
    setUncrossed();  // Only normal value will be used for Load/Save
    setBlockEcho( true);  // Don't save own updates
    setIgnoreSameValue( true);
    _arnPersist  = arnPersist;
    _storeId     = 0;
    _isMandatory = false;
    _storeType   = StoreType::DataBase;
}


ArnItemPersist::~ArnItemPersist()
{
}


void  ArnItemPersist::arnImport( const QByteArray& data, int ignoreSame)
{
    ArnLinkHandle  handleData;
    handleData.flags().set( ArnLinkHandle::Flags::FromPersist);
    ArnBasicItem::arnImport( data, ignoreSame, handleData);
}


void  ArnItemPersist::setValue( const QByteArray& value, int ignoreSame)
{
    ArnLinkHandle  handleData;
    handleData.flags().set( ArnLinkHandle::Flags::FromPersist);
    ArnBasicItem::setValue( value, ignoreSame, handleData);
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



ArnPersistPrivate::ArnPersistPrivate()
{
    _db            = new QSqlDatabase;
    _archiveDir    = new QDir("archive");
    _persistDir    = new QDir("persist");
    _xsm           = new XStringMap;
    _sapiCommon    = new ArnPersistSapi;
    _vcs           = new ArnVcs;
    _arnMountPoint = 0;
    _query         = 0;
    _depOffer      = 0;
}

ArnPersistPrivate::~ArnPersistPrivate()
{
    delete _db;
    delete _archiveDir;
    delete _persistDir;
    delete _xsm;
    delete _sapiCommon;
    delete _vcs;
    if (_arnMountPoint)  delete _arnMountPoint;
    if (_query)  delete _query;
    if (_depOffer)  delete _depOffer;
}


void ArnPersist::init()
{
    Q_D(ArnPersist);

    setupSapi( d->_sapiCommon);
}


ArnPersist::ArnPersist( QObject* parent)
    : QObject( parent)
    , d_ptr( new ArnPersistPrivate)
{
    init();
}


ArnPersist::ArnPersist( ArnPersistPrivate& dd, QObject* parent)
    : QObject( parent)
    , d_ptr( &dd)
{
    init();
}


ArnPersist::~ArnPersist()
{
    delete d_ptr;
}


void  ArnPersist::setPersistDir(const QString &path)
{
    Q_D(ArnPersist);

    d->_persistDir->setPath( path);
}


void  ArnPersist::setArchiveDir(const QString &path)
{
    Q_D(ArnPersist);

    d->_archiveDir->setPath( path);
}


void  ArnPersist::setVcs( ArnVcs* vcs)
{
    Q_D(ArnPersist);

    if (d->_vcs) {
        delete d->_vcs;
        d->_vcs = 0;
    }
    if (!vcs)  return;  // No use of VCS

    d->_vcs = vcs;
    d->_vcs->setParent( this);

    connect( d->_vcs, SIGNAL(checkoutR()), this, SLOT(vcsCheckoutR()));
    ArnRpc::Mode  mode;
    d->_sapiCommon->batchConnect( d->_vcs, QRegExp("(.+)"), "rq_vcs\\1");
    d->_sapiCommon->batchConnect( QRegExp("^pv_vcs(.+)"), d->_vcs, "\\1");
}


ArnItemPersist*  ArnPersist::getPersistItem( const QString& path)
{
    Q_D(ArnPersist);

    ArnItemPersist*  item = new ArnItemPersist( this);
    item->open( path);
    uint  linkId = item->linkId();

    if (d->_itemPersistMap.contains( linkId)) { // Already as persist
        qDebug() << "getPersistItem already persist: path=" << path << " link=" << linkId;
        delete item;
        item = d->_itemPersistMap.value( linkId);

        return item;
    }

    item->setIgnoreSameValue( false);
    item->setDelay( 5000);

    connect( item->toArnItem(), SIGNAL(changed()), this, SLOT(doArnUpdate()));
    connect( item->toArnItem(), SIGNAL(arnLinkDestroyed()), this, SLOT(doArnDestroy()));
    d->_itemPersistMap.insert( linkId, item);

    return item;
}


ArnItemPersist*  ArnPersist::setupMandatory( const QString& path, bool isMandatory)
{
    Q_D(ArnPersist);

    ArnItemPersist::StoreType  st;
    ArnItemPersist*  item = 0;

    if (isMandatory) {
        item = getPersistItem( path);
        item->setMandatory(true);
        if ((item->storeType() == st.DataBase) && !d->_pathPersistMap.contains( path)) {
            d->_pathPersistMap.insert( path, item->linkId());
            item->addMode( Arn::ObjectMode::Save);
        }
    }
    else {
        uint  linkId = d->_pathPersistMap.value( path);
        if (!linkId)  return 0;

        item = d->_itemPersistMap.value( linkId);
        Q_ASSERT(item);
        item->setMandatory(false);
        if (item->storeType() != st.File) {  // Not persist file
            d->_pathPersistMap.remove( path);  // Map only needed for mandatory & file
            if (!item->getMode().is( Arn::ObjectMode::Save)) {
                d->_itemPersistMap.remove( linkId);
                delete item;
            }
        }
    }
    return item;
}


void  ArnPersist::removeFilePersistItem( const QString& path)
{
    Q_D(ArnPersist);

    uint  linkId = d->_pathPersistMap.value( path);
    if (!linkId)  return;  // path not found

    ArnItemPersist*  item = d->_itemPersistMap.value( linkId);
    Q_ASSERT(item);
    ArnItemPersist::StoreType  st;
    if (item->storeType() != st.File)  return;  // Not persist file

    item->setStoreType( st.DataBase);
    setupMandatory( path, item->isMandatory());
}


void  ArnPersist::doArnDestroy()
{
    Q_D(ArnPersist);

    ArnItemPersist*  item = ArnItemPersist::fromArnItem( qobject_cast<ArnItem*>( sender()));
    if (!item)  return;  // No valid sender

    // qDebug() << "Persist destroyArn: path=" << item->path();
    d->_itemPersistMap.remove( item->linkId());
    d->_pathPersistMap.remove( item->path());
    item->deleteLater();
}


void  ArnPersist::doArnModeChanged( const QString& path, uint linkId, Arn::ObjectMode mode)
{
    Q_D(ArnPersist);

    // qDebug() << "Persist modeChanged: path=" << path << " mode=" << mode.f;
    if (!mode.is( mode.Save))  return;  // Not save-mode
    if (Arn::isProviderPath( path))  return;  // Only value path is used (non provider)
    if (d->_itemPersistMap.contains( linkId))  return;  // Already in map
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
    Q_D(ArnPersist);

    ArnItemPersist*  item;
    item = ArnItemPersist::fromArnItem( qobject_cast<ArnItem*>( sender()));
    if (!item) {
        ArnM::errorLog( QString(tr("Can't get ArnItemPersist sender for doArnUpdate")),
                            ArnError::Undef);
        return;
    }

    switch (item->storeType()) {
    case ArnItemPersist::StoreType::DataBase:
    {
        // qDebug() << "Persist arnUpdate DB: path=" << item->path();
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
        QFile  file( d->_persistDir->absoluteFilePath( relPath));
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
    Q_D(ArnPersist);

    if (!d->_db->isOpen()) {
        ArnM::errorLog( QString(tr("DataBase required before mountPoint")),
                            ArnError::NotOpen);
        return false;
    }

    if (d->_arnMountPoint)  delete d->_arnMountPoint;

    d->_arnMountPoint = new ArnItem( this);
    bool  isOk = d->_arnMountPoint->openFolder( path);
    if (isOk) {
        connect( d->_arnMountPoint, SIGNAL(arnModeChanged(QString,uint,Arn::ObjectMode)),
                 this, SLOT(doArnModeChanged(QString,uint,Arn::ObjectMode)));

        // Load all persist files & mandatory into arn
        doLoadFiles();
        doLoadMandatory();

        if (d->_depOffer)  delete d->_depOffer;
        // Persist service is available
        d->_depOffer = new ArnDependOffer( this);
        d->_depOffer->advertise("$Persist");
    }

    return isOk;
}


bool  ArnPersist::setupDataBase( const QString& dbName)
{
    Q_D(ArnPersist);

    if (d->_query)  delete d->_query;

    *d->_db = QSqlDatabase::addDatabase("QSQLITE", "ArnPersist");
    //db.setHostName("bigblue");
    d->_db->setDatabaseName( dbName);
    //db.setUserName("acarlson");
    //db.setPassword("1uTbSbAs");
    if (!d->_db->open()) {
        ArnM::errorLog( QString(tr("DataBase open: ") + d->_db->lastError().text()),
                            ArnError::ConnectionError);
        return false;
    }
    d->_query = new QSqlQuery( *d->_db);

    int  curArnDbVer = 100;  // Default for db with no meta table
    // qDebug() << "Persist-Db tables:" << d->_db->tables();
    if (d->_db->tables().contains("meta"))
        curArnDbVer = metaDbValue("ver", "101").toInt();
    bool  hasStoreTable = d->_db->tables().contains("store");

    //// Legacy conversion of db
    if (curArnDbVer <= 100) {
        d->_query->exec("CREATE TABLE meta ("
                     "attr TEXT PRIMARY KEY,"
                     "value TEXT)");
        ArnM::errorLog("Creating Persist meta-table", ArnError::Info);
    }
    if (curArnDbVer <= 101)
        setMetaDbValue("ver", "102");

    if ((curArnDbVer < 200) && hasStoreTable) {
        d->_query->exec("ALTER TABLE store RENAME TO store_save");
        ArnM::errorLog("Saving old Persist data to table 'store_save'", ArnError::Info);
    }
    if ((curArnDbVer < 200) || !hasStoreTable) {
        d->_query->exec("CREATE TABLE store ("
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
        d->_query->exec("INSERT INTO store (path, value, isUsed, isMandatory) "
                     "SELECT path, value, isUsed, isMandatory FROM store_save");
        d->_query->exec("DROP TABLE store_save");
        ArnM::errorLog("Converting Persist data to ArnDB 2.0", ArnError::Info);
    }

    return true;
}


bool  ArnPersist::flush( const QString& path)
{
    Q_D(ArnPersist);

    bool  isOk = true;
    QString  fullPath = Arn::fullPath( path);
    foreach (ArnItemPersist* item, d->_itemPersistMap) {
        if (path.isEmpty() || item->path().startsWith( fullPath)) {
            // if (item->isDelayPending())
            //     qDebug() << "Persist flush: path=" << item->path();
            item->bypassDelayPending();
        }
    }
    return isOk;
}


QString  ArnPersist::metaDbValue( const QString& attr, const QString& def)
{
    Q_D(ArnPersist);

    QString  retVal = def;

    d->_query->prepare("SELECT value FROM meta WHERE attr = :attr");
    d->_query->bindValue(":attr", attr);
    d->_query->exec();
    if (d->_query->next()) {
        retVal = d->_query->value(0).toString();
        if (retVal.isNull())  // Successful query must not be null
            retVal = "";
    }
    d->_query->finish();

    return retVal;
}


bool  ArnPersist::setMetaDbValue( const QString& attr, const QString& value)
{
    Q_D(ArnPersist);

    QString  curVal = metaDbValue( attr);
    if (value == curVal)  return true;  // Already set to value

    if (curVal.isNull())  // Meta attr not present, insert new
        d->_query->prepare("INSERT INTO meta (attr, value) VALUES (:attr, :value)");
    else  // Meta attr present, update it
        d->_query->prepare("UPDATE meta SET value = :value WHERE attr = :attr");

    d->_query->bindValue(":attr", attr);
    d->_query->bindValue(":value", value);
    bool  retVal = d->_query->exec();
    d->_query->finish();

    return retVal;
}


bool  ArnPersist::getDbId( const QString& path, int& storeId)
{
    Q_D(ArnPersist);

    bool  retVal = false;
    int   isUsed = 1;

    d->_query->prepare("SELECT id, isUsed FROM store WHERE path = :path");
    d->_query->bindValue(":path", path);
    d->_query->exec();
    if (d->_query->next()) {
        storeId = d->_query->value(0).toInt();
        isUsed  = d->_query->value(1).toInt();
        retVal  = true;
    }
    d->_query->finish();

    if (!isUsed)  updateDbUsed( storeId, 1);
    return retVal;
}


/*! meta, valueTxt and value comes from database and vill be converted to a new
 *  value to be used in ArnItemB::arnImport()
 */
void  ArnPersist::dbSetupReadValue( const QString& meta, const QString& valueTxt,
                                    QByteArray& value)
{
    Q_D(ArnPersist);

    if (!value.isEmpty())  return;  // Non textual is used

    if (!meta.isEmpty()) {
        d->_xsm->fromXString( meta.toLatin1());
        QByteArray  variantType = d->_xsm->value("V");
        if (!variantType.isEmpty()) {  // Variant is stored
            value = char( Arn::ExportCode::VariantTxt)
                  + variantType + ":" + valueTxt.toUtf8();
            return;
        }
    }
    value = valueTxt.toUtf8();
    if (!value.isEmpty()) {
        if (value.at(0) < 32) {  // Starting char conflicting with Export-code
            value.insert( 0, char( Arn::ExportCode::String));  // Stuff String-code
        }
    }
}


/*! value comes from ArnItemB::arnExport() and vill be converted to new
 *  meta, valueTxt and value to be used in database
 */
void ArnPersist::dbSetupWriteValue( QString& meta, QString& valueTxt, QByteArray& value)
{
    Q_D(ArnPersist);

    valueTxt = "";
    meta = "";
    if (value.isEmpty())  return;

    uchar  c = value.at(0);
    if (c == Arn::ExportCode::VariantTxt) {
        int  sepPos = value.indexOf(':', 1);
        Q_ASSERT(sepPos > 0);

        QByteArray  variantType( value.constData() + 1, sepPos - 1);
        d->_xsm->clear();
        d->_xsm->add("V", variantType);
        meta = QString::fromLatin1( d->_xsm->toXString().constData());
        valueTxt = QString::fromUtf8( value.constData() + sepPos + 1, value.size() - sepPos - 1);
        value = QByteArray();
    }
    else if (c == Arn::ExportCode::String) {
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
    Q_D(ArnPersist);

    bool  retVal = false;
    int   isUsed = 1;
    QByteArray  meta;
    QString  valueTxt;
    d->_query->prepare("SELECT meta, path, valueTxt, value, isUsed FROM store WHERE id = :id");
    d->_query->bindValue(":id", storeId);
    d->_query->exec();
    if (d->_query->next()) {
        meta     = d->_query->value(0).toByteArray();
        path     = d->_query->value(1).toString();
        valueTxt = d->_query->value(2).toString();
        value    = d->_query->value(3).toByteArray();
        isUsed   = d->_query->value(4).toInt();
        retVal   = true;
        dbSetupReadValue( meta, valueTxt, value);
    }
    d->_query->finish();

    if (!isUsed)  updateDbUsed( storeId, 1);
    return retVal;
}


bool  ArnPersist::getDbValue( const QString& path, QByteArray& value, int& storeId)
{
    Q_D(ArnPersist);

    bool  retVal = false;
    int   isUsed = 1;
    QByteArray  meta;
    QString  valueTxt;
    d->_query->prepare("SELECT id, meta, valueTxt, value, isUsed FROM store WHERE path = :path");
    d->_query->bindValue(":path", path);
    d->_query->exec();
    if (d->_query->next()) {
        storeId  = d->_query->value(0).toInt();
        meta     = d->_query->value(1).toByteArray();
        valueTxt = d->_query->value(2).toString();
        value    = d->_query->value(3).toByteArray();
        isUsed   = d->_query->value(4).toInt();
        retVal   = true;
        dbSetupReadValue( meta, valueTxt, value);
    }
    d->_query->finish();

    if (!isUsed)  updateDbUsed( storeId, 1);
    return retVal;
}


bool  ArnPersist::getDbMandatoryList( QList<int>& storeIdList)
{
    Q_D(ArnPersist);

    bool  retVal = false;
    storeIdList.clear();
    d->_query->prepare("SELECT id FROM store WHERE isMandatory = 1");
    d->_query->exec();
    while (d->_query->next()) {
        storeIdList += d->_query->value(0).toInt();
        retVal  = true;
    }
    d->_query->finish();

    return retVal;
}


bool  ArnPersist::getDbList( bool isUsed, QList<int> &storeIdList)
{
    Q_D(ArnPersist);

    bool  retVal = false;
    storeIdList.clear();
    d->_query->prepare("SELECT id FROM store WHERE isUsed = :isUsed");
    d->_query->bindValue(":isUsed", int(isUsed));
    d->_query->exec();
    while (d->_query->next()) {
        storeIdList += d->_query->value(0).toInt();
        retVal  = true;
    }
    d->_query->finish();

    return retVal;
}


bool  ArnPersist::insertDbValue( const QString& path, const QByteArray& value)
{
    Q_D(ArnPersist);

    bool  retVal = false;

    QString  meta;
    QString  valueTxt;
    QByteArray  value_ = value;
    dbSetupWriteValue( meta, valueTxt, value_);

    d->_query->prepare("INSERT INTO store (meta, path, valueTxt, value) "
                   "VALUES (:meta, :path, :valueTxt, :value)");
    d->_query->bindValue(":meta", meta);
    d->_query->bindValue(":path", path);
    d->_query->bindValue(":valueTxt", valueTxt);
    d->_query->bindValue(":value", value_);
    retVal = d->_query->exec();
    d->_query->finish();

    return retVal;
}


bool  ArnPersist::updateDbValue( int storeId, const QByteArray& value)
{
    Q_D(ArnPersist);

    // qDebug() << "Persist updateDb: id=" << storeId << " value=" << value;
    bool  retVal = false;

    QString  meta;
    QString  valueTxt;
    QByteArray  value_ = value;
    dbSetupWriteValue( meta, valueTxt, value_);

    d->_query->prepare("UPDATE store SET meta = :meta, valueTxt = :valueTxt, value = :value "
                    "WHERE id = :id");
    d->_query->bindValue(":id", storeId);
    d->_query->bindValue(":meta", meta);
    d->_query->bindValue(":valueTxt", valueTxt);
    d->_query->bindValue(":value", value_);
    retVal = d->_query->exec();
    d->_query->finish();

    return retVal;
}


bool  ArnPersist::updateDbUsed( int storeId, int isUsed)
{
    Q_D(ArnPersist);

    // qDebug() << "UpdateDb: id=" << storeId << " isUsed=" << isUsed;
    bool  retVal = false;

    d->_query->prepare("UPDATE store SET isUsed = :isUsed WHERE id = :id");
    d->_query->bindValue(":id", storeId);
    d->_query->bindValue(":isUsed", isUsed);
    retVal = d->_query->exec();
    d->_query->finish();

    return retVal;
}


bool  ArnPersist::updateDbMandatory( int storeId, int isMandatory)
{
    Q_D(ArnPersist);

    // qDebug() << "UpdateDb: id=" << storeId << " isMandatory=" << isMandatory;
    bool  retVal = false;

    d->_query->prepare("UPDATE store SET isMandatory = :isMandatory WHERE id = :id");
    d->_query->bindValue(":id", storeId);
    d->_query->bindValue(":isMandatory", isMandatory);
    retVal = d->_query->exec();
    d->_query->finish();

    return retVal;
}


bool  ArnPersist::doArchive( const QString& name)
{
    Q_D(ArnPersist);

    QString  arFileName;

    if (name.isNull()) {
        QDateTime  dateTime = QDateTime::currentDateTime();
        QString  nowStr = dateTime.toString("yyMMdd-hhmmss");
        arFileName = d->_archiveDir->absoluteFilePath( QString("persist_%1.db").arg( nowStr));
    }
    else {
        arFileName = d->_archiveDir->absoluteFilePath( name);
    }
    QString  dbFileName = d->_db->databaseName();

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
    Q_D(ArnPersist);

    QStringList  flist;
    getFileList( flist, *d->_persistDir);
    foreach (const QString& relPath, flist) {
        loadFile( relPath);
    }
}


void  ArnPersist::loadFile( const QString& relPath)
{
    Q_D(ArnPersist);

    QString  arnPath = Arn::convertPath( relPath, Arn::NameF::EmptyOk);
    // qDebug() << "Persist loadFile: relPath=" << relPath;

    ArnItemPersist*  item;
    if (!d->_pathPersistMap.contains( arnPath)) {  // First time loaded
        item = getPersistItem( arnPath);
        item->setDelay( 500);  // MW: ok?
        d->_pathPersistMap.insert( arnPath, item->linkId());
    }
    else {
        uint  linkId = d->_pathPersistMap.value( arnPath);
        item = d->_itemPersistMap.value( linkId);
        Q_ASSERT(item);
    }
    item->setStoreType( ArnItemPersist::StoreType::File);

    QFile  file( d->_persistDir->absoluteFilePath( relPath));
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


void  ArnPersist::setupSapi( ArnPersistSapi* sapi)
{
    typedef ArnRpc::Mode  Mode;
    sapi->open( QString(), Mode::Provider);
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


void  ArnPersist::sapiFlush( const QString& path)
{
    Q_D(ArnPersist);

    bool  isOk = flush( path);
    emit d->_sapiCommon->rq_flushR( isOk, path);
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
    Q_D(ArnPersist);

    QStringList  flist;
    getFileList( flist, *d->_persistDir);
    convertFileList( flist, Arn::NameF::EmptyOk);

    if (path.isEmpty())
        emit d->_sapiCommon->rq_lsR( flist);
    else  // Filter only files beginning with path
        emit d->_sapiCommon->rq_lsR( flist.filter( QRegExp("^" + QRegExp::escape( path))));
}


void  ArnPersist::sapiRm( const QString& path)
{
    Q_D(ArnPersist);

    QString  relPath = Arn::convertPath( path, Arn::NameF::Relative);
    bool  isOk = true;
    if (Arn::isFolderPath( relPath)) {
        QStringList  flist;
        getFileList( flist, *d->_persistDir);
        foreach (const QString& delPath, flist) {
            if (delPath.startsWith( relPath)) {
                isOk &= d->_persistDir->remove( delPath);
                removeFilePersistItem( Arn::convertPath( delPath, Arn::NameF::EmptyOk));
                if (isOk) {
                    d->_persistDir->rmpath( Arn::parentPath( delPath));  // Remove empty paths if possible
                }
            }
        }
    }
    else {
        isOk = d->_persistDir->remove( relPath);
        removeFilePersistItem( path);
        if (isOk) {
            d->_persistDir->rmpath( Arn::parentPath( relPath));  // Remove empty paths if possible
        }
    }

    emit d->_sapiCommon->rq_rmR( isOk);
}


void  ArnPersist::sapiTouch( const QString& path)
{
    Q_D(ArnPersist);

    if (path.endsWith('/'))  return;  // Don't touch a dir

    bool  isOk = true;
    QString  relPath = Arn::convertPath( path, Arn::NameF::Relative);
    QString  filePath = d->_persistDir->absoluteFilePath( relPath);
    // Make any needed directories
    isOk &= d->_persistDir->mkpath( QFileInfo( filePath).dir().path());

    //// Touch the file
    QFile  file( filePath);
    isOk &= file.open( QIODevice::ReadWrite);
    file.close();
    if (isOk)
        loadFile( relPath);

    emit d->_sapiCommon->rq_touchR( isOk);
}


void  ArnPersist::sapiDbMandatory( const QString& path, bool isMandatory)
{
    Q_D(ArnPersist);

    bool isOk = true;

    if (Arn::isFolderPath( path)) {
        QList<int>  storeIdList;
        if (!getDbList( true, storeIdList)) {
            emit d->_sapiCommon->rq_dbMandatoryR( isOk);
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
    emit d->_sapiCommon->rq_dbMandatoryR( isOk);
}


void  ArnPersist::sapiDbMandatoryLs( const QString& path)
{
    Q_D(ArnPersist);

    QStringList  retList;
    QList<int>  storeIdList;
    if (!getDbMandatoryList( storeIdList)) {
        emit d->_sapiCommon->rq_dbMandatoryLsR( retList);
        return;
    }

    QString  pathDb;
    QByteArray  value;
    foreach (int storeId, storeIdList) {
        if (!getDbValue( storeId, pathDb, value))  continue;
        if (!pathDb.startsWith( path))  continue;

        retList += pathDb;
    }

    emit d->_sapiCommon->rq_dbMandatoryLsR( retList);
}


void  ArnPersist::sapiDbLs( const QString& path, bool isUsed)
{
    Q_D(ArnPersist);

    QStringList  retList;
    QList<int>  storeIdList;
    if (!getDbList( isUsed, storeIdList)) {
        emit d->_sapiCommon->rq_dbLsR( retList);
        return;
    }

    QString  pathDb;
    QByteArray  value;
    foreach (int storeId, storeIdList) {
        if (!getDbValue( storeId, pathDb, value))  continue;
        if (!pathDb.startsWith( path))  continue;

        retList += pathDb;
    }

    emit d->_sapiCommon->rq_dbLsR( retList);
}


void  ArnPersist::sapiDbMarkUnused( const QString& path)
{
    Q_D(ArnPersist);

    QList<int>  storeIdList;
    if (!getDbList( true, storeIdList)) {
        emit d->_sapiCommon->rq_dbMarkUnusedR(false);  // Error
        return;
    }

    QString  pathDb;
    QByteArray  value;
    foreach (int storeId, storeIdList) {
        if (!getDbValue( storeId, pathDb, value))  continue;
        if (!pathDb.startsWith( path))  continue;

        if (!updateDbUsed( storeId, false)) {
            emit d->_sapiCommon->rq_dbMarkUnusedR(false);  // Error
            return;
        }
    }
    emit d->_sapiCommon->rq_dbMarkUnusedR(true);  // Success
}


void  ArnPersist::sapiInfo()
{
    Q_D(ArnPersist);

    emit d->_sapiCommon->rq_infoR("Arn Persist", "1.0");
}
