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

#include <iostream>
#include <QStringList>
#include <QVector>
#include <QDebug>
#include <QEvent>
#include <QMutex>
#include <QWaitCondition>
#include <QThreadStorage>
#include <QThread>
#include <QCoreApplication>
#include <QMetaType>

#include "Arn.hpp"


const bool gDebugThreading = 0;
const bool gDebugLinkRef   = 0;
const bool gDebugRecInOut  = 0;
const bool gDebugMonitor   = 0;

/////////////// ArnThreadCom

//! \cond ADV
class ArnThreadComStorage : public QThreadStorage<ArnThreadCom*> {};
ArnThreadComStorage*  ArnThreadCom::_threadStorage = new ArnThreadComStorage;
//! \endcond


ArnThreadCom*  ArnThreadCom::getThreadCom()
{
    if (!_threadStorage->hasLocalData()) {
        _threadStorage->setLocalData( new ArnThreadCom);
    }
    return _threadStorage->localData();
}


ArnThreadComCaller::ArnThreadComCaller()
{
    _p = ArnThreadCom::getThreadCom();
    if (gDebugThreading)  qDebug() << "ThreadComCaller start p=" << _p;
    _p->_mutex.lock();
}


ArnThreadComCaller::~ArnThreadComCaller()
{
    _p->_mutex.unlock();
    if (gDebugThreading)  qDebug() << "ThreadComCaller end p=" << _p;
}


void  ArnThreadComCaller::waitCommandEnd()
{
    _p->_commandEnd.wait( &_p->_mutex);  // Wait main-thread Proxy is finished
}


ArnThreadComProxyLock::ArnThreadComProxyLock( ArnThreadCom* threadCom) :
        _p( threadCom)
{
    if (gDebugThreading)  qDebug() << "ThreadComProxy start p=" << _p;
    _p->_mutex.lock();
}


ArnThreadComProxyLock::~ArnThreadComProxyLock()
{
    _p->_commandEnd.wakeOne();
    _p->_mutex.unlock();
    if (gDebugThreading)  qDebug() << "ThreadComProxy end p=" << _p;
}


/////////////// Arn


int  ArnM::valueInt( const QString& path)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::F(0));
    if (!link)  return 0;

    int  retVal = link->toInt();
    link->deref();
    return retVal;
}


double  ArnM::valueDouble( const QString& path)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::F(0));
    if (!link)  return 0;

    double  retVal = link->toDouble();
    link->deref();
    return retVal;
}


QString  ArnM::valueString( const QString& path)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::F(0));
    if (!link)  return QString();

    QString  retVal = link->toString();
    link->deref();
    return retVal;
}


QByteArray  ArnM::valueByteArray( const QString& path)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::F(0));
    if (!link)  return QByteArray();

    QByteArray  retVal = link->toByteArray();
    link->deref();
    return retVal;
}


QVariant  ArnM::valueVariant( const QString& path)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::F(0));
    if (!link)  return QVariant();

    QVariant  retVal = link->toVariant();
    link->deref();
    return retVal;
}


void  ArnM::itemsProxy( ArnThreadCom* threadCom, const QString& path)
{
    ArnThreadComProxyLock  proxyLock( threadCom);

    if (gDebugThreading)  qDebug() << "itemsProxy: path=" << path;
    threadCom->_retStringList = itemsMain( path);
    if (gDebugThreading)  qDebug() << "itemsProxy: waking thread";
}


QStringList  ArnM::items( const QString& path)
{
    if (isMainThread()) {
        return itemsMain( path);
    }
    else {  // Threaded - must be threadsafe
        ArnThreadComCaller  threadCom;

        threadCom.p()->_retStringList.clear();  // Just in case ...
        if (gDebugThreading)  qDebug() << "items-thread: start path=" << path;
        QMetaObject::invokeMethod( &instance(),
                                   "itemsProxy",
                                   Qt::QueuedConnection,
                                   Q_ARG( ArnThreadCom*, threadCom.p()),
                                   Q_ARG( QString, path));
        threadCom.waitCommandEnd();  // Wait main-thread gives retStringList
        QStringList  retStringList = threadCom.p()->_retStringList;
        threadCom.p()->_retStringList.clear();  // Manually release memory
        if (gDebugThreading)  qDebug() << "items-thread: end stringList=" << threadCom.p()->_retStringList;

        return retStringList;
    }
}


QStringList  ArnM::itemsMain( const ArnLink *parent)
{
    if (!parent) {
        qWarning() << "items: empty path";
        return QStringList();
    }

    QObjectList  children = parent->children();
    QStringList  childnames;

    for (int i = 0; i < children.size(); i++) {
        QObject *child = children.at(i);
        ArnLink *childLink = qobject_cast<ArnLink*>(child);

        if (childLink != 0) {
            if (childLink->isFolder()) {
                childnames << child->objectName() + "/";
            }
            else {
                childnames << child->objectName();
            }
        }
    }

    return childnames;
}


QStringList  ArnM::itemsMain( const QString& path)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::Folder);
    return itemsMain( link);
}


bool  ArnM::isMainThread()
{
    QThread*  mainThread = instance()._mainThread;
    if (gDebugThreading) {
        qDebug() << "isMainThread: appThr=" << QCoreApplication::instance()->thread()
                 << " mainThr=" << mainThread
                 << " curThr=" << QThread::currentThread();
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


QString  ArnM::itemName( const QString &path)
{
    int  from = path.endsWith('/') ? -2 : -1;
    int  pos = path.lastIndexOf('/', from);

    if (pos < 0)  return path;
    return ArnLink::convertName( path.mid( pos + 1));
}


QString  ArnM::childPath( const QString &parentPath, const QString &posterityPath)
{
    QString  parentPath_ = parentPath;
    if (!parentPath_.endsWith('/'))  parentPath_ += '/';
    if (!posterityPath.startsWith( parentPath_))  return QString();  // Null, posterity not belonging tp parent

    int  i = posterityPath.indexOf('/', parentPath_.size());
    if (i >= 0)  // The child part has folder(s)
        return posterityPath.left(i + 1);
    else
        return posterityPath;
}


QString  ArnM::makePath( const QString &parentPath, const QString &itemName)
{
    QString  parentPath_ = parentPath;
    if (!parentPath_.endsWith('/'))  parentPath_ += '/';

    return parentPath_ + ArnLink::convertName( itemName, ArnLink::NameF::EmptyOk);
}


QString  ArnM::addPath( const QString &parentPath, const QString &childRelPath, ArnLink::NameF nameF)
{
    QString  retPath = parentPath;
    if (!retPath.endsWith('/'))
        retPath += '/';
    retPath += childRelPath;

    return convertPath( retPath, nameF);
}


QString  ArnM::convertPath(const QString &path, ArnLink::NameF nameF)
{
    nameF.set( nameF.NoFolderMark, false);  // Foldermark '/' must be ...
    if (nameF.is( nameF.Relative))
        nameF.set( nameF.EmptyOk, false);   // Relative implicates no emty links

    QString  retPath;
    if (!nameF.is( nameF.Relative))
        retPath += '/';  // Start of absolute path

    QString  pathNorm = path.trimmed();
    bool  isFolder = pathNorm.isEmpty() || pathNorm.endsWith("/");
    if (isFolder && !pathNorm.isEmpty())
        pathNorm.resize( pathNorm.size() - 1);  // Remove '/' at end  (Also root become "")

    QStringList  linkNames = pathNorm.split("/", QString::KeepEmptyParts);
    bool needSeparator = false;

    for (int i = 0; i < linkNames.size(); i++) {
        QString  linkName = linkNames.at(i);
        if (linkName.isEmpty()  &&  i == 0)  // If link is root, go for next link
            continue;
        if (needSeparator)
            retPath += '/';  // Add link separator

        retPath += ArnLink::convertName( linkName, nameF);
        needSeparator = true;
    }
    if (isFolder && !pathNorm.isEmpty())  // Folder that is not root
        retPath += '/';  // Add folder mark

    return retPath;
}


QString  ArnM::twinPath( const QString& path)
{
    if (path.endsWith('/'))  return path;  // Can't return twin for a folder

    if (path.endsWith('!'))  return path.left( path.size() - 1);
    return path + '!';
}


bool  ArnM::isProviderPath( const QString &path)
{
    return path.endsWith('!');
}


bool  ArnM::exist( const QString &path)
{
    ArnLink::Flags  flags;
    ArnLink*  link = ArnM::link( path, flags.SilentError);

    if (!link)  return false;
    link->deref();
    return true;
}


bool  ArnM::isFolder( const QString& path)
{
    ArnLink::Flags  flags;
    ArnLink*  link = ArnM::link( path, flags.Folder | flags.SilentError);

    if (!link)  return false;
    link->deref();
    return true;
}


bool  ArnM::isLeaf( const QString& path)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::SilentError);

    if (!link)  return false;
    link->deref();
    return true;
}


void  ArnM::setValue( const QString& path, const QString& value)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::CreateAllowed);

    if (link) {
        link->setValue(value);
        link->deref();
    }
}


void  ArnM::setValue( const QString& path, int value)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::CreateAllowed);

    if (link) {
        link->setValue( value);
        link->deref();
    }
}


void  ArnM::setValue( const QString& path, double value)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::CreateAllowed);

    if (link) {
        link->setValue( value);
        link->deref();
    }
}


void  ArnM::setValue( const QString& path, const QByteArray& value)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::CreateAllowed);

    if (link) {
        link->setValue( value);
        link->deref();
    }
}


void  ArnM::setValue( const QString& path, const QVariant& value)
{
    ArnLink*  link = ArnM::link( path, ArnLink::Flags::CreateAllowed);

    if (link) {
        link->setValue( value);
        link->deref();
    }
}


void ArnM::setValue(const QString& path, const char* value)
{
    setValue( path, QString::fromUtf8( value));
}


#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Must onlty be called fronm main thread (application)
ArnLink*  ArnM::root()
{
    return instance()._root;
}


void  ArnM::linkProxy( ArnThreadCom* threadCom, const QString& path, int flagValue, int syncMode)
{
    ArnThreadComProxyLock  proxyLock( threadCom);

    if (gDebugThreading)  qDebug() << "linkProxy: path=" << path;
    threadCom->_retObj = linkMain( path, ArnLink::Flags::F( flagValue),
                                    ArnItem::SyncMode::F( syncMode));
    if (gDebugThreading)  qDebug() << "linkProxy: waking thread";
}


ArnLink*  ArnM::link( const QString& path, ArnLink::Flags flags, ArnItem::SyncMode syncMode)
{
    if (isMainThread())  return linkMain(   path, flags, syncMode);
    else                 return linkThread( path, flags, syncMode);
}


/// Threaded - must be threadsafe
ArnLink*  ArnM::linkThread( const QString& path, ArnLink::Flags flags, ArnItem::SyncMode syncMode)
{
    flags.f |= flags.Threaded;

    ArnThreadComCaller  threadCom;

    threadCom.p()->_retObj = 0;  // Just in case ...
    if (gDebugThreading)  qDebug() << "link-thread: start path=" << path;
    QMetaObject::invokeMethod( &instance(),
                               "linkProxy",
                               Qt::QueuedConnection,
                               Q_ARG( ArnThreadCom*, threadCom.p()),
                               Q_ARG( QString, path), Q_ARG( int, flags.f),
                               Q_ARG( int, syncMode.f));
    threadCom.waitCommandEnd();  // Wait main-thread gives retLink
    ArnLink*  retLink = qobject_cast<ArnLink*>( threadCom.p()->_retObj);
    if (retLink)  if (gDebugThreading)  qDebug() << "link-thread: end path=" << retLink->linkPath();

    return retLink;
}


ArnLink*  ArnM::linkMain( const QString& path, ArnLink::Flags flags, ArnItem::SyncMode syncMode)
{
    // qDebug() << "### link-main: path=" << path;
    QString  pathNorm = path;
    if (pathNorm.endsWith("/")) {
        flags.f |= flags.Folder;
        pathNorm.resize( pathNorm.size() - 1);  // Remove '/' at end  (Also root become "")
    }

    ArnLink*  currentLink = root();
    QStringList pathlist = pathNorm.split("/", QString::KeepEmptyParts);

    for (int i = 0; i < pathlist.size(); i++) {
        ArnLink::Flags  subFlags;
        subFlags.f = flags.f | flags.flagIf( i < pathlist.size() - 1, flags.Folder);
        QString  subpath = pathlist.at(i);
        if (subpath.isEmpty()  &&  i == 0) {  // If subpath is root, go for next subpath
            continue;
        }

        currentLink = ArnM::linkMain( currentLink, subpath, subFlags, syncMode);
        if (currentLink == 0) {
            return 0;
        }
    }

    currentLink->ref();
    return currentLink;
}


ArnLink*  ArnM::link( ArnLink *parent, const QString& name, ArnLink::Flags flags, ArnItem::SyncMode syncMode)
{
    if (isMainThread()) {
        ArnLink*  link = linkMain( parent, name, flags, syncMode);
        if (link)  link->ref();
        return link;
    }
    else {  // Threaded
        if (!parent) {
            if (!flags.is( flags.SilentError)) {
                errorLog( QString(tr("Threaded, can't handle SubItem=")) + name,
                          ArnError::FolderNotOpen);
            }
            return 0;
        }
        if (!parent->isFolder()) {
            if (!flags.is( flags.SilentError)) {
                errorLog( QString(tr("Threaded, is not folder, Path=")) + parent->linkPath(),
                          ArnError::CreateError);
            }
            return 0;
        }
        return linkThread( parent->linkPath() + name, flags, syncMode);
    }
}


ArnLink*  ArnM::linkMain( ArnLink *parent, const QString& name, ArnLink::Flags flags, ArnItem::SyncMode syncMode)
{
    if (!parent) {  // No parent (folder) error
        if (!flags.is( flags.SilentError)) {
            errorLog( QString(tr("Can't handle SubItem:")) + name,
                      ArnError::FolderNotOpen);
        }
        return 0;
    }

    QString  nameNorm = name;
    if (nameNorm.endsWith("/")) {
        flags.f |= flags.Folder;
        nameNorm.resize( nameNorm.size() - 1);  // Remove '/' at end
    }

    ArnLink*  child;
    child = getRawLink( parent, nameNorm, flags);
    if (!child) {   // Error getting link
        return 0;
    }

    if (!flags.is( flags.Folder)  &&  nameNorm.endsWith('!')
        &&  flags.is( flags.CreateAllowed)) {
        // Make sure a provider link has a twin, ie a value link
        addTwinMain( child, syncMode, flags);
    }

    child->setupEnd( syncMode.f);
    return child;
}


ArnLink*  ArnM::addTwin( ArnLink* link, ArnItem::SyncMode syncMode, ArnLink::Flags flags)
{
    if (!link)  return 0;

    if (isMainThread()) {
        ArnLink*  retLink = addTwinMain( link, syncMode, flags);
        if (retLink)  retLink->ref();
        return retLink;
    }

    /// Threaded version of addTwin
    if (!link->twinLink()) {  // This link has no twin, create one
        ArnLink*  parent = qobject_cast<ArnLink*>( link->parent());
        QString  twinPath = parent->linkPath() + link->twinName();
        return linkThread( twinPath, flags.f | flags.CreateAllowed, syncMode);
        /// Reference is increased in main-thread by linkMain
    }

    return link->twinLink();
}


ArnLink*  ArnM::addTwinMain( ArnLink* link, ArnItem::SyncMode syncMode, ArnLink::Flags flags)
{
    if (!link) {
        return 0;
    }

    if (!link->twinLink()) {  // This link has no twin, create one
        QString  twinName = link->twinName();
        ArnLink*  parent = qobject_cast<ArnLink*>( link->parent());
        ArnLink*  twinLink;
        twinLink = getRawLink( parent, twinName, flags.f | flags.CreateAllowed);
        // qDebug() << "addTwin: parent=" << parent->linkPath() << " twinName=" << twinName;
        if (twinLink) {  // if twin ok, setup cross links betwen value & provider
            if (link->isThreaded())  link->_mutex.lock();
            twinLink->_twin = link;
            link->_twin = twinLink;
            if (link->isThreaded())  link->_mutex.unlock();
            twinLink->setupEnd( syncMode.f);
            emit link->modeChanged( link->linkPath(), link->linkId());   // This is now Bidirectional mode
        }
    }

    return link->twinLink();
}
#endif


ArnLink*  ArnM::getRawLink( ArnLink *parent, const QString& name, ArnLink::Flags flags)
{
    bool  showErrors = !flags.is( flags.SilentError);
    if (!parent) {  // No parent (folder) error
        if (showErrors) {
            errorLog( QString(tr("Can't handle SubItem:")) + name,
                      ArnError::FolderNotOpen);
        }
        return 0;
    }
    if (parent->isRetired()) {
        if (showErrors) {
            errorLog( QString(tr("parent:")) + parent->linkPath(),
                      ArnError::Retired);
        }
        return 0;
    }

    ArnLink *child = parent->findLink( name);

    if (child == 0) {   // link not existing, create it ?
        if (!flags.is( flags.CreateAllowed)) {
            // Creating new items are not allowed
            if (showErrors) {
                errorLog( QString(tr("Path:")) + parent->linkPath() +
                          QString(tr(" Item:")) + name,
                          ArnError::NotFound);
            }
            return 0;
        }
        if (name.isEmpty()  &&  !flags.is( flags.Folder)) {
            // Empty names only allowed for folders
            if (showErrors) {
                errorLog( QString(tr("Empty leaf name, Path:")) + parent->linkPath(),
                          ArnError::CreateError);
            }
            return 0;
        }
        if (name.endsWith("!!")) {
            // Invalid, must not have double '!' at end
            if (showErrors) {
                errorLog( QString(tr("Invalid name, Path:")) + parent->linkPath(),
                          ArnError::CreateError);
            }
            return 0;
        }
        // Create folders or items when needed
        child = new ArnLink(parent, name, flags);
        connect( child, SIGNAL(zeroRef()), &instance(), SLOT(doZeroRefLink()));
    }
    else {
        if (child->isRetired()) {
            if (showErrors) {
                errorLog( QString(tr("child:")) + child->linkPath(),
                          ArnError::Retired);
            }
            return 0;
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
            return 0;
        }
    }
    /// Make sure threaded flag is updated for the twins
    if (child) {
        child->_isThreaded |= flags.is( flags.Threaded);
        if (child->_twin) {
            child->_twin->_isThreaded |= flags.is( flags.Threaded);
        }
    }

    return child;
}


void  ArnM::destroyLink( ArnLink* link)
{
    if (!link)  return;

    if (isMainThread()) {
        qDebug() << "destroyLink-mainA: start path=" << link->linkPath();
        destroyLinkMain( link);
        return;
    }

    /// Threaded version of destroyLink
    destroyLink( link->linkPath());
}


void  ArnM::destroyLink( const QString& path)
{
    if (isMainThread()) {
        ArnLink::Flags  flags;
        ArnLink*  link = ArnM::link( path, flags.SilentError);
        if (link) {
            link->deref();  // Ok, as this is main thread. Avoid locking this link
            qDebug() << "destroyLink-mainB: start path=" << link->linkPath();
            destroyLinkMain( link);
        }
        return;
    }

    /// Threaded version of destroyLink
    if (gDebugThreading)  qDebug() << "destroyLink-thread: start path=" << path;
    QMetaObject::invokeMethod( &instance(),
                               "destroyLink",
                               Qt::QueuedConnection,
                               Q_ARG( QString, path));
}


/// This will be called recursively in main-thread
void  ArnM::destroyLinkMain( ArnLink* link)
{
    if (!link)  return;

    QObjectList objList = link->children();
    int  childNum = objList.size();
    /// Childless link is directly retired
    if (childNum == 0) {
        ArnLink*  twin = link->twinLink();
        link->setRetired();
        if (twin)
            twin->setRetired();
        return;
    }
    /// Make all childs retired by recursion
    for (int i = 0; i < childNum; ++i) {
        destroyLinkMain( qobject_cast<ArnLink*>( objList.at( i)));
    }
}


void  ArnM::doZeroRefLink( QObject* /*obj*/)
{
    ArnLink*  link = qobject_cast<ArnLink*>( instance().sender());
    if (!link)  return;

    link->setRefCount( -1);  // Mark link as fully de-referenced
    // qDebug() << "ZeroRef: set fully deref path=" << link->linkPath();

    while (link->isRetired()  &&
           link->refCount() < 0  &&
           link->children().size() == 0) {  // Has no children
        ArnLink*  parent = qobject_cast<ArnLink*>( link->parent());
        // qDebug() << "ZeroRef: delete link path=" << link->linkPath();
        delete link;
        link = parent;
    }
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
                                   Q_ARG( uint, code),
                                   Q_ARG( QString, instance()._errTextTab.at( code)));
    }

    //// Connect logging to Error logger
    connect( &instance(), SIGNAL(errorLogSig(QString,uint,void*)),
             errLog, SLOT(add(QString,uint,void*)));
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


/// Creator is run by first usage of Arn (see instance())
ArnM::ArnM()
{
    _defaultIgnoreSameValue = false;
    _isThreadedApp          = false;
    _mainThread             = QThread::currentThread();
    _root                   = new ArnLink( 0, "", ArnLink::Flags::Folder);

    qRegisterMetaType<ArnThreadCom*>();
    qRegisterMetaType<ArnLinkHandle>("ArnLinkHandle");
    qRegisterMetaType<QVariant>("QVariant");

    //// Error setup
    _consoleError = true;
    _errorLogger  = 0;
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
}


ArnM::~ArnM()
{
    // Should never be used;
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
