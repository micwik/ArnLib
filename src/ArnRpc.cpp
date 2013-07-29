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

#include "ArnRpc.hpp"
#include <QMetaType>
#include <QMetaMethod>
#include <QTimer>
#include <QRegExp>
#include <QVariant>
#include <QDebug>

#define RPC_STORAGE_NAME "_ArnRpcStorage"


//! \cond ADV
class RpcReceiverStorage : public QObject
{
public:
    ArnRpc*  _rpcSender;

    explicit  RpcReceiverStorage( QObject* parent) :
        QObject( parent)
    {
        _rpcSender = 0;
    }
};


class DynamicSignals : public QObject
{
public:
    explicit  DynamicSignals( ArnRpc* rpc);
    int  qt_metacall( QMetaObject::Call call, int id, void **arguments);
    bool  addSignal( QObject *sender, int signalId, QByteArray funcName);

private:
    struct Slot {
        QByteArray  funcName;
        QList<QByteArray>  typeNames;
        QList<QByteArray>  parNames;
        ArnRpc::Invoke  invokeFlags;
    };
    QList<Slot>  _slotTab;

    int  _slotIdCount;
    ArnRpc*  _rpc;
};


DynamicSignals::DynamicSignals( ArnRpc* rpc) :
    QObject( rpc)
{
    _slotIdCount = QObject::staticMetaObject.methodCount();
    _rpc = rpc;
}


int  DynamicSignals::qt_metacall( QMetaObject::Call call, int id, void **arguments)
{
    id = QObject::qt_metacall( call, id, arguments);
    if ((id < 0) || (call != QMetaObject::InvokeMetaMethod))
        return id;

    Q_ASSERT(id < _slotTab.size());

    MQGenericArgument  args[8];
    const Slot&  slot = _slotTab.at( id);
    int  argNum = slot.typeNames.size();

    for (int i = 0; i < argNum; ++i) {
        args[i] = MQGenericArgument( slot.typeNames.at(i).constData(),
                                     slot.parNames.at(i).constData(),
                                     arguments[i + 1]);
    }
    _rpc->invoke( QString::fromLatin1( slot.funcName),
                  slot.invokeFlags,
                  args[0],
                  args[1],
                  args[2],
                  args[3],
                  args[4],
                  args[5],
                  args[6],
                  args[7]);

    return -1;
}


bool  DynamicSignals::addSignal( QObject *sender, int signalId, QByteArray funcName)
{
    const QMetaObject*  metaObject = sender->metaObject();
    const QMetaMethod&  method = metaObject->method( signalId);

    Slot  slot;
    slot.funcName    = funcName;
    slot.typeNames   = method.parameterTypes();
    slot.parNames    = method.parameterNames();
    ArnRpc::Invoke  ivf;
    ivf.set( ivf.NoQueue, QByteArray( method.tag()) == "no_queue");
    slot.invokeFlags = ivf;
    _slotTab += slot;

    bool  status = QMetaObject::connect( sender, signalId, this, _slotIdCount);
    ++_slotIdCount;
    return status;
}
//! \endcond


ArnRpc::ArnRpc( QObject* parent) :
    QObject( parent)
{
    _pipe                = 0;
    _receiver            = 0;
    _receiverStorage     = 0;
    _isIncludeSender     = false;
    _dynamicSignals      = new DynamicSignals( this);
    _isHeartBeatOk       = true;
    _timerHeartBeatSend  = new QTimer( this);
    _timerHeartBeatCheck = new QTimer( this);

    connect( _timerHeartBeatSend, SIGNAL(timeout()), this, SLOT(timeoutHeartBeatSend()));
    connect( _timerHeartBeatCheck, SIGNAL(timeout()), this, SLOT(timeoutHeartBeatCheck()));
}


QString  ArnRpc::pipePath()  const
{
    if (!_pipe)  return QString();

    return _pipe->path();
}


bool  ArnRpc::open( QString pipePath)
{
    setPipe(0);  // Remove any existing pipe

    ArnPipe*  pipe = new ArnPipe;
    //pipe->setPipeMode();
    if (!_mode.is(_mode.Provider))
        pipe->setMaster();
    if (_mode.is(_mode.AutoDestroy))
        pipe->setAutoDestroy();

    bool  stat;
    if (_mode.is(_mode.UuidPipe))
        stat = pipe->openUuidPipe( pipePath);
    else
        stat = pipe->open( pipePath);

    if (stat) {
        pipe->setUseSendSeq( _mode.is(_mode.SendSequence));
        pipe->setUseCheckSeq( _mode.is(_mode.CheckSequence));
    }
    else {
        delete pipe;
        return false;
    }

    setPipe( pipe);
    return true;
}


void  ArnRpc::setPipe( ArnPipe* pipe)
{
    if (_pipe) {
        qDebug() << "Rpc delete pipe: path=" << _pipe->path();
        _pipe->deleteLater();
    }
    _pipe = pipe;
    if (_pipe) {
        _pipe->setParent( this);
        connect( _pipe, SIGNAL(changed(QByteArray)), this, SLOT(pipeInput(QByteArray)),
                 Qt::QueuedConnection);
        connect( _pipe, SIGNAL(arnLinkDestroyed()), this, SLOT(destroyPipe()));
        connect( _pipe, SIGNAL(outOfSequence()), this, SIGNAL(outOfSequence()));
    }
}


void  ArnRpc::setReceiver( QObject *receiver)
{
    _receiver = receiver;

    _receiverStorage = _receiver->findChild<RpcReceiverStorage*>( RPC_STORAGE_NAME);
    if (!_receiverStorage) {
        _receiverStorage = new RpcReceiverStorage( _receiver);
        _receiverStorage->setObjectName( RPC_STORAGE_NAME);
    }
}


void  ArnRpc::setMethodPrefix( QString prefix)
{
    _methodPrefix = prefix.toLatin1();
}


void  ArnRpc::setIncludeSender( bool v)
{
    _isIncludeSender = v;
}


void  ArnRpc::setMode( Mode mode)
{
    _mode = mode;
}


ArnRpc::Mode  ArnRpc::mode()  const
{
    return _mode;
}


void  ArnRpc::setHeartBeatSend( int time)
{
    if (time == 0)
        _timerHeartBeatSend->stop();
    else
        _timerHeartBeatSend->start( time * 1000);
}


void  ArnRpc::setHeartBeatCheck( int time)
{
    if (time == 0)
        _timerHeartBeatCheck->stop();
    else
        _timerHeartBeatCheck->start( time * 1000);
}


bool  ArnRpc::isHeartBeatOk()  const
{
    return _isHeartBeatOk;
}


void  ArnRpc::addSenderSignals( QObject* sender, QString prefix)
{
    const QMetaObject*  metaObject = sender->metaObject();
    int  methodCount = metaObject->methodCount();
    QByteArray  methodSignCompHead;
    for (int i = 0; i < methodCount; ++i) {
        const QMetaMethod&  method = metaObject->method(i);
        if (method.methodType() != QMetaMethod::Signal)  continue;

        QByteArray  methodSign = methodSignature( method);
        if (!methodSign.startsWith( prefix.toLatin1()))  continue;

        QByteArray  methodName = methodSign.left( methodSign.indexOf('('));
        QByteArray  funcName   = methodName.mid( prefix.size());

        QByteArray  methodSignComp = methodSign;
        methodSignComp.chop(1);  // Remove last ")"
        if (!_mode.is( Mode::NoDefaultArgs)  // When using Default args ...
        && methodSignCompHead.startsWith( methodSignComp))  // Same method with less param
            continue;  // Skip it, otherwise multiple rpc calls.

        methodSignCompHead = methodSignComp;
        _dynamicSignals->addSignal( sender, i, funcName);
    }
}


ArnRpc*  ArnRpc::rpcSender()
{
    if (!_receiverStorage)  return 0;

    return _receiverStorage->_rpcSender;
}


ArnRpc*  ArnRpc::rpcSender( QObject *receiver)
{
    if (!receiver)  return 0;

    RpcReceiverStorage*  recStore = receiver->findChild<RpcReceiverStorage*>( RPC_STORAGE_NAME);
    if (!recStore)  return 0;

    return recStore->_rpcSender;
}


bool  ArnRpc::invoke( const QString& funcName,
                      MQGenericArgument arg1,
                      MQGenericArgument arg2,
                      MQGenericArgument arg3,
                      MQGenericArgument arg4,
                      MQGenericArgument arg5,
                      MQGenericArgument arg6,
                      MQGenericArgument arg7,
                      MQGenericArgument arg8)
{
    if (!_pipe || !_pipe->isOpen()) {
        errorLog( QString(tr("Pipe not open")),
                  ArnError::RpcInvokeError);
        return false;
    }

    XStringMap  xsmCall;
    xsmCall.add("", funcName.toLatin1());

    int  nArg = 0;
    bool stat = true;  // Default ok
    stat &= xsmAddArg( xsmCall, arg1, 1, nArg);
    stat &= xsmAddArg( xsmCall, arg2, 2, nArg);
    stat &= xsmAddArg( xsmCall, arg3, 3, nArg);
    stat &= xsmAddArg( xsmCall, arg4, 4, nArg);
    stat &= xsmAddArg( xsmCall, arg5, 5, nArg);
    stat &= xsmAddArg( xsmCall, arg6, 6, nArg);
    stat &= xsmAddArg( xsmCall, arg7, 7, nArg);
    stat &= xsmAddArg( xsmCall, arg8, 8, nArg);

    if (stat) {
        *_pipe = xsmCall.toXString();
    }
    return stat;
}


bool  ArnRpc::invoke( const QString& funcName,
                      Invoke  invokeFlags,
                      MQGenericArgument arg1,
                      MQGenericArgument arg2,
                      MQGenericArgument arg3,
                      MQGenericArgument arg4,
                      MQGenericArgument arg5,
                      MQGenericArgument arg6,
                      MQGenericArgument arg7,
                      MQGenericArgument arg8)
{
    if (!_pipe || !_pipe->isOpen()) {
        errorLog( QString(tr("Pipe not open")),
                  ArnError::RpcInvokeError);
        return false;
    }

    XStringMap  xsmCall;
    xsmCall.add("", funcName.toLatin1());

    int  nArg = 0;
    bool stat = true;  // Default ok
    stat &= xsmAddArg( xsmCall, arg1, 1, nArg);
    stat &= xsmAddArg( xsmCall, arg2, 2, nArg);
    stat &= xsmAddArg( xsmCall, arg3, 3, nArg);
    stat &= xsmAddArg( xsmCall, arg4, 4, nArg);
    stat &= xsmAddArg( xsmCall, arg5, 5, nArg);
    stat &= xsmAddArg( xsmCall, arg6, 6, nArg);
    stat &= xsmAddArg( xsmCall, arg7, 7, nArg);
    stat &= xsmAddArg( xsmCall, arg8, 8, nArg);

    if (stat) {
        if (invokeFlags.is( Invoke::NoQueue)) {
            QRegExp rx("^" + funcName + "\\b");
            _pipe->setValuePipeOverwrite( xsmCall.toXString(), rx);
        }
        else
            *_pipe = xsmCall.toXString();
    }
    return stat;
}


bool  ArnRpc::xsmAddArg( XStringMap& xsm, const MQGenericArgument& arg, uint index, int& nArg)
{
    if (!arg.name() || !arg.label() || !arg.data())  return true;  // Empty arg
    if (nArg + 1 != int( index))  return true;  // Out of seq - Finished

    //// Get arg type info
    bool  isListType = false;
    QByteArray  typeName = arg.name();
    if (typeName == "QStringList") {
        isListType = true;
    }
    int  type = QMetaType::type( typeName.constData());
    if (!type) {
        errorLog( QString(tr("Unknown type:") + typeName.constData()),
                  ArnError::RpcInvokeError);
        return false;
    }

    //// Get arg label
    QByteArray  argLabel = arg.label();

    //// Export data from arg
    QByteArray  argDataDump;
    QStringList  argDataList;
    bool  isBinaryType = false;
    QVariant  varArg( type, arg.data());
    if (isListType) {
        argDataList = varArg.toStringList();
    }
    else if (varArg.canConvert( QVariant::String)) {
        argDataDump = varArg.toString().toUtf8();
    }
    else {
        QDataStream  stream( &argDataDump, QIODevice::WriteOnly);
        if (!QMetaType::save( stream, type, arg.data())) {
            errorLog( QString(tr("Can't export type:") + typeName.constData()),
                      ArnError::RpcInvokeError);
            return false;
        }
        isBinaryType = true;
    }

    //// Make arg prefix and/or type
    QByteArray  argPrefix;
    if (typeName == "int")
        argPrefix = "int";
    else if (typeName == "uint")
        argPrefix = "uint";
    else if (typeName == "bool")
        argPrefix = "bool";
    else if (typeName == "QByteArray")
        argPrefix = "ba";
    else if (typeName == "QStringList")
        argPrefix = "list";
    else if (typeName != "QString")
        xsm.add((isBinaryType ? "tb" : "t"), typeName);

    //// Output argument to xsm
    if (argPrefix.isEmpty())
        argPrefix = "a";
    if (!argLabel.isEmpty())
        argPrefix += ".";
    if (isListType) {  // Handle list (QStringList)
        int i = 0;
        QString  argData;
        if (!argDataList.isEmpty() && !argDataList.at(0).isEmpty()) {
            argData = argDataList.at(0);
            ++i;
        }
        xsm.add( argPrefix + argLabel, argData);
        // Output each element in list
        for(; i < argDataList.size(); ++i) {
            argData = argDataList.at(i);
            if (argData.isEmpty())
                xsm.add("le", "");
            else
                xsm.add("", argData);
        }
    }
    else {
        xsm.add( argPrefix + argLabel, argDataDump);
    }

    ++nArg;
    return true;  // 1 arg added
}


void  ArnRpc::pipeInput( QByteArray data)
{
    if (data.startsWith('"')) {  // Text is received
        int  endSize = data.endsWith('"') ? (data.size() - 1) : data.size();
        if (endSize < 1)
            endSize = 1;
        emit textReceived( QString::fromUtf8( data.mid( 1, endSize - 1), endSize - 1));
        return;
    }

    XStringMap  xsmCall( data);
    QByteArray  rpcFunc = xsmCall.value(0);
    if (rpcFunc == "$heartbeat") {  // Built in Heart beat support
        return funcHeartBeat( xsmCall);
    }
    if (rpcFunc == "$help") {  // Built in Help
        return funcHelp( xsmCall);
    }

    QGenericArgument  args[10];
    int argc = 0;

    // qDebug() << "rpc pipeInput: data=" << data;
    QByteArray  methodName = _methodPrefix + rpcFunc;

    if (_isIncludeSender) {
        args[ argc++] = Q_ARG( ArnRpc*, this);
    }

    int  argStart = argc;
    bool  stat = true;  // Default ok
    int  index = 1;  // Start after function name in xsm
    while (index > 0) {
        if ((index >= xsmCall.size()) || (xsmCall.key( index) == "n"))
            break;  // End of args
        if (argc > 9) {
            errorLog( QString(tr("To many args:") + QString::number( argc))
                      + tr(" method:") + methodName.constData(),
                      ArnError::RpcReceiveError);
            return;
        }
        stat = xsmLoadArg( xsmCall, args[ argc++], index, methodName);
        if (!stat)  break;
    }

    if (stat) {
        _receiverStorage->_rpcSender = this;
        stat = QMetaObject::invokeMethod( _receiver,
                                          methodName.constData(),
                                          Qt::AutoConnection,
                                          args[0],
                                          args[1],
                                          args[2],
                                          args[3],
                                          args[4],
                                          args[5],
                                          args[6],
                                          args[7],
                                          args[8],
                                          args[9]);
        _receiverStorage->_rpcSender = 0;
        if(!stat) {
            errorLog( QString(tr("Can't invoke method:")) + methodName.constData(),
                      ArnError::RpcReceiveError);
            sendText("Can't invoke method, use $help");
        }
    }

    //// Clean up - destroy allocated argument data
    for (int i = argStart; i < argc; ++i) {
        int  type = QMetaType::type( args[i].name());
        if (args[i].data()) {
            QMetaType::destroy( type, args[i].data());
        }
    }
}


bool  ArnRpc::xsmLoadArg( const XStringMap& xsm, QGenericArgument& arg, int &index,
                               const QByteArray& methodName)
{
    static const QByteArray  qlistName   = "QStringList";
    static const QByteArray  qstringName = "QString";
    static const QByteArray  qbaName     = "QByteArray";
    static const QByteArray  intName     = "int";
    static const QByteArray  uintName    = "uint";
    static const QByteArray  boolName    = "bool";

    //// Get arg type info
    const QByteArray*  typeName = &qstringName;  // Default type
    bool  isBinaryType = false;
    bool  isListType   = false;
    bool  argInType    = false;
    if (xsm.keyRef( index).startsWith("f")) {  // Legacy
        isBinaryType = true;
        ++index;
    }
    const QByteArray&  typeKey = xsm.keyRef( index);
    if (typeKey == "tb") {
        isBinaryType = true;
        typeName = &xsm.valueRef( index);
        ++index;
    }
    else if (typeKey.startsWith("t")) {
        typeName = &xsm.valueRef( index);
        ++index;
    }
    else if (typeKey.startsWith("int")) {
        typeName  = &intName;
        argInType = true;
    }
    else if (typeKey.startsWith("uint")) {
        typeName  = &uintName;
        argInType = true;
    }
    else if (typeKey.startsWith("bool")) {
        typeName  = &boolName;
        argInType = true;
    }
    else if (typeKey.startsWith("ba")) {
        typeName  = &qbaName;
        argInType = true;
    }
    else if (typeKey.startsWith("li")) {
        typeName   = &qlistName;
        isListType = true;
        argInType  = true;
    }
    int  type = QMetaType::type( typeName->constData());
    if (!type) {
        errorLog( QString(tr("Unknown type:") + typeName->constData())
                  + tr(" method:") + methodName.constData(),
                  ArnError::RpcReceiveError);
        return false;
    }
    //qDebug() << "--- typeName=" << typeName->constData() << " type=" << type;

    //// Get arg data
    forever {
        if (index >= xsm.size()) {
            index = -1;  // Mark no more args
            return true;
        }
        const QByteArray  key = xsm.keyRef( index);
        if ( argInType || (key == "") || key.startsWith("a"))
            break;  // Found arg
        ++index;
    }
    const QByteArray&  argDataDump = xsm.valueRef( index);
    QStringList  argDataList;
    if (isListType) {  // Handle list (QStringList)
        if (!argDataDump.isEmpty())
            argDataList += QString::fromUtf8( argDataDump.constData(), argDataDump.size());
        ++index;
        while (index < xsm.size()) {
            const QByteArray  key = xsm.keyRef( index);
            if ((key != "") && (key != "le"))
                break;
            const QByteArray  arg = xsm.valueRef( index);
            argDataList += QString::fromUtf8( arg.constData(), arg.size());
            ++index;
        }
    }
    else {
        ++index;
    }

    //// Import data to arg
    if (isListType) {
        QVariant  varArg( argDataList);
#if QT_VERSION >= 0x050000
        void*  argData = QMetaType::create( type, varArg.constData());
#else
        void*  argData = QMetaType::construct( type, varArg.constData());
#endif
        Q_ASSERT( argData);
        arg = QGenericArgument( typeName->constData(), argData);  // Assign arg as it has been allocated
    }
    else if (isBinaryType) {
#if QT_VERSION >= 0x050000
        void*  argData = QMetaType::create( type);
#else
        void*  argData = QMetaType::construct( type);
#endif
        Q_ASSERT( argData);
        arg = QGenericArgument( typeName->constData(), argData);  // Assign arg as it has been allocated
        QDataStream  stream( argDataDump);
        if (!QMetaType::load( stream, type, argData)) {
            errorLog( QString(tr("Can't' import ds type:") + typeName->constData())
                      + tr(" method:") + methodName.constData(),
                      ArnError::RpcReceiveError);
            return false;
        }
    }
    else {
        QVariant  varArg( QString::fromUtf8( argDataDump.constData(), argDataDump.size()));
        if (!varArg.convert( QVariant::Type( type))) {
            errorLog( QString(tr("Can't' import str type:") + typeName->constData())
                      + tr(" method:") + methodName.constData(),
                      ArnError::RpcReceiveError);
            return false;
        }
#if QT_VERSION >= 0x050000
        void*  argData = QMetaType::create( type, varArg.constData());
#else
        void*  argData = QMetaType::construct( type, varArg.constData());
#endif
        Q_ASSERT( argData);
        arg = QGenericArgument( typeName->constData(), argData);  // Assign arg as it has been allocated
    }

    return true;
}


void  ArnRpc::funcHeartBeat( const XStringMap& xsm)
{
    QByteArray  time = xsm.value(1);
    if (time == "off") {  // Remote turn off heart beat function for both directions
        _timerHeartBeatSend->stop();
        invoke("$heartbeat", MQ_ARG( QString, time, "off1"));
    }
    if (time == "off1") {  // Remote turn off heart beat function for this direction
        _timerHeartBeatSend->stop();
        invoke("$heartbeat", MQ_ARG( QString, time, "0"));
    }
    else if (time == "0") {  // Remote turn off heart beat checking for this direction
        _timerHeartBeatCheck->stop();
        _isHeartBeatOk = true;
    }
    else
        emit heartBeatReceived();

    if (_timerHeartBeatCheck->isActive())
        _timerHeartBeatCheck->start();  // Restart heart beat check timer
}


void  ArnRpc::funcHelp( const XStringMap& xsm)
{
    Q_UNUSED(xsm)

    const QMetaObject*  metaObject = _receiver->metaObject();
    int  methodIndexHead = -1;
    int  parCountMin = 10;
    QByteArray  methodNameHead;
    QByteArray  methodSignHead;
    int  methodCount = metaObject->methodCount();
    for (int i = 0; i < methodCount; ++i) {
        const QMetaMethod&  method = metaObject->method(i);
        QByteArray  methodSign = methodSignature( method);
        if (!methodSign.startsWith( _methodPrefix))  continue;

        methodSign.chop(1);  // Remove last ")"
        QList<QByteArray>  parNames = method.parameterNames();
        int  parCount = parNames.size();

        if (methodSignHead.startsWith( methodSign)) {  // Same method with less param
            parCountMin = parCount;
        }
        else {
            if (methodIndexHead >= 0)
                funcHelpMethod( metaObject->method( methodIndexHead),
                                methodNameHead, parCountMin);
            methodIndexHead = i;
            methodSignHead  = methodSign;
            methodNameHead  = methodSign.left( methodSign.indexOf('('));
            parCountMin     = parCount;
        }
    }
    if (methodIndexHead >= 0)
        funcHelpMethod( metaObject->method( methodIndexHead), methodNameHead, parCountMin);
    sendText("$heartbeat [time|off|off1]");
    sendText("$help");
}


void  ArnRpc::funcHelpMethod( const QMetaMethod &method, QByteArray name, int parNumMin)
{
    QString  line = QString::fromLatin1( name.mid( _methodPrefix.size()));

    QList<QByteArray>  typesNames = method.parameterTypes();
    QList<QByteArray>  parNames   = method.parameterNames();

    bool  wasListType = false;
    int  parCount = parNames.size();
    for (int i = _isIncludeSender; i < parCount; ++i) {
        QByteArray  param;
        QByteArray  parName = parNames.at(i);
        QByteArray  typeName = typesNames.at(i);

        int  type = QMetaType::type( typeName.constData());
        QVariant  varPar( type);
        bool  isBinaryType = (type == 0) || !varPar.canConvert( QVariant::String);
        bool  isListType   = false;
        QByteArray  parType;
        if (typeName == "int")
            parType = "int";
        else if (typeName == "uint")
            parType = "uint";
        else if (typeName == "bool")
            parType = "bool";
        else if (typeName == "QByteArray")
            parType = "ba";
        else if (typeName == "QStringList") {
            parType = "list";
            isListType = true;
        }
        else if (typeName == "QString")
            parType = wasListType ? "a" : "";
        else {
            parType = "a";
            param += (isBinaryType ? "tb" : "t") + QByteArray("=") + typeName + " ";
        }

        if (!parType.isEmpty())
            param += parType + "=";
        param += "<" + parName + ">";
        if (i >= parNumMin)
            param = "[" + param + "]";
        line += " " + QString::fromLatin1( param);

        wasListType = isListType;
    }
    sendText( line);
}


void  ArnRpc::destroyPipe()
{
    qDebug() << "Rpc Destroy pipe: path=" << _pipe->path();
    emit pipeClosed();
    setPipe(0);
}


void  ArnRpc::timeoutHeartBeatSend()
{
    invoke("$heartbeat",
           MQ_ARG( QString, time, QByteArray::number( _timerHeartBeatSend->interval() / 1000)));
}


void  ArnRpc::timeoutHeartBeatCheck()
{
    if (_isHeartBeatOk) {
        _isHeartBeatOk = false;
        emit heartBeatChanged( _isHeartBeatOk);
    }
}


void  ArnRpc::sendText( QString txt)
{
    *_pipe = "\"" + txt + "\"";
}


void  ArnRpc::errorLog( QString errText, ArnError err, void* reference)
{
    QString  idText;
    if (_pipe) {
        idText += " pipe:" + _pipe->path();
    }
    ArnM::errorLog( errText + idText, err, reference);
}


void  ArnRpc::batchConnect( const QObject *sender, const QRegExp &rgx,
                                 const QObject *receiver, const QString& replace,
                                 Mode mode)
{
    QList<QByteArray>  signalSignTab;
    QList<QByteArray>  methodSignLowTab;
    if (mode.is( mode.Debug))
        qDebug() << "batchConnect: regExp=" << rgx.pattern() << " replace=" << replace;

    //// Find matching signals in sender and convert to corresponding methods
    const QMetaObject*  metaObject = sender->metaObject();
    QList<QByteArray>  doneMethodSignTab;
    int  methodCount = metaObject->methodCount();
    for (int i = 0; i < methodCount; ++i) {
        const QMetaMethod&  method = metaObject->method(i);
        if (method.methodType() != QMetaMethod::Signal)  continue;  // Must be a Signal

        QByteArray  signalSign = methodSignature( method);
        if (doneMethodSignTab.contains( signalSign))  continue;  // Already done (inherited)
        doneMethodSignTab += signalSign;

        QString  signalName = QString::fromLatin1( signalSign.left( signalSign.indexOf('(')).constData());
        QByteArray  paramSign  = signalSign.mid( signalName.size());
        if (rgx.indexIn( signalName) >= 0) {  // Match of signal in regExp
            QString  methodName = replace;
            for (int j = 1; j <= rgx.captureCount(); ++j) {
                methodName.replace("\\" + QString::number(j), rgx.cap(j));
            }
            signalSignTab    += signalSign;
            QByteArray  methodSignLow = (methodName.toLatin1() + paramSign).toLower();
            methodSignLowTab += methodSignLow;
            if (mode.is( mode.Debug))
                qDebug() << "batchConnect: try match signal=" << signalSign <<
                            " method(low)=" << methodSignLow;
        }
        else if (mode.is( mode.Debug))
            qDebug() << "batchConnect: No regExp match on signal=" << signalSign;
    }

    //// Find matching methods in receiver and connect
    metaObject = receiver->metaObject();
    methodCount = metaObject->methodCount();
    doneMethodSignTab.clear();
    QByteArray  methodSignCompHead;
    for (int i = 0; i < methodCount; ++i) {
        const QMetaMethod&  method = metaObject->method(i);
        QByteArray  methodSign = methodSignature( method);
        if (doneMethodSignTab.contains( methodSign))  continue;  // Already done (inherited)
        doneMethodSignTab += methodSign;

        QByteArray  methodSignComp = methodSign;
        methodSignComp.chop(1);  // Remove last ")"
        if (!mode.is( mode.NoDefaultArgs)  // When using Default args ...
        && methodSignCompHead.startsWith( methodSignComp))  // Same method with less param
            continue;  // Skip it, otherwise multiple calls.
        methodSignCompHead = methodSignComp;

        QByteArray  methodSignLow = methodSign.toLower();
        int index = methodSignLowTab.indexOf( methodSignLow);
        if (index >= 0) {  // Match signal to method
            const char*  methodPrefix = (method.methodType() == QMetaMethod::Signal) ? "2" : "1";
            connect( sender, "2" + signalSignTab[ index],
                     receiver, methodPrefix + methodSign);
            if (mode.is( mode.Debug))
                qDebug() << "batchConnect: connect signal=" << signalSignTab[ index] <<
                            " method=" << methodSign;
        }
        else if (mode.is( mode.Debug))
            qDebug() << "batchConnect: No match on method=" << methodSignLow;
    }
}


QByteArray  ArnRpc::methodSignature( const QMetaMethod &method)
{
#if QT_VERSION >= 0x050000
    return method.methodSignature();
#else
    return QByteArray( method.signature());
#endif
}

