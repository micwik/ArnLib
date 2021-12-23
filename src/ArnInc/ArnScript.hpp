// Copyright (C) 2010-2020 Michael Wiklund.
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

#ifndef ARNSCRIPT_HPP
#define ARNSCRIPT_HPP

// #define ARN_JSENGINE_VER    0x050e00  // Full support

#include "ArnLib_global.hpp"
#include "ArnInterface.hpp"
#include "ArnM.hpp"
#include <QObject>

#if ARNUSE_SCRIPTJS
  #include <QJSValue>
  #include "ArnMonitor.hpp"
  #include "ArnItem.hpp"
  #include "ArnDepend.hpp"
  #define ARN_JSENGINE QJSEngine
  #define ARN_JSVALUE QJSValue
  #define ARN_JSVALUE_LIST QJSValueList
  #define ARN_JSCONTEXT QJSContext
#else
  #include <QScriptable>
  #include <QScriptValue>
  #define ARN_JSENGINE QScriptEngine
  #define ARN_JSVALUE QScriptValue
  #define ARN_JSVALUE_LIST QScriptValueList
  #define ARN_JSCONTEXT QScriptContext
#endif

class ARN_JSENGINE;
class ArnClient;
class ArnScript;


//! \cond ADV
#ifdef ARNUSE_SCRIPTJS
class ARNLIBSHARED_EXPORT ArnItemJs : public ArnItem
{
    Q_OBJECT
    //! The type used inside the variant, e.g. QString
    Q_PROPERTY( QString variantType  READ variantType        WRITE setVariantType)
    Q_PROPERTY( QString defaultType  READ variantType        WRITE setVariantType)  // Legacy
    //! Select to use ArnItem::openUuid()
    Q_PROPERTY( bool useUuid         READ useUuid            WRITE setUseUuid)
    //! The path of this ArnItem
    Q_PROPERTY( QString path         READ path               WRITE setPath)
    //! The Arn data type of this ArnItem
    Q_PROPERTY( ArnInterface::DataType  type
                                     READ type)
    //! The ArnItem value as a QVariant
    Q_PROPERTY( QVariant variant     READ toVariant          WRITE setVariant)
    Q_PROPERTY( QVariant value       READ toVariant          WRITE setVariant)  // Legacy
    //! The ArnItem value as a QString
    Q_PROPERTY( QString string       READ toString           WRITE setValue)
    //! The ArnItem value as a QByteArray
    Q_PROPERTY( QByteArray bytes     READ toByteArray        WRITE setValue)
    //! The ArnItem value as an ARNREAL
#ifdef ARNREAL_FLOAT
    Q_PROPERTY( float num            READ toReal             WRITE setValue)
#else
    Q_PROPERTY( double num           READ toReal             WRITE setValue)
#endif
    //! The ArnItem value as an int
    Q_PROPERTY( int intNum           READ toInt              WRITE setValue)
    //! See Arn::ObjectMode::BiDir
    Q_PROPERTY( bool biDirMode       READ isBiDirMode        WRITE setBiDirMode)
    //! See Arn::ObjectMode::Pipe
    Q_PROPERTY( bool pipeMode        READ isPipeMode         WRITE setPipeMode)
    //! See Arn::ObjectMode::Save
    Q_PROPERTY( bool saveMode        READ isSaveMode         WRITE setSaveMode)
    //! See Arn::ObjectSyncMode::Master
    Q_PROPERTY( bool masterMode      READ isMaster           WRITE setMaster)
    Q_PROPERTY( bool smMaster        READ isMaster           WRITE setMaster)  // Legacy
    //! See Arn::ObjectSyncMode::AutoDestroy
    Q_PROPERTY( bool autoDestroyMode READ isAutoDestroy      WRITE setAutoDestroy)
    Q_PROPERTY( bool smAutoDestroy   READ isAutoDestroy      WRITE setAutoDestroy)  // Legacy
    //! See ArnBasicItem::setAtomicOpProvider()
    Q_PROPERTY( bool atomicOpProvider
                                     READ isAtomicOpProvider WRITE setAtomicOpProvider)
    //! See ArnItem::setIgnoreSameValue()
    Q_PROPERTY( bool ignoreSameValue READ isIgnoreSameValue  WRITE setIgnoreSameValue)
    //! See ArnItem::setDelay()
    Q_PROPERTY( int delay            READ delay              WRITE setDelay)
    Q_PROPERTY( bool templateMode    READ isTemplate         WRITE setTemplate)
    Q_PROPERTY( bool smTemplate      READ isTemplate         WRITE setTemplate)  // Legacy

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

public:
    Q_INVOKABLE ArnItemJs( QObject* parent = arnNullptr);
    Q_INVOKABLE ArnItemJs( const QString& path, QObject* parent = arnNullptr);
    Q_INVOKABLE ArnItemJs( const QJSValue& itemTemplate, const QString& path, QObject* parent = arnNullptr);

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

    bool  useUuid()  const;
    void  setUseUuid( bool useUuid);

signals:
    void  changedVoid();
#ifdef ARNREAL_FLOAT
    void  changedNum( float value);
#else
    void  changedNum( double value);
#endif
    void  changedString( const QString& value);

private:
    void init();
    const ArnItem& arnItemFromJsValue( const QJSValue& jsValue, QObject& defParent);

    QString  _path;
    int  _variantType = 0;
    bool  _useUuid    = false;
};


class ARNLIBSHARED_EXPORT ArnMonitorJs : public ArnMonitor
{
    Q_OBJECT
    //! The client id. Set whith ArnClient::registerClient(). Use "std" if not set.
    Q_PROPERTY( QString clientId     READ clientId     WRITE setClient)
    //! The path to be monitored at the server.
    Q_PROPERTY( QString monitorPath  READ monitorPath  WRITE setMonitorPath)
public slots:
    //! Restart the monitor
    /*! All signals for found childs will be emitted again.
     */
    void  reStart();

public:
    Q_INVOKABLE ArnMonitorJs( QObject* parent = arnNullptr);
};


class ARNLIBSHARED_EXPORT ArnDepOfferJs : public ArnDependOffer
{
    Q_OBJECT
    Q_PROPERTY( QString stateName  READ stateName  WRITE setStateName)
    Q_PROPERTY( int     stateId    READ stateId    WRITE setStateId)
public slots:
    void  advertise( const QString& serviceName);

public:
    Q_INVOKABLE ArnDepOfferJs( QObject* parent = arnNullptr);
};


class ARNLIBSHARED_EXPORT ArnDepJs : public ArnDepend
{
    Q_OBJECT
public slots:
    void  add( const QString& serviceName, int stateId = -1);
    void  add( const QString& serviceName, const QString& stateName);
    void  setMonitorName( const QString& name);
    void  startMonitor();

public:
    Q_INVOKABLE ArnDepJs( QObject* parent = arnNullptr);
};


class ARNLIBSHARED_EXPORT ArnScript : public QObject
{
    friend class ArnJsGlobal;
    Q_OBJECT
public:
    explicit  ArnScript( QObject* parent = arnNullptr);
    ArnScript( ARN_JSENGINE* engine, QObject* parent = arnNullptr);
    ARN_JSENGINE&  engine()  const;
    void  addObject( const QString& id, QObject* obj);

    bool  evaluate( const QByteArray& script, const QString& idName, const QString& typeName = QString());
    bool  evaluateFile( const QString& fileName);
    ARN_JSVALUE  globalProperty( const QString& id);
    ARN_JSVALUE  callFunc( ARN_JSVALUE& func, const ARN_JSVALUE& thisObj, const ARN_JSVALUE_LIST& args);

    QString  idName()  const;
    void  setInterruptedText( const QString& interruptedText);

signals:
    void  errorText( QString txt);

public slots:

private slots:

protected:
    void  errorLog( const QString& errText, ArnError err = ArnError::Undef, void* reference = 0);

    ARN_JSENGINE*  _engine;

private:
    void  init( ARN_JSENGINE* engine);
    bool  doJsResult( const ARN_JSVALUE& jsResult, const QString& typeName = QString());

    QString  _idName;
    QString  _interruptedText;
};


class ArnJsGlobal : public QObject
{
    Q_OBJECT
public:
    explicit  ArnJsGlobal( ArnScript& arnScript, QObject* parent = arnNullptr);

public slots:
    void print( const QString& txt);

private:
    ArnScript& _arnScript;
};

#else
class ArnItemScr : public ArnItem
{
    Q_OBJECT
public:
    ArnItemScr( QObject* parent = 0);
    ArnItemScr( const QString& path, QObject* parent = 0);
    ArnItemScr( const ArnItem& itemTemplate, const QString& path, QObject* parent = 0);
    virtual  ~ArnItemScr();

    int  _defaultType;

signals:
    void  changedVoid();
#ifdef ARNREAL_FLOAT
    void  changedNum( float value);
#else
    void  changedNum( double value);
#endif
    void  changedString( const QString& value);

private:
    void  init();
};


class ARNLIBSHARED_EXPORT ArnItemProto : public QObject, public QScriptable
{
    Q_OBJECT
    Q_PROPERTY( QString defaultType READ defaultType   WRITE setDefaultType)
    Q_PROPERTY( QString path        READ path          WRITE setPath)
    Q_PROPERTY( QVariant value      READ value         WRITE setValue)
    Q_PROPERTY( QString string      READ string        WRITE setString)
    // Property  num  is set manually
    Q_PROPERTY( bool pipeMode       READ isPipeMode    WRITE setPipeMode)
    Q_PROPERTY( bool saveMode       READ isSaveMode    WRITE setSaveMode)
    Q_PROPERTY( bool smMaster       READ isMaster      WRITE setMaster)
    Q_PROPERTY( bool smAutoDestroy  READ isAutoDestroy WRITE setAutoDestroy)
    Q_PROPERTY( bool smTemplate     READ isTemplate    WRITE setTemplate)
public:
    ArnItemProto( ArnScript* parent = 0);

    QString  defaultType() const;
    void  setDefaultType( const QString& typeName);
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
    void  advertise( const QString& serviceName);

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
    void  add( const QString& serviceName, int stateId = -1);
    void  add( const QString& serviceName, const QString& stateName);
    void  setMonitorName( const QString& name);
    void  startMonitor();

public:
    ArnDepProto( ArnScript* parent = 0);

    static QScriptValue  constructor( QScriptContext* context, QScriptEngine* engine);
};
//! \endcond


class ARNLIBSHARED_EXPORT ArnScript : public QObject
{
    Q_OBJECT
public:
    explicit  ArnScript( QObject* parent = 0);
    ArnScript( QScriptEngine* engine, QObject* parent = 0);
    QScriptEngine&  engine()  const;
    void  addObject( const QString& id, QObject* obj);

    bool  evaluate( const QByteArray& script, const QString& idName, const QString& typeName = QString());
    bool  evaluateFile( const QString& fileName);
    ARN_JSVALUE  globalProperty( const QString& id);
    ARN_JSVALUE  callFunc( ARN_JSVALUE& func, const ARN_JSVALUE& thisObj, const ARN_JSVALUE_LIST& args);

    bool  logUncaughtError( QScriptValue& scriptValue, const QString& typeName = QString());
    QString  idName()  const;

    void  setInterruptedText( const QString& interruptedText);

signals:
    void  errorText( QString txt);

public slots:

private slots:
    void  doSignalException( const QScriptValue& exception);

protected:
    void  errorLog( const QString& errText, ArnError err = ArnError::Undef, void* reference = 0);
    static QScriptValue  printFunction( QScriptContext* context, QScriptEngine* engine);

    QScriptEngine*  _engine;
    ArnItemProto*  _itemProto;
    ArnMonitorProto* _monitorProto;
    ArnDepOfferProto*  _depOfferProto;
    ArnDepProto*  _depProto;

private:
    void  init( QScriptEngine* engine);

    QString  _idName;
};
#endif

#endif // ARNSCRIPT_HPP
