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

#include "ArnInc/ArnQml.hpp"
#include "ArnInc/ArnQmlMSystem.hpp"
#include "ArnInc/ArnQmlMQt.hpp"
#include "ArnInc/ArnInterface.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/ArnLib.hpp"
#include <QTimerEvent>
#include <QThread>
#include <QDebug>

using namespace Arn;


ArnQml::ArnQml()
    : QObject(0)
{
    _arnRootPath = "/";
    _arnNetworkAccessManagerFactory = new ArnNetworkAccessManagerFactory;
}

ArnQml::~ArnQml()
{
    delete _arnNetworkAccessManagerFactory;
}


QString  ArnQml::arnRootPath()
{
    return instance()._arnRootPath;
}


void  ArnQml::setArnRootPath( const QString& path)
{
    instance()._arnRootPath = path;
}


QObject*  ArnQml::constructorArnInterface( QML_ENGINE* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    ArnInterface *arnIterface = new ArnInterface();
    return arnIterface;
}


void  ArnQml::setup( QML_ENGINE* qmlEngine, ArnQml::UseFlags flags)
{
    ArnQml&  in = ArnQml::instance();
    flags.set( flags.ArnLib);  // Always include ArnLib

    if (flags.is( flags.ArnLib) && !in._regedUse.is( flags.ArnLib)) {
        in._regedUse.set( flags.ArnLib);
        qmlRegisterType<ArnItemQml>(   "ArnLib", 1, 0, "ArnItem");
        qmlRegisterType<ArnMonitorQml>("ArnLib", 1, 0, "ArnMonitor");
        qmlRegisterType<ArnSapiQml>(   "ArnLib", 1, 0, "ArnSapi");
#ifdef QML_Qt4
        qmlRegisterType<ArnInterface>( "ArnLib", 1, 0, "Arn");
#else
        qmlRegisterSingletonType<ArnInterface>("ArnLib", 1, 0, "Arn", constructorArnInterface);
#endif
    }
    if (flags.is( flags.MSystem) && !in._regedUse.is( flags.MSystem)) {
        in._regedUse.set( flags.MSystem);
        qmlRegisterType<QmlMFileIO>("MSystem", 1, 0, "MFileIO");
    }
    if (flags.is( flags.MQt) && !in._regedUse.is( flags.MQt)) {
        in._regedUse.set( flags.MQt);
        qmlRegisterType<QmlMQtObject>("MQtQml", 1, 0, "MQtObject");
    }

    if (qmlEngine) {
        qmlEngine->setNetworkAccessManagerFactory( in._arnNetworkAccessManagerFactory);

        // For compatibility to Qt4 QML
        qmlEngine->rootContext()->setContextProperty("arn", new ArnInterface( qmlEngine));

        qmlEngine->rootContext()->setContextProperty("mSys", new QmlMSys( qmlEngine));
    }
}


ArnQml&  ArnQml::instance()
{
    static ArnQml  ins;

    return ins;
}


///////// ArnItemQml

ArnItemQml::ArnItemQml( QObject* parent)
    : ArnItem( parent)
{
    _isCompleted = false;
    _useUuid     = false;

    setIgnoreSameValue( true);
}


QString  ArnItemQml::variantType()  const
{
    if (!_variantType)  return QString();

    const char*  typeName = QMetaType::typeName(_variantType);
    if (!typeName)  return QString();

    return typeName;
}


void  ArnItemQml::setVariantType( const QString& typeName)
{
    if (typeName.isEmpty()) {
        _variantType = 0;
    }
    else {
        int  type = QMetaType::type( typeName.toLatin1().constData());
        if (!type) {
            qWarning() << "ItemQml setVariantType, Unknown: type=" + typeName + " path=" + path();
            return;
        }

        _variantType = type;
    }

    emit variantTypeChanged();
}


QString  ArnItemQml::path()  const
{
    return _path;
}


void  ArnItemQml::setPath( const QString& path)
{
    _path = path;
    if (_isCompleted) {
        QString  arnPath = Arn::changeBasePath("/", ArnQml::arnRootPath(), path);
        if (_useUuid)
            openUuid( arnPath);
        else
            open( arnPath);

        if (ArnItem::type() != Arn::DataType::Null)  // Value already present ...
            emit valueChanged();
    }

    emit pathChanged();
}


void  ArnItemQml::setVariant( const QVariant& value)
{
    if (!_variantType)  // No variantType, no conversion
        ArnItem::setValue( value);
    else {  // Use variantType
        QVariant  val = value;
        if (val.convert( QVariant::Type( _variantType))) {
            ArnItem::setValue( val);
        }
        else {
            qWarning() << "ItemQml setVariant, Can't convert: type="
                       << _variantType  << " path=" + path();
        }
    }
}


void  ArnItemQml::setBiDirMode( bool isBiDirMode)
{
    if (isBiDirMode)
        ArnItem::setBiDirMode();
}


void  ArnItemQml::setPipeMode( bool isPipeMode)
{
    if (isPipeMode)
        ArnItem::setPipeMode();
}


void  ArnItemQml::setMaster( bool isMaster)
{
    if (isMaster)
        ArnItem::setMaster();
}


void  ArnItemQml::setAutoDestroy( bool isAutoDestroy)
{
    if (isAutoDestroy)
        ArnItem::setAutoDestroy();
}


void  ArnItemQml::setSaveMode( bool isSaveMode)
{
    if (isSaveMode)
        ArnItem::setSaveMode();
}


bool  ArnItemQml::useUuid()  const
{
    return _useUuid;
}


void  ArnItemQml::setUseUuid( bool useUuid)
{
    _useUuid = useUuid;
}


void  ArnItemQml::classBegin()
{
}


void  ArnItemQml::componentComplete()
{
    _isCompleted = true;
    if (!_path.isEmpty())
        setPath( _path);
}


void  ArnItemQml::itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value)
{
    ArnItem::itemUpdated( handleData, value);

    if (delayTimerId() == 0)  // No delay timer used
        emit valueChanged();
}


void  ArnItemQml::itemCreatedBelow( const QString& path)
{
    QString  qmlPath = Arn::changeBasePath( ArnQml::arnRootPath(), "/", path);
    emit arnItemCreated( qmlPath);
}


void  ArnItemQml::itemModeChangedBelow( const QString& path, uint linkId, ObjectMode mode)
{
    QString  qmlPath = Arn::changeBasePath( ArnQml::arnRootPath(), "/", path);
    emit arnModeChanged( qmlPath, linkId, mode);
}


void  ArnItemQml::timerEvent( QTimerEvent* ev)
{
    if (ev->timerId() == delayTimerId()) {
        // qDebug() << "ArnItemQml delay doUpdate: path=" << path();
        emit valueChanged();
    }

    return ArnItem::timerEvent( ev);
}


///////// ArnMonitorQml

ArnMonitorQml::ArnMonitorQml( QObject* parent)
    : ArnMonitor( parent)
{
    _isCompleted = false;
}


void ArnMonitorQml::reStart()
{
    ArnMonitor::reStart();
}


void  ArnMonitorQml::setClientId( const QString& id)
{
    _clientId = id;
    setClient( id);
}


QString  ArnMonitorQml::clientId()  const
{
    return _clientId;
}


void  ArnMonitorQml::setMonitorPath( const QString& path)
{
    _path = path;
    if (_isCompleted) {
        this->start( path);
    }

    emit pathChanged();
}


QString  ArnMonitorQml::monitorPath()  const
{
    return _path;
}


void ArnMonitorQml::classBegin()
{
}


void ArnMonitorQml::componentComplete()
{
    _isCompleted = true;
    if (!_path.isEmpty())
        setMonitorPath( _path);
}


QString ArnMonitorQml::outPathConvert(const QString& path)
{
    return Arn::changeBasePath( ArnQml::arnRootPath(), "/", path);
}


QString ArnMonitorQml::inPathConvert(const QString& path)
{
    return Arn::changeBasePath("/", ArnQml::arnRootPath(), path);
}


///////// ArnSapiQml

ArnSapiQml::ArnSapiQml( QObject* parent)
    : ArnRpc( parent)
{
    _isCompleted     = false;
    _providerPrefix  = "pv_";
    _requesterPrefix = "rq_";
}


void  ArnSapiQml::setPipePath( const QString& path)
{
    _path = path;

    if (_isCompleted) {
        setConvVariantPar( true);
        if (!receiver())
            ArnRpc::setReceiver( this->parent(), false);

        QString  receivePrefix;
        if (ArnRpc::mode().is( ArnRpc::Mode::Provider)) {
            receivePrefix = _providerPrefix;
            _sendPrefix   = _requesterPrefix;
        }
        else {
            receivePrefix = _requesterPrefix;
            _sendPrefix   = _providerPrefix;
        }
        ArnRpc::setMethodPrefix( receivePrefix);
        ArnRpc::addSenderSignals( ArnRpc::receiver(), _sendPrefix);

        QString  arnPath = Arn::changeBasePath("/", ArnQml::arnRootPath(), path);
        open( arnPath);
    }

    emit pathChanged();
}


QString  ArnSapiQml::pipePath()  const
{
    return _path;
}


void  ArnSapiQml::classBegin()
{
}


void  ArnSapiQml::componentComplete()
{
    _isCompleted = true;
    if (!_path.isEmpty())
        setPipePath( _path);
}



///////// QmlMSys

namespace Arn {

QmlMSys::QmlMSys( QObject* parent)
    : QObject( parent)
{
}


int QmlMSys::quickTypeRun()
{
    return QML_QUICK_TYPE;
}
}


///////// ArnNetworkReply

ArnNetworkReply::ArnNetworkReply( QObject* parent)
    : QNetworkReply( parent)
{
    _readPos    = 0;
    _isFinished = false;
}


bool  ArnNetworkReply::isFinished()  const
{
    return _isFinished;
}


qint64  ArnNetworkReply::bytesAvailable()  const
{
    return QNetworkReply::bytesAvailable() + _data.size() - _readPos;
}


bool  ArnNetworkReply::isSequential()  const
{
    return true;
}


qint64 ArnNetworkReply::size() const
{
    return _data.size();
}


qint64  ArnNetworkReply::readData( char* data, qint64 maxlen)
{
    int  len = qMin( _data.size() - _readPos, int(maxlen));
    if (len > 0) {
        memcpy( data, _data.constData() + _readPos, size_t( len));
        _readPos += len;
    }

    if ((len == 0) && (bytesAvailable() == 0)) {
        return -1;  // Everything has been read
    }

    return len;
}


void  ArnNetworkReply::abort()
{
    close();
}


QByteArray  ArnNetworkReply::data()  const
{
    return _data;
}


void  ArnNetworkReply::setup( const QString& path)
{
    _arnPath    = path;
    _isFinished = false;

    QByteArray  arnVal;
    if (path.endsWith("/qmldir"))
        arnVal = "";
    else {
        //// Normal values from Arn
        if (Arn::debugQmlNetwork)  qDebug() << "ArnQmlNetw. reply setup later path="
                                            << path;
        QMetaObject::invokeMethod( this, "postSetup", Qt::QueuedConnection);
        return;
    }

    //// Direct return of special values
    if (Arn::debugQmlNetwork)  qDebug() << "ArnQmlNetw. reply setup now path="
                                        << path;
    setData( arnVal);
}


void  ArnNetworkReply::postSetup()
{
    if (Arn::debugQmlNetwork)  qDebug() << "ArnQmlNetw. reply postSetup path="
                                        << _arnPath;
    bool  wasLocalExist = ArnM::exist( _arnPath);
    if (Arn::debugQmlNetwork)  qDebug() << "ArnQmlNetw. reply postSetup 2";

    _arnItem.open( _arnPath);
    if (Arn::debugQmlNetwork)  qDebug() << "ArnQmlNetw. reply postSetup 3";

    if (!wasLocalExist && (_arnItem.type() == Arn::DataType::Null)) {
        // Note: Open can have resulted in a persistent loading of the item.
        if (Arn::debugQmlNetwork)  qDebug() << "ArnQmlNetw. wait for data: path=" << _arnPath;
        connect( &_arnItem, SIGNAL(changed()), this, SLOT(dataArived()));
        return;
    }

    if (Arn::debugQmlNetwork)  qDebug() << "ArnQmlNetw. direct using data: path=" << _arnPath;
    setData( _arnItem.toByteArray());
}


void  ArnNetworkReply::dataArived()
{
    disconnect( &_arnItem, SIGNAL(changed()), this, SLOT(dataArived()));

    if (Arn::debugQmlNetwork)  qDebug() << "ArnQmlNetw. data arived: path=" << _arnPath;
    setData( _arnItem.toByteArray());
}


void  ArnNetworkReply::setData( const QByteArray& data)
{
    _data       = data;
    _readPos    = 0;
    _isFinished = true;
    open( ReadOnly);

#ifndef QML_Qt4
    int posStart = _data.indexOf("import QtQuick 1.");
    if (posStart >= 0) {
        int posEnd = _data.indexOf('\n', posStart);
        if (posEnd >= 0) {
            _data.replace( posStart, posEnd - posStart, "import QtQuick 2.3");
        }
    }
#endif

    QMetaObject::invokeMethod (this, "downloadProgress", Qt::QueuedConnection,
                               Q_ARG( qint64, _data.size()),
                               Q_ARG( qint64, _data.size()));
    QMetaObject::invokeMethod( this, "readyRead", Qt::QueuedConnection);
    QMetaObject::invokeMethod( this, "finished", Qt::QueuedConnection);
}



///////// ArnNetworkAccessManager

ArnNetworkAccessManager::ArnNetworkAccessManager( QObject* parent)
    : QNetworkAccessManager( parent)
{
}


QNetworkReply*  ArnNetworkAccessManager::createRequest( QNetworkAccessManager::Operation op,
                                                        const QNetworkRequest& request,
                                                        QIODevice* outgoingData)
{
    QUrl  url = request.url();
    if (url.scheme() != "arn")
        return QNetworkAccessManager::createRequest( op, request, outgoingData);

    if (Arn::debugQmlNetwork)  qDebug() << "ArnQmlNetw. Create request url="
                                        << url.toString();
    ArnNetworkReply*  reply = new ArnNetworkReply;
    // QNetworkRequest  mRequest = request;
    // mRequest.setAttribute( QNetworkRequest::CacheSaveControlAttribute, false);
    // mRequest.setAttribute( QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    reply->setRequest( request);
    reply->setOperation( op);
    reply->setUrl( url);

    switch (op) {
    case GetOperation:
    {
        QString  path = url.path();
        QString  arnPath = Arn::changeBasePath("/", ArnQml::arnRootPath(), path);
        arnPath = Arn::convertPath( arnPath, Arn::NameF::EmptyOk);
        reply->setup( arnPath);
        break;
    }
    default:
        qWarning() << "ArnNetworkAccessManager: Operation not supported op=" << op;
        break;
    }

    return reply;
}


///////// ArnNetworkAccessManagerFactory

QNetworkAccessManager*  ArnNetworkAccessManagerFactory::create( QObject* parent)
{
    return new ArnNetworkAccessManager( parent);
}
