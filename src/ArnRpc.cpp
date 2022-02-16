// Copyright (C) 2010-2022 Michael Wiklund.
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

#include "ArnInc/ArnRpc.hpp"
#include "private/ArnRpc_p.hpp"
#include "ArnInc/ArnM.hpp"
#include "ArnInc/XStringMap.hpp"
#include "ArnInc/ArnLib.hpp"
#include "ArnInc/ArnCompat.hpp"
#include <QMetaType>
#include <QMetaMethod>
#include <QTimer>
#include <QDataStream>
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
    {"float",    "float",       QMetaType::Float},
    {"double",   "double",      QMetaType::Double},
    {"bytes",    "QByteArray",  QMetaType::QByteArray},
    {"date",     "QDate",       QMetaType::QDate},
    {"time",     "QTime",       QMetaType::QTime},
    {"datetime", "QDateTime",   QMetaType::QDateTime},
    {"list",     "QStringList", QMetaType::QStringList},
    {"string",   "QString",     QMetaType::QString},
    {"",         "",            0}  // End marker & Null case
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
    bool  addSignal( QObject *sender, int signalId, const QByteArray& funcName);

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
    if (Arn::debugRPC)  qDebug() << "Rpc metaCall-1: call=" << call << " id=" << id;

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

    if (Arn::debugRPC)  qDebug() << "Rpc metaCall-2: call=" << call << " id=" << id
                                 << " func=" << slot.funcName << " flags=" << slot.invokeFlags;

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


bool  ArnDynamicSignals::addSignal( QObject *sender, int signalId, const QByteArray& funcName)
{
    const QMetaObject*  metaObject = sender->metaObject();
    QMetaMethod  method = metaObject->method( signalId);

    Slot  slot;
    slot.funcName  = funcName;
    slot.typeNames = method.parameterTypes();
    slot.parNames  = method.parameterNames();
    ArnRpc::Invoke  ivf;
    ivf.set( ivf.NoQueue, QByteArray( method.tag()).contains("no_queue"));
    slot.invokeFlags = ivf;
    _slotTab += slot;

    bool  status = QMetaObject::connect( sender, signalId, this, _slotIdCount);
    ++_slotIdCount;
    return status;
}
//! \endcond



ArnRpcPrivate::ArnRpcPrivate()
{
    _pipe                 = arnNullptr;
    _receiver             = arnNullptr;
    _receiverStorage      = arnNullptr;
    _receiverMethodsParam = arnNullptr;
    _convVariantPar       = false;
    _isIncludeSender      = false;
    _dynamicSignals       = arnNullptr;
    _isHeartBeatOk        = true;
    _timerHeartBeatSend   = new QTimer;
    _timerHeartBeatCheck  = new QTimer;
}


ArnRpcPrivate::~ArnRpcPrivate()
{
    delete _timerHeartBeatSend;
    delete _timerHeartBeatCheck;
}


void ArnRpc::init()
{
    Q_D(ArnRpc);

    d->_dynamicSignals = new ArnDynamicSignals( this);

    connect( d->_timerHeartBeatSend, SIGNAL(timeout()), this, SLOT(timeoutHeartBeatSend()));
    connect( d->_timerHeartBeatCheck, SIGNAL(timeout()), this, SLOT(timeoutHeartBeatCheck()));
}


ArnRpc::ArnRpc( QObject* parent)
    : QObject( parent)
    , d_ptr( new ArnRpcPrivate)
{
    init();
}


ArnRpc::ArnRpc( ArnRpcPrivate& dd, QObject* parent)
    : QObject( parent)
    , d_ptr( &dd)
{
    init();
}


ArnRpc::~ArnRpc()
{
    delete d_ptr;
}


QString  ArnRpc::pipePath()  const
{
    Q_D(const ArnRpc);

    if (!d->_pipe)  return QString();

    return d->_pipe->path();
}


bool  ArnRpc::open( const QString& pipePath)
{
    Q_D(ArnRpc);

    setPipe( arnNullptr);  // Remove any existing pipe

    //// Make path allways in accordance with provider / requester mode
    QString  path = pipePath;
    if (d->_mode.is( Mode::Provider) != Arn::isProviderPath( path))
        path = Arn::twinPath( path);

    ArnPipe*  pipe = new ArnPipe;
    if (!d->_mode.is( Mode::Provider))
        pipe->setMaster();
    if (d->_mode.is( Mode::AutoDestroy))
        pipe->setAutoDestroy();

    bool  stat;
    if (d->_mode.is( Mode::UuidPipe))
        stat = pipe->openUuid( path);
    else
        stat = pipe->open( path);

    if (stat) {
        pipe->setSendSeq( d->_mode.is( Mode::SendSequence));
        pipe->setCheckSeq( d->_mode.is( Mode::CheckSequence));
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
    Q_D(ArnRpc);

    if (d->_pipe) {
        if (Arn::debugRPC)  qDebug() << "Rpc delete pipe: path=" << d->_pipe->path();
        d->_pipe->deleteLater();
    }
    d->_pipe = pipe;
    if (d->_pipe) {
        d->_pipe->setParent( this);
        connect( d->_pipe, SIGNAL(changed(QByteArray)), this, SLOT(pipeInput(QByteArray)),
                 Qt::QueuedConnection);
        connect( d->_pipe, SIGNAL(arnLinkDestroyed()), this, SLOT(destroyPipe()));
        connect( d->_pipe, SIGNAL(outOfSequence()), this, SIGNAL(outOfSequence()));
    }
}


ArnPipe*  ArnRpc::pipe()  const
{
    Q_D(const ArnRpc);

    return d->_pipe;
}


bool ArnRpc::setReceiver( QObject* receiver, bool useTrackRpcSender)
{
    Q_D(ArnRpc);

    bool stat = true;

    d->_receiver = receiver;
    deleteReceiverMethodsParam();
    d->_receiverStorage = arnNullptr;  // Default: don't need be able to track rpcSender
    if (!receiver)  return stat;

    bool  isSameThread = receiver->thread() == this->thread();
    if (useTrackRpcSender && isSameThread) {
        d->_receiverStorage = d->_receiver->findChild<ArnRpcReceiverStorage*>( RPC_STORAGE_NAME);
        if (!d->_receiverStorage) {
            d->_receiverStorage = new ArnRpcReceiverStorage( d->_receiver);
            d->_receiverStorage->setObjectName( RPC_STORAGE_NAME);
        }
    }
    else if (useTrackRpcSender) {
        errorLog( QString(tr("Can't track rpcSender to receiver in other thread")),
                  ArnError::Warning);
        stat = false;
    }

    return stat;
}


QObject*ArnRpc::receiver() const
{
    Q_D(const ArnRpc);

    return d->_receiver;
}


void  ArnRpc::setMethodPrefix( const QString& prefix)
{
    Q_D(ArnRpc);

    d->_methodPrefix = prefix.toLatin1();
    deleteReceiverMethodsParam();
}


QString  ArnRpc::methodPrefix()  const
{
    Q_D(const ArnRpc);

    return d->_methodPrefix;
}


bool  ArnRpc::isConvVariantPar()  const
{
    Q_D(const ArnRpc);

    return d->_convVariantPar;
}


void  ArnRpc::setConvVariantPar( bool convVariantPar)
{
    Q_D(ArnRpc);

    d->_convVariantPar = convVariantPar;
}


void  ArnRpc::setIncludeSender( bool v)
{
    Q_D(ArnRpc);

    d->_isIncludeSender = v;
}


void  ArnRpc::setMode( Mode mode)
{
    Q_D(ArnRpc);

    d->_mode = mode;
}


ArnRpc::Mode  ArnRpc::mode()  const
{
    Q_D(const ArnRpc);

    return d->_mode;
}


void  ArnRpc::setHeartBeatSend( int time)
{
    Q_D(ArnRpc);

    if (time == 0)
        d->_timerHeartBeatSend->stop();
    else
        d->_timerHeartBeatSend->start( time * 1000);
}


int  ArnRpc::getHeartBeatSend()  const
{
    Q_D(const ArnRpc);

    return d->_timerHeartBeatSend->interval() / 1000;
}


void  ArnRpc::setHeartBeatCheck( int time)
{
    Q_D(ArnRpc);

    if (time == 0)
        d->_timerHeartBeatCheck->stop();
    else
        d->_timerHeartBeatCheck->start( time * 1000);
}


int  ArnRpc::getHeartBeatCheck()  const
{
    Q_D(const ArnRpc);

    return d->_timerHeartBeatCheck->interval() / 1000;
}


bool  ArnRpc::isHeartBeatOk()  const
{
    Q_D(const ArnRpc);

    return d->_isHeartBeatOk;
}


void  ArnRpc::addSenderSignals( QObject* sender, const QString& prefix)
{
    Q_D(ArnRpc);

    const QMetaObject*  metaObject = sender->metaObject();
    int  methodCount = metaObject->methodCount();
    QByteArray  methodSignCompHead;
    int  methodIdCompHead = -1;
    for (int methodId = 0; methodId < methodCount; ++methodId) {
        QMetaMethod  method = metaObject->method( methodId);
        if (method.methodType() != QMetaMethod::Signal)  continue;

        QByteArray  methodSign = methodSignature( method);
        if (!methodSign.startsWith( prefix.toLatin1()))  continue;

        QByteArray  methodName = methodSign.left( methodSign.indexOf('('));
        QByteArray  funcName   = methodName.mid( prefix.size());

        QByteArray  methodSignComp = methodSign;
        methodSignComp.chop(1);  // Remove last ")"

        if (!d->_mode.is( Mode::NoDefaultArgs)  // When using Default args ...
        && methodSignCompHead.startsWith( methodSignComp)  // Starts with same signatur ...
        && hasSameParamNames( method, metaObject->method( methodIdCompHead)))  // and same param names
            continue;  // Skip it, to prohibit multiple rpc calls.

        methodSignCompHead = methodSignComp;
        methodIdCompHead   = methodId;
        d->_dynamicSignals->addSignal( sender, methodId, funcName);
    }
}


ArnRpc*  ArnRpc::rpcSender()
{
    Q_D(ArnRpc);

    if (!d->_receiverStorage)  return arnNullptr;

    return d->_receiverStorage->_rpcSender;
}


ArnRpc*  ArnRpc::rpcSender( QObject *receiver)
{
    if (!receiver)  return arnNullptr;

    ArnRpcReceiverStorage*  recStore = receiver->findChild<ArnRpcReceiverStorage*>( RPC_STORAGE_NAME);
    if (!recStore)  return arnNullptr;

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
    Q_D(ArnRpc);

    if (!d->_pipe || !d->_pipe->isOpen()) {
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
        *d->_pipe = xsmCall.toXString();
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
    Q_D(ArnRpc);

    if (!d->_pipe || !d->_pipe->isOpen()) {
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
            ARN_RegExp rx("^" + funcName + "\\b");
            d->_pipe->setValueOverwrite( xsmCall.toXString(), rx);
        }
        else
            *d->_pipe = xsmCall.toXString();
    }
    return stat;
}


bool  ArnRpc::xsmAddArg( XStringMap& xsm, const MQGenericArgument& arg, uint index, int& nArg)
{
    Q_D(ArnRpc);

    if (!arg.name() || !arg.label() || !arg.data())  return true;  // Empty arg
    if (nArg + 1 != int( index))  return true;  // Out of seq - Finished

    //// Get arg type info
    QByteArray  typeName = arg.name();

#if QT_VERSION >= 0x060000
    QMetaType mType = QMetaType::fromName( typeName);
    int  type = mType.id();
#else
    int  type = QMetaType::type( typeName.constData());
#endif

    if (!type) {
        errorLog( QString(tr("Unknown type:") + typeName.constData()),
                  ArnError::RpcInvokeError);
        return false;
    }
    const RpcTypeInfo&  rpcTypeInfo = typeInfoFromId( type);

    //// Get arg label
    QByteArray  argLabel = arg.label();

    //// Export data from arg
    QByteArray  argDataDump;
    QStringList  argDataList;
    bool  isBinaryType = false;
#if QT_VERSION >= 0x060000
    QVariant  varArg( mType, arg.data());
#else
    QVariant  varArg( type, arg.data());
#endif
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

    //// Make arg key with rpcType or typeGen e.g "t<QImage>"
    QByteArray  argKey;
    if (!rpcTypeInfo.typeId)
        argKey = (isBinaryType ? "tb<" : "t<") + typeName + ">";
    else
        argKey = rpcTypeInfo.rpcTypeName;
    if (!argLabel.isEmpty()) {
        if (d->_mode.is( Mode::NamedArg) && rpcTypeInfo.typeId)
            argKey = argLabel;
        else if (d->_mode.is( Mode::NamedArg) || d->_mode.is( Mode::NamedTypedArg))
            argKey = argLabel + ":" + argKey;
        else
            argKey += "." + argLabel;
    }

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
            if (argData.isEmpty() || argData.contains( QChar('=')))
                xsm.add("+", argData);
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


void  ArnRpc::pipeInput( const QByteArray& data)
{
    Q_D(ArnRpc);

    if (Arn::debugRPC)  qDebug() << "Rpc pipeInput: path=" << d->_pipe->path()
                                 << " itemId=" << d->_pipe->itemId() << " data=" << data;

    if (!d->_receiver) {
        errorLog( QString(tr("Can't invoke method: receiver=0")), ArnError::RpcReceiveError);
        return;
    }

    //// Handle received text message
    if (data.startsWith('"')) {  // Text is received
        int  endSize = data.endsWith('"') ? (data.size() - 1) : data.size();
        if (endSize < 1)
            endSize = 1;
        emit textReceived( QString::fromUtf8( data.mid( 1, endSize - 1), endSize - 1));
        return;
    }

    XStringMap  xsmCall( data);

    //// Handle built in commands
    QByteArray  rpcFunc = xsmCall.value(0);
    if (rpcFunc == "$heartbeat")  // Built in Heart beat support
        return funcHeartBeat( xsmCall);
    if (rpcFunc == "$arg")
        return funcArg( xsmCall);
    if (rpcFunc == "$help")  // Built in Help
        return funcHelp( xsmCall);

    //// Check for defaultCall
    // qDebug() << "rpc pipeInput: data=" << data;
    QByteArray  methodName = d->_methodPrefix + rpcFunc;
    if (d->_mode.is( Mode::UseDefaultCall)) {
        setupReceiverMethodsParam();  // Setup searching method data structure

        int  pslotIndex = d->_receiverMethodsParam->methodNames.indexOf( methodName);
        if (pslotIndex < 0) {  // Method not found
            emit defaultCall( data);
            return;
        }
    }

    //// Start processing normal rpc function call
    ArgInfo  argInfo[21];  // 0..9: Used args, 10..19: Default args, 20: Null arg
    int  argc = 0;

    if (d->_isIncludeSender) {
        argInfo[ argc].arg = Q_ARG( ArnRpc*, this);
        ++argc;
    }

    bool  stat = true;  // Default ok
    int  index = 1;  // Start after function name in xsm
    while (index > 0) {
        if (index >= xsmCall.size())
            break;  // End of args
        if (argc > 10) {
            errorLog( QString(tr("To many args:") + QString::number( argc))
                      + tr(" method=") + methodName.constData(),
                      ArnError::RpcReceiveError);
            stat = false;
            break;
        }
        stat = xsmLoadArg( xsmCall, argInfo[ argc], index, methodName);
        if (!stat)  break;
        ++argc;
    }

    char  argOrder[10];
    if (stat) {
        for (int i = 0; i < 10; ++i) {
            argOrder[i] = char(i);  // Set default order 1 to 1
        }
        stat = argLogic( argInfo, argOrder, argc, methodName);
        for (int i = argc; i < 10; ++i) {
            argOrder[i] = char(20);  // Set unused to null arg
        }
    }

    if (stat) {
        bool  useVarPar = checkConvVarPar( methodName, argc);
        for (int i = d->_isIncludeSender; i < argc; ++i) {
            stat = importArgData( argInfo[ int(argOrder[i])], methodName, useVarPar);
            if (!stat)  break;
        }
    }

    if (stat) {
        if (d->_receiverStorage)
            d->_receiverStorage->_rpcSender = this;
        stat = QMetaObject::invokeMethod( d->_receiver,
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
        if (d->_receiverStorage)
            d->_receiverStorage->_rpcSender = arnNullptr;
        if(!stat) {
            errorLog( QString(tr("Can't invoke method:")) + methodName.constData(),
                      ArnError::RpcReceiveError);
            sendText("Can't invoke method, use $help");
        }
    }

    //// Clean up - destroy allocated argument data
    for (int i = d->_isIncludeSender; i < 20; ++i) {
        ArgInfo&  aiSlot = argInfo[i];
        if (aiSlot.isArgAlloc && aiSlot.arg.data()) {
            int  type = QMetaType::type( aiSlot.arg.name());
            QMetaType::destroy( type, aiSlot.arg.data());
        }
        if (aiSlot.isDataAlloc) {
            void*  data = const_cast<void*>( aiSlot.data);
            QMetaType::destroy( aiSlot.typeId, data);
        }
    }
}


bool  ArnRpc::xsmLoadArg( const XStringMap& xsm, ArgInfo& argInfo, int &index,
                          const QByteArray& methodName)
{
    Q_D(ArnRpc);

    //// Get arg type and name
    const QByteArray&  typeKey = xsm.keyRef( index);  // MW: Why ref &typeKey not working in debugger?
    QByteArray  rpcType;
    argInfo.isPositional = true;  // Default
    int  sepPos = typeKey.indexOf(':');
    if (sepPos >=0)
        argInfo.isPositional = false;
    else
        sepPos = typeKey.indexOf('.');
    if (sepPos >=0) {
        QByteArray  t1  = typeKey.left( sepPos);
        QByteArray  t2  = typeKey.mid( sepPos + 1);
        rpcType         = argInfo.isPositional ? t1 : t2;
        argInfo.name    = argInfo.isPositional ? t2 : t1;
        argInfo.hasType = !rpcType.isEmpty();
        argInfo.hasName = !argInfo.name.isEmpty();
    }
    else {
        rpcType = typeKey;  // Assume type (can also be name)
    }

    //// Check for typeGen  e.g "t<QImage>"
    bool  isTypeGen = false;
    if (rpcType.startsWith("t<"))
        isTypeGen = true;
    else if (rpcType.startsWith("tb<")) {
        isTypeGen = true;
        argInfo.isBinary = true;
    }

    //// Setup Arg info
    const RpcTypeInfo&  rpcTypeInfo = (isTypeGen || (!argInfo.hasType && d->_mode.is( Mode::NamedArg)))
                                    ? typeInfoNull() : typeInfoFromRpc( rpcType);
    bool  isListFormat = rpcTypeInfo.typeId == QMetaType::QStringList;
    if (isTypeGen) {
        int  posStart = rpcType.indexOf('<') + 1;
        int  posEnd   = rpcType.lastIndexOf('>');
        argInfo.qtType  = rpcType.mid( posStart, (posEnd < 0 ? -1 : posEnd - posStart));
        argInfo.typeId  = QMetaType::type( argInfo.qtType.constData());
        argInfo.hasType = true;
    }
    else if (rpcTypeInfo.typeId) {
        argInfo.qtType = rpcTypeInfo.qtTypeName;
        argInfo.typeId = rpcTypeInfo.typeId;
    }
    else if (rpcType.isEmpty() && argInfo.name.isEmpty()) {
        argInfo.qtType = "QString";  // Default type;
        argInfo.typeId = QMetaType::QString;
    }
    else {
        if (!argInfo.hasType)
            rpcType.clear();  // No type given
        if (!argInfo.hasName) {
            argInfo.name         = typeKey;  // This must be a name
            argInfo.isPositional = false;
        }
    }

    bool  possibleNameArg = !d->_mode.is( Mode::OnlyPosArgIn) && !argInfo.name.isEmpty();

    //// Check type (not for pure nameArg)
    if ((!argInfo.typeId)
    &&  (argInfo.hasType || argInfo.isPositional || !possibleNameArg)) {
        errorLog( QString(tr("Unknown type:"))
                  + (argInfo.qtType.isEmpty() ? rpcType.constData() : argInfo.qtType.constData())
                  + (argInfo.hasName ? (tr(" name=") + argInfo.name.constData()) : QString())
                  + tr(" method=") + methodName.constData(),
                  ArnError::RpcReceiveError);
        return false;
    }

    //// Get arg data
    const QByteArray&  argDataDump = xsm.valueRef( index);
    if (isListFormat) {  // Handle list (QStringList)
        QStringList*  argDataList = new QStringList;
        if (!argDataDump.isEmpty())
            *argDataList += QString::fromUtf8( argDataDump.constData(), argDataDump.size());
        ++index;
        while (index < xsm.size()) {
            const QByteArray&  key = xsm.keyRef( index);
            if ((key != "") && (key != "+"))
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
    Q_D(ArnRpc);

    if (d->_mode.is( Mode::OnlyPosArgIn))  return true;  // Only allowed call by positional argument
    if (d->_isIncludeSender)  return true;  // IncludeSender is deprecated and never namedArg

    bool  isOnlyPositional = true;
    bool  isOnlyNamed      = true;
    for (int i = 0; i < argc; ++i) {
        if ( argInfo[i].isPositional)
            isOnlyNamed = false;
        else
            isOnlyPositional = false;
    }
    if (isOnlyPositional && (argc > 0))
        return true;  // Only positional arguments in call has been used

    if (!isOnlyNamed) {
        errorLog( QString(tr("Mixed positional & named arg call not supported, method="))
                  + methodName.constData(),
                  ArnError::RpcReceiveError);
        return false;
    }

    int  methodIndex = argLogicFindMethod( argInfo, argc, methodName);
    if (methodIndex < 0)  return false;  // Failed finding a method

    const QMetaObject*  metaObject = d->_receiver->metaObject();
    QMetaMethod  method = metaObject->method( methodIndex);
    QList<QByteArray>  parNames = method.parameterNames();
    QList<QByteArray>  parTypes = method.parameterTypes();
    int  parCount = parNames.size();
    int  defArgIndex = 10;
    for (int parIndex = 0; parIndex < parCount; ++parIndex) {
        bool  match = false;
        for (int argIndex = 0; argIndex < argc; ++argIndex) {
            ArgInfo&  aiSlot = argInfo[ argIndex];
            if (aiSlot.name == parNames.at( parIndex)) {  // Found arg name
                QByteArray  parType = parTypes.at( parIndex);
                if (!aiSlot.qtType.isEmpty()) {  // Type already known for arg
                    if ((parType != aiSlot.qtType)
                    &&  ((parType != "QVariant") || !d->_convVariantPar)) {
                        errorLog( QString(tr("Type mismatch, arg=")) + parNames.at( parIndex)
                                  + tr(" in method=") + methodName.constData(),
                                  ArnError::RpcReceiveError);
                        return false;
                    }
                }
                else {  // Type for arg is defined by the method parameter
                    aiSlot.qtType = parType;
                    aiSlot.typeId = QMetaType::type( aiSlot.qtType.constData());

                    const RpcTypeInfo&  typeInfo = typeInfoFromId( aiSlot.typeId);
                    if ((aiSlot.typeId == QMetaType::QVariant) && d->_convVariantPar) {
                        aiSlot.qtType = "QString";  // Will be QString in QVariant
                        aiSlot.typeId = QMetaType::QString;
                    }
                    else if ((!typeInfo.typeId)
                    || typeInfo.typeId == QMetaType::QStringList) {
                        errorLog( QString(tr("Type mandatory, arg=")) + parNames.at( parIndex)
                                  + tr(" type=") + aiSlot.qtType.constData()
                                  + tr(" in method=") + methodName.constData(),
                                  ArnError::RpcReceiveError);
                        return false;
                    }
                }
                argOrder[ parIndex] = char( argIndex);
                match = true;
                break;
            }
        }
        if (!match) {  // Parameter not given by arg, use default constructor
            ArgInfo&  aiSlot = argInfo[ defArgIndex];
            aiSlot.name   = parNames.at( parIndex);
            aiSlot.qtType = parTypes.at( parIndex);
            aiSlot.typeId = QMetaType::type( aiSlot.qtType.constData());
#if QT_VERSION >= 0x050000
            aiSlot.data   = QMetaType::create( aiSlot.typeId);
#else
            aiSlot.data   = QMetaType::construct( aiSlot.typeId);
#endif
            aiSlot.isDataAlloc = true;
            aiSlot.dataAsArg   = true;

            argOrder[ parIndex] = char( defArgIndex);
            ++defArgIndex;
        }
    }
    argc = parCount;  // New argc will be exactly as number of parameters

    return true;
}


int  ArnRpc::argLogicFindMethod( const ArnRpc::ArgInfo* argInfo, int argc, const QByteArray& methodName)
{
    Q_D(ArnRpc);

    setupReceiverMethodsParam();  // Setup searching method data structure

    int  pslotIndex = d->_receiverMethodsParam->methodNames.indexOf( methodName);
    if (pslotIndex < 0) {
        errorLog( QString(tr("Not found, method=")) + methodName.constData(),
                  ArnError::RpcReceiveError);
        return -1;
    }

    const MethodsParam::Params&  pslot = d->_receiverMethodsParam->paramTab.at( pslotIndex);

    // Only 1 method with zero parameters, use it
    if ((pslot.paramNames.size() == 1) && pslot.paramNames.at(0).isEmpty())
        return pslot.methodIdsTab.at(0).at(0);

    int  foundArgCount = 0;
    QList<int>  methodCand = pslot.allMethodIds;  // Start with all methods as candidates (same name)
    for (int argIndex = 0; argIndex < argc; ++argIndex) {
        int  parIndex = pslot.paramNames.indexOf( argInfo[ argIndex].name);
        if (parIndex < 0)  // arg not used at all, Ok (unneeded)
            continue;

        //// This arg is found as parameter in at least 1 method
        ++foundArgCount;

        //// Intersect method candidates with list of methods using the parameter (=arg)
        const QList<int>&  methodIds = pslot.methodIdsTab.at( parIndex);
        for (int i = 0; i < methodCand.size();) {
            if (methodIds.contains( methodCand.at(i))) {
                ++i;
                continue;
            }
            methodCand.removeAt(i);  // Remove method not using the parameter
        }
    }

    if (methodCand.isEmpty()) {
        errorLog( QString(tr("Not found method with matching parameters, method=")) + methodName.constData(),
                  ArnError::RpcReceiveError);
        return -1;
    }

    if (methodCand.size() == 1)
        return methodCand.at(0);  // Match with exactly 1 method

    //// Filter candidates to only have same number of params as the found args
    const QMetaObject*  metaObject = d->_receiver->metaObject();
    for (int i = 0; i < methodCand.size();) {
        QMetaMethod  method = metaObject->method( methodCand.at(i));
        if (method.parameterNames().size() == foundArgCount) {
            ++i;
            continue;
        }
        methodCand.removeAt(i);  // Remove method with not same number of params
    }

    if (methodCand.size() == 1)
        return methodCand.at(0);  // Match with exactly 1 method having same number of params

    errorLog( QString(tr("Many methods with matching parameters, method=")) + methodName.constData(),
              ArnError::RpcReceiveError);
    return -1;
}


void  ArnRpc::setupReceiverMethodsParam()
{
    Q_D(ArnRpc);

    if (d->_receiverMethodsParam)  return;  // Already done

    MethodsParam*  mpar = new MethodsParam;
    d->_receiverMethodsParam = mpar;

    const QMetaObject*  metaObject = d->_receiver->metaObject();
    int  methodCount = metaObject->methodCount();
    QByteArray  lastMethodName;
    for (int methodIndex = 0; methodIndex < methodCount; ++methodIndex) {
        QMetaMethod  method = metaObject->method( methodIndex);
        QByteArray  methodSign = methodSignature( method);
        if (!methodSign.startsWith( d->_methodPrefix))  continue;

        //// Found a method
        QByteArray  methodName = methodSign.left( methodSign.indexOf('('));
        if (methodName != lastMethodName) {
            mpar->methodNames += methodName;
            mpar->paramTab    += MethodsParam::Params();
            lastMethodName = methodName;
        }
        MethodsParam::Params&  pslot = mpar->paramTab[ mpar->paramTab.size() - 1];

        pslot.allMethodIds += methodIndex;

        QList<QByteArray>  parNames = method.parameterNames();
        int  parCount = parNames.size();
        int  parIndexStart = parCount == 0 ? -1 : 0;
        for (int parIndex = parIndexStart; parIndex < parCount; ++parIndex) {
            // method with no parameters will store using parName="" (parIndex=-1)
            const char*  parName = parIndex < 0 ? "" : parNames.at( parIndex).constData();
            int  parI = pslot.paramNames.indexOf( parName);
            if (parI < 0) {
                pslot.paramNames   += parName;
                pslot.methodIdsTab += QList<int>();
                parI = pslot.paramNames.size() - 1;
            }
            pslot.methodIdsTab[ parI] += methodIndex;
        }
    }
}


void ArnRpc::deleteReceiverMethodsParam()
{
    Q_D(ArnRpc);

    if (!d->_receiverMethodsParam)  return;  // Already done

    delete d->_receiverMethodsParam;
    d->_receiverMethodsParam = arnNullptr;
}


bool  ArnRpc::hasSameParamNames( const QMetaMethod& method1, const QMetaMethod& method2)
{
    QList<QByteArray>  parNames1 = method1.parameterNames();
    QList<QByteArray>  parNames2 = method2.parameterNames();
    int  parCount = qMin( parNames1.size(), parNames2.size());
    for (int i = 0; i < parCount; ++i) {
        if (parNames2.at(i) != parNames1.at(i))
            return false;
    }
    return true;
}


bool  ArnRpc::checkConvVarPar( const QByteArray& methodName, int argc)
{
    Q_D(ArnRpc);

    if (!d->_convVariantPar)  return false;

    bool  useVarPar = false;
    setupReceiverMethodsParam();  // Setup searching method data structure

    int  pslotIndex = d->_receiverMethodsParam->methodNames.indexOf( methodName);
    if (pslotIndex < 0)  return false;

    useVarPar = true;
    const MethodsParam::Params&  pslot = d->_receiverMethodsParam->paramTab.at( pslotIndex);
    const QMetaObject*  metaObject = d->_receiver->metaObject();
    foreach (int  methodIndex, pslot.allMethodIds) {
        QMetaMethod  method = metaObject->method( methodIndex);
        QList<QByteArray>  typesNames = method.parameterTypes();
        if (typesNames.size() != argc)  continue;

        foreach (const QByteArray& typeName, typesNames) {
            useVarPar &= typeName == "QVariant";
        }
    }

    return useVarPar;
}


bool  ArnRpc::importArgData( ArnRpc::ArgInfo& argInfo, const QByteArray& methodName, bool useVarPar)
{
    if (argInfo.typeId == QMetaType::QByteArray)
        argInfo.dataAsArg = true;

    if (argInfo.dataAsArg) {
        argInfo.arg = QGenericArgument( argInfo.qtType.constData(), argInfo.data);
        return true;
    }

    const QByteArray&  argDataDump = *static_cast<const QByteArray*>( argInfo.data);
    if (argInfo.isBinary) {
        if ((argDataDump.size() < 2) || (argDataDump.at(1) != DATASTREAM_VER)) {
            errorLog( QString(tr("Not same DataStream version, method=")) + methodName.constData(),
                      ArnError::RpcReceiveError);
            return false;
        }
#if QT_VERSION >= 0x050000
        void*  argData = QMetaType::create( argInfo.typeId);
#else
        void*  argData = QMetaType::construct( argInfo.typeId);
#endif
        Q_ASSERT( argData);
        argInfo.arg = QGenericArgument( argInfo.qtType.constData(), argData);  // Assign arg as it has been allocated
        argInfo.isArgAlloc = true;
        QDataStream  stream( argDataDump);
        stream.setVersion( DATASTREAM_VER);
        stream.skipRawData(2);
        if (!QMetaType::load( stream, argInfo.typeId, argData)) {
            errorLog( QString(tr("Can't' import bin type:") + argInfo.qtType.constData())
                      + tr(" method=") + methodName.constData(),
                      ArnError::RpcReceiveError);
            return false;
        }
    }
    else {  // Textual type
        QVariant  varArg( QString::fromUtf8( argDataDump.constData(), argDataDump.size()));
        if (!varArg.convert( QVariant::Type( argInfo.typeId))) {
            errorLog( QString(tr("Can't' import str type:") + argInfo.qtType.constData())
                      + tr(" method=") + methodName.constData(),
                      ArnError::RpcReceiveError);
            return false;
        }

        void*  argData = arnNullptr;
        if (useVarPar) {
            argData = new QVariant( varArg);
            argInfo.typeId = QMetaType::QVariant;
            argInfo.qtType = "QVariant";
        }
        else {
#if QT_VERSION >= 0x050000
            argData = QMetaType::create( argInfo.typeId, varArg.constData());
#else
            argData = QMetaType::construct( argInfo.typeId, varArg.constData());
#endif
        }
        Q_ASSERT( argData);
        argInfo.arg = QGenericArgument( argInfo.qtType.constData(), argData);  // Assign arg as it has been allocated
        argInfo.isArgAlloc = true;
    }

    return true;
}


void  ArnRpc::funcHeartBeat( const XStringMap& xsm)
{
    Q_D(ArnRpc);

    QByteArray  time = xsm.value(1);
    if (time == "off") {  // Remote turn off heart beat function for both directions
        d->_timerHeartBeatSend->stop();
        invoke("$heartbeat", MQ_ARG( QString, time, "off1"));
    }
    if (time == "off1") {  // Remote turn off heart beat function for this direction
        d->_timerHeartBeatSend->stop();
        invoke("$heartbeat", MQ_ARG( QString, time, "0"));
    }
    else if (time == "0") {  // Remote turn off heart beat checking for this direction
        d->_timerHeartBeatCheck->stop();
        d->_isHeartBeatOk = true;
    }
    else {
        emit heartBeatReceived();
        if (!d->_isHeartBeatOk) {
            d->_isHeartBeatOk = true;
            emit heartBeatChanged( d->_isHeartBeatOk);
        }
    }

    if (d->_timerHeartBeatCheck->isActive())
        d->_timerHeartBeatCheck->start();  // Restart heart beat check timer
}


void  ArnRpc::funcHelp( const XStringMap& xsm)
{
    Q_D(ArnRpc);

    if (!d->_receiver) {
        sendText("$help: rpc-receiver = 0");
        return;
    }

    QByteArray  modePar = xsm.value(1);
    int  flags = -1;  // Default is faulty parameter

    if (modePar.isEmpty()) {  // Ok, standard
        bool  useNamedArgHelp = d->_mode.isAny( Mode::AnyNamedArg) & !d->_mode.is( Mode::OnlyPosArgIn);
        flags = d->_mode.flagIf( useNamedArgHelp, Mode::NamedArg);
    }
    else if (modePar.startsWith("p")) {  // Positional
        flags = 0;
    }
    else if (modePar.startsWith("n")) {  // Named
        flags = Mode::NamedArg;
    }
    else {
        sendText("$help: Unknown mode");
    }

    if (flags >= 0) {
        if (d->_mode.is( Mode::OnlyPosArgIn))
            sendText("* Only positional args allowed.");
        if (d->_convVariantPar)
            sendText("* Type VAR can be any, e.g. string or int.");

        const QMetaObject*  metaObject = d->_receiver->metaObject();
        int  methodIdHead = -1;
        int  parCountMin = 10;
        QByteArray  methodNameHead;
        QByteArray  methodSignHead;
        int  methodCount = metaObject->methodCount();
        for (int methodId = 0; methodId < methodCount; ++methodId) {
            QMetaMethod  method = metaObject->method(methodId);
            QByteArray  methodSign = methodSignature( method);
            if (!methodSign.startsWith( d->_methodPrefix))  continue;

            methodSign.chop(1);  // Remove last ")"
            QList<QByteArray>  parNames = method.parameterNames();
            int  parCount = parNames.size();

            if (!d->_mode.is( Mode::NoDefaultArgs)  // When using Default args ...
            && methodSignHead.startsWith( methodSign)  // Starts with same signatur ...
            && hasSameParamNames( method, metaObject->method( methodIdHead)))  // and same param names
                parCountMin = parCount;  // Same method with less param
            else {
                if (methodIdHead >= 0)
                    funcHelpMethod( metaObject->method( methodIdHead),
                                    methodNameHead, parCountMin, flags);
                methodIdHead   = methodId;
                methodSignHead = methodSign;
                methodNameHead = methodSign.left( methodSign.indexOf('('));
                parCountMin    = parCount;
            }
        }
        if (methodIdHead >= 0)
            funcHelpMethod( metaObject->method( methodIdHead), methodNameHead, parCountMin, flags);
    }

    sendText("$arg pos|named|typed");
    sendText("$heartbeat [`time`|off|off1]");
    sendText("$help [pos|named]");
}


void  ArnRpc::funcHelpMethod( const QMetaMethod &method, const QByteArray& name, int parNumMin, int flags)
{
    Q_D(ArnRpc);

    QString  line = QString::fromLatin1( name.mid( d->_methodPrefix.size()));

    QList<QByteArray>  typesNames = method.parameterTypes();
    QList<QByteArray>  parNames   = method.parameterNames();

    bool  wasListType = false;
    int  parCount = parNames.size();
    for (int i = d->_isIncludeSender; i < parCount; ++i) {
        QByteArray  param;
        QByteArray  parName = parNames.at(i);
        QByteArray  typeName = typesNames.at(i);

        int  type = QMetaType::type( typeName.constData());
        QVariant  varPar( type);
        bool  isBinaryType = (type == 0) || !varPar.canConvert( QVariant::String);
        QByteArray  rpcType;
        const RpcTypeInfo&  rpcTypeInfo = typeInfoFromQt( typeName);
        bool  isList = rpcTypeInfo.typeId == QMetaType::QStringList;
        bool  isTypeGen = false;
        bool  isTypeVar = false;
        if (rpcTypeInfo.typeId == QMetaType::QString) {
            rpcType = wasListType ? rpcTypeInfo.rpcTypeName : "";
        }
        else if (rpcTypeInfo.typeId) {
            rpcType = rpcTypeInfo.rpcTypeName;
        }
        else if ((type == QMetaType::QVariant) && d->_convVariantPar) {
            rpcType = "VAR";
            isTypeVar = true;
        }
        else {
            rpcType = (isBinaryType ? "tb<" : "t<") + typeName + ">";
            isTypeGen = true;
        }

        if ((flags & Mode::NamedArg) && !parName.isEmpty()) {
            param += parName;
            if (isTypeGen | isTypeVar)
                param += ":" + rpcType + "=`data`";
            else {
                QByteArray  rpcTypeVal = rpcType.isEmpty() ? QByteArray("string") : rpcType;
                param += "=`" + rpcTypeVal + "`";
            }
        }
        else {
            if (!rpcType.isEmpty()) {
                param += rpcType;
                if (d->_mode.is( Mode::NamedArg) && !isTypeGen)
                    param += ".";  // Force typename
                param += "=";
            }
            param += "`" + parName + "`";
        }

        if (i >= parNumMin)
            param = "[" + param + "]";
        line += " " + QString::fromLatin1( param);

        wasListType = isList;
    }
    sendText( line);
}


void  ArnRpc::funcArg( const Arn::XStringMap& xsm)
{
    Q_D(ArnRpc);

    QByteArray  modePar = xsm.value(1);

    if (modePar.startsWith("p")) {  // Positional
        d->_mode.set( Mode::NamedArg,      false);
        d->_mode.set( Mode::NamedTypedArg, false);
    }
    else if (modePar.startsWith("n")) {  // Named
        d->_mode.set( Mode::NamedArg,      true);
        d->_mode.set( Mode::NamedTypedArg, false);
    }
    else if (modePar.startsWith("t")) {  // NamedTyped
        d->_mode.set( Mode::NamedArg,      false);
        d->_mode.set( Mode::NamedTypedArg, true);
    }
    else {
        sendText("$arg: Unknown mode, use $help");
    }
}


void  ArnRpc::destroyPipe()
{
    Q_D(ArnRpc);

    if (Arn::debugRPC)  qDebug() << "Rpc Destroy pipe: path=" << d->_pipe->path();
    emit pipeClosed();
    setPipe( arnNullptr);
}


void  ArnRpc::timeoutHeartBeatSend()
{
    Q_D(ArnRpc);

    if (!d->_pipe || !d->_pipe->isOpen())  return;

    invoke("$heartbeat", Invoke::NoQueue,
           MQ_ARG( QString, time, QByteArray::number( d->_timerHeartBeatSend->interval() / 1000)));
}


void  ArnRpc::timeoutHeartBeatCheck()
{
    Q_D(ArnRpc);

    if (d->_isHeartBeatOk) {
        d->_isHeartBeatOk = false;
        emit heartBeatChanged( d->_isHeartBeatOk);
    }
}


void  ArnRpc::sendText( const QString& txt)
{
    Q_D(ArnRpc);

    if (d->_pipe)
        *d->_pipe = "\"" + txt.toUtf8() + "\"";
}


void  ArnRpc::errorLog( const QString& errText, ArnError err, void* reference)
{
    Q_D(ArnRpc);

    QString  idText;
    if (d->_pipe) {
        idText += " pipe:" + d->_pipe->path();
    }
    ArnM::errorLog( errText + idText, err, reference);
}


void  ArnRpc::batchConnect( const QObject *sender, const ARN_RegExp& rgx,
                                 const QObject* receiver, const QString& replace,
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
        QMetaMethod  method = metaObject->method(i);
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
    int  methodIdCompHead = -1;
    for (int methodId = 0; methodId < methodCount; ++methodId) {
        QMetaMethod  method = metaObject->method(methodId);
        QByteArray  methodSign = methodSignature( method);
        if (doneMethodSignTab.contains( methodSign))  continue;  // Already done (inherited)
        doneMethodSignTab += methodSign;

        QByteArray  methodSignComp = methodSign;
        methodSignComp.chop(1);  // Remove last ")"

        if (!mode.is( Mode::NoDefaultArgs)  // When using Default args ...
        && methodSignCompHead.startsWith( methodSignComp)  // Starts with same signatur ...
        && hasSameParamNames( method, metaObject->method( methodIdCompHead)))  // and same param names
            continue;  // Skip it, to prohibit multiple calls.

        methodSignCompHead = methodSignComp;
        methodIdCompHead   = methodId;

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


const ArnRpc::RpcTypeInfo&  ArnRpc::typeInfoFromRpc( const QByteArray& rpcTypeName)
{
    RpcTypeInfo*  typeInfo = _rpcTypeInfoTab;
    while (typeInfo->typeId) {
        if (rpcTypeName == typeInfo->rpcTypeName)
            break;
        ++typeInfo;
    }

    return *typeInfo;
}


const ArnRpc::RpcTypeInfo&  ArnRpc::typeInfoFromQt( const QByteArray& qtTypeName)
{
    RpcTypeInfo*  typeInfo = _rpcTypeInfoTab;
    while (typeInfo->typeId) {
        if (qtTypeName == typeInfo->qtTypeName)
            break;
        ++typeInfo;
    }

    return *typeInfo;
}


const ArnRpc::RpcTypeInfo&  ArnRpc::typeInfoFromId( int typeId)
{
    RpcTypeInfo*  typeInfo = _rpcTypeInfoTab;
    while (typeInfo->typeId) {
        if (typeId == typeInfo->typeId)
            break;
        ++typeInfo;
    }

    return *typeInfo;
}


const ArnRpc::RpcTypeInfo&  ArnRpc::typeInfoNull()
{
    return _rpcTypeInfoTab[ sizeof(_rpcTypeInfoTab) / sizeof(RpcTypeInfo) - 1];  // Last slot is Null
}
