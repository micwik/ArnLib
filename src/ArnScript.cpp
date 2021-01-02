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

#include "ArnInc/ArnScript.hpp"
#include "ArnInc/ArnDepend.hpp"
#include "ArnInc/ArnMonitor.hpp"
#include "ArnInc/Arn.hpp"
#include <QFile>
#include <QDebug>

#ifdef ARNUSE_SCRIPTJS
#include <QJSValue>
#include <QJSEngine>
#include <QObject>


ArnJsGlobal::ArnJsGlobal( ArnScript& arnScript, QObject* parent)
    : QObject( parent)
    , _arnScript( arnScript)
{
}


void ArnJsGlobal::print( const QString &txt)
{
    _arnScript.errorLog( txt, ArnError::Info);
}


///////////////////

ArnScript::ArnScript( QObject* parent) :
    QObject( parent)
{
    init( arnNullptr);
}


ArnScript::ArnScript( ARN_JSENGINE* engine, QObject* parent) :
    QObject( parent)
{
    init( engine);
}


ARN_JSENGINE&  ArnScript::engine()  const
{
    return *_engine;
}


void ArnScript::addObject( const QString& id, QObject* obj)
{
    if ((id.isEmpty()) || (obj == 0))  return;

    if ((obj != this) && (!obj->parent()))
        obj->setParent( this);  // Reparent to this script handler

    ARN_JSVALUE jsObj = _engine->newQObject( obj);
    _engine->globalObject().setProperty( id, jsObj);
}


bool  ArnScript::evaluate( const QByteArray& script, const QString& idName, const QString& typeName)
{
    _idName = idName;
    ARN_JSVALUE  result = _engine->evaluate( QString::fromUtf8( script.constData()));
    bool isOk = doJsResult( result, typeName);
    return isOk;
}


bool  ArnScript::evaluateFile( const QString& fileName)
{
    QFile  file( fileName);
    file.open( QIODevice::ReadOnly);

    return evaluate( file.readAll(), fileName);
}


QJSValue ArnScript::globalProperty( const QString& id)
{
    return _engine->globalObject().property( id);;
}


QJSValue ArnScript::callFunc( QJSValue& func, const QJSValue& thisObj, const QJSValueList& args)
{
    QJSValue result = func.callWithInstance( thisObj, args);
    doJsResult( result);

    return result;
}


QString  ArnScript::idName()  const
{
    return _idName;
}


void  ArnScript::setInterruptedText( const QString& interruptedText)
{
    _interruptedText = interruptedText;
}


bool  ArnScript::doJsResult( const ARN_JSVALUE& jsResult, const QString& typeName)
{
    bool isAborted = _engine->isInterrupted();
    if (isAborted) {
        _engine->setInterrupted( false);
    }
    bool isError = jsResult.isError();
    if (!isError && !isAborted) return true;

    // Exception properties: name, message, fileName, lineNumber, stack
    int lineNo = jsResult.property("lineNumber").toInt();
    QString lineTxt = lineNo > 0 ? " @line:" + QString::number( lineNo) : QString();
    QString errTxt;
    if (!typeName.isEmpty())  errTxt += "From type " + typeName + ": ";
    errTxt += isAborted ? _interruptedText : jsResult.toString();
    errTxt += lineTxt;
    errorLog( errTxt, ArnError::ScriptError);

    return false;
}


void ArnScript::init( ARN_JSENGINE* engine)
{
    _interruptedText = "JS-Engine interrupted";

    if (engine)
        _engine = engine;
    else
        _engine = new ARN_JSENGINE( this);

    //// Define global functions: print()
    // TODO: vararg print
    // QJSValue print = fEngine->evaluate("function() { printCallback(Array.prototype.slice.apply(arguments));}");
    // fEngine->globalObject().setProperty("print", print);
    ArnJsGlobal* arnJsGlobal = new ArnJsGlobal( *this, this);
    QJSValue jsGlobalAdd = _engine->newQObject( arnJsGlobal);
    QJSValue jsPrintFunc =  jsGlobalAdd.property("print");
    _engine->globalObject().setProperty( "print", jsPrintFunc);

    //// Define prototypes
    QJSValue jsMetaObject;
    jsMetaObject = _engine->newQMetaObject( &ArnItemJs::staticMetaObject);
    _engine->globalObject().setProperty( "ArnItem", jsMetaObject);

    jsMetaObject = _engine->newQMetaObject( &ArnMonitorJs::staticMetaObject);
    _engine->globalObject().setProperty( "ArnMonitor", jsMetaObject);

    jsMetaObject = _engine->newQMetaObject( &ArnDepOfferJs::staticMetaObject);
    _engine->globalObject().setProperty( "ArnDependOffer", jsMetaObject);

    jsMetaObject = _engine->newQMetaObject( &ArnDepJs::staticMetaObject);
    _engine->globalObject().setProperty( "ArnDepend", jsMetaObject);
    //// End define prototypes

    _engine->globalObject().setProperty( "arn", _engine->newQObject( new ArnInterface( this)));
}


void  ArnScript::errorLog( const QString& errText, ArnError err, void* reference)
{
    QString  scriptText = " Script:" + _idName;

    ArnM::errorLog( errText + scriptText, err, reference);
    emit errorText( errText);
}


///////// ArnItem

void ArnItemJs::init()
{
    QObject::connect( this, static_cast<void(ArnItem::*)(void)>(&ArnItem::changed),
                      this, static_cast<void(ArnItemJs::*)(void)>(&ArnItemJs::changedVoid));
    QObject::connect( this, static_cast<void(ArnItem::*)(ARNREAL)>(&ArnItem::changed),
                      this, static_cast<void(ArnItemJs::*)(ARNREAL)>(&ArnItemJs::changedNum));
    QObject::connect( this, static_cast<void(ArnItem::*)(const QString&)>(&ArnItem::changed),
                      this, static_cast<void(ArnItemJs::*)(const QString&)>(&ArnItemJs::changedString));
}


ArnItemJs::ArnItemJs( QObject* parent)
    : ArnItem( parent)
{
    init();
}


ArnItemJs::ArnItemJs( const QString& path, QObject* parent)
    : ArnItem( path, parent)
{
    _path = path;
    init();
}


ArnItemJs::ArnItemJs( const QJSValue& itemTemplate, const QString& path, QObject* parent)
    : ArnItem( arnItemFromJsValue( itemTemplate, *this), path, parent)
{
    _path = path;
    init();
}


QString  ArnItemJs::variantType()  const
{
    if (!_variantType)  return QString();

    const char*  typeName = QMetaType::typeName(_variantType);
    if (!typeName)  return QString();

    return typeName;
}


void  ArnItemJs::setVariantType( const QString& typeName)
{
    if (typeName.isEmpty()) {
        _variantType = 0;
    }
    else {
        int  type = QMetaType::type( typeName.toLatin1().constData());
        if (!type) {
            qWarning() << "ItemJS setVariantType, Unknown: type=" + typeName + " path=" + path();
            return;
        }

        _variantType = type;
    }
}


QString  ArnItemJs::path()  const
{
    return _path;
}


void  ArnItemJs::setPath( const QString& path)
{
    _path = path;
    if (_useUuid)
        openUuid( path);
    else
        open( path);
}


void  ArnItemJs::setVariant( const QVariant& value)
{
    if (!_variantType)  // No variantType, no conversion
        ArnItem::setValue( value);
    else {  // Use variantType
        QVariant  val = value;
        if (val.convert( QVariant::Type( _variantType))) {
            ArnItem::setValue( val);
        }
        else {
            qWarning() << "ItemJS setVariant, Can't convert: type="
                       << _variantType  << " path=" + path();
        }
    }
}


void  ArnItemJs::setBiDirMode( bool isBiDirMode)
{
    if (isBiDirMode)
        ArnItem::setBiDirMode();
}


void  ArnItemJs::setPipeMode( bool isPipeMode)
{
    if (isPipeMode)
        ArnItem::setPipeMode();
}


void  ArnItemJs::setMaster( bool isMaster)
{
    if (isMaster)
        ArnItem::setMaster();
}


void  ArnItemJs::setAutoDestroy( bool isAutoDestroy)
{
    if (isAutoDestroy)
        ArnItem::setAutoDestroy();
}


void  ArnItemJs::setSaveMode( bool isSaveMode)
{
    if (isSaveMode)
        ArnItem::setSaveMode();
}


bool  ArnItemJs::useUuid()  const
{
    return _useUuid;
}


void  ArnItemJs::setUseUuid( bool useUuid)
{
    _useUuid = useUuid;
}


///////// ArnMonitor

ArnMonitorJs::ArnMonitorJs( QObject* parent)
    : ArnMonitor( parent)
{
}


void ArnMonitorJs::reStart()
{
    ArnMonitor::reStart();
}


///////// DependOffer

ArnDepOfferJs::ArnDepOfferJs( QObject *parent)
    : ArnDependOffer( parent)
{
}


void ArnDepOfferJs::advertise( const QString &serviceName)
{
    ArnDependOffer::advertise( serviceName);
}


///////// Depend

ArnDepJs::ArnDepJs( QObject *parent)
    : ArnDepend( parent)
{
}


void ArnDepJs::add( const QString &serviceName, int stateId)
{
    ArnDepend::add( serviceName, stateId);
}


void ArnDepJs::add( const QString &serviceName, const QString &stateName)
{
    ArnDepend::add( serviceName, stateName);
}


void ArnDepJs::setMonitorName( const QString &name)
{
    ArnDepend::setMonitorName( name);
}


void ArnDepJs::startMonitor()
{
    ArnDepend::startMonitor();
}


const ArnItem& ArnItemJs::arnItemFromJsValue( const QJSValue& jsValue, QObject& defParent)
{
    // When jsValue not contains an ArnItem, return a fresh ArnItem owned by defParent
    const ArnItem* arnItem = qobject_cast<const ArnItem*>( jsValue.toQObject());
    return arnItem ? *arnItem : *new ArnItem( &defParent);
}

#else
#include <QtScript>
#include <QScriptValue>
#include <QScriptEngine>

Q_DECLARE_METATYPE(ArnItemScr*)
Q_DECLARE_METATYPE(ArnMonitor*)
Q_DECLARE_METATYPE(ArnDependOffer*)
Q_DECLARE_METATYPE(ArnDepend*)


void  ArnItemScr::init()
{
    _defaultType = 0;
#if QT_VERSION >= 0x050200
    QObject::connect( this, static_cast<void(ArnItem::*)(void)>(&ArnItem::changed),
                      this, static_cast<void(ArnItemScr::*)(void)>(&ArnItemScr::changedVoid));
    QObject::connect( this, static_cast<void(ArnItem::*)(ARNREAL)>(&ArnItem::changed),
                      this, static_cast<void(ArnItemScr::*)(ARNREAL)>(&ArnItemScr::changedNum));
    QObject::connect( this, static_cast<void(ArnItem::*)(const QString&)>(&ArnItem::changed),
                      this, static_cast<void(ArnItemScr::*)(const QString&)>(&ArnItemScr::changedString));
#else
    connect( this, SIGNAL(changed()), this, SIGNAL(changedVoid()));
  #ifdef ARNREAL_FLOAT
    connect( this, SIGNAL(changed(float)), this, SIGNAL(changedNum(float)));
  #else
    connect( this, SIGNAL(changed(double)), this, SIGNAL(changedNum(double)));
  #endif
    connect( this, SIGNAL(changed(QString)), this, SIGNAL(changedString(QString)));
#endif
}


ArnItemScr::ArnItemScr( QObject* parent) :
    ArnItem( parent)
{
    init();
}


ArnItemScr::ArnItemScr( const QString& path, QObject* parent) :
    ArnItem( path, parent)
{
    init();
}


ArnItemScr::ArnItemScr( const ArnItem& itemTemplate, const QString& path, QObject* parent) :
    ArnItem( itemTemplate, path, parent)
{
    init();
}


ArnItemScr::~ArnItemScr()
{
}


ArnScript::ArnScript( QObject* parent) :
    QObject( parent)
{
    init( arnNullptr);
}


ArnScript::ArnScript( QScriptEngine* engine, QObject* parent) :
    QObject( parent)
{
    init( engine);
}


QScriptEngine&  ArnScript::engine()  const
{
    return *_engine;
}


void  ArnScript::addObject( const QString& id, QObject* obj)
{
    if ((id.isEmpty()) || (obj == 0))  return;

    if ((obj != this) && (!obj->parent()))
        obj->setParent( this);  // Reparent to this script handler

    ARN_JSVALUE objScr = _engine->newQObject( obj, QScriptEngine::QtOwnership,
                                                   QScriptEngine::ExcludeSuperClassContents);
    _engine->globalObject().setProperty( id, objScr);
}


bool  ArnScript::evaluate( const QByteArray& script, const QString& idName, const QString& typeName)
{
    _idName = idName;
    QScriptValue  result = _engine->evaluate( QString::fromUtf8( script.constData()));
    if (logUncaughtError( result, typeName)) {
        return false;
    }
    return true;
}


bool  ArnScript::evaluateFile( const QString& fileName)
{
    QFile  file( fileName);
    file.open( QIODevice::ReadOnly);

    return evaluate( file.readAll(), fileName);
}


QScriptValue ArnScript::globalProperty( const QString& id)
{
    return _engine->globalObject().property( id);;
}


QScriptValue  ArnScript::callFunc( QScriptValue& func, const QScriptValue& thisObj, const QScriptValueList& args)
{
    QScriptValue  result = func.call( thisObj, args);
    logUncaughtError( result);

    return result;
}


bool  ArnScript::logUncaughtError( QScriptValue& scriptValue, const QString& typeName)
{
    // qDebug() << "logUncaughtError: has=" << _engine->hasUncaughtException();
    if (_engine->hasUncaughtException()) {
        QString  errDesc = scriptValue.toString();
        // qDebug() << "logUncaughtError: errDesc=" << errDesc;
        if (!errDesc.isEmpty()) {
            QString preTxt = typeName.isEmpty() ? QString() : ("From type " + typeName + ": ");
            int lineNo = _engine->uncaughtExceptionLineNumber();
            errorLog( preTxt + errDesc + " @line:" + QString::number( lineNo),
                      ArnError::ScriptError);
        }
        return true;
    }
    return false;
}


QString  ArnScript::idName()  const
{
    return _idName;
}


void  ArnScript::doSignalException(const QScriptValue& exception)
{
    int lineNo = _engine->uncaughtExceptionLineNumber();
    errorLog( exception.toString() + " sig@line:" + QString::number( lineNo),
              ArnError::ScriptError);
}


QScriptValue  ArnScript::printFunction( QScriptContext* context, QScriptEngine* engine)
{
    QString result;
    for (int i = 0; i < context->argumentCount(); ++i) {
        if (i > 0)
            result.append(" ");
        result.append( context->argument(i).toString());
    }

    QScriptValue  calleeData = context->callee().data();
    ArnScript*  arnScript = qobject_cast<ArnScript*>( calleeData.toQObject());
    Q_ASSERT( arnScript);
    arnScript->errorLog( result, ArnError::Info);

    return engine->undefinedValue();
}


void ArnScript::init( QScriptEngine* engine)
{
    if (engine)
        _engine = engine;
    else
        _engine = new QScriptEngine( this);
    connect( _engine, SIGNAL(signalHandlerException(QScriptValue)), this, SLOT(doSignalException(QScriptValue)));

    //// Redefine print()
    QScriptValue  printFunc = _engine->newFunction( ArnScript::printFunction);
     printFunc.setData( _engine->newQObject( this));  // Save internal pointer to this ArnScript
     _engine->globalObject().setProperty("print", printFunc);


    //// Define prototypes
    _itemProto     = new ArnItemProto( this);
    _monitorProto  = new ArnMonitorProto( this);
    _depOfferProto = new ArnDepOfferProto( this);
    _depProto      = new ArnDepProto( this);

    QScriptValue itemProtoScr     = _engine->newQObject( _itemProto);
    QScriptValue monitorProtoScr  = _engine->newQObject( _monitorProto);
    QScriptValue depOfferProtoScr = _engine->newQObject( _depOfferProto);
    QScriptValue depProtoScr      = _engine->newQObject( _depProto);

    _engine->setDefaultPrototype( qMetaTypeId<ArnItemScr*>(),        itemProtoScr);
    _engine->setDefaultPrototype( qMetaTypeId<ArnMonitor*>(),     monitorProtoScr);
    _engine->setDefaultPrototype( qMetaTypeId<ArnDependOffer*>(), depOfferProtoScr);
    _engine->setDefaultPrototype( qMetaTypeId<ArnDepend*>(),      depProtoScr);

    QScriptValue itemConstrScr     = _engine->newFunction( ArnItemProto::constructor,     itemProtoScr);
    QScriptValue monitorConstrScr  = _engine->newFunction( ArnMonitorProto::constructor,  monitorProtoScr);
    QScriptValue depOfferConstrScr = _engine->newFunction( ArnDepOfferProto::constructor, depOfferProtoScr);
    QScriptValue depConstrScr      = _engine->newFunction( ArnDepProto::constructor,      depProtoScr);

    _engine->globalObject().setProperty("ArnItem",        itemConstrScr);
    _engine->globalObject().setProperty("ArnMonitor",     monitorConstrScr);
    _engine->globalObject().setProperty("ArnDependOffer", depOfferConstrScr);
    _engine->globalObject().setProperty("ArnDepend",      depConstrScr);

    // Add properties to prototypes (manually)
    itemProtoScr.setProperty("num", _engine->newFunction( ArnItemProto::getSetNum),
                    QScriptValue::PropertyGetter|QScriptValue::PropertySetter);
    //// End define prototypes

    _engine->globalObject().setProperty( "arn", _engine->newQObject( new ArnInterface( this),
            QScriptEngine::QtOwnership, QScriptEngine::ExcludeSuperClassContents));
}


void  ArnScript::errorLog( const QString& errText, ArnError err, void* reference)
{
    QString  scriptText = " Script:" + _idName;

    ArnM::errorLog( errText + scriptText, err, reference);
    emit errorText( errText);
}


///////// ArnItem

ArnItemProto::ArnItemProto( ArnScript* parent)
: QObject( parent)
{
}


QString  ArnItemProto::defaultType() const
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (!item)  return QString();
    if (!item->_defaultType)  return QString();

    const char*  typeName = QMetaType::typeName( item->_defaultType);
    if (!typeName)  return QString();

    return typeName;
}


void  ArnItemProto::setDefaultType( const QString &typeName)
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (!item)  return;

    if (typeName.isEmpty()) {
        item->_defaultType = 0;
        return;
    }

    int  type = QMetaType::type( typeName.toLatin1().constData());
    if (!type) {
        context()->throwError( QScriptContext::TypeError,
                               "Setting unknown defaultType=" + typeName);
        return;
    }


    item->_defaultType = type;
}


QString  ArnItemProto::path() const
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  return item->path();
    return QString();
}


void  ArnItemProto::setPath( const QString &path)
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  item->open( path);
}


QVariant  ArnItemProto::value() const
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (!item)  return QVariant();

    QVariant  val = item->toVariant();
    int  type = val.type();

    switch (type) {
    case QMetaType::QTime:
        val = QVariant( QDateTime( QDate( 1, 1, 1), val.toTime()));
        break;
    default:
        break;
    }

    return val;
}


void  ArnItemProto::setValue( const QVariant &value)
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (!item)  return;

    if (!item->_defaultType)  // No defaultType, no conversion
        item->setValue( value);
    else {  // Use defaultType
        QVariant  val = value;
        if (val.convert( QVariant::Type( item->_defaultType))) {
            item->setValue( val);
        }
        else {
            context()->throwError( QScriptContext::TypeError,
                                   "Can't convert to defaultType=" + defaultType());
        }
    }
}


QString  ArnItemProto::string() const
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  return item->toString();
    return QString();
}


void  ArnItemProto::setString( const QString &value)
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  item->setValue( value);
}


bool  ArnItemProto::isPipeMode() const
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  return item->isPipeMode();
    return false;
}


void  ArnItemProto::setPipeMode( bool /*isPipeMode*/)
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  item->setPipeMode();
}


bool  ArnItemProto::isMaster() const
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  return item->isMaster();
    return false;
}


void  ArnItemProto::setMaster( bool /*isMaster*/)
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)
        item->setMaster();
}


bool  ArnItemProto::isAutoDestroy() const
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  return item->isAutoDestroy();
    return false;
}


void  ArnItemProto::setAutoDestroy( bool /*isAutoDestroy*/)
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  item->setAutoDestroy();
}


bool  ArnItemProto::isSaveMode() const
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  return item->isSaveMode();
    return false;
}


void  ArnItemProto::setSaveMode( bool /*isSaveMode*/)
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  item->setSaveMode();
}


bool  ArnItemProto::isTemplate() const
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  return item->isTemplate();
    return false;
}


void  ArnItemProto::setTemplate( bool isTemplate)
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( thisObject());
    if (item)  item->setTemplate( isTemplate);
}


QScriptValue ArnItemProto::getSetNum(QScriptContext* context, QScriptEngine* engine)
{
    ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( context->thisObject());
    QScriptValue  result;
    if (context->argumentCount() == 1) {  // Set property
        result = context->argument(0);
        // Bugg in qt ScriptValue::toNumber (only gives float), using toString as workaround
        item->setValue( result.toString());
        //qDebug() << "ArnScr set path=" << item->path() << " value=" << result.toString();
    }
    else {  // Get property
        result = QScriptValue( engine, qsreal( item->toDouble()));
    }
    return result;
}


QScriptValue  ArnItemProto::constructor( QScriptContext* context, QScriptEngine* engine)
{
    if (!context->isCalledAsConstructor()) {
        return context->throwError( QScriptContext::SyntaxError,
                                    "use the 'new' operator");
    }

    ArnItemScr*  arnItem;
    if (context->argumentCount() >= 2) {  // (Item, Path) as arguments
        ArnItemScr*  item = qscriptvalue_cast<ArnItemScr*>( context->argument(0));
        if (!item) {
            return context->throwError(QScriptContext::TypeError,
                                       "is not ArnItem as first argument");
        }
        QString  path = qscriptvalue_cast<QString>( context->argument(1));
        if (path.isNull()) {
            return context->throwError(QScriptContext::TypeError,
                                       "is not String (path) as second argument");
        }
        arnItem = new ArnItemScr( *item, path);
    }
    else if (context->argumentCount() >= 1) {  // Path as argument
        QString  path = qscriptvalue_cast<QString>( context->argument(0));
        if (path.isNull()) {
            return context->throwError(QScriptContext::TypeError,
                                       "is not String (path) as first argument");
        }
        arnItem = new ArnItemScr( path);
    }
    else {  // No argument
        arnItem = new ArnItemScr;
    }

    // let the engine manage the new object's lifetime.
    return engine->newQObject( arnItem, QScriptEngine::ScriptOwnership);
}


///////// ArnMonitor

ArnMonitorProto::ArnMonitorProto( ArnScript* parent)
: QObject( parent)
{
}


void  ArnMonitorProto::reStart()
{
    ArnMonitor*  arnMon = qscriptvalue_cast<ArnMonitor*>( thisObject());
    if (arnMon)  arnMon->reStart();
}


void  ArnMonitorProto::setClientId( const QString& id)
{
    ArnMonitor*  arnMon = qscriptvalue_cast<ArnMonitor*>( thisObject());
    ArnScript*  arnScr = qobject_cast<ArnScript*>(parent());
    if (arnMon && arnScr) {
        arnMon->setClient( id);
    }
}


QString  ArnMonitorProto::clientId()  const
{
    ArnMonitor*  arnMon = qscriptvalue_cast<ArnMonitor*>( thisObject());
    if (arnMon)  return arnMon->clientId();
    return QString();
}


void  ArnMonitorProto::setMonitorPath( const QString& name)
{
    ArnMonitor*  arnMon = qscriptvalue_cast<ArnMonitor*>( thisObject());
    if (arnMon)  arnMon->start( name, arnMon->client());
}


QString  ArnMonitorProto::monitorPath()  const
{
    ArnMonitor*  arnMon = qscriptvalue_cast<ArnMonitor*>( thisObject());
    if (arnMon)  return arnMon->monitorPath();
    return QString();
}


QScriptValue  ArnMonitorProto::constructor( QScriptContext* context, QScriptEngine* engine)
{
    if (!context->isCalledAsConstructor()) {
        return context->throwError( QScriptContext::SyntaxError,
                                    "use the 'new' operator");
    }
    ArnMonitor*  arnMon = new ArnMonitor;
    // let the engine manage the new object's lifetime.
    return engine->newQObject( arnMon, QScriptEngine::ScriptOwnership);
}


///////// DependOffer

ArnDepOfferProto::ArnDepOfferProto( ArnScript* parent)
: QObject( parent)
{
}


void  ArnDepOfferProto::advertise( const QString& serviceName)
{
    ArnDependOffer*  depOffer = qscriptvalue_cast<ArnDependOffer*>( thisObject());
    if (depOffer)  depOffer->advertise( serviceName);
}


void  ArnDepOfferProto::setStateName( const QString& name)
{
    ArnDependOffer*  depOffer = qscriptvalue_cast<ArnDependOffer*>( thisObject());
    if (depOffer)  depOffer->setStateName( name);
}


QString  ArnDepOfferProto::stateName()  const
{
    ArnDependOffer*  depOffer = qscriptvalue_cast<ArnDependOffer*>( thisObject());
    if (depOffer)  return depOffer->stateName();
    return QString();
}


void  ArnDepOfferProto::setStateId(int id)
{
    ArnDependOffer*  depOffer = qscriptvalue_cast<ArnDependOffer*>( thisObject());
    if (depOffer)  depOffer->setStateId( id);
}


int  ArnDepOfferProto::stateId()  const
{
    ArnDependOffer*  depOffer = qscriptvalue_cast<ArnDependOffer*>( thisObject());
    if (depOffer)  return depOffer->stateId();
    return 0;
}


QScriptValue  ArnDepOfferProto::constructor( QScriptContext* context, QScriptEngine* engine)
{
    if (!context->isCalledAsConstructor()) {
        return context->throwError( QScriptContext::SyntaxError,
                                    "use the 'new' operator");
    }
    ArnDependOffer*  depOffer = new ArnDependOffer;
    // let the engine manage the new object's lifetime.
    return engine->newQObject( depOffer, QScriptEngine::ScriptOwnership);
}


///////// Depend

ArnDepProto::ArnDepProto( ArnScript* parent)
: QObject( parent)
{
}


void  ArnDepProto::add( const QString& serviceName, const QString& stateName)
{
    ArnDepend*  dep = qscriptvalue_cast<ArnDepend*>( thisObject());
    // qDebug() << "DepProto add serv=" << serviceName << " stateName=" << stateName;
    if (dep)  dep->add( serviceName, stateName);
}


void  ArnDepProto::add( const QString& serviceName, int stateId)
{
    ArnDepend*  dep = qscriptvalue_cast<ArnDepend*>( thisObject());
    // qDebug() << "DepProto add serv=" << serviceName << " stateId=" << stateId;
    if (dep)  dep->add( serviceName, stateId);
}


void  ArnDepProto::setMonitorName( const QString& name)
{
    ArnDepend*  dep = qscriptvalue_cast<ArnDepend*>( thisObject());
    // qDebug() << "DepProto set monitorName=" << name;
    if (dep)  dep->setMonitorName( name);;
}


void  ArnDepProto::startMonitor()
{
    ArnDepend*  dep = qscriptvalue_cast<ArnDepend*>( thisObject());
    if (dep)  dep->startMonitor();
}


QScriptValue  ArnDepProto::constructor( QScriptContext* context, QScriptEngine* engine)
{
    if (!context->isCalledAsConstructor()) {
        return context->throwError( QScriptContext::SyntaxError,
                                    "use the 'new' operator");
    }
    ArnDepend*  dep = new ArnDepend;
    // let the engine manage the new object's lifetime.
    return engine->newQObject( dep, QScriptEngine::ScriptOwnership);
}
#endif
