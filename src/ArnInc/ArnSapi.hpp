// Copyright (C) 2010-2014 Michael Wiklund.
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

<b>Example usage</b> \n \code
class ChatSapi : public ArnSapi
{
    Q_OBJECT
public:
    explicit ChatSapi( QObject* parent = 0) : ArnSapi( parent)  {}

signals:
MQ_PUBLIC_ACCESS
    //// Provider API
    void  pv_list();
    void  pv_newMsg( QString name, QString msg);
    void  pv_infoQ();

    //// Requester API
    void  rq_updateMsg( int seq, QString name, QString msg);
    void  rq_info( QString name, QString ver);
};

    // In class declare  (MyClass)
    ChatSapi*  _commonSapi;

    // In class code  (MyClass)
    _commonSapi = new ChatSapi( this);
    _commonSapi->open("//Chat/Pipes/pipeCommon!", ArnSapi::Mode::Provider);
    _commonSapi->batchConnect( QRegExp("^pv_(.+)"), this, "chat\\1");
.
.
void  ServerMain::chatNewMsg( QString name, QString msg)
{
    int  seq = ...;
    _commonSapi->rq_updateMsg( seq, name, msg);
}

void  MyClass::chatInfoQ()
{
    ChatSapi*  sapi = qobject_cast<ChatSapi*>( sender());
    Q_ASSERT(sapi);
    sapi->rq_info("Arn Chat Demo", "1.0");
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
    bool  open( QString pipePath, Mode mode = Mode(),
                const char* providerPrefix = 0, const char* requesterPrefix = 0);

    using ArnRpc::pipePath;
    using ArnRpc::mode;
    using ArnRpc::setHeartBeatSend;
    using ArnRpc::setHeartBeatCheck;
    using ArnRpc::isHeartBeatOk;
    using ArnRpc::invoke;
    using ArnRpc::batchConnect;

private:
    using ArnRpc::setPipe;
    using ArnRpc::setReceiver;
    using ArnRpc::setMethodPrefix;
    using ArnRpc::setIncludeSender;
    using ArnRpc::setMode;
    using ArnRpc::addSenderSignals;
    using ArnRpc::rpcSender;
};


#endif // ARNSAPI_HPP
