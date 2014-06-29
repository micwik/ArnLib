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

#ifndef ARNRPC_HPP
#define ARNRPC_HPP

#define no_queue  // MetaObject tag to give Invoke::NoQueue

#include "ArnLib_global.hpp"
#include "Arn.hpp"
#include "ArnPipe.hpp"
#include "XStringMap.hpp"
#include "MQFlags.hpp"
#include <QGenericArgument>
#include <QString>
#include <QByteArray>
#include <QObject>

//! Similar to Q_ARG but with added argument label (parameter name)
#define MQ_ARG(type, label, data) MQArgument<type >(#type, #label, data)

class ArnRpcReceiverStorage;
class ArnDynamicSignals;
class QMetaMethod;
class QRegExp;
class QTimer;

//! Similar to QGenericArgument but with added argument label (parameter name)
class ARNLIBSHARED_EXPORT MQGenericArgument : public QGenericArgument
{
public:
    inline  MQGenericArgument( const char *aName = 0, const char *aLabel = 0, const void *aData = 0)
        : QGenericArgument( aName, aData), _label(aLabel) {}
    inline  MQGenericArgument( const QGenericArgument& qgenArg)
        : QGenericArgument( qgenArg), _label("") {}
    inline const char*  label()  const {return _label;}

private:
    const char *_label;
};


//! Similar to QArgument but with added argument label (parameter name)
template <class T>
class MQArgument: public MQGenericArgument
{
public:
    inline MQArgument( const char *aName, const char *aLabel, const T &aData)
        : MQGenericArgument( aName, aLabel, static_cast<const void *>(&aData))
        {}
};


//! Remote Procedure Call.
/*!
[About RPC and SAPI](\ref gen_rpc)

This is the basic funtionality of RPC. It's recommended to use ArnSapi which uses
a higher level model. For now the ArnRpc class is more sparsely documented.

<b>Example usage</b> \n \code
    // In class declare  (MyClass)
    ArnRpc*  _rpcCommon;

    // In class code  (MyClass)
    _rpcCommon = new ArnRpc( this);
    _rpcCommon->setIncludeSender( true);
    _rpcCommon->setMethodPrefix("rpc_");
    _rpcCommon->setReceiver( this);
    _rpcCommon->setMode( ArnRpc::Mode::Provider);
    _rpcCommon->open("//Pipes/pipeCommon");
.
.
void  MyClass::rpc_test( ArnRpc* sender, QByteArray ba, QString str, int i)
{
    if (sender) qDebug() << "RPC sender=" << sender->pipePath();
    qDebug() << "RPC-test ba=" << ba << " str=" << str << " int=" << i;
}

void  MyClass::rpc_ver( ArnRpc* sender)
{
    // Reply to requester the version text
    sender->invoke("ver", MQ_ARG( QString, verText, "MySytem Version 1.0"));
}
\endcode
*/
class ARNLIBSHARED_EXPORT ArnRpc : public QObject
{
    Q_OBJECT
public:
    struct Mode {
        enum E {
            //! Provider side (opposed to requester)
            Provider      = 0x01,
            //! Use _AutoDestroy_ for the pipe, i.e. it is closed when tcp/ip is broken
            AutoDestroy   = 0x02,
            //! Use an unique uuid in the pipe name
            UuidPipe      = 0x04,
            //! If guarantied no default arguments, member name overload is ok
            NoDefaultArgs = 0x08,
            //! Send sequence order information to pipe
            SendSequence  = 0x10,
            //! Check sequence order information from pipe. Can generate signal outOfSequence().
            CheckSequence = 0x20,
            //! Debug mode, dumping done batch connections
            Debug         = 0x40,
            //! Convenience, combined _UuidPipe_ and _AutoDestroy_
            UuidAutoDestroy = UuidPipe | AutoDestroy
        };
        MQ_DECLARE_FLAGS( Mode)
    };

    struct Invoke {
        enum E {
            //! This invoke is not queued, multiple calls to same method might overwrite
            NoQueue = 0x01
        };
        MQ_DECLARE_FLAGS( Invoke)
    };

    explicit  ArnRpc( QObject* parent = 0);

    //! Get the path for the used _pipe_
    /*! \retval false if error
     *  \see \ref gen_bidirArnobj
     */
    QString  pipePath() const;

    bool  open( QString pipePath);
    void  setPipe( ArnPipe* pipe);
    void  setReceiver( QObject* receiver);
    void  setMethodPrefix( QString prefix);
    void  setIncludeSender( bool v);
    void  setMode( Mode mode);

    //! Get the mode
    /*! \return current _mode_
     */
    Mode  mode()  const;

    //! Set period time for sending heart beat message
    /*! Setting time to zero will turn off sending.
     *  \param[in] time is the time period in seconds
     *  \see setHeartBeatCheck();
     */
    void  setHeartBeatSend( int time);

    //! Set max time period for receiving heart beat message
    /*! Setting time to zero will turn off checking.
     *  \param[in] time is the time period in seconds
     *  \see setHeartBeatSend();
     */
    void  setHeartBeatCheck( int time);

    //! Get the state of heart beat
    /*! \retval false if not getting heart beat in time
     *  \see heartBeatChanged()
     */
    bool  isHeartBeatOk()  const;

    void  addSenderSignals( QObject* sender, QString prefix);

    //! Calls a named remote procedure
    /*! This is the low level way to call a remote procedure. It can freely call
     *  anything without declaring it. For high level calls use ArnSapi.
     *
     *  This function works similar to QMetaObject::invokeMethod().
     *  The called name is prefixed before the final call is made.
     *  Using the label in MQ_ARG() makes dubugging easier, as the parameter is named.
     *
     *  Example: `rpc->invoke("myfunc", MQ_ARG( QString, mypar, "Test XYZ"));`
     *
     *  \param[in] funcName is the name of the called procedure.
     *  \param[in] val0 first arg.
     *  \param[in] val1
     *  \param[in] val2
     *  \param[in] val3
     *  \param[in] val4
     *  \param[in] val5
     *  \param[in] val6
     *  \param[in] val7
     */
    bool invoke( const QString& funcName,
                 MQGenericArgument val0 = MQGenericArgument(0),
                 MQGenericArgument val1 = MQGenericArgument(),
                 MQGenericArgument val2 = MQGenericArgument(),
                 MQGenericArgument val3 = MQGenericArgument(),
                 MQGenericArgument val4 = MQGenericArgument(),
                 MQGenericArgument val5 = MQGenericArgument(),
                 MQGenericArgument val6 = MQGenericArgument(),
                 MQGenericArgument val7 = MQGenericArgument());

    //! Calls a named remote procedure using invoke flags
    /*! This is the low level way to call a remote procedure. It can freely call
     *  anything without declaring it. For high level calls use ArnSapi.
     *
     *  This function works similar to QMetaObject::invokeMethod().
     *  The called name is prefixed before the final call is made.
     *  Using the label in MQ_ARG() makes dubugging easier, as the parameter is named.
     *
     *  Example: `rpc->invoke("myfunc", ArnRpc::Invoke::NoQueue, MQ_ARG( QString, mypar, "Test XYZ"));`
     *
     *  \param[in] funcName is the name of the called procedure.
     *  \param[in] invokeFlags is flags for controlling the invoke
     *  \param[in] val0 first arg.
     *  \param[in] val1
     *  \param[in] val2
     *  \param[in] val3
     *  \param[in] val4
     *  \param[in] val5
     *  \param[in] val6
     *  \param[in] val7
     */
    bool invoke( const QString& funcName,
                 Invoke  invokeFlags,
                 MQGenericArgument val0 = MQGenericArgument(0),
                 MQGenericArgument val1 = MQGenericArgument(),
                 MQGenericArgument val2 = MQGenericArgument(),
                 MQGenericArgument val3 = MQGenericArgument(),
                 MQGenericArgument val4 = MQGenericArgument(),
                 MQGenericArgument val5 = MQGenericArgument(),
                 MQGenericArgument val6 = MQGenericArgument(),
                 MQGenericArgument val7 = MQGenericArgument());

    ArnRpc*  rpcSender();
    static ArnRpc*  rpcSender( QObject* receiver);

    //! Make batch connection from one senders signals to another receivers slots
    /*! Used when there is a pattern in the naming of the signals and slots.
     *  It's assumed that naming for slots are unique regardless of its case i.e.
     *  using both test() and tesT() are not allowed.
     *
     *  Example: `batchConnect( _commonSapi, QRegExp("^rq_(.+)"), this, "chat\\1");`
     *  connects signal: `rq_info(QString,QString)` to slot: `chatInfo(QString,QString)`
     *
     *  \param[in] sender is the sending QObject.
     *  \param[in] rgx is the regular expression for selecting signals.
     *  \param[in] receiver is the receiving QObject.
     *  \param[in] replace is the conversion for naming the slots.
     *  \param[in] mode Used modes: _Debug_, _NoDefaultArgs_
     */
    static void  batchConnect( const QObject* sender, const QRegExp& rgx,
                               const QObject* receiver, const QString &replace,
                               Mode mode = Mode());

    //! Make batch connection from this ArnRpc:s signals to another receivers slots
    /*! Used when there is a pattern in the naming of the signals and slots.
     *  It's assumed that naming for slots are unique regardless of its case i.e.
     *  using both test() and tesT() are not allowed.
     *
     *  Example: `_commonSapi.batchConnect( QRegExp("^rq_(.+)"), this, "chat\\1");`
     *  connects signal: `rq_info(QString,QString)` to slot: `chatInfo(QString,QString)`
     *
     *  \param[in] rgx is the regular expression for selecting signals.
     *  \param[in] receiver is the receiving QObject.
     *  \param[in] replace is the conversion for naming the slots.
     *  \param[in] mode
     *  \see batchConnect(const QObject*, const QRegExp&, const QObject*,
     *       const QString&, Mode)
     */
    void  batchConnect( const QRegExp& rgx,
                        const QObject* receiver, const QString &replace,
                        Mode mode = Mode()) {
        batchConnect( this, rgx, receiver, replace, mode);
    }

    //! Make batch connection from one senders signals to this ArnRpc:s slots
    /*! Used when there is a pattern in the naming of the signals and slots.
     *  It's assumed that naming for slots are unique regardless of its case i.e.
     *  using both test() and tesT() are not allowed.
     *
     *  Example: `_commonSapi.batchConnect( _commonSapi, QRegExp("^rq_(.+)"), "chat\\1");`
     *  connects signal: `rq_info(QString,QString)` to slot: `chatInfo(QString,QString)`
     *
     *  \param[in] sender is the sending QObject.
     *  \param[in] rgx is the regular expression for selecting signals.
     *  \param[in] replace is the conversion for naming the slots.
     *  \param[in] mode
     *  \see batchConnect(const QObject*, const QRegExp&, const QObject*,
     *       const QString&, Mode)
     */
    void  batchConnect( const QObject* sender, const QRegExp& rgx,
                        const QString &replace,
                        Mode mode = Mode()) {
        batchConnect( sender, rgx, this, replace, mode);
    }

signals:
    //! Signal emitted when the used pipe is closed.
    /*! The _pipe_ closes when its _Arn Data Object_ is destroyed, i.e. the session
     *  is considered ended.
     */
    void  pipeClosed();

    //! Signal emitted when a general text message is received.
    /*! The text message is received from the other end of the used _pipe_.
     *  \param[in] text is the received text
     *  \see sendText();
     */
    void  textReceived( QString text);

    //! Signal emitted when checked sequence order is wrong.
    void  outOfSequence();

    //! Signal emitted when Heart beat changes state.
    /*! Heart beat messages are detected and expected within a check time.
     *  If this is satisfied, the state of heart beat is ok.
     *  \param[in] isOk is the Heart beat state, false = Not received.
     */
    void  heartBeatChanged( bool isOk);

    //! Signal emitted when Heart beat message is received.
    void  heartBeatReceived();

public slots:
    //! Send a general text message to the other end of the used _pipe_
    /*! Is used by ArnRpc to give errors and help messages, mostly for debugging.
     *  \param[in] txt is the text to be sent
     *  \see textReceived();
     */
    void  sendText( QString txt);

private slots:
    void  pipeInput( QByteArray data);
    void  destroyPipe();
    void  timeoutHeartBeatSend();
    void  timeoutHeartBeatCheck();

    //! \cond ADV
protected:
    void  errorLog( QString errText, ArnError err = ArnError::Undef, void* reference = 0);
    //! \endcond

private:
    struct RpcTypeInfo {
        const char*  rpcTypeName;
        const char*  qtTypeName;
        int  typeId;
    };
    struct ArgInfo {
        QGenericArgument  arg;
        QByteArray  name;
        QByteArray  rpcType;
        const char*  qtType;
        int  typeId;
        const void*  data;
        bool  hasName;
        bool  hasType;
        bool  dataAsArg;
        bool  isBinary;
        bool  isDataAlloc;
        bool  isArgAlloc;
        ArgInfo() {
            qtType      = "";
            typeId      = QMetaType::Void;
            data        = 0;
            hasName     = false;
            hasType     = false;
            dataAsArg   = false;
            isBinary    = false;
            isDataAlloc = false;
            isArgAlloc  = false;
        }
    };

    bool  xsmAddArg( Arn::XStringMap& xsm, const MQGenericArgument& arg, uint index, int& nArg);
    bool  xsmLoadArg( const Arn::XStringMap& xsm, ArgInfo& argInfo, int &index, const QByteArray& methodName);
    bool  argLogic( ArgInfo* argInfo, char* argOrder, int& argc, const QByteArray& methodName);
    bool  importArgData( ArgInfo& argInfo, const QByteArray& methodName);
    void  funcHeartBeat( const Arn::XStringMap& xsm);
    void  funcHelp( const Arn::XStringMap& xsm);
    void  funcHelpMethod( const QMetaMethod& method, QByteArray name, int parNumMin);
    static QByteArray  methodSignature( const QMetaMethod& method);
    static const RpcTypeInfo& typeInfofromRpc( const QByteArray& rpcTypeName);
    static const RpcTypeInfo& typeInfofromQt( const QByteArray& qtTypeName);
    static const RpcTypeInfo& typeInfofromId( int typeId);

    ArnDynamicSignals*  _dynamicSignals;
    ArnRpcReceiverStorage*  _receiverStorage;
    ArnPipe*  _pipe;
    QObject*  _receiver;
    QByteArray  _methodPrefix;
    bool  _isIncludeSender;
    Mode  _mode;
    bool  _isHeartBeatOk;
    QTimer*  _timerHeartBeatSend;
    QTimer*  _timerHeartBeatCheck;

    static RpcTypeInfo _rpcTypeInfoTab[];
};

MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnRpc::Mode)
MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnRpc::Invoke)
Q_DECLARE_METATYPE(ArnRpc*)

#endif // ARNRPC_HPP
