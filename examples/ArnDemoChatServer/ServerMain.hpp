// Copyright (C) 2010-2013 Michael Wiklund.
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
#ifndef SERVERMAIN_HPP
#define SERVERMAIN_HPP

#include "ChatSapi.hpp"
#include <ArnInc/ArnItem.hpp>
#include <ArnInc/ArnServer.hpp>
#include <QTimer>
#include <QStringList>
#include <QObject>


class ServerMain : public QObject
{
    Q_OBJECT
public:
    explicit ServerMain( QObject* parent = 0);

signals:

public slots:

private slots:
    void  doNewSession( QString path);
    void  doTimeUpdate();

    // Chat Provider routines
    void  chatList();
    void  chatNewMsg( QString name, QString msg);
    void  chatInfoQ();

private:
    QStringList  _chatNameList;
    QStringList  _chatMsgList;
    QTimer  _timer;

    ArnItem  _arnTime;
    ArnServer*  _server;
    ChatSapi*  _commonSapi;
};

#endif // SERVERMAIN_HPP
//! [code]
