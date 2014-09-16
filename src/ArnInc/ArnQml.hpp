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

#ifndef ARNQML_HPP
#define ARNQML_HPP

#include "ArnLib_global.hpp"
#include "ArnItem.hpp"
#include "ArnMonitor.hpp"
#include "ArnRpc.hpp"
#include <QQmlParserStatus>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QQmlNetworkAccessManagerFactory>

class QQmlEngine;
class ArnNetworkAccessManagerFactory;


//! ARN QML.
/*!
This class is the central point for ArnQml.
It's a singleton that is setup in the application.
ArnQml can be used for creating GUI-applications in Qml that has integrated access to the
ARN objects and some of the ArnLib funtionality.

ArnBrowser is using this to run Qml applications in an opaque style, i.e. without specific
application support. This resembles somewhat a web browser running a web application.

Note that you must not use empty folder in QUrl for an Arn path.
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

    property ArnItem arnT1: ArnItem {path: "//El/UpdClock/value"}

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
            Text {text: "El updClock: " + arnElUpdClock.intNum + ", " + arnT1.intNum}
            Text {text: "Msg: " + info.testMsg}
            Text {text: arn.info}  // ArnLib version info
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
    static void  setup( QQmlEngine* qmlEngine, UseFlags flags = UseFlags::ArnLib);

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


class  ArnItemQml : public ArnItem, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY( QString variantType  READ variantType   WRITE setVariantType  NOTIFY variantTypeChanged)
    Q_PROPERTY( QString path         READ path          WRITE setPath         NOTIFY pathChanged)
    Q_PROPERTY( QVariant variant     READ toVariant     WRITE setVariant      NOTIFY valueChanged)
    Q_PROPERTY( QString string       READ toString      WRITE setValue        NOTIFY valueChanged)
    Q_PROPERTY( QByteArray bytes     READ toByteArray   WRITE setValue        NOTIFY valueChanged)
    Q_PROPERTY( double num           READ toDouble      WRITE setValue        NOTIFY valueChanged)
    Q_PROPERTY( int intNum           READ toInt         WRITE setValue        NOTIFY valueChanged)
    Q_PROPERTY( bool pipeMode        READ isPipeMode    WRITE setPipeMode     NOTIFY dummyNotifier)
    Q_PROPERTY( bool saveMode        READ isSaveMode    WRITE setSaveMode     NOTIFY dummyNotifier)
    Q_PROPERTY( bool masterMode      READ isMaster      WRITE setMaster       NOTIFY dummyNotifier)
    Q_PROPERTY( bool autoDestroyMode READ isAutoDestroy WRITE setAutoDestroy  NOTIFY dummyNotifier)
    // Q_PROPERTY( bool smTemplate     READ isTemplate    WRITE setTemplate)

public:
    explicit ArnItemQml( QObject* parent = 0);

    QString  variantType()  const;
    void  setVariantType( const QString& typeName);

    QString  path()  const;

    void  setPath( const QString& path);

    void  setVariant( const QVariant& value);

    void  setPipeMode( bool isPipeMode);
    void  setMaster( bool isMaster);
    void  setAutoDestroy( bool isAutoDestroy);
    void  setSaveMode( bool isSaveMode);
    // bool  isTemplate() const;
    // void  setTemplate( bool isTemplate);

    virtual void classBegin();
    virtual void componentComplete();

signals:
    void  valueChanged();
    void  pathChanged();
    void  variantTypeChanged();
    void  dummyNotifier();

protected:
    virtual void  itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value = 0);
    virtual void  itemCreatedBelow( QString path);
    virtual void  itemModeChangedBelow( QString path, uint linkId, Arn::ObjectMode mode);

private:
    bool  _isCompleted;
    QString  _path;
    int  _valueType;
};


class ArnMonitorQml : public ArnMonitor, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY( QString clientId     READ clientId     WRITE setClientId     NOTIFY dummyNotifier)
    Q_PROPERTY( QString monitorPath  READ monitorPath  WRITE setMonitorPath  NOTIFY pathChanged)
public slots:
    void  reStart();

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

private:
    bool  _isCompleted;
    QString  _path;
    QString  _clientId;
};


class ArnSapiQml : public ArnRpc, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY( QString pipePath   READ pipePath    WRITE setPipePath    NOTIFY pathChanged)
    Q_PROPERTY( bool isProvider    READ isProvider  WRITE setIsProvider  NOTIFY dummyNotifier)
    Q_PROPERTY( QObject* receiver  READ receiver    WRITE setReceiver    NOTIFY dummyNotifier)
public slots:

public:
    explicit ArnSapiQml( QObject* parent = 0);

    void  setPipePath( const QString& path);
    QString  pipePath() const;

    bool  isProvider()  const;
    void  setIsProvider( bool isProvider);

    virtual void classBegin();
    virtual void componentComplete();

signals:
    void  pathChanged();
    void  dummyNotifier();

protected:

private:
    bool  _isCompleted;
    QString  _path;
    QString  _sendPrefix;
    QString  _providerPrefix;
    QString  _requesterPrefix;
    bool  _isProvider;
};


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



class ArnNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    virtual QNetworkAccessManager*  create( QObject *parent);
};
//! \endcond

#endif // ARNQML_HPP
