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

#ifndef ARNM_HPP
#define ARNM_HPP

#include "ArnLib_global.hpp"
#include "Arn.hpp"
#include "MQFlags.hpp"
#include "ArnError.hpp"
#include "ArnItem.hpp"
#include <QIODevice>
#include <QStringList>
#include <QVector>
#include <QMetaType>
#include <QObject>
#include <QMutex>
#include <QWaitCondition>

class ArnThreadComStorage;
class QDir;
class QTimer;


//! \cond ADV
class ArnThreadCom
{
    friend class ArnThreadComCaller;
    friend class ArnThreadComProxyLock;

public:
    ArnThreadCom() {
        _retObj = 0;
    }

    //// Payload of "return value" from proxy to caller
    void*  _retObj;
    QStringList  _retStringList;

private:
    static ArnThreadComStorage*  getThreadComStorage();
    static ArnThreadCom*  getThreadCom();

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
    ArnThreadCom*  p();
};


class ArnThreadComProxyLock
{
    ArnThreadCom*  _p;
public:
    ArnThreadComProxyLock( ArnThreadCom* threadCom);
    ~ArnThreadComProxyLock();
};
//! \endcond


//! Arn main class
/*!
[About Arn Data Object](\ref gen_arnobj)

This singleton class is the main reference to the Active Registry Network.
 */
class ARNLIBSHARED_EXPORT ArnM : public QObject
{
    Q_OBJECT
    friend class ArnBasicItem;

public:
    static ArnM&  instance();

    static void  setConsoleError( bool isConsoleError);

    //! Set system default skipping of equal assignment value
    /*! \param[in] isIgnore If true, assignment of equal value don't give a changed signal.
     */
    static void  setDefaultIgnoreSameValue( bool isIgnore = true);

    /*! \retval true if default skipping equal assignment value
     *  \see setDefaultIgnoreSameValue()
     */
    static bool  defaultIgnoreSameValue();

    /*! \retval true if this is the main thread in the application
     */
    static bool  isMainThread();

    /*! \retval true if this is a threaded application
     */
    static bool  isThreadedApp();

    //! Get the value of _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \return The _Arn Data Object_ as an _integer_
     */
    static int  valueInt( const QString& path);

    //! Get the value of _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \return The _Arn Data Object_ as a _double_
     */
    static double  valueDouble( const QString& path);

    //! Get the value of _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \return The _Arn Data Object_ as an _ARNREAL_
     */
    static ARNREAL  valueReal( const QString& path);

    //! Get the value of _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \return The _Arn Data Object_ as a _QString_
     */
    static QString  valueString( const QString& path);

    //! Get the value of _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \return The _Arn Data Object_ as a _QByteArray_
     */
    static QByteArray  valueByteArray( const QString& path);

    //! Get the value of _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \return The _Arn Data Object_ as a _QVariant_
     */
    static QVariant  valueVariant( const QString& path);

    //! Get the childrens of the folder at _path_
    /*! Example: return list = {"test"; "folder/"; "@/"; "value"}
     *  \param[in] path
     *  \return The items (children)
     */
    static QStringList  items( const QString& path);

    /*! \param[in] path
     *  \retval true if _Arn Data Object_ exist at _path_
     */
    static bool  exist(const QString& path);

    /*! \param[in] path
     *  \retval true if _Arn Data Object_ at _path_ is a folder
     */
    static bool  isFolder( const QString& path);

    /*! \param[in] path
     *  \retval true if _Arn Data Object_ at _path_ is a leaf (non folder)
     */
    static bool  isLeaf( const QString& path);

    //! Assign an _integer_ to an _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \param[in] value to be assigned
     */
    static void  setValue( const QString& path, int value);

    //! Assign an _ARNREAL_ to an _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \param[in] value to be assigned
     */
    static void  setValue( const QString& path, ARNREAL value);

    //! Assign a _QString_ to an _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \param[in] value to be assigned
     */
    static void  setValue( const QString& path, const QString& value);

    //! Assign a _QByteArray_ to an _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \param[in] value to be assigned
     */
    static void  setValue( const QString& path, const QByteArray& value);

    //! Assign a _QVariant_ to an _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \param[in] value to be assigned
     *  \param[in] typeName to convert variant into, default no conversion
     */
    static void  setValue( const QString& path, const QVariant& value, const char* typeName = 0);

    //! Assign a _char*_ to an _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \param[in] value to be assigned
     */
    static void  setValue( const QString& path, const char* value);

    //! Load from a file to an _Arn Data Object_ at _path_
    /*! \param[in] path is the path of the _Arn Data Object_
     *  \param[in] fileName is the file to be loaded
     *  \param[in] coding indicates if text or binary mode will be used
     *  \retval true if loading from file is successful
     */
    static bool  loadFromFile( const QString& path, const QString& fileName, Arn::Coding coding);

    //! Load relative a directory root to an _Arn Data Object_ at _path_
    /*! Example: _path_ = "//Doc/help.txt", _dirRoot_ = "/usr/local", will load file from
     *  "/usr/local/@/Doc/help.txt" to _Arn_ path at "//Doc/help.txt".
     *  \param[in] path is the path of the _Arn Data Object_ and also path relative to _dirRoot_
     *  \param[in] dirRoot is the file directory to be used as root for the _path_
     *  \param[in] coding indicates if text or binary mode will be used
     *  \retval true if loading from file is successful
     */
    static bool  loadFromDirRoot( const QString& path, const QDir& dirRoot, Arn::Coding coding);

    //! Save to a file from an _Arn Data Object_ at _path_
    /*! \param[in] path is the path of the _Arn Data Object_
     *  \param[in] fileName is the file to be saved
     *  \param[in] coding indicates if text or binary mode will be used
     *  \retval true if saving to file is successful
     */
    static bool  saveToFile( const QString& path, const QString& fileName, Arn::Coding coding);

    static void  errorLog( QString errText, ArnError err = ArnError::Undef, void* reference = 0);
    static QString  errorSysName();

    //! Give information about this library
    /*! \return The info, e.g. "Name=ArnLib Ver=1.0.0 Date=12-12-30 Time=00:37"
     */
    static QByteArray  info();

    //! Return mode skip "/Local/Sys/" loading
    /*! \return mode skipLocalSysLoading
     *  \see setSkipLocalSysLoading()
     */
    bool  skipLocalSysLoading()  const;

    //! Set mode skip "/Local/Sys/" loading
    /*! Can disable auto loading of _ARN Data Objects_ into "/Local/Sys/ tree".
     *  \param[in] skipLocalSysLoading
     *  \note Must be called before entering the Qt event loop
     *  \note Check the rules for [Local path](\ref gen_localPath)
     *  \see skipLocalSysLoading()
     */
    void  setSkipLocalSysLoading( bool skipLocalSysLoading);

    //! Destroy the local _Arn Data Object_ at _path_
    /*! The link (_Arn Data Object_) will be removed locally. Server is allways forcing
     *  global destroy.
     *  \param[in] path
     *  \see destroyLink()
     */
    static void  destroyLinkLocal( const QString& path)
    { destroyLink( path, false); }

public slots:
    //! Destroy the _Arn Data Object_ at _path_
    /*! The link (_Arn Data Object_) will be removed locally and optionally from server
     *  and all connected clients. Server is allways forcing global destroy.
     *  \param[in] path
     *  \param[in] isGlobal If true, removes from server and all connected clients,
     *                      otherwise only local link.
     *  \see destroyLinkLocal()
     */
    static void  destroyLink( const QString& path, bool isGlobal = true);

    static void  setupErrorlog( QObject* errLog);

signals:
    void  errorLogSig( const QString& errText, uint errCode, void* reference);

protected:
#ifndef DOXYGEN_SKIP
    virtual void  customEvent( QEvent* ev);

    static ArnLink*  root();
    static ArnLink*  link( const QString& path, Arn::LinkFlags flags,
                           Arn::ObjectSyncMode syncMode = Arn::ObjectSyncMode());
    static ArnLink*  addTwin( const QString& path, ArnLink* child,
                              Arn::ObjectSyncMode syncMode = Arn::ObjectSyncMode(),
                              Arn::LinkFlags flags = Arn::LinkFlags());
    static void  destroyLink( ArnLink* link, bool isGlobal);
    static void  destroyLinkMain( ArnLink* link, ArnLink* startLink, bool isGlobal);
    static void  changeRefCounter( int step);
#endif

private slots:
    void  postSetup();
    void  onTimerMetrics();
    static void  linkProxy( ArnThreadCom* threadCom, const QString& path,
                            int flagValue, int syncMode = 0);
    static void  itemsProxy( ArnThreadCom* threadCom, const QString& path);

private:
    /// Private constructor/destructor to keep this class singleton
    ArnM();
    ArnM( const ArnM&);
    ~ArnM();
    ArnM&  operator=( const ArnM&);

    static ArnLink*  linkMain( const QString& path, Arn::LinkFlags flags,
                               Arn::ObjectSyncMode syncMode = Arn::ObjectSyncMode());
    static ArnLink*  linkThread( const QString& path, Arn::LinkFlags flags,
                                 Arn::ObjectSyncMode syncMode = Arn::ObjectSyncMode());
    static ArnLink*  linkMain( const QString& path, ArnLink *parent, const QString& name,
                               Arn::LinkFlags flags, Arn::ObjectSyncMode syncMode = Arn::ObjectSyncMode());
    static ArnLink*  addTwinMain( const QString& path, ArnLink* child,
                                  Arn::ObjectSyncMode syncMode = Arn::ObjectSyncMode(),
                                  Arn::LinkFlags flags = Arn::LinkFlags::fromInt(0));
    static ArnLink*  getRawLink( ArnLink *parent, const QString& name, Arn::LinkFlags flags);
    static QStringList  itemsMain( const ArnLink *parent);
    static QStringList  itemsMain( const QString& path);

    static void  doZeroRefLink( ArnLink* link);

    // The root object of all other arn data
    ArnLink*  _root;

    QVector<QString>  _errTextTab;
    bool  _consoleError;
    bool  _defaultIgnoreSameValue;
    bool  _skipLocalSysLoading;
    QObject*  _errorLogger;

    volatile bool  _isThreadedApp;
    QThread*  _mainThread;

    static int  _countFolder;
    static int  _countLeaf;
    static QAtomicInt  _countRef;
    ArnLink*  _countFolderLink;
    ArnLink*  _countLeafLink;
    ArnLink*  _countRefLink;
    QTimer*  _timerMetrics;
};

#endif // ARNM_HPP

