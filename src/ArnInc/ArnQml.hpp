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
#include <QQmlParserStatus>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QQmlNetworkAccessManagerFactory>
#include <QMap>

class QQmlEngine;
class ArnNetworkAccessManagerFactory;


class ARNLIBSHARED_EXPORT ArnQml : public QObject
{
    Q_OBJECT
public:
    struct UseFlags {
        enum E {
            //!
            ArnLib  = 0x01,
            //!
            MSystem = 0x02
        };
        MQ_DECLARE_FLAGS( UseFlags)
    };

    static void  setup( QQmlEngine* qmlEngine, UseFlags flags = UseFlags::ArnLib);

    static ArnQml&  instance();

    static QString  arnRootPath();
    static void  setArnRootPath( const QString& path);

    static QByteArray  arnCachedValue( const QString path);

private:
    ArnQml();

    QString  _arnRootPath;
    ArnNetworkAccessManagerFactory*  _arnNetworkAccessManagerFactory;
    UseFlags  _regedUse;
    QMap<QString,QByteArray>  _arnCache;
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
    Q_PROPERTY( bool pipeMode        READ isPipeMode    WRITE setPipeMode)
    Q_PROPERTY( bool saveMode        READ isSaveMode    WRITE setSaveMode)
    Q_PROPERTY( bool masterMode      READ isMaster      WRITE setMaster)
    Q_PROPERTY( bool autoDestroyMode READ isAutoDestroy WRITE setAutoDestroy)
    // Q_PROPERTY( bool smTemplate     READ isTemplate    WRITE setTemplate)

public:
    ArnItemQml( QObject* parent = 0);

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

protected:
    virtual void  itemUpdated( const ArnLinkHandle& handleData, const QByteArray* value = 0);

private:
    bool  _isCompleted;
    QString  _path;
    int  _valueType;
};


class ArnNetworkReply : public QNetworkReply
{
    Q_OBJECT
public:
    explicit ArnNetworkReply( QObject* parent = 0);

    void  setOperation( QNetworkAccessManager::Operation operation)
    { QNetworkReply::setOperation( operation);}

    void  setRequest( const QNetworkRequest& request)
    { QNetworkReply::setRequest( request);}

    bool  isFinished()  const;
    qint64  bytesAvailable()  const;
    bool  isSequential()  const;
    qint64  size()  const;

    QByteArray  data()  const;
    void  setData( const QByteArray& data);

protected:
    virtual void  abort();
    virtual qint64  readData( char *data, qint64 maxlen);

private slots:

private:
    QByteArray  _data;
    int  _readPos;
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

#endif // ARNQML_HPP
