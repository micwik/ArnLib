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

#ifndef ARNQML_HPP
#define ARNQML_HPP

#include "ArnLib_global.hpp"
#include "ArnInterface.hpp"
#include "ArnItem.hpp"
#include "ArnMonitor.hpp"
#include "ArnRpc.hpp"
#include "XStringMap.hpp"
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QVariantMap>

#if QT_VERSION >= 0x050000
#  include <QtQml>
#  include <QQmlParserStatus>
#  include <QQmlNetworkAccessManagerFactory>
#  include <QQmlEngine>
#  define QML_QUICK_TYPE        2
#  define QML_ENGINE            QQmlEngine
#  define QML_PARSER_STATUS     QQmlParserStatus
#  define QML_NETACC_FACTORY    QQmlNetworkAccessManagerFactory
#  define QML_LIST_PROPERTY     QQmlListProperty
#else
#  include <QtDeclarative>
#  include <QDeclarativeParserStatus>
#  include <QDeclarativeNetworkAccessManagerFactory>
#  include <QDeclarativeEngine>
#  define QML_Qt4
#  define QML_QUICK_TYPE        1
#  define QML_ENGINE            QDeclarativeEngine
#  define QML_PARSER_STATUS     QDeclarativeParserStatus
#  define QML_NETACC_FACTORY    QDeclarativeNetworkAccessManagerFactory
#  define QML_LIST_PROPERTY     QDeclarativeListProperty
#endif

class QJSEngine;
class ArnNetworkAccessManagerFactory;


//! ARN QML.
/*!
\note This class must be partly thread-safe

This class is the central point for ArnQml.
It's a singleton that is setup in the application.
ArnQml can be used for creating GUI-applications in Qml that has integrated access to the
ARN objects and some of the ArnLib funtionality.

For information about available ArnLib components in Qml see:

| QmlType    | See           |
|------------|---------------|
| Arn        | ArnInterface  |
| ArnItem    | ArnItemQml    |
| ArnMonitor | ArnMonitorQml |
| ArnSapi    | ArnSapiQml    |
| XStringMap | XStringMapQml |

If the Qml code must run in both Quick1 (Qt4) and Quick2 (Qt5), following apply:
Only Quick1 code will be able to run in both environments. When this code is run in Quick2
its "import QtQuick 1" will be changed internally to "import QtQuick 2".
"arn" is now an instantiation of ArnInterface and "Arn" is the type.
In qml "arn.quickTypeRun" will give a 1 when running in a QtQuick1 environment and a 2
for QtQuick2.

When the Qml code only is to be run in Quick2 it should use "import QtQuick 2". In this case
"Arn" will be a singleton instantiation of ArnInterface. "arn" is then not needed.

ArnBrowser is using this class to run Qml applications in an opaque style, i.e. without specific
application support. This resembles somewhat a web browser running a web application.

Note that you must not use any empty folders in QUrl for an ARN path.
Example: path "//Qml/test.qml" can be set to the equal path "/@/Qml/test.qml".
Also this conversion can be made by Arn::convertPath("//Qml/test.qml", Arn::NameF()).

<b>Example usage</b> \n \code
// In c++
//
    QQuickView*  view = new QQuickView;
    ArnQml::setup( view->engine(), ArnQml::UseFlags::All);

    QString  qmlPathInArn = "//Qml/test.qml"
    QUrl  url;
    url.setScheme("arn");
    url.setPath( Arn::convertPath( qmlPathInArn, Arn::NameF()));
    view->setSource( url);
    view->show();

    connect( engine(), SIGNAL(quit()), this, SLOT(onClose()));
    connect( view, SIGNAL(closing(QQuickCloseEvent*)), this, SLOT(onClose()));

// In Qml
//
import QtQuick 2.0
import ArnLib 1.0

Rectangle {
    width: 370;  height: 400

    ArnMonitor {
        clientId: "std"
        monitorPath: "//Test/List/"
        onArnChildFound: console.log("Found list item: " + path);
    }

    Image {
        anchors.top: parent.top;  anchors.right: parent.right;
        source: "arn:///@/Test/Data/pic.png"
    }

    ArnItem {id: arnElUpdClock;  path: "//El/UpdClock/value"}

    Item {
        id: sapiTest
        ArnSapi {pipePath: "//Test/pipe"}

        // Provider API
        signal pv_readFileTest( string fileName)

        // Requester API
        signal rq_test2( string par1)
        function rq_test( p1) {
            console.log("rq_test: p1=" + p1);
        }

        Component.onCompleted: {
            sapiTest.rq_test2.connect( info.setTestMsg);
            sapiTest.pv_readFileTest("myfile");
        }
    }

    Rectangle {
        id: info
        property string testMsg: ""
        anchors.bottom: parent.bottom; anchors.left: parent.left;  anchors.right: parent.right
        height: 80
        Column {
            anchors.fill: parent;
            Text {text: "El updClock: " + arnElUpdClock.intNum}
            Text {text: "Msg: " + info.testMsg}
            Text {text: Arn.info}  // ArnLib version info
        }

        function setTestMsg( msg) {
            info.testMsg = msg;
        }
    }
}
\endcode
*/
class ARNLIBSHARED_EXPORT ArnQml : public QObject
{
    Q_OBJECT
public:
    struct UseFlags {
        enum E {
            //! Note: ArnLib is always included
            ArnLib  = 0x01,
            //! Include some system fuctions like file-io
            MSystem = 0x02,
            //! Include some Qt extensions like MQtObject
            MQt     = 0x04,
            //! Include everything
            All     = 0xff
        };
        MQ_DECLARE_FLAGS( UseFlags)
    };

    //! Add ArnLib support to a Qml instance
    /*! ArnLib module is always included.
     *  \param[in] qmlEngine is the qml instance engine
     *  \param[in] flags gives the modules to include
     */
    static void  setup( QML_ENGINE* qmlEngine, UseFlags flags = UseFlags::ArnLib);

    static ArnQml&  instance();

    //! Gives current ARN root path for all qml instances.
    /*!
     *  \return the root path
     *  \see setArnRootPath
     */
    static QString  arnRootPath();

    //! Change ARN root path for all qml instances.
    /*! This is set once in the application and must be set before any qml instances are
     *  setup.
     *
     *  Example:
     *  setArnRootPath("/@myHost/");
     *  will map a path "/Test/value" in Qml to an ARN object at path "/@myHost/Test/value".
     *
     *  \param[in] path is the root path
     *  \see arnRootPath
     */
    static void  setArnRootPath( const QString& path);

//! \cond ADV
    static QObject *constructorArnInterface( QML_ENGINE* engine, QJSEngine* scriptEngine);
//! \endcond

private:
    /// Private constructor/destructor to keep this class singleton
    ArnQml();
    ArnQml( const ArnQml&);
    ~ArnQml();
    ArnQml&  operator=( const ArnQml&);

    QString  _arnRootPath;
    ArnNetworkAccessManagerFactory*  _arnNetworkAccessManagerFactory;
    UseFlags  _regedUse;
};

MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnQml::UseFlags)


//! ARN Item QML.
/*!
This class is the Qml version of ArnItem.

\see ArnQml

<b>Example usage</b> \n \code
// In Qml
//
import QtQuick 2.0
import ArnLib 1.0

Rectangle {
    width: 370;  height: 400

    property ArnItem arnT1: ArnItem {path: "//El/UpdClock/value"}

    ArnItem {id: arnElUpdClock;  path: "//El/UpdClock/value"}
    ArnItem {id: arnTest;        path: "//Test/test"}

    Rectangle {
        id: info
        anchors.bottom: parent.bottom; anchors.left: parent.left;  anchors.right: parent.right
        height: 80
        Column {
            anchors.fill: parent;
            Text {text: "El updClock 1: " + arnElUpdClock.intNum}
            Text {text: "El updClock 2: " + arnT1.intNum}
        }
    }

    Component.onCompleted: {
        arnTest.setValue("Start ...", Arn.SameValue_Accept);
    }
}
\endcode
*/
class  ArnItemQml : public ArnItem, public QML_PARSER_STATUS
{
    Q_OBJECT

#ifdef QML_Qt4
    Q_INTERFACES( QDeclarativeParserStatus)
#else
    Q_INTERFACES( QQmlParserStatus)
#endif

    //! The type used inside the variant, e.g. QString
    Q_PROPERTY( QString variantType  READ variantType        WRITE setVariantType      NOTIFY variantTypeChanged)
    //! Select to use ArnItem::openUuid()
    Q_PROPERTY( bool useUuid         READ useUuid            WRITE setUseUuid          NOTIFY dummyNotifier)
    //! The path of this ArnItem
    Q_PROPERTY( QString path         READ path               WRITE setPath             NOTIFY pathChanged)
    //! The Arn data type of this ArnItem
    Q_PROPERTY( ArnInterface::DataType  type
                                     READ type                                         NOTIFY valueChanged)
    //! The ArnItem value as a QVariant
    Q_PROPERTY( QVariant variant     READ toVariant          WRITE setVariant          NOTIFY valueChanged)
    //! The ArnItem value as a QString
    Q_PROPERTY( QString string       READ toString           WRITE setValue            NOTIFY valueChanged)
    //! The ArnItem value as a QByteArray
    Q_PROPERTY( QByteArray bytes     READ toByteArray        WRITE setValue            NOTIFY valueChanged)
    //! The ArnItem value as an ARNREAL
#ifdef ARNREAL_FLOAT
    Q_PROPERTY( float num            READ toReal             WRITE setValue            NOTIFY valueChanged)
#else
    Q_PROPERTY( double num           READ toReal             WRITE setValue            NOTIFY valueChanged)
#endif
    //! The ArnItem value as an int
    Q_PROPERTY( int intNum           READ toInt              WRITE setValue            NOTIFY valueChanged)
    //! See Arn::ObjectMode::BiDir
    Q_PROPERTY( bool biDirMode       READ isBiDirMode        WRITE setBiDirMode        NOTIFY dummyNotifier)
    //! See Arn::ObjectMode::Pipe
    Q_PROPERTY( bool pipeMode        READ isPipeMode         WRITE setPipeMode         NOTIFY dummyNotifier)
    //! See Arn::ObjectMode::Save
    Q_PROPERTY( bool saveMode        READ isSaveMode         WRITE setSaveMode         NOTIFY dummyNotifier)
    //! See Arn::ObjectSyncMode::Master
    Q_PROPERTY( bool masterMode      READ isMaster           WRITE setMaster           NOTIFY dummyNotifier)
    //! See Arn::ObjectSyncMode::AutoDestroy
    Q_PROPERTY( bool autoDestroyMode READ isAutoDestroy      WRITE setAutoDestroy      NOTIFY dummyNotifier)
    //! See ArnBasicItem::setAtomicOpProvider()
    Q_PROPERTY( bool atomicOpProvider
                                     READ isAtomicOpProvider WRITE setAtomicOpProvider NOTIFY dummyNotifier)
    //! See ArnItem::setIgnoreSameValue()
    Q_PROPERTY( bool ignoreSameValue READ isIgnoreSameValue  WRITE setIgnoreSameValue  NOTIFY dummyNotifier)
    //! See ArnItem::setDelay()
    Q_PROPERTY( int delay            READ delay              WRITE setDelay            NOTIFY dummyNotifier)
    // Q_PROPERTY( bool smTemplate     READ isTemplate    WRITE setTemplate)

public slots:
    //! AtomicOp assign an _integer_ to specified bits in an _Arn Data Object_
    /*! \see ArnItem::setBits()
    */
    void  setBits( int mask, int value)
    {ArnItemB::setBits( mask, value);}

    //! AtomicOp adds an _integer_ to an _Arn Data Object_
    /*! \see ArnItem::addValue()
    */
    void  addIntNum( int value)
    {ArnItemB::addValue( value);}

    //! AtomicOp adds an _ARNREAL_ to an _Arn Data Object_
    /*! \see ArnItem::addValue()
    */
#ifdef ARNREAL_FLOAT
    void  addNum( float value)
#else
    void  addNum( double value)
#endif
    {ArnItemB::addValue( value);}

    //! Add _general mode_ settings for this _Arn Data Object_
    /*! \see ArnItem::addMode()
    */
    void  addMode( ArnInterface::ObjectMode mode)
    {ArnItemB::addMode( Arn::ObjectMode::fromInt( mode));}

    /*! \return The _general mode_ of the _Arn Data Object_
    *  \see ArnItem::getMode()
    */
    ArnInterface::ObjectMode  getMode()  const
    {return ArnInterface::ObjectMode( ArnItemB::getMode().toInt());}

//! \cond ADV
public:
    explicit ArnItemQml( QObject* parent = 0);

    QString  variantType()  const;
    void  setVariantType( const QString& typeName);

    QString  path()  const;

    ArnInterface::DataType  type()  const
    {return ArnInterface::DataType( ArnItem::type().toInt());}

    void  setPath( const QString& path);

    void  setVariant( const QVariant& value);

    void  setBiDirMode( bool isBiDirMode);
    void  setPipeMode( bool isPipeMode);
    void  setMaster( bool isMaster);
    void  setAutoDestroy( bool isAutoDestroy);
    void  setSaveMode( bool isSaveMode);
    void  setAtomicOpProvider( bool isAtomicOpPv);
    // bool  isTemplate() const;
    // void  setTemplate( bool isTemplate);

    bool  useUuid()  const;
    void  setUseUuid( bool useUuid);

    virtual void  classBegin();
    virtual void  componentComplete();

signals:
    void  valueChanged();
    void  pathChanged();
    void  variantTypeChanged();
    void  dummyNotifier();

protected:
    virtual void  itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value = 0);
    virtual void  itemCreatedBelow( const QString& path);
    virtual void  itemModeChangedBelow( const QString& path, uint linkId, Arn::ObjectMode mode);
    virtual void  timerEvent( QTimerEvent* ev);
//! \endcond

private:
    bool  _isCompleted;
    QString  _path;
    int  _variantType;
    bool  _useUuid;
};


//! ARN Monitor QML.
/*!
This class is the Qml version of the ArnMonitor.

\see ArnQml

<b>Example usage</b> \n \code
// In Qml
//
import QtQuick 2.0
import ArnLib 1.0

Rectangle {
    width: 370;  height: 400

    ArnMonitor {
        clientId: "std"
        monitorPath: "//Test/List/"
        onArnChildFound: console.log("Found list item: " + path);
    }
}
\endcode
*/
class ArnMonitorQml : public ArnMonitor, public QML_PARSER_STATUS
{
    Q_OBJECT

#ifdef QML_Qt4
    Q_INTERFACES( QDeclarativeParserStatus)
#else
    Q_INTERFACES( QQmlParserStatus)
#endif

    //! The client id. Set whith ArnClient::registerClient(). Use "std" if not set.
    Q_PROPERTY( QString clientId     READ clientId     WRITE setClientId     NOTIFY dummyNotifier)
    //! The path to be monitored at the server.
    Q_PROPERTY( QString monitorPath  READ monitorPath  WRITE setMonitorPath  NOTIFY pathChanged)
public slots:
    //! Restart the monitor
    /*! All signals for found childs will be emitted again.
     */
    void  reStart();

//! \cond ADV
public:
    explicit ArnMonitorQml( QObject* parent = 0);

    void  setClientId( const QString& id);
    QString  clientId() const;
    void  setMonitorPath( const QString& path);
    QString  monitorPath() const;

    virtual void classBegin();
    virtual void componentComplete();

signals:
    void  pathChanged();
    void  dummyNotifier();

protected:
    virtual QString  outPathConvert( const QString& path);
    virtual QString  inPathConvert( const QString& path);
//! \endcond

private:
    bool  _isCompleted;
    QString  _path;
    QString  _clientId;
};


//! ARN Sapi QML.
/*!
This class is the Qml version of the ArnSapi.

\see ArnQml

<b>Example usage</b> \n \code
// In Qml
//
import QtQuick 2.0
import ArnLib 1.0

Rectangle {
    width: 370;  height: 400

    Item {
        id: sapiTest
        ArnSapi {
            pipePath: "//Test/pipe"
            mode: ArnSapi.NamedArg
        }

        // Provider API
        signal pv_readFileTest( string fileName)

        // Requester API
        signal rq_test2( string par1)
        function rq_test( p1) {
            console.log("rq_test: p1=" + p1);
        }

        Component.onCompleted: {
            sapiTest.rq_test2.connect( info.setTestMsg);
            sapiTest.pv_readFileTest("myfile");
        }
    }

    Rectangle {
        id: info
        property string testMsg: ""
        anchors.bottom: parent.bottom; anchors.left: parent.left;  anchors.right: parent.right
        height: 80
        Column {
            anchors.fill: parent;
            Text {text: "Msg: " + info.testMsg}
            Text {text: Arn.info}  // ArnLib version info
        }

        function setTestMsg( msg) {
            info.testMsg = msg;
        }
    }
}
\endcode
*/
class ArnSapiQml : public ArnRpc, public QML_PARSER_STATUS
{
    Q_OBJECT

#ifdef QML_Qt4
    Q_INTERFACES( QDeclarativeParserStatus)
#else
    Q_INTERFACES( QQmlParserStatus)
#endif

    //! Path of the pipe for this Sapi
    Q_PROPERTY( QString pipePath    READ pipePath           WRITE setPipePath        NOTIFY pathChanged)
    //! Sapi modes
    Q_PROPERTY( Mode mode           READ mode               WRITE setMode            NOTIFY dummyNotifier)
    //! The receiving object of incomming Sapi calls. Default: parent
    Q_PROPERTY( QObject* receiver   READ receiver           WRITE setReceiver        NOTIFY dummyNotifier)

    //! Period time for sending heart beat message
    /*! \see ArnRpc::setHeartBeatSend()
     */
    Q_PROPERTY( int heartBeatSend   READ getHeartBeatSend   WRITE setHeartBeatSend   NOTIFY dummyNotifier)

    //! Max time period for receiving heart beat message
    /*! \see ArnRpc::setHeartBeatCheck()
     */
    Q_PROPERTY( int heartBeatCheck  READ getHeartBeatCheck  WRITE setHeartBeatCheck  NOTIFY dummyNotifier)
public:
    enum Mode {
        //! Provider side (opposed to requester)
        Provider       = ArnRpc::Mode::Provider,
        //! Use _AutoDestroy_ for the pipe, i.e. it is closed when tcp/ip is broken
        AutoDestroy    = ArnRpc::Mode::AutoDestroy,
        //! Use an unique uuid in the pipe name
        UuidPipe       = ArnRpc::Mode::UuidPipe,
        //! If guarantied no default arguments, full member name overload is ok
        NoDefaultArgs  = ArnRpc::Mode::NoDefaultArgs,
        //! Send sequence order information to pipe
        SendSequence   = ArnRpc::Mode::SendSequence,
        //! Check sequence order information from pipe. Can generate signal outOfSequence().
        CheckSequence  = ArnRpc::Mode::CheckSequence,
        //! When calling out, uses named argument e.g "myFunc count=123"
        NamedArg       = ArnRpc::Mode::NamedArg,
        //! When calling out, uses named argument with type e.g "myFunc count:int=123"
        NamedTypedArg  = ArnRpc::Mode::NamedTypedArg,
        //! When receiver method missing, send defaultCall() signal instead of error
        UseDefaultCall = ArnRpc::Mode::UseDefaultCall,
        //! Convenience, combined _UuidPipe_ and _AutoDestroy_
        UuidAutoDestroy = int(UuidPipe) | int(AutoDestroy)
    };
    Q_ENUMS( Mode)

public slots:
    bool  isHeartBeatOk()
    {return ArnRpc::isHeartBeatOk();}

//! \cond ADV
public:
    explicit ArnSapiQml( QObject* parent = 0);

    void  setPipePath( const QString& path);
    QString  pipePath() const;

    Mode  mode()  const
    {return Mode( ArnRpc::mode().toInt());}
    void  setMode( Mode m)
    {ArnRpc::setMode( ArnRpc::Mode::fromInt(m));}

    virtual void classBegin();
    virtual void componentComplete();

signals:
    void  pathChanged();
    void  dummyNotifier();
//! \endcond

private:
    bool  _isCompleted;
    QString  _path;
    QString  _sendPrefix;
    QString  _providerPrefix;
    QString  _requesterPrefix;
};


namespace Arn {

class  XStringMapQml : public QObject, public QML_PARSER_STATUS, public XStringMap
{
    Q_OBJECT

#ifdef QML_Qt4
    Q_INTERFACES( QDeclarativeParserStatus)
#else
    Q_INTERFACES( QQmlParserStatus)
#endif

    //! The map serialized as xstring
    Q_PROPERTY( QString xstring      READ toXStringString    WRITE fromXString         NOTIFY dummyNotifier)
    //! Number of items
    Q_PROPERTY( int size             READ size                                         NOTIFY dummyNotifier)

    // XStringMap&  addNum( const QString& key, int val);
    // XStringMap&  addNum( const QString& key, uint val);
    // XStringMap&  addNum( const QString& key, double val, int precision = -1);
    // XStringMap&  operator+=( const XStringMap& other);
    // XStringMap&  operator+=( const QVariantMap& other);
    // QByteArray  info();

public slots:
    //! Clear and free mem
    /*!
    */
    void  clear()
    {XStringMap::clear( true);}

    int  indexOf( const QString& key, int from = 0)  const
    {return XStringMap::indexOf( key, from);}

    int  indexOfValue( const QString& value, int from = 0)  const
    {return XStringMap::indexOfValue( value, from);}

    QObject*  add( const QString& key, const QString& val)
    {XStringMap::add( key, val); return this;}

    QObject*  add( QObject* other);

    QObject*  set( int i, const QString& val)
    {XStringMap::set( i, val); return this;}

    QObject*  set( const QString& key, const QString& val)
    {XStringMap::set( key, val); return this;}

    QString  key( int i, const QString& def = QString())  const
    {return XStringMap::keyString( i, def);}

    QString  key( const QString& value, const QString& def = QString())  const
    {return XStringMap::keyString( value, def);}

    QString  value( int i, const QString& def = QString())  const
    {return XStringMap::valueString( i, def);}

    QString  value( const QString& key, const QString& def = QString())  const
    {return XStringMap::valueString( key, def);}

    QObject*  remove( int index)
    {XStringMap::remove( index); return this;}

    QObject*  remove( const QString& key)
    {XStringMap::remove( key); return this;}

    QObject*  removeValue( const QString& val)
    {XStringMap::removeValue( val); return this;}

    void  setEmptyKeysToValue()
    {XStringMap::setEmptyKeysToValue();}

    QStringList  keys()  const
    {return XStringMap::keys();}

    QStringList  values()  const
    {return XStringMap::values();}

    MQVariantMap  toMap()  const
    {return XStringMap::toVariantMap( true);}

//! \cond ADV
public:
    explicit XStringMapQml( QObject* parent = 0);

    virtual void  classBegin();
    virtual void  componentComplete();

signals:
    void  dummyNotifier();

protected:
//! \endcond

private:
    bool  _isCompleted;
};


class QmlMSys : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY( int quickTypeRun  READ quickTypeRun)

public slots:
    QVariantMap  xstringToEnum( const QString& xstring);

//! \cond ADV
public:
    explicit QmlMSys( QObject* parent = 0);

    int  quickTypeRun();
//! \endcond
};
}


//! \cond ADV
class ArnNetworkReply : public QNetworkReply
{
    Q_OBJECT
public:
    explicit ArnNetworkReply( QObject* parent = 0);

    void  setOperation( QNetworkAccessManager::Operation operation)
    { QNetworkReply::setOperation( operation);}

    void  setRequest( const QNetworkRequest& request)
    { QNetworkReply::setRequest( request);}

    void  setUrl( const QUrl& url)
    { QNetworkReply::setUrl( url);}

    bool  isFinished()  const;
    qint64  bytesAvailable()  const;
    bool  isSequential()  const;
    qint64  size()  const;

    void  setup( const QString& path);
    QByteArray  data()  const;
    void  setData( const QByteArray& data);

protected:
    virtual void  abort();
    virtual qint64  readData( char *data, qint64 maxlen);

private slots:
    void  postSetup();
    void  dataArived();

private:
    QString  _arnPath;
    QByteArray  _data;
    int  _readPos;
    bool  _isFinished;
    ArnItem  _arnItem;
};


class ArnNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit ArnNetworkAccessManager( QObject* parent = 0);

protected:
    virtual QNetworkReply*  createRequest( Operation op, const QNetworkRequest &request, QIODevice *outgoingData);

private slots:

private:
};


class ArnNetworkAccessManagerFactory : public QML_NETACC_FACTORY
{
public:
    virtual QNetworkAccessManager*  create( QObject *parent);
};
//! \endcond

#endif // ARNQML_HPP
