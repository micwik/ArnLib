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
#include "ArnDefs.hpp"
#include "ArnError.hpp"
#include "ArnLink.hpp"
#include "ArnItem.hpp"
#include <QStringList>
#include <QVector>
#include <QMetaType>
#include <QObject>
#include <QMutex>
#include <QWaitCondition>

//! \cond ADV
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
//! \endcond


//! Arn main class
/*!
[About Arn Data Object](\ref gen_arnobj)

This singleton class is the main reference to the Active Registry Network.
 */
class ARNLIBSHARED_EXPORT ArnM : public QObject
{
Q_OBJECT
    friend class ArnItemB;

public:
    static ArnM&  instance();

    //! \deprecated
    static ArnM&  getInstance()  {return instance();}  // For compatibility ... (do not use)

    static void  setConsoleError( bool isConsoleError);

    //! Set system default skipping of equal value
    /*! \param[in] isIgnore If true, assignment of equal value don't give a changed signal.
     */
    static void  setDefaultIgnoreSameValue( bool isIgnore = true);

    /*! \retval true if default skipping equal values
     *  \see setDefaultIgnoreSameValue()
     */
    static bool  defaultIgnoreSameValue();

    /*! \retval true if this is the main thread in the application
     */
    static bool  isMainThread();

    /*! \retval true if this is a threaded application
     */
    static bool  isThreadedApp();

    //! Test if _path_ is a _provider path_
    /*! [About Bidirectional Arn Data Objects](\ref gen_bidirArnobj)
     *  \param[in] path.
     *  \retval true if _path_ is a _provider path_, i.e. ends with a "!".
     */
    static bool  isProviderPath( const QString& path);

    /*! \return The itemName, i.e. the last part of the path after last "/"
     */
    static QString  itemName( const QString& path);

    //! Get substring for child from a path
    /*! _parentPath_ don't have to end with a "/", if missing it's added.
     *
     *  If _posterityPath_ not starts with _parentPath_, QString() is returned.
     *  Otherwise given the _posterityPath_ the child to _parentPath_ is returned.
     *
     *  Example 1: _posterityPath_ = "//Measure/depth/value",
     *  _parentPath_ = "//Measure/" ==> return = "//Measure/depth/"
     *
     *  Example 2: _posterityPath_ = "//Measure/depth/value",
     *  _parentPath_ = "//Measure/depth/" ==> return = //Measure/depth/value"
     *  \param[in] parentPath
     *  \param[in] posterityPath
     *  \return The _child path_
     */
    static QString  childPath( const QString& parentPath, const QString& posterityPath);

    //! Make a path from a parent and an item name
    /*! _parentPath_ don't have to end with a "/", if missing it's added.
     *  Empty folder _itemName_ is allowed on returned path.
     *
     *  Example: _parentPath_ = "//Measure/depth/", _itemName_ = "value"
     *  ==> return = "//Measure/depth/value"
     *  \param[in] parentPath
     *  \param[in] itemName
     *  \return The _path_
     */
    static QString  makePath( const QString& parentPath, const QString& itemName);

    //! Make a path from a parent and an additional relative path
    /*! _parentPath_ don't have to end with a "/", if missing it's added.
     *
     *  Example: _parentPath_ = "//Measure/", _childRelPath_ = "depth/value"
     *  ==> return = "//Measure/depth/value"
     *  \param[in] parentPath
     *  \param[in] childRelPath
     *  \param[in] nameF is the path naming format
     *  \return The _path_
     *  \see convertPath()
     */
    static QString  addPath( const QString& parentPath, const QString& childRelPath,
                             Arn::NameF nameF = Arn::NameF::EmptyOk);

    //! Convert a path to a specific format
    /*! Example: _path_ = "//Measure/depth/value", nameF = Relative
     *  ==> return = "@/Measure/depth/value"
     *  \param[in] path
     *  \param[in] nameF is the path naming format
     *  \return The converted _path_
     */
    static QString  convertPath( const QString& path,
                                 Arn::NameF nameF = Arn::NameF::EmptyOk);
    //! Get the bidirectional twin to a given _path_
    /*! Example: _path_ = "//Measure/depth/value!"
     *  ==> return = "//Measure/depth/value"
     *  \param[in] path
     *  \return The twin _path_
     *  \see \ref gen_bidirArnobj
     */
    static QString  twinPath( const QString& path);

    //! Get the value of _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \return The _Arn Data Object_ as an _integer_
     */
    static int   valueInt( const QString& path);

    //! Get the value of _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \return The _Arn Data Object_ as a _double_
     */
    static double   valueDouble( const QString& path);

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

    //! Assign a _double_ to an _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \param[in] value to be assigned
     */
    static void  setValue( const QString& path, double value);

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
     */
    static void  setValue( const QString& path, const QVariant& value);

    //! Assign a _char*_ to an _Arn Data Object_ at _path_
    /*! \param[in] path
     *  \param[in] value to be assigned
     */
    static void  setValue( const QString& path, const char* value);

    static void  errorLog( QString errText, ArnError err = ArnError::Undef, void* reference = 0);
    static QString  errorSysName();

    //! Give information about this library
    /*! \return The info, e.g. "Name=ArnLib Ver=1.0.0 Date=12-12-30 Time=00:37"
     */
    static QByteArray  info();

public slots:
    //! Destroy the _Arn Data Object_ at _path_
    /*! The link (_Arn Data Object_) will be removed locally, from server and all
     *  connected clients.
     *  \param[in] path
     */
    static void  destroyLink( const QString& path);

    static void  setupErrorlog( QObject* errLog);

signals:
    void  errorLogSig( QString errText, uint errCode, void* reference);

protected:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    static ArnLink*  root();
    static ArnLink*  link( const QString& path, ArnLink::Flags flags,
                           ArnItem::SyncMode syncMode = ArnItem::SyncMode());
    static ArnLink*  link( ArnLink *parent, const QString& name, ArnLink::Flags flags,
                           ArnItem::SyncMode syncMode = ArnItem::SyncMode());
    static ArnLink*  addTwin( ArnLink* child, ArnItem::SyncMode syncMode = ArnItem::SyncMode(),
                              ArnLink::Flags flags = ArnLink::Flags());
    static void  destroyLink( ArnLink* link);
    static void  destroyLinkMain( ArnLink* link);
#endif

private slots:
    static void  linkProxy( ArnThreadCom* threadCom, const QString& path,
                            int flagValue, int syncMode = 0);
    static void  itemsProxy( ArnThreadCom* threadCom, const QString& path);
    static void  doZeroRefLink( QObject* obj = 0);

private:
    /// Private constructor/destructor to keep this class singleton
    ArnM();
    ArnM( const ArnM&);
    ~ArnM();
    ArnM&  operator=( const ArnM&);

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
    QThread*  _mainThread;
};

#endif // ARN_HPP

