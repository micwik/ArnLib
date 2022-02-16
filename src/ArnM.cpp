// Copyright (C) 2010-2022 Michael Wiklund.
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

#include "ArnInc/ArnM.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnInc/ArnEvent.hpp"
#include "ArnLink.hpp"
#include <QMutex>
#include <QWaitCondition>
#include <QThreadStorage>
#include <QThread>
#include <QCoreApplication>
#include <QMetaType>
#include <QMetaObject>
#include <QMetaEnum>
#include <QFile>
#include <QDir>
#include <iostream>
#include <QTimer>
#include <QDateTime>
#include <QStringList>
#include <QVector>
#include <QDebug>


/////////////// ArnThreadCom

//! \cond ADV
class ArnThreadComStorage : public QThreadStorage<ArnThreadCom*> {};
//! \endcond


ArnThreadComStorage*  ArnThreadCom::getThreadComStorage()
{
    static ArnThreadComStorage  threadComStorage;

    return &threadComStorage;
}


ArnThreadCom*  ArnThreadCom::getThreadCom()
{
    ArnThreadComStorage*  threadComStorage = getThreadComStorage();
    if (!threadComStorage->hasLocalData()) {
        threadComStorage->setLocalData( new ArnThreadCom);
    }
    return threadComStorage->localData();
}


ArnThreadComCaller::ArnThreadComCaller()
{
    _p = ArnThreadCom::getThreadCom();
    if (Arn::debugThreading)  qDebug() << "ThreadComCaller start p=" << _p;
    _p->_mutex.lock();
}


ArnThreadComCaller::~ArnThreadComCaller()
{
    _p->_mutex.unlock();
    if (Arn::debugThreading)  qDebug() << "ThreadComCaller end p=" << _p;
}


void  ArnThreadComCaller::waitCommandEnd()
{
    _p->_commandEnd.wait( &_p->_mutex);  // Wait main-thread Proxy is finished
}


ArnThreadCom*  ArnThreadComCaller::p()
{
    return _p;
}


ArnThreadComProxyLock::ArnThreadComProxyLock( ArnThreadCom* threadCom) :
        _p( threadCom)
{
    if (Arn::debugThreading)  qDebug() << "ThreadComProxy start p=" << _p;
    _p->_mutex.lock();  // Sync caller waiting
    _p->_mutex.unlock();
}


ArnThreadComProxyLock::~ArnThreadComProxyLock()
{
    _p->_commandEnd.wakeOne();  // wake up caller
    if (Arn::debugThreading)  qDebug() << "ThreadComProxy end p=" << _p;
}


/////////////// ArnM


int  ArnM::_countFolder(0);
int  ArnM::_countLeaf(0);
QAtomicInt  ArnM::_countRef(0);


/// Creator is run by first usage of Arn (see instance())
ArnM::ArnM()
{
    ArnLink::arnM( this);

    _countFolder            = 0;
    _countLeaf              = 0;
    _countRef               = 0;
    _countFolderLink        = arnNullptr;
    _countLeafLink          = arnNullptr;
    _countRefLink           = arnNullptr;
    _timerMetrics           = new QTimer( this);

    _defaultIgnoreSameValue = false;
    _skipLocalSysLoading    = false;
    _isThreadedApp          = false;
    _mainThread             = QThread::currentThread();
    _root                   = new ArnLink( arnNullptr, "", Arn::LinkFlags::Folder);

#if QT_VERSION >= 0x050a00
#else
    qsrand( uint(QDateTime::currentMSecsSinceEpoch()));
#endif

    qRegisterMetaType<ArnThreadCom*>();
    qRegisterMetaType<ArnLinkHandle>("ArnLinkHandle");
    qRegisterMetaType<QVariant>("QVariant");

    //// Error setup
    _consoleError = true;
    _errorLogger  = arnNullptr;

    _errTextTab.resize( ArnError::Err_N);
    _errTextTab[ ArnError::Ok]              = QString(tr("Ok"));
    _errTextTab[ ArnError::Warning]         = QString(tr("Warning"));
    _errTextTab[ ArnError::CreateError]     = QString(tr("Can't create"));
    _errTextTab[ ArnError::NotFound]        = QString(tr("Not found"));
    _errTextTab[ ArnError::NotOpen]         = QString(tr("Not open"));
    _errTextTab[ ArnError::AlreadyExist]    = QString(tr("Already exist"));
    _errTextTab[ ArnError::AlreadyOpen]     = QString(tr("Already open"));
    _errTextTab[ ArnError::FolderNotOpen]   = QString(tr("Folder is not open"));
    _errTextTab[ ArnError::ItemNotOpen]     = QString(tr("Item is not open"));
    _errTextTab[ ArnError::ItemNotSet]      = QString(tr("Item is not set"));
    _errTextTab[ ArnError::Retired]         = QString(tr("Access to retired"));
    _errTextTab[ ArnError::NotMainThread]   = QString(tr("Not main thread"));
    _errTextTab[ ArnError::ConnectionError] = QString(tr("Connection error"));
    _errTextTab[ ArnError::RecUnknown]      = QString(tr("Unknown record type"));
    _errTextTab[ ArnError::ScriptError]     = QString(tr("Script"));
    _errTextTab[ ArnError::RpcInvokeError]  = QString(tr("Rpc Invoke error"));
    _errTextTab[ ArnError::RpcReceiveError] = QString(tr("Rpc Receive error"));
    _errTextTab[ ArnError::LoginBad]        = QString(tr("Login error"));
    _errTextTab[ ArnError::RecNotExpected]  = QString(tr("Not expected record type here"));
    _errTextTab[ ArnError::OpNotAllowed]    = QString(tr("Operation denied, no privilege"));

    if (Arn::debugSizes) {
        qDebug() << "====== Arn Sizes ======";
        qDebug() << "QObject: " << sizeof(QObject);
        qDebug() << "  QScopedPtr: " << sizeof(QScopedPointer<QObjectData>);
        qDebug() << "  QObjectData: " << sizeof(QObjectData);
        qDebug() << "  QObjectList: " << sizeof(QObjectList);
        qDebug() << "QString: " << sizeof(QString);
        qDebug() << "QVariant: " << sizeof(QVariant);
        qDebug() << "ArnLink: " << sizeof(ArnLink);
        qDebug() << "  AnrLinkList: " << sizeof(ArnLinkList);
        qDebug() << "ArnItemB: " << sizeof(ArnItemB);
        qDebug() << "ArnItem: " << sizeof(ArnItem);
        qDebug() << "DataType: " << sizeof(Arn::DataType);
        qDebug() << "DataType::E: " << sizeof(Arn::DataType::E);
        qDebug() << "=======================";
    }

    QTimer::singleShot( 0, this, SLOT(postSetup()));
}


void  ArnM::postSetup()
{
    QString  legalPath = Arn::pathLocalSys + "Legal/";

    int  lgplStat = 0;
#if defined(ARNLIB_COMPILE)
    lgplStat = 1;
#endif
    setValue( legalPath + "ArnLib_LGPL/value", lgplStat);
    setValue( legalPath + "ArnLib_LGPL/set", "0=Seemes_Ok 1=Not_Ok,_statically_linked_to_application");

    QString  metricPath = Arn::pathLocalSys + "Metric/";
    _countFolderLink  = ArnM::link( metricPath + "ObjectFolders/value", Arn::LinkFlags::CreateAllowed);
    _countLeafLink    = ArnM::link( metricPath + "ObjectLeaves/value",  Arn::LinkFlags::CreateAllowed);
    _countRefLink     = ArnM::link( metricPath + "ObjectRef/value",     Arn::LinkFlags::CreateAllowed);
    _timerMetrics->start( 5000);
    connect( _timerMetrics, SIGNAL(timeout()), this, SLOT(onTimerMetrics()));
    onTimerMetrics();

    if (_skipLocalSysLoading)  return;

    //// Loading Licence files
    QString  licensesPath = legalPath + "Licenses/";
    QDir  dirArnRoot( Arn::resourceArnRoot);
    loadFromDirRoot( licensesPath + "LICENSE_ARNLIB.txt",  dirArnRoot, Arn::Coding::Text);
    loadFromDirRoot( licensesPath + "LICENSE_LGPL.txt",    dirArnRoot, Arn::Coding::Text);
    loadFromDirRoot( licensesPath + "LGPL_EXCEPTION.txt",  dirArnRoot, Arn::Coding::Text);
    loadFromDirRoot( licensesPath + "LICENSE_GPL3.txt",    dirArnRoot, Arn::Coding::Text);
    loadFromDirRoot( licensesPath + "LICENSE_MDNS.txt",    dirArnRoot, Arn::Coding::Text);
    loadFromDirRoot( licensesPath + "LICENSE_APACHE2.txt", dirArnRoot, Arn::Coding::Text);
}


ArnM::~ArnM()
{
    // Should never be used;
}


int  ArnM::valueInt( const QString& path)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);
    if (!link)  return 0;

    int  retVal = link->toInt();
    link->deref();
    return retVal;
}


double  ArnM::valueDouble( const QString& path)
{
    return valueReal( path);
}


ARNREAL  ArnM::valueReal(const QString& path)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);
    if (!link)  return 0;

    ARNREAL  retVal = link->toReal();
    link->deref();
    return retVal;
}


QString  ArnM::valueString( const QString& path)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);
    if (!link)  return QString();

    QString  retVal = link->toString();
    link->deref();
    return retVal;
}


QByteArray  ArnM::valueByteArray( const QString& path)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);
    if (!link)  return QByteArray();

    QByteArray  retVal = link->toByteArray();
    link->deref();
    return retVal;
}


QVariant  ArnM::valueVariant( const QString& path)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);
    if (!link)  return QVariant();

    QVariant  retVal = link->toVariant();
    link->deref();
    return retVal;
}


void  ArnM::itemsProxy( ArnThreadCom* threadCom, const QString& path)
{
    ArnThreadComProxyLock  proxyLock( threadCom);

    if (Arn::debugThreading)  qDebug() << "itemsProxy: path=" << path;
    threadCom->_retStringList = itemsMain( path);
    if (Arn::debugThreading)  qDebug() << "itemsProxy: waking thread";
}


QStringList  ArnM::items( const QString& path)
{
    if (isMainThread()) {
        return itemsMain( path);
    }
    else {  // Threaded - must be threadsafe
        ArnThreadComCaller  threadCom;

        threadCom.p()->_retStringList.clear();  // Just in case ...
        if (Arn::debugThreading)  qDebug() << "items-thread: start path=" << path;
        QMetaObject::invokeMethod( &instance(),
                                   "itemsProxy",
                                   Qt::QueuedConnection,
                                   Q_ARG( ArnThreadCom*, threadCom.p()),
                                   Q_ARG( QString, path));
        threadCom.waitCommandEnd();  // Wait main-thread gives retStringList
        QStringList  retStringList = threadCom.p()->_retStringList;
        threadCom.p()->_retStringList.clear();  // Manually release memory
        if (Arn::debugThreading)  qDebug() << "items-thread: end stringList=" << threadCom.p()->_retStringList;

        return retStringList;
    }
}


QStringList  ArnM::itemsMain( const ArnLink *parent)
{
    if (!parent) {
        qWarning() << "items: empty path";
        return QStringList();
    }

    const ArnLinkList&  children = parent->children();
    QStringList  childnames;

    for (int i = 0; i < children.size(); i++) {
        ArnLink*  childLink = children.at(i);

        if (childLink != arnNullptr) {
            if (childLink->isFolder()) {
                childnames << childLink->objectName() + "/";
            }
            else {
                childnames << childLink->objectName();
            }
        }
    }

    return childnames;
}


QStringList  ArnM::itemsMain( const QString& path)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::Folder);
    QStringList  retVal = itemsMain( link);
    if (link) {
        link->deref();
    }

    return retVal;
}


bool  ArnM::isMainThread()
{
    QThread*  mainThread = instance()._mainThread;
    if (Arn::debugThreading) {
        qDebug() << "isMainThread: appThr=" << QCoreApplication::instance()->thread()
                 << " mainThr=" << mainThread
                 << " curThr=" << QThread::currentThread()
                 << " ArnMThr=" << instance().thread();
    }
    if (QThread::currentThread() != mainThread) {
        if (!instance()._isThreadedApp)  instance()._isThreadedApp = true;
        return false;
    }
    return true;
}


bool  ArnM::isThreadedApp()
{
    return instance()._isThreadedApp;
}


bool  ArnM::exist( const QString &path)
{
    Arn::LinkFlags  flags;
    ArnLink*  link = ArnM::link( path, flags.SilentError);

    if (!link)  return false;
    link->deref();
    return true;
}


bool  ArnM::isFolder( const QString& path)
{
    Arn::LinkFlags  flags;
    ArnLink*  link = ArnM::link( path, flags.Folder | flags.SilentError);

    if (!link)  return false;
    link->deref();
    return true;
}


bool  ArnM::isLeaf( const QString& path)
{
    if (Arn::isFolderPath( path))  return false;

    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::SilentError);

    if (!link)  return false;
    link->deref();
    return true;
}


void  ArnM::setAtomicOpProvider( const QString& path)
{
    if (Arn::isFolderPath( path))  return;

    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);

    if (link) {
        link->setAtomicOpProvider( true);
        link->deref();
    }
}


bool  ArnM::isAtomicOpProvider( const QString& path)
{
    if (Arn::isFolderPath( path))  return false;

    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::SilentError);

    if (!link)  return false;
    bool  retVal = link->isAtomicOpProvider();
    link->deref();
    return retVal;
}


void  ArnM::setValue( const QString& path, const QString& value)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);

    if (link) {
        link->setValue( value);
        link->deref();
    }
}


void  ArnM::setValue( const QString& path, int value)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);

    if (link) {
        link->setValue( value);
        link->deref();
    }
}


void  ArnM::setValue( const QString& path, ARNREAL value)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);

    if (link) {
        link->setValue( value);
        link->deref();
    }
}


void  ArnM::setValue( const QString& path, const QByteArray& value)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);

    if (link) {
        link->setValue( value);
        link->deref();
    }
}


void  ArnM::setValue( const QString& path, const QVariant& value, const char* typeName)
{
    ArnLink*  link = ArnM::link( path, Arn::LinkFlags::CreateAllowed);

    if (link) {
        int  valueType = 0;
        if (typeName && *typeName)
            valueType = QMetaType::type( typeName);

        if (!valueType)
            link->setValue( value);
        else {
            QVariant  val = value;
            if (val.convert( QVariant::Type( valueType))) {
                link->setValue( val);
            }
            else {
                errorLog( QString(tr("Can't convert variant', Path:")) + path + " type=" + typeName,
                          ArnError::Undef);
            }
        }
        link->deref();
    }
}


void  ArnM::setValue( const QString& path, const char* value)
{
    setValue( path, QString::fromUtf8( value));
}


bool  ArnM::loadFromFile( const QString& path, const QString& fileName, Arn::Coding coding)
{
    bool  isText = coding.is( coding.Text);

    QFile  file( fileName);
    if (!file.open( QIODevice::ReadOnly))  return false;
    file.setTextModeEnabled( isText);
    QByteArray  data = file.readAll();

    if (isText)
        ArnM::setValue( path, QString::fromUtf8( data.constData(), data.size()));
    else
        ArnM::setValue( path, data);

    return true;
}


bool  ArnM::loadFromDirRoot( const QString& path, const QDir& dirRoot, Arn::Coding coding)
{
    QString  arnFullPath = Arn::fullPath( path);
    QString  fileAbsPath = dirRoot.absoluteFilePath( Arn::convertPath( arnFullPath, Arn::NameF::Relative));

    return  loadFromFile( arnFullPath, fileAbsPath, coding);
}


bool  ArnM::saveToFile( const QString& path, const QString& fileName, Arn::Coding coding)
{
    bool  isText = coding.is( coding.Text);

    QFile  file( fileName);
    if (!file.open( QIODevice::WriteOnly))  return false;
    file.setTextModeEnabled( isText);

    QByteArray  data = ArnM::valueByteArray( path);
    return (file.write( data) >= 0);
}


#ifndef DOXYGEN_SKIP
// Must onlty be called fronm main thread (application)
ArnLink*  ArnM::root()
{
    return instance()._root;
}


void  ArnM::linkProxy( ArnThreadCom* threadCom, const QString& path, int flagValue, int syncMode)
{
    ArnThreadComProxyLock  proxyLock( threadCom);

    if (Arn::debugThreading)  qDebug() << "linkProxy: path=" << path;
    threadCom->_retObj = linkMain( path, Arn::LinkFlags::fromInt( flagValue),
                                   Arn::ObjectSyncMode::fromInt( syncMode));
    if (Arn::debugThreading)  qDebug() << "linkProxy: waking thread";
}


ArnLink*  ArnM::link( const QString& path, Arn::LinkFlags flags, Arn::ObjectSyncMode syncMode)
{
    if (isMainThread())  return linkMain(   path, flags, syncMode);
    else                 return linkThread( path, flags, syncMode);
}


/// Threaded - must be threadsafe
ArnLink*  ArnM::linkThread( const QString& path, Arn::LinkFlags flags, Arn::ObjectSyncMode syncMode)
{
    flags.set( flags.Threaded);

    ArnThreadComCaller  threadCom;

    threadCom.p()->_retObj = arnNullptr;  // Just in case ...
    if (Arn::debugThreading)  qDebug() << "link-thread: start path=" << path;
    QMetaObject::invokeMethod( &instance(),
                               "linkProxy",
                               Qt::QueuedConnection,
                               Q_ARG( ArnThreadCom*, threadCom.p()),
                               Q_ARG( QString, path), Q_ARG( int, flags.toInt()),
                               Q_ARG( int, syncMode.toInt()));
    if (Arn::debugThreading)  qDebug() << "link-thread: invoked path=" << path;
    threadCom.waitCommandEnd();  // Wait main-thread gives retLink
    ArnLink*  retLink = static_cast<ArnLink*>( threadCom.p()->_retObj);
    if (retLink)  if (Arn::debugThreading)  qDebug() << "link-thread: end path=" << retLink->linkPath();

    return retLink;
}


ArnLink*  ArnM::linkMain( const QString& path, Arn::LinkFlags flags, Arn::ObjectSyncMode syncMode)
{
    // qDebug() << "### link-main: path=" << path;
    QString  pathNorm = Arn::fullPath( path);
    if (pathNorm.endsWith("/")) {
        flags.set( flags.Folder);
        pathNorm.resize( pathNorm.size() - 1);  // Remove '/' at end  (Also root become "")
    }

    ArnLink*  currentLink = root();
    QStringList  pathlist = pathNorm.split("/");
    QString  growPath = "/";
    int  pathListSize = pathlist.size();

    for (int i = 1; i < pathListSize; ++i) {
        Arn::LinkFlags  subFlags;
        subFlags.f = flags.f | flags.flagIf( i < pathListSize - 1, flags.Folder);
        subFlags.set( flags.LastLink, i == pathListSize - 1);
        QString  subPath = pathlist.at(i);

        growPath += subPath;
        if (subFlags.is( subFlags.Folder))
            growPath += "/";

        currentLink = ArnM::linkMain( growPath, currentLink, subPath, subFlags, syncMode);
        if (currentLink == arnNullptr) {
            return arnNullptr;
        }
    }

    currentLink->ref();
    return currentLink;
}


ArnLink*  ArnM::linkMain( const QString& path, ArnLink *parent, const QString& name, Arn::LinkFlags flags,
                          Arn::ObjectSyncMode syncMode)
{
    if (!parent) {  // No parent (folder) error
        if (!flags.is( flags.SilentError)) {
            errorLog( QString(tr("Can't handle SubItem:")) + name,
                      ArnError::FolderNotOpen);
        }
        return arnNullptr;
    }

    QString  nameNorm = name;
    if (nameNorm.endsWith("/")) {
        flags.set( flags.Folder);
        nameNorm.resize( nameNorm.size() - 1);  // Remove '/' at end
    }

    ArnLink*  child;
    child = getRawLink( parent, nameNorm, flags);
    if (!child) {   // Error getting link
        return arnNullptr;
    }

    if (!flags.is( flags.Folder)
    &&  nameNorm.endsWith('!')
    &&  flags.is( flags.CreateAllowed)) {
        // Make sure a provider link has a twin, ie a value link
        addTwinMain( path, child, syncMode, flags);
    }

    child->setupEnd( path, syncMode, flags);
    return child;
}


ArnLink*  ArnM::addTwin( const QString& path, ArnLink* link,
                         Arn::ObjectSyncMode syncMode, Arn::LinkFlags flags)
{
    if (!link)  return arnNullptr;

    if (isMainThread()) {
        ArnLink*  retLink = addTwinMain( path, link, syncMode, flags);
        if (retLink)  retLink->ref();
        return retLink;
    }

    /// Threaded version of addTwin
    if (!link->twinLink()) {  // This link has no twin, create one
        ArnLink*  parent = link->parent();
        QString  twinPath = parent->linkPath() + link->twinName();
        return linkThread( twinPath, flags.f | flags.CreateAllowed, syncMode);
        /// Reference is increased in main-thread by linkMain
    }

    return link->twinLink();
}


ArnLink*  ArnM::addTwinMain( const QString& path, ArnLink* link,
                             Arn::ObjectSyncMode syncMode, Arn::LinkFlags flags)
{
    if (!link) {
        return arnNullptr;
    }

    if (!link->twinLink()) {  // This link has no twin, create one
        QString  twinName = link->twinName();
        ArnLink*  parent = link->parent();
        ArnLink*  twinLink;
        twinLink = getRawLink( parent, twinName, flags.f | flags.CreateAllowed);
        // qDebug() << "addTwin: parent=" << parent->linkPath() << " twinName=" << twinName;
        if (twinLink) {  // if twin ok, setup cross links betwen value & provider
            bool isThreaded = false;
            if (link->isThreaded()  // If anything indicates threaded, set twins as threded
            || twinLink->isThreaded()
            || flags.is( flags.Threaded)) {
                isThreaded = true;
                link->setThreaded();
                twinLink->setThreaded();
                link->lock();
                twinLink->lock();
            }
            twinLink->_twin = link;
            link->_twin = twinLink;
            if (isThreaded) {
                link->unlock();
                twinLink->unlock();
            }
            flags.set( flags.LastLink);  // A twin must be last link (also a leaf)
            twinLink->setupEnd( Arn::twinPath( path), syncMode, flags);
            link->doModeChanged();   // This is now Bidirectional mode
        }
    }

    return link->twinLink();
}
#endif


ArnLink*  ArnM::getRawLink( ArnLink *parent, const QString& name, Arn::LinkFlags flags)
{
    bool  showErrors = !flags.is( flags.SilentError);
    if (!parent) {  // No parent (folder) error
        if (showErrors) {
            errorLog( QString(tr("Can't handle SubItem:")) + name,
                      ArnError::FolderNotOpen);
        }
        return arnNullptr;
    }
    if (parent->isRetired()) {
        if (showErrors) {
            errorLog( QString(tr("parent:")) + parent->linkPath(),
                      ArnError::Retired);
        }
        return arnNullptr;
    }

    ArnLink *child = parent->findLink( name);

    if (child == arnNullptr) {   // link not existing, create it ?
        if (!flags.is( flags.CreateAllowed)) {
            // Creating new items are not allowed
            if (showErrors) {
                errorLog( QString(tr("Path:")) + parent->linkPath() +
                          QString(tr(" Item:")) + name,
                          ArnError::NotFound);
            }
            return arnNullptr;
        }
        if (name.isEmpty()  &&  !flags.is( flags.Folder)) {
            // Empty names only allowed for folders
            if (showErrors) {
                errorLog( QString(tr("Empty leaf name, Path:")) + parent->linkPath(),
                          ArnError::CreateError);
            }
            return arnNullptr;
        }
        if (name.endsWith("!!")) {
            // Invalid, must not have double '!' at end
            if (showErrors) {
                errorLog( QString(tr("Invalid name, Path:")) + parent->linkPath(),
                          ArnError::CreateError);
            }
            return arnNullptr;
        }
        // Create folders or items when needed
        child = new ArnLink( parent, name, flags);
        if (flags.is( flags.Folder))
            ++_countFolder;
        else
            ++_countLeaf;
    }
    else {
        if (child->isRetired()) {
            if (showErrors) {
                errorLog( QString(tr("child:")) + child->linkPath(),
                          ArnError::Retired);
            }
            return arnNullptr;
        }
        if (child->isFolder() != flags.is( flags.Folder)) {
            // There is already a link with this name, but it is of
            // the wrong type. This is an error.
            if (showErrors) {
                if (flags.is( flags.Folder)) {
                    errorLog( QString(tr("Is not folder, Path:")) + child->linkPath(),
                              ArnError::CreateError);
                }
                else {
                    errorLog( QString(tr("Is folder, Path:")) + child->linkPath(),
                              ArnError::CreateError);
                }
            }
            return arnNullptr;
        }
    }
    /// Make sure threaded flag is updated for the twins
    if (flags.is( flags.Threaded)) {
        if (child) {
            child->setThreaded();
            if (child->_twin) {
                child->_twin->setThreaded();
            }
        }
    }

    return child;
}


void  ArnM::destroyLink( ArnLink* link, bool isGlobal)
{
    if (!link)  return;

    if (isMainThread()) {
        if (Arn::debugLinkDestroy) qDebug() << "destroyLink-mainA: start path=" << link->linkPath();
        destroyLinkMain( link, link, isGlobal);
        return;
    }

    /// Threaded version of destroyLink
    destroyLink( link->linkPath(), isGlobal);
}


void  ArnM::destroyLink( const QString& path, bool isGlobal)
{
    if (isMainThread()) {
        Arn::LinkFlags  flags;
        ArnLink*  link = ArnM::link( path, flags.SilentError);
        if (link) {
            link->deref();  // Ok, as this is main thread. Avoid locking this link
            if (Arn::debugLinkDestroy) qDebug() << "destroyLink-mainB: start path=" << link->linkPath();
            destroyLinkMain( link, link, isGlobal);
        }
        return;
    }

    /// Threaded version of destroyLink
    if (Arn::debugThreading)  qDebug() << "destroyLink-thread: start path=" << path;
    QMetaObject::invokeMethod( &instance(),
                               "destroyLink",
                               Qt::QueuedConnection,
                               Q_ARG( QString, path),
                               Q_ARG( bool, isGlobal));
}


/// This will be called recursively in main-thread
void  ArnM::destroyLinkMain( ArnLink* link, ArnLink* startLink, bool isGlobal)
{
    if (!link)  return;
    if (link->isRetired())  return;  // This link is already retired

    /// Mark this link as retired
    ArnLink::RetireType  rt;
    rt = startLink->isFolder() ? rt.Tree
                               : isGlobal ? rt.LeafGlobal
                                          : rt.LeafLocal;
    ArnLink*  twin = link->twinLink();
    link->setRetired( rt);
    if (twin)
        twin->setRetired( rt);
    link->ref();  // At least one ref to protect this link

    /// Make all childs retired by recursion
    for (int i = 0; i < link->children().size();) {
        ArnLink*  dLink = link->children().at( i);
        if (dLink->isRetired())
            ++i;
        else
            destroyLinkMain( dLink, startLink, isGlobal);
    }

    /// Make this link retired
    link->doRetired( startLink, isGlobal);
    if (twin)
        twin->doRetired( startLink, isGlobal);
    link->deref();  // Remove link protection, can also trigger zero-ref event
    /// Now link might be deleted
}


void  ArnM::doZeroRefLink( ArnLink* link)
{
    if (!link)  return;
    link->decZeroRefs();
    if (!link->isLastZeroRef())  return;  // Link reused & more zeroRefs will come

    link->setRefCount( -1);  // Mark link as fully de-referenced
    // qDebug() << "ZeroRef: set fully deref path=" << link->linkPath();

    while (link->isRetired()  &&
           link->refCount() < 0  &&
           link->children().size() == 0) {
        ArnLink*  parent = link->parent();
        if (Arn::debugLinkDestroy)  qDebug() << "ZeroRef: delete link path=" << link->linkPath();

        if (link->isFolder())
            --_countFolder;
        else {
            --_countLeaf;
            if (link->isBiDirMode())
                --_countLeaf;
        }
        delete link;  // This will also delete an existing twin

        link = parent;
    }
}


void  ArnM::changeRefCounter( int step)
{
    _countRef.fetchAndAddRelaxed( step);
}


QString  ArnM::errorSysName()
{
    return QString(tr("Arn"));
}


QByteArray  ArnM::info()
{
    return QByteArray("Name=ArnLib Ver=" ARNLIBVER " Date=" ARNBUILDDATE " Time=" ARNBUILDTIME);
}


void  ArnM::setupErrorlog( QObject* errLog)
{
    if (!errLog)  return;

    instance()._errorLogger = errLog;
    setConsoleError( false);

    //// Transfer system name to Error logger
    QMetaObject::invokeMethod( errLog,
                               "setSysName",
                               Qt::QueuedConnection,  // Thread safe
                               Q_ARG( QString, errorSysName()));

    //// Transfer error codes and text to Error logger
    for (int code = 0; code < instance()._errTextTab.size(); ++code) {
        QMetaObject::invokeMethod( errLog,
                                   "setErrorCode",
                                   Qt::QueuedConnection,  // Thread safe
                                   Q_ARG( uint, uint( code)),
                                   Q_ARG( QString, instance()._errTextTab.at( code)));
    }

    //// Connect logging to Error logger
    connect( &instance(), SIGNAL(errorLogSig(QString,uint,void*)),
             errLog, SLOT(add(QString,uint,void*)));
}


void  ArnM::onTimerMetrics()
{
    _countFolderLink->setValue( _countFolder);
    _countLeafLink->setValue( _countLeaf);
    _countRefLink->setValue( _countRef);
}


void  ArnM::customEvent( QEvent* ev)
{
    int  evIdx = ev->type() - ArnEvent::baseType();
    switch (evIdx) {
    case ArnEvent::Idx::ZeroRef:
    {
        ArnEvZeroRef*  e = static_cast<ArnEvZeroRef*>( ev);
        // qDebug() << "ArnEvZeroRef: path=" << e->arnLink()->linkPath();
        doZeroRefLink( e->arnLink());
        return;
    }
    default:;
    }
}


void  ArnM::errorLog( QString errText, ArnError err, void* reference )
{
    QString errTextSum;

    if (!instance()._errorLogger) {  // If no error logger has been setup, add standard error text
        if (err.e < err.Err_N) {   // Common message text in table
            errTextSum += instance()._errTextTab.at( err.e);
        }
        else {
            errTextSum += QString(tr("Error ")) + QString::number( err.e);
        }
        if (!errTextSum.isEmpty())  errTextSum += ": ";
    }
    errTextSum += errText;

    if (instance()._consoleError) {
        std::cerr << errTextSum.toUtf8().constData()
                << (QString(tr(" In ")) + errorSysName()).toUtf8().constData() << std::endl;
    }
    emit instance().errorLogSig( errTextSum, err.e, reference);
}


ArnM&  ArnM::instance()
{
    static ArnM  instance_;

    return instance_;
}


void  ArnM::setConsoleError( bool isConsoleError)
{
    instance()._consoleError = isConsoleError;
}


void  ArnM::setDefaultIgnoreSameValue( bool isIgnore)
{
    instance()._defaultIgnoreSameValue = isIgnore;
}


bool  ArnM::defaultIgnoreSameValue()
{
    return instance()._defaultIgnoreSameValue;
}


bool  ArnM::skipLocalSysLoading()  const
{
    return _skipLocalSysLoading;
}


void  ArnM::setSkipLocalSysLoading( bool skipLocalSysLoading)
{
    _skipLocalSysLoading = skipLocalSysLoading;
}
