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

#include "ArnInc/ArnRpc.hpp"
#include "ArnInc/ArnM.hpp"
#include <QMetaType>
#include <QMetaMethod>
#include <QTimer>
#include <QRegExp>
#include <QVariant>
#include <QDebug>

using Arn::XStringMap;

#define RPC_STORAGE_NAME "_ArnRpcStorage"

ArnRpc::RpcTypeInfo  ArnRpc::_rpcTypeInfoTab[] = {
//  rpcTypeName  qtTypeName     typeId
    {"int",      "int",         QMetaType::Int},
    {"uint",     "uint",        QMetaType::UInt},
    {"int64",    "qint64",      QMetaType::LongLong},
    {"uint64",   "quint64",     QMetaType::ULongLong},
    {"bool",     "bool",        QMetaType::Bool},
    {"double",   "double",      QMetaType::Double},
    {"bytes",    "QByteArray",  QMetaType::QByteArray},
    {"date",     "QDate",       QMetaType::QDate},
    {"time",     "QTime",       QMetaType::QTime},
    {"datetime", "QDateTime",   QMetaType::QDateTime},
    {"list",     "QStringList", QMetaType::QStringList},
    {"string",   "QString",     QMetaType::QString},
    {"",         "",            QMetaType::Void}  // End marker & Null case
};


//! \cond ADV
class ArnRpcReceiverStorage : public QObject
{
public:
    ArnRpc*  _rpcSender;

    explicit  ArnRpcReceiverStorage( QObject* parent) :
        QObject( parent)
    {
        _rpcSender = 0;
    }
};


class ArnDynamicSignals : public QObject
{
public:
    explicit  ArnDynamicSignals( ArnRpc* rpc);
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


ArnDynamicSignals::ArnDynamicSignals( ArnRpc* rpc) :
    QObject( rpc)
{
    _slotIdCount = QObject::staticMetaObject.methodCount();
    _rpc = rpc;
}


int  ArnDynamicSignals::qt_metacall( QMetaObject::Call call, int id, void **arguments)
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


bool  ArnDynamicSignals::addSignal( QObject *sender, int signalId, QByteArray funcName)
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
    _dynamicSignals      = new ArnDynamicSignals( this);
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
        stat = pipe->openUuid( pipePath);
    else
        stat = pipe->open( pipePath);

    if (stat) {
        pipe->setSendSeq( _mode.is(_mode.SendSequence));
        pipe->setCheckSeq( _mode.is(_mode.CheckSequence));
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
        if (Arn::debugRPC)  qDebug() << "Rpc delete pipe: path=" << _pipe->path();
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

    _receiverStorage = _receiver->findChild<ArnRpcReceiverStorage*>( RPC_STORAGE_NAME);
    if (!_receiverStorage) {
        _receiverStorage = new ArnRpcReceiverStorage( _receiver);
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

    ArnRpcReceiverStorage*  recStore = receiver->findChild<ArnRpcReceiverStorage*>( RPC_STORAGE_NAME);
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
            _pipe->setValueOverwrite( xsmCall.toXString(), rx);
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
    QByteArray  typeName = arg.name();
    int  type = QMetaType::type( typeName.constData());
    if (!type) {
        errorLog( QString(tr("Unknown type:") + typeName.constData()),
                  ArnError::RpcInvokeError);
        return false;
    }
    const RpcTypeInfo&  rpcTypeInfo = typeInfofromId( type);

    //// Get arg label
    QByteArray  argLabel = arg.label();

    //// Export data from arg
    QByteArray  argDataDump;
    QStringList  argDataList;
    bool  isBinaryType = false;
    QVariant  varArg( type, arg.data());
    if (type == QMetaType::QStringList) {
        argDataList = varArg.toStringList();
    }
    else if (type == QMetaType::QByteArray) {
        argDataDump = varArg.toByteArray();
    }
    else if (varArg.canConvert( QVariant::String)) {
        argDataDump = varArg.toString().toUtf8();
    }
    else {
        QDataStream  stream( &argDataDump, QIODevice::WriteOnly);
        stream.setVersion( DATASTREAM_VER);
        stream << quint8(0);  // Spare
        stream << quint8( DATASTREAM_VER);
        if (!QMetaType::save( stream, type, arg.data())) {
            errorLog( QString(tr("Can't export type:") + typeName.constData()),
                      ArnError::RpcInvokeError);
            return false;
        }
        isBinaryType = true;
    }

    //// Make arg key with rpcType or Qt-type
    QByteArray  argKey;
    if (rpcTypeInfo.typeId != QMetaType::Void)
        argKey = rpcTypeInfo.rpcTypeName;
    else {
        argKey = "a";
        xsm.add((isBinaryType ? "tb" : "t"), typeName);
    }
    if (!argLabel.isEmpty())
        argKey += "." + argLabel;

    //// Output argument to xsm
    if (type == QMetaType::QStringList) {  // Handle list
        int i = 0;
        QString  argData;
        if (!argDataList.isEmpty() && !argDataList.at(0).isEmpty()) {
            argData = argDataList.at(0);
            ++i;
        }
        xsm.add( argKey, argData);
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
        xsm.add( argKey, argDataDump);
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

    ArgInfo  argInfo[10];
    int argc = 0;

    // qDebug() << "rpc pipeInput: data=" << data;
    QByteArray  methodName = _methodPrefix + rpcFunc;

    if (_isIncludeSender) {  // MW: more ...
        argInfo[ argc].arg = Q_ARG( ArnRpc*, this);
        ++argc;
    }

    int  argStart = argc;
    bool  stat = true;  // Default ok
    int  index = 1;  // Start after function name in xsm
    while (index > 0) {
        if (index >= xsmCall.size())
            break;  // End of args
        if (argc > 9) {
            errorLog( QString(tr("To many args:") + QString::number( argc))
                      + tr(" method:") + methodName.constData(),
                      ArnError::RpcReceiveError);
            stat = false;
            break;
        }
        stat = xsmLoadArg( xsmCall, argInfo[ argc++], index, methodName);
        if (!stat)  break;
    }

    char  argOrder[10];
    if (stat) {
        for (int i = 0; i < 10; ++i) {
            argOrder[i] = char(i);  // Set default order 1 to 1
        }
        stat = argLogic( argInfo, argOrder, argc, methodName);
    }

    if (stat) {
        for (int i = argStart; i < argc; ++i) {
            stat = importArgData( argInfo[i], methodName);
            if (!stat)  break;
        }
    }

    if (stat) {
        _receiverStorage->_rpcSender = this;
        stat = QMetaObject::invokeMethod( _receiver,
                                          methodName.constData(),
                                          Qt::AutoConnection,
                                          argInfo[ int(argOrder[0])].arg,
                                          argInfo[ int(argOrder[1])].arg,
                                          argInfo[ int(argOrder[2])].arg,
                                          argInfo[ int(argOrder[3])].arg,
                                          argInfo[ int(argOrder[4])].arg,
                                          argInfo[ int(argOrder[5])].arg,
                                          argInfo[ int(argOrder[6])].arg,
                                          argInfo[ int(argOrder[7])].arg,
                                          argInfo[ int(argOrder[8])].arg,
                                          argInfo[ int(argOrder[9])].arg);
        _receiverStorage->_rpcSender = 0;
        if(!stat) {
            errorLog( QString(tr("Can't invoke method:")) + methodName.constData(),
                      ArnError::RpcReceiveError);
            sendText("Can't invoke method, use $help");
        }
    }

    //// Clean up - destroy allocated argument data
    for (int i = argStart; i < argc; ++i) {
        if (argInfo[i].isArgAlloc && argInfo[i].arg.data()) {
            int  type = QMetaType::type( argInfo[i].arg.name());
            QMetaType::destroy( type, argInfo[i].arg.data());
        }
        if (argInfo[i].isDataAlloc) {
            void*  data = const_cast<void*>( argInfo[i].data);
            QMetaType::destroy( argInfo[i].typeId, data);
        }
    }
}


bool  ArnRpc::xsmLoadArg( const XStringMap& xsm, ArgInfo& argInfo, int &index,
                          const QByteArray& methodName)
{
    //// Get arg type info
    const QByteArray&  typeKey = xsm.keyRef( index);
    int  dotPos = typeKey.indexOf('.');
    if (dotPos >=0) {
        argInfo.rpcType = typeKey.left( dotPos);
        argInfo.name    = typeKey.mid( dotPos + 1);
        argInfo.hasType = !argInfo.rpcType.isEmpty();
        argInfo.hasName = !argInfo.name.isEmpty();
    }
    else {
        argInfo.rpcType = typeKey;  // Can be type
        argInfo.name    = typeKey;  // or name
    }

    const RpcTypeInfo&  rpcTypeInfo = typeInfofromRpc( argInfo.rpcType);
    bool  isList = false;
    if (rpcTypeInfo.typeId != QMetaType::Void) {
        argInfo.qtType = rpcTypeInfo.qtTypeName;
        argInfo.typeId = rpcTypeInfo.typeId;
        isList = argInfo.typeId == QMetaType::QStringList;
    }
    else if (typeKey.isEmpty()) {
        argInfo.qtType = "QString";  // Default type;
    }
    else if (typeKey == "tb") {
        argInfo.isBinary = true;
        argInfo.qtType = xsm.valueRef( index).constData();
        ++index;
    }
    else if (typeKey.startsWith("t")) {
        argInfo.qtType = xsm.valueRef( index).constData();
        ++index;
    }    

    if (argInfo.typeId == QMetaType::Void)
        argInfo.typeId = QMetaType::type( argInfo.qtType);
    if (!argInfo.typeId) {
        errorLog( QString(tr("Unknown type:") + argInfo.qtType)
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
        if ((rpcTypeInfo.typeId != QMetaType::Void) || (key == "") || key.startsWith("a"))
            break;  // Found arg
        ++index;
    }
    const QByteArray&  argDataDump = xsm.valueRef( index);
    if (isList) {  // Handle list (QStringList)
        QStringList*  argDataList = new QStringList;
        if (!argDataDump.isEmpty())
            *argDataList += QString::fromUtf8( argDataDump.constData(), argDataDump.size());
        ++index;
        while (index < xsm.size()) {
            const QByteArray&  key = xsm.keyRef( index);
            if ((key != "") && (key != "le"))
                break;
            const QByteArray&  arg = xsm.valueRef( index);
            *argDataList += QString::fromUtf8( arg.constData(), arg.size());
            ++index;
        }
        argInfo.data        = argDataList;
        argInfo.dataAsArg   = true;
        argInfo.isDataAlloc = true;
    }
    else {
        ++index;
        argInfo.data = &argDataDump;
    }

    return true;
}


bool  ArnRpc::argLogic( ArgInfo* argInfo, char* argOrder, int& argc, const QByteArray& methodName)
{
    return true;
}


bool  ArnRpc::importArgData( ArnRpc::ArgInfo& argInfo, const QByteArray& methodName)
{
    if (argInfo.typeId == QMetaType::QByteArray)
        argInfo.dataAsArg = true;

    if (argInfo.dataAsArg) {
        argInfo.arg = QGenericArgument( argInfo.qtType, argInfo.data);
        return true;
    }

    const QByteArray&  argDataDump = *static_cast<const QByteArray*>( argInfo.data);
    if (argInfo.isBinary) {
        if ((argDataDump.size() < 2) || (argDataDump.at(1) != DATASTREAM_VER)) {
            errorLog( QString(tr("Not same DataStream version, method:")) + methodName.constData(),
                      ArnError::RpcReceiveError);
            return false;
        }
#if QT_VERSION >= 0x050000
        void*  argData = QMetaType::create( argInfo.typeId);
#else
        void*  argData = QMetaType::construct( argInfo.typeId);
#endif
        Q_ASSERT( argData);
        argInfo.arg = QGenericArgument( argInfo.qtType, argData);  // Assign arg as it has been allocated
        argInfo.isArgAlloc = true;
        QDataStream  stream( argDataDump);
        stream.setVersion( DATASTREAM_VER);
        stream.skipRawData(2);
        if (!QMetaType::load( stream, argInfo.typeId, argData)) {
            errorLog( QString(tr("Can't' import bin type:") + argInfo.qtType)
                      + tr(" method:") + methodName.constData(),
                      ArnError::RpcReceiveError);
            return false;
        }
    }
    else {  // Textual type
        QVariant  varArg( QString::fromUtf8( argDataDump.constData(), argDataDump.size()));
        if (!varArg.convert( QVariant::Type( argInfo.typeId))) {
            errorLog( QString(tr("Can't' import str type:") + argInfo.qtType)
                      + tr(" method:") + methodName.constData(),
                      ArnError::RpcReceiveError);
            return false;
        }
#if QT_VERSION >= 0x050000
        void*  argData = QMetaType::create( argInfo.typeId, varArg.constData());
#else
        void*  argData = QMetaType::construct( argInfo.typeId, varArg.constData());
#endif
        Q_ASSERT( argData);
        argInfo.arg = QGenericArgument( argInfo.qtType, argData);  // Assign arg as it has been allocated
        argInfo.isArgAlloc = true;
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
    else {
        emit heartBeatReceived();
        if (!_isHeartBeatOk) {
            _isHeartBeatOk = true;
            emit heartBeatChanged( _isHeartBeatOk);
        }
    }

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
    sendText("$heartbeat [<time>|off|off1]");
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
        const char*  rpcType = "";
        const RpcTypeInfo&  rpcTypeInfo = typeInfofromQt( typeName);
        bool  isList = rpcTypeInfo.typeId == QMetaType::QStringList;
        if (rpcTypeInfo.typeId == QMetaType::QString) {
            rpcType = wasListType ? rpcTypeInfo.rpcTypeName : "";
        }
        else if (rpcTypeInfo.typeId != QMetaType::Void) {
            rpcType = rpcTypeInfo.rpcTypeName;
        }
        else {
            rpcType = "a";
            param += (isBinaryType ? "tb" : "t") + QByteArray("=") + typeName + " ";
        }

        if (*rpcType) {
            param += rpcType;
            param += "=";
        }
        param += "<" + parName + ">";
        if (i >= parNumMin)
            param = "[" + param + "]";
        line += " " + QString::fromLatin1( param);

        wasListType = isList;
    }
    sendText( line);
}


void  ArnRpc::destroyPipe()
{
    if (Arn::debugRPC)  qDebug() << "Rpc Destroy pipe: path=" << _pipe->path();
    emit pipeClosed();
    setPipe(0);
}


void  ArnRpc::timeoutHeartBeatSend()
{
    if (!_pipe || !_pipe->isOpen())  return;

    invoke("$heartbeat", Invoke::NoQueue,
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
    if (_pipe)
        *_pipe = "\"" + txt.toUtf8() + "\"";
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


const ArnRpc::RpcTypeInfo&  ArnRpc::typeInfofromRpc( const QByteArray& rpcTypeName)
{
    RpcTypeInfo*  typeInfo = _rpcTypeInfoTab;
    while (typeInfo->typeId != QMetaType::Void) {
        if (rpcTypeName == typeInfo->rpcTypeName)
            break;
        ++typeInfo;
    }

    return *typeInfo;
}


const ArnRpc::RpcTypeInfo&  ArnRpc::typeInfofromQt( const QByteArray& qtTypeName)
{
    RpcTypeInfo*  typeInfo = _rpcTypeInfoTab;
    while (typeInfo->typeId != QMetaType::Void) {
        if (qtTypeName == typeInfo->qtTypeName)
            break;
        ++typeInfo;
    }

    return *typeInfo;
}


const ArnRpc::RpcTypeInfo&  ArnRpc::typeInfofromId( int typeId)
{
    RpcTypeInfo*  typeInfo = _rpcTypeInfoTab;
    while (typeInfo->typeId != QMetaType::Void) {
        if (typeId == typeInfo->typeId)
            break;
        ++typeInfo;
    }

    return *typeInfo;
}
