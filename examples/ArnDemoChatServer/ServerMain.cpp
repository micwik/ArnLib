// Copyright (C) 2010-2014 Michael Wiklund.
// All rights reserved.
// Contact: arnlib@wiklunden.se
//
// This file is part of the ArnDemoChat - Active Registry Network Demo Chat.
// Parts of ArnDemoChat depend on Qt 4 and/or other libraries that have their own
// licenses. ArnDemoChat is independent of these licenses; however, use of these other
// libraries is subject to their respective license agreements.
//
// The MIT License (MIT)
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
// THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//! [code]
#include "ServerMain.hpp"
#include <ArnInc/ArnItem.hpp>
#include <ArnInc/ArnDiscoverRemote.hpp>
#include <QTime>
#include <QSocketNotifier>
#include <QCoreApplication>
#include <QDebug>


ServerMain::ServerMain( QObject* parent) :
    QObject( parent)
{
    _timer.start(1000);
    connect( &_timer, SIGNAL(timeout()), this, SLOT(doTimeUpdate()));

    _server = new ArnServer( ArnServer::Type::NetSync, this);
    _server->start(0);  // Start server on dynamic port

    //// Setuo discover remote advertise
    _discoverRemote = new ArnDiscoverRemote( this);
    _discoverRemote->setService("Demo Chat Server");
    _discoverRemote->addGroup("arndemo/chat");
    _discoverRemote->addCustomProperty("ChatProtoVer", "1.0");
    _discoverRemote->startUseServer( _server);

    _arnTime.open("//Chat/Time/value");

    //// Create common service-api, used for calling requesters by "broadcast"
    _commonSapi = new ChatSapi( this);
    _commonSapi->open("//Chat/Pipes/pipeCommon!", ArnSapi::Mode::Provider);
    _commonSapi->batchConnect( QRegExp("^pv_(.+)"), this, "chat\\1");

    //// Monitor pipe folder for new connecting requesters
    ArnItem*  arnPipes = new ArnItem("//Chat/Pipes/", this);
    connect( arnPipes, SIGNAL(arnItemCreated(QString)), this, SLOT(doNewSession(QString)));

    //// Setup shutdown logic
    qWarning() << "\nPress <Enter> to quit.\n";
    QSocketNotifier*  stdinNotifier = new QSocketNotifier( 0, QSocketNotifier::Read, this);
    connect( stdinNotifier, SIGNAL(activated(int)), this, SLOT(shutdown()), Qt::QueuedConnection);
    _isShuttingDown = false;
}


void  ServerMain::doNewSession( QString path)
{
    if (!Arn::isProviderPath( path))  return;  // Only provider pipe is used

    ChatSapi*  soleSapi = new ChatSapi( this);
    soleSapi->open( path, ArnSapi::Mode::Provider);
    soleSapi->batchConnect( QRegExp("^pv_(.+)"), this, "chat\\1");
    connect( soleSapi, SIGNAL(pipeClosed()), soleSapi, SLOT(deleteLater()));
}


void  ServerMain::doTimeUpdate()
{
    _arnTime = QTime::currentTime().toString();
}


void  ServerMain::shutdown()
{
    if (_isShuttingDown)  return;  // Guard against multiple shutdowns

    _isShuttingDown = true;
    delete _discoverRemote;  // Must be deleted while still in the main eventloop
    QCoreApplication::quit();
}


void  ServerMain::chatList()
{
    //// Get calling service-api, to give a private "answer" (the list)
    ChatSapi*  sapi = qobject_cast<ChatSapi*>( sender());
    Q_ASSERT(sapi);
    for (int i = 0; i < _chatNameList.size(); ++i) {
        sapi->rq_updateMsg( i, _chatNameList.at(i), _chatMsgList.at(i));
    }
}


void  ServerMain::chatNewMsg( QString name, QString msg)
{
    _chatNameList += name;
    _chatMsgList  += msg;
    int  seq = _chatNameList.size() - 1;

    //// Broadcast the new message to all requesters
    _commonSapi->rq_updateMsg( seq, name, msg);
}


void  ServerMain::chatInfoQ()
{
    //// Get calling service-api, to give a private "answer" (the info)
    ChatSapi*  sapi = qobject_cast<ChatSapi*>( sender());
    Q_ASSERT(sapi);
    sapi->rq_info("Arn Chat Demo", "1.0");
}
//! [code]
