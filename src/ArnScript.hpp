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

#ifndef ARNSCRIPT_HPP
#define ARNSCRIPT_HPP


#include "Arn.hpp"
#include "ArnLib_global.hpp"
#include <QObject>
#include <QScriptable>
#include <QScriptValue>

class QScriptEngine;
class ArnClient;
class ArnScript;


class ARNLIBSHARED_EXPORT ArnItemProto : public QObject, public QScriptable
{
    Q_OBJECT
    Q_PROPERTY( QString path       READ path          WRITE setPath)
    Q_PROPERTY( QVariant value     READ value         WRITE setValue)
    Q_PROPERTY( QString string     READ string        WRITE setString)
    // Property  num  is set manually
    Q_PROPERTY( bool pipeMode      READ isPipeMode    WRITE setPipeMode)
    Q_PROPERTY( bool saveMode      READ isSaveMode    WRITE setSaveMode)
    Q_PROPERTY( bool smMaster      READ isMaster      WRITE setMaster)
    Q_PROPERTY( bool smAutoDestroy READ isAutoDestroy WRITE setAutoDestroy)
    Q_PROPERTY( bool smTemplate    READ isTemplate    WRITE setTemplate)
public:
    ArnItemProto( ArnScript* parent = 0);

    QString  path() const;
    void  setPath( const QString& path);
    QVariant  value() const;
    void  setValue( const QVariant& value);
    QString  string() const;
    void  setString( const QString& value);
    bool  isPipeMode() const;
    void  setPipeMode( bool isPipeMode);
    bool  isMaster() const;
    void  setMaster( bool isMaster);
    bool  isAutoDestroy() const;
    void  setAutoDestroy( bool isAutoDestroy);
    bool  isSaveMode() const;
    void  setSaveMode( bool isSaveMode);
    bool  isTemplate() const;
    void  setTemplate( bool isTemplate);
    // Manually defining property "num", due to bugg in QT
    static QScriptValue getSetNum( QScriptContext* context, QScriptEngine* engine);

    static QScriptValue  constructor( QScriptContext* context, QScriptEngine* engine);
};


class ARNLIBSHARED_EXPORT ArnMonitorProto : public QObject, public QScriptable
{
    Q_OBJECT
    Q_PROPERTY( QString clientId     READ clientId     WRITE setClientId)
    Q_PROPERTY( QString monitorPath  READ monitorPath  WRITE setMonitorPath)
public slots:
    void  reStart();

public:
    ArnMonitorProto( ArnScript* parent = 0);
    void  setClientId( const QString& id);
    QString  clientId() const;
    void  setMonitorPath( const QString& name);
    QString  monitorPath() const;

    static QScriptValue  constructor( QScriptContext* context, QScriptEngine* engine);
};


class ARNLIBSHARED_EXPORT ArnDepOfferProto : public QObject, public QScriptable
{
    Q_OBJECT
    Q_PROPERTY( QString stateName  READ stateName  WRITE setStateName)
    Q_PROPERTY( int     stateId    READ stateId    WRITE setStateId)
public slots:
    void  advertise( QString serviceName);

public:
    ArnDepOfferProto( ArnScript* parent = 0);
    void  setStateName( const QString& name);
    QString  stateName() const;
    void  setStateId( int id);
    int  stateId() const;

    static QScriptValue  constructor( QScriptContext* context, QScriptEngine* engine);
};


class ARNLIBSHARED_EXPORT ArnDepProto : public QObject, public QScriptable
{
    Q_OBJECT
public slots:
    void  add( QString serviceName, int stateId = -1);
    void  add( QString serviceName, QString stateName);
    void  setMonitorName( QString name);
    void  startMonitor();

public:
    ArnDepProto( ArnScript* parent = 0);

    static QScriptValue  constructor( QScriptContext* context, QScriptEngine* engine);
};


class ARNLIBSHARED_EXPORT ArnInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString info  READ info )
public:
    explicit  ArnInterface( QObject* parent = 0) {Q_UNUSED(parent);}

    QString  info()                             {return QString::fromUtf8( ArnM::instance().info().constData());}

signals:

public slots:
    QVariant  value( const QString& path)       {return ArnM::instance().valueVariant( path);}
    QString  string( const QString& path)       {return ArnM::instance().valueString( path);}
    double  num( const QString& path)           {return ArnM::instance().valueDouble( path);}
    QStringList  items( const QString& path)    {return ArnM::instance().items( path);}
    bool  exist(const QString& path)            {return ArnM::instance().exist( path);}

    bool  isFolder( const QString& path)        {return ArnM::instance().isFolder( path);}
    bool  isLeaf( const QString& path)          {return ArnM::instance().isLeaf( path);}
    void  setValue( const QString& path, const QVariant& value)
                                                {ArnM::instance().setValue( path, value);}
    void  setString( const QString& path, const QString& value)
                                                {ArnM::instance().setValue( path, value);}
    void  setNum( const QString& path, double value)
                                                {ArnM::instance().setValue( path, value);}
    //// "static" help functions
    QString  itemName( const QString& path)     {return ArnM::instance().itemName( path);}
    QString  childPath( const QString &parentPath, const QString &posterityPath)
                                                {return ArnM::instance().childPath( parentPath, posterityPath);}

    QString  makePath( const QString &parentPath, const QString &itemName)
                                                {return ArnM::instance().makePath( parentPath, itemName);}
private:
};


class ARNLIBSHARED_EXPORT ArnScript : public QObject
{
    Q_OBJECT
public:
    explicit  ArnScript( QObject* parent = 0);
    QScriptEngine&  engine()  const {return *_engine;}
    bool  evaluate( QByteArray script, QString idName);
    bool  evaluateFile( QString fileName);
    bool  logUncaughtError( QScriptValue& scriptValue);
    QString  idName()  const {return _idName;}

    // To be reimplemented for accessing higher layer info
    virtual ArnClient*  getClient( QString clientId);

signals:
    void  errorText( QString txt);

public slots:

private slots:
    void  doSignalException( const QScriptValue& exception);

protected:
    void  errorLog( QString errText, ArnError err = ArnError::Undef, void* reference = 0);
    static QScriptValue  printFunction( QScriptContext* context, QScriptEngine* engine);

    QScriptEngine*  _engine;
    ArnItemProto*  _itemProto;
    ArnMonitorProto* _monitorProto;
    ArnDepOfferProto*  _depOfferProto;
    ArnDepProto*  _depProto;

private:
    QString  _idName;
};

#endif // ARNSCRIPT_HPP
