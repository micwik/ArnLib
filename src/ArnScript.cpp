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

#include "ArnScript.hpp"
#include "ArnDepend.hpp"
#include "ArnMonitor.hpp"
#include <QtScript>
#include <QScriptValue>
#include <QScriptEngine>
#include <QFile>
#include <QDebug>

Q_DECLARE_METATYPE(ArnItem*)
Q_DECLARE_METATYPE(ArnMonitor*)
Q_DECLARE_METATYPE(ArnDependOffer*)
Q_DECLARE_METATYPE(ArnDepend*)


ArnScript::ArnScript( QObject* parent) :
    QObject( parent)
{
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

    _engine->setDefaultPrototype( qMetaTypeId<ArnItem*>(),        itemProtoScr);
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
    // Legacy RegistryLib
    _engine->globalObject().setProperty("RegistryItem",        itemConstrScr);
    _engine->globalObject().setProperty("RegistryNetMon",      monitorConstrScr);
    _engine->globalObject().setProperty("RegistryDependOffer", depOfferConstrScr);
    _engine->globalObject().setProperty("RegistryDepend",      depConstrScr);

    // Add properties to prototypes (manually)
    itemProtoScr.setProperty("num", _engine->newFunction( ArnItemProto::getSetNum),
                    QScriptValue::PropertyGetter|QScriptValue::PropertySetter);
    //// End define prototypes

    _engine->globalObject().setProperty( "arn", _engine->newQObject( new ArnInterface( this),
            QScriptEngine::QtOwnership, QScriptEngine::ExcludeSuperClassContents));
    // Legacy RegistryLib
    _engine->globalObject().setProperty( "registry", _engine->newQObject( new ArnInterface( this),
            QScriptEngine::QtOwnership, QScriptEngine::ExcludeSuperClassContents));
}


bool  ArnScript::evaluate( QByteArray script, QString idName)
{
    _idName = idName;
    QScriptValue  result = _engine->evaluate( QString::fromUtf8( script.constData()));
    if (logUncaughtError( result)) {
        return false;
    }
    return true;
}


bool  ArnScript::evaluateFile(QString fileName)
{
    QFile  file( fileName);
    file.open( QIODevice::ReadOnly);

    return evaluate( file.readAll(), fileName);
}


bool  ArnScript::logUncaughtError( QScriptValue& scriptValue)
{
    //qDebug() << "logUncaughtError: has=" << _engine->hasUncaughtException();
    if (_engine->hasUncaughtException()) {
        QString  errDesc = scriptValue.toString();
        if (!errDesc.isEmpty()) {
            int lineNo = _engine->uncaughtExceptionLineNumber();
            errorLog( errDesc + " @line:" + QString::number( lineNo),
                      ArnError::ScriptError);
        }
        return true;
    }
    return false;
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


void  ArnScript::errorLog( QString errText, ArnError err, void* reference)
{
    QString  scriptText = " Script:" + _idName;

    ArnM::errorLog( errText + scriptText, err, reference);
    emit errorText( errText);
}


ArnClient*  ArnScript::getClient( QString /*clientId*/)
{
    return 0;
}


///////// ArnItem

ArnItemProto::ArnItemProto( ArnScript* parent)
: QObject( parent)
{
}


QString  ArnItemProto::path() const
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  return item->path();
    return QString();
}


void  ArnItemProto::setPath( const QString &path)
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  item->open( path);
}


QVariant  ArnItemProto::value() const
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  return item->toVariant();
    return QVariant();
}


void  ArnItemProto::setValue( const QVariant &value)
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  item->setValue( value);
}


QString  ArnItemProto::string() const
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  return item->toString();
    return QString();
}


void  ArnItemProto::setString( const QString &value)
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  item->setValue( value);
}


bool  ArnItemProto::isPipeMode() const
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  return item->isPipeMode();
    return false;
}


void  ArnItemProto::setPipeMode( bool /*isPipeMode*/)
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  item->setPipeMode();
}


bool  ArnItemProto::isMaster() const
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  return item->isMaster();
    return false;
}


void  ArnItemProto::setMaster( bool /*isMaster*/)
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)
        item->setMaster();
}


bool  ArnItemProto::isAutoDestroy() const
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  return item->isAutoDestroy();
    return false;
}


void  ArnItemProto::setAutoDestroy( bool /*isAutoDestroy*/)
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  item->setAutoDestroy();
}


bool  ArnItemProto::isSaveMode() const
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  return item->isSaveMode();
    return false;
}


void  ArnItemProto::setSaveMode( bool /*isSaveMode*/)
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  item->setSaveMode();
}


bool  ArnItemProto::isTemplate() const
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  return item->isTemplate();
    return false;
}


void  ArnItemProto::setTemplate( bool isTemplate)
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( thisObject());
    if (item)  item->setTemplate( isTemplate);
}


QScriptValue ArnItemProto::getSetNum(QScriptContext* context, QScriptEngine* engine)
{
    ArnItem*  item = qscriptvalue_cast<ArnItem*>( context->thisObject());
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

    ArnItem*  arnItem;
    if (context->argumentCount() >= 2) {  // (Item, Path) as arguments
        ArnItem*  item = qscriptvalue_cast<ArnItem*>( context->argument(0));
        if (!item) {
            return context->throwError(QScriptContext::TypeError,
                                       "is not ArnItem as first argument");
        }
        QString  path = qscriptvalue_cast<QString>( context->argument(1));
        if (path.isNull()) {
            return context->throwError(QScriptContext::TypeError,
                                       "is not String (path) as second argument");
        }
        arnItem = new ArnItem( *item, path);
    }
    else if (context->argumentCount() >= 1) {  // Path as argument
        QString  path = qscriptvalue_cast<QString>( context->argument(0));
        if (path.isNull()) {
            return context->throwError(QScriptContext::TypeError,
                                       "is not String (path) as first argument");
        }
        arnItem = new ArnItem( path);
    }
    else {  // No argument
        arnItem = new ArnItem;
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
        ArnClient*  client = arnScr->getClient( id);
        arnMon->setClient( client, id);
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
    if (arnMon)  arnMon->setMonitorPath( name);
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


void  ArnDepOfferProto::advertise( QString serviceName)
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


void  ArnDepProto::add( QString serviceName, QString stateName)
{
    ArnDepend*  dep = qscriptvalue_cast<ArnDepend*>( thisObject());
    // qDebug() << "DepProto add serv=" << serviceName << " stateName=" << stateName;
    if (dep)  dep->add( serviceName, stateName);
}


void  ArnDepProto::add( QString serviceName, int stateId)
{
    ArnDepend*  dep = qscriptvalue_cast<ArnDepend*>( thisObject());
    // qDebug() << "DepProto add serv=" << serviceName << " stateId=" << stateId;
    if (dep)  dep->add( serviceName, stateId);
}


void  ArnDepProto::setMonitorName( QString name)
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
