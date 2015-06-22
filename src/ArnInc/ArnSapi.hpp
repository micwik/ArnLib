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

#ifndef ARNSAPI_HPP
#define ARNSAPI_HPP

#include "ArnLib_global.hpp"
#include "ArnRpc.hpp"
#include <QString>
#include <QByteArray>
#include <QObject>

#if defined(__DATE__) && defined(Q_SIGNALS)  // Handle QT Creator code completion
# define MQ_PUBLIC_ACCESS public:
#else
# define MQ_PUBLIC_ACCESS
#endif


//! Service API.
/*!
[About RPC and SAPI](\ref gen_rpc)

This class serves as a base class for _Service Application Programming Interface_.
It should be derived to a custom class that descibe a specific _SAPI_.

By default all _provider_ services are prefixed by "pv_" and all _requester_
"services" are prefixed by "rq_". This standard can be changed.

The meta prefix _no_queue_ is used to limit the filling of sendqueue with recuring RPC
calls during some kind of client disconnection. Matched function name in sendqueue is
overwritten by the last call. This functionality uses
[pipe anti congest](\ref gen_pipeAntiCongest). This is internally used for _heart beat_,
but other typical usages can be _ping_, _request update_ etc.

<b>Example usage</b> \n \code
class ChatSapi : public ArnSapi
{
    Q_OBJECT
public:
    explicit ChatSapi( QObject* parent = 0) : ArnSapi( parent)  {}

signals:
MQ_PUBLIC_ACCESS
    //// Provider API
    no_queue void  pv_list();
    void  pv_newMsg( QString name, QString msg);
    void  pv_infoQ();

    //// Requester API
    void  rq_updateMsg( int seq, QString name, QString msg);
    void  rq_info( QString name, QString ver);
};

    // In class declare  (MyClass)
    ChatSapi*  _commonSapi;

    // In class code  (MyClass)
    typedef ArnSapi::Mode  SMode;
    _commonSapi = new ChatSapi( this);
    _commonSapi->open("//Chat/Pipes/pipeCommon", SMode::Provider | SMode::UseDefaultCall);
    _commonSapi->batchConnectTo( this, "sapi");
.
.
void  ServerMain::sapiNewMsg( QString name, QString msg)
{
    int  seq = ...;
    _commonSapi->rq_updateMsg( seq, name, msg);
}

void  MyClass::sapiInfoQ()
{
    ChatSapi*  sapi = qobject_cast<ChatSapi*>( sender());
    sapi->rq_info("Arn Chat Demo", "1.0");
}

void  MainWindow::sapiDefault( const QByteArray& data)
{
    ChatSapi*  sapi = qobject_cast<ChatSapi*>( sender());
    qDebug() << "chatDefault:" << data;
    sapi->sendText("Chat Sapi: Can't find method, use $help.");
}
\endcode
*/
class ARNLIBSHARED_EXPORT ArnSapi : public ArnRpc
{
    Q_OBJECT
public:
    explicit  ArnSapi( QObject* parent = 0);

    //! Open a new Service API
    /*! The opened Sapi can be either the _provider_ side or the _requester_ side,
     *  which is indicated by _mode_.
     *  The provider marker "!" in the _pipePath_ will automatically be set/removed in
     *  accordance to the _mode_.
     *
     *  Typically the _provider_ is only using _mode_ _Provider_.
     *  The _requester_ can use default _mode_ for a static _pipe_ and typically use
     *  the _UuidAutoDestroy_ _mode_ for dynamic session _pipes_.
     *  \param[in] pipePath is the path used for Sapi
     *  \param[in] mode
     *  \param[in] providerPrefix to set a custom prefix for _provider_ signals.
     *  \param[in] requesterPrefix to set a custom prefix for _requester_ signals.
     *  \retval false if error
     *  \see \ref gen_pipeArnobj
     */
    bool  open( const QString& pipePath, Mode mode = Mode(),
                const char* providerPrefix = 0, const char* requesterPrefix = 0);

    //! Make batch connection from this ArnSapi:s signals to another receivers slots/signals
    /*! Used when there is a specific pattern in the naming of the signals and slots.
     *  It's assumed that naming for slots are unique regardless of its case i.e.
     *  using both test() and tesT() are not allowed.
     *
     *  When Mode::UseDefaultCall is active, then also the defaultCall() signal is connected
     *  to the receiver. Method name will be using the prefix and end with "Default". E.g.
     *  prefix is "sapi" will give method name "sapiDefault".
     *
     *  Example: Provider doing `_commonSapi.batchConnectTo( myReceiver, "sapi");`
     *  Can connect signal: `pv_newMsg(QString,QString)` to slot: `sapiNewMsg(QString,QString)`
     *
     *  \param[in] receiver is the receiving QObject.
     *  \param[in] prefix is the prefix for receiving slot/signal names.
     *  \param[in] mode
     *  \see ArnRpc::batchConnect( const QObject*, const QRegExp&, const QObject*,
     *       const QString&, Mode)
     */
    void  batchConnectTo( const QObject* receiver, const QString& prefix = QString(),
                          Mode mode = Mode());

    //! Make batch connection from one senders signals to this ArnSapi:s signals
    /*! Used when there is a specific pattern in the naming of the signals.
     *  It's assumed that naming for signals are unique regardless of its case i.e.
     *  using both test() and tesT() are not allowed.
     *
     *  Example: Requester doing `_commonSapi.batchConnectFrom( mySender, "sapi");`
     *  Can connect signal: `sapiNewMsg(QString,QString)` to signal: `pv_newMsg(QString,QString)`
     *
     *  \param[in] sender is the sending QObject.
     *  \param[in] prefix is the prefix for sending signal names.
     *  \param[in] mode
     *  \see ArnRpc::batchConnect(const QObject*, const QRegExp&, const QObject*,
     *       const QString&, Mode)
     */
    void  batchConnectFrom( const QObject* sender, const QString& prefix = QString(),
                            Mode mode = Mode());

private:
    //// Hide these from SAPI base interface
    void  setPipe( ArnPipe* pipe);
    void  setReceiver( QObject* receiver);
    bool  setReceiver( QObject* receiver, bool useTrackRpcSender);
    void  setMethodPrefix( const QString& prefix);
    void  setIncludeSender( bool v);
    void  setMode( Mode mode);
    void  addSenderSignals( QObject* sender, const QString& prefix);
    ArnRpc*  rpcSender();
    static ArnRpc*  rpcSender( QObject* receiver);

    QString  _receivePrefix;
    QString  _sendPrefix;
};


#endif // ARNSAPI_HPP
