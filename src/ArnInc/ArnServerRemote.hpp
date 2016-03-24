// Copyright (C) 2010-2016 Michael Wiklund.
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

#ifndef ARNSERVERREMOTE_HPP
#define ARNSERVERREMOTE_HPP

#include "ArnLib_global.hpp"
#include "ArnItem.hpp"
#include "MQFlags.hpp"
#include <QObject>

class ArnServer;
class ArnServerSession;
class ArnServerRemote;
class ArnServerRemotePrivate;
class QHostInfo;
class QTimer;

namespace ArnPrivate {
class KillMode {
    Q_GADGET
    Q_ENUMS(E)
public:
    enum E {
        Off,
        Delay10Sec,
        Delay60Sec
    };
    MQ_DECLARE_ENUMTXT( KillMode)
};
}

class ArnServerRemoteSession : public QObject
{
    Q_OBJECT
public:
    typedef ArnPrivate::KillMode  KillMode;

    ArnServerRemoteSession( ArnServerSession* arnServerSession, ArnServerRemote* arnServerRemote);

signals:

private slots:
    void  onInfoReceived( int type);
    void  onLoginCompleted();
    void  onIpLookup( const QHostInfo& host);
    void  doKillChanged();
    void  doKillCountdown();
    void  doChatAdd( const QString& txt);
    void  onMessageReceived( int type, const QByteArray& data);
    void  shutdown();

private:
    ArnServerRemote*  _arnServerRemote;
    ArnServerSession*  _arnServerSession;
    QString  _sessionPath;
    QTimer*  _timerKill;
    ArnItem  _arnKill;
    uint  _killCountdown;
    ArnItem  _arnChatPv;
};


class ARNLIBSHARED_EXPORT ArnServerRemote : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ArnServerRemote)

public:
    explicit ArnServerRemote( QObject* parent = 0);

    void  startUseServer( ArnServer* arnServer);

    ~ArnServerRemote();

signals:

public slots:

    //! \cond ADV
protected:
    ArnServerRemote( ArnServerRemotePrivate& dd, QObject* parent);
    ArnServerRemotePrivate* const  d_ptr;
    //! \endcond

private slots:
    void  onNewSession();

private:
    void  init();
};

#endif // ARNSERVERREMOTE_HPP
