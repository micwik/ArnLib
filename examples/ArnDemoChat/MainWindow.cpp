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
#include "MainWindow.hpp"
#include "tmp/ui_MainWindow.h"
#include <ArnInc/ArnDiscoverRemote.hpp>


MainWindow::MainWindow( QWidget* parent) :
    QMainWindow( parent),
    _ui( new Ui::MainWindow)
{
    _ui->setupUi( this);
    _ui->userEdit->setFocus();
    connect( _ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(doSendLine()));

    //// Select Arn tree to sync (//) and auto reconnect
    _arnClient.addMountPoint("//");
    _arnClient.setAutoConnect(true);

    //// Setuo discover connect to the Demo Chat Server
    ArnDiscoverConnector*  connector = new ArnDiscoverConnector( _arnClient, "DemoChat");
    connector->setResolver( new ArnDiscoverResolver());
    connector->setService("Demo Chat Server");
    connector->start();

    _arnTime.open("//Chat/Time/value");
    connect( &_arnTime, SIGNAL(changed(QString)), this, SLOT(doTimeUpdate(QString)));

    //// Setup common service-api, used for calling requesters by "broadcast"
    _commonSapi.open("//Chat/Pipes/pipeCommon");
    _commonSapi.batchConnect( QRegExp("^rq_(.+)"), this, "chat\\1");

    //// Setup sole service-api, used for 2 point requester provider calls
    //// Pipe is uniquely named (uuid) and auto destroyed when disconnected
    _soleSapi.open("//Chat/Pipes/pipe", ArnSapi::Mode::UuidAutoDestroy);
    _soleSapi.batchConnect( QRegExp("^rq_(.+)"), this, "chat\\1");

    //// Request info and listing from provider
    _soleSapi.pv_infoQ();
    _soleSapi.pv_list();
}


MainWindow::~MainWindow()
{
    delete _ui;
}


void  MainWindow::doTimeUpdate( QString timeStr)
{
    _ui->timeEdit->setTime( QTime::fromString( timeStr));
}


void  MainWindow::doSendLine()
{
    QString  myName = _ui->userEdit->text();
    QString  line   = _ui->lineEdit->text();
    _ui->lineEdit->clear();

    _soleSapi.pv_newMsg( myName, line);
}


void  MainWindow::chatUpdateMsg( int seq, QString name, QString msg)
{
    if (seq >= _chatNameList.size()) {
        _chatNameList.resize( seq + 1);
        _chatMsgList.resize(  seq + 1);
    }
    _chatNameList[ seq] = name;
    _chatMsgList[  seq] = msg;

    QString  text;
    for (int i = 0; i < _chatNameList.size(); ++i) {
        text += _chatNameList.at(i) + ":  " + _chatMsgList.at(i) + "\n";
    }
    _ui->textEdit->setText( text);
}


void  MainWindow::chatInfo( QString name, QString ver)
{
    _ui->appNameLabel->setText( name);
    _ui->verLabel->setText( ver);
}
//! [code]
