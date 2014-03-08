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

#ifndef ARNSERVER_HPP
#define ARNSERVER_HPP

#include "ArnLib_global.hpp"
#include "Arn.hpp"
#include "MQFlags.hpp"
#include <QObject>
#include <QHostAddress>

class QTcpServer;


//! Class for making an _Arn Server_.
/*!
[About Sharing Arn Data Objects](\ref gen_shareArnobj)

<b>Example usage</b> \n \code
    // In class declare
    ArnServer*  _server;

    // In class code
    _server = new ArnServer( ArnServer::Type::NetSync, this);
    _server->start();
\endcode
*/
class ARNLIBSHARED_EXPORT ArnServer : public QObject
{
    Q_OBJECT
public:
    struct Type {
        enum E {
            NetSync
        };
        MQ_DECLARE_ENUM( Type)
    };

    //! Create an Arn _server_ object
    /*! \param[in] serverType For now only _NetSync_ is available.
     */
    ArnServer( Type serverType, QObject *parent = 0);

    //! Start the Arn _server_
    /*! \param[in] port is the port number (default 2022).
     *  \param[in] listenAddr is the interface address to listen for connections (default any)
     */
    void  start( int port = -1, QHostAddress listenAddr = QHostAddress::Any);

    //! Port number of the Arn _server_
    /*! \retval is the port number.
     */
    int  port();

    //! Address of the interface used to listening for connections to the Arn _server_
    /*! \retval is the address (which usually is QHostAddress::Any).
     *  \see start()
     */
    QHostAddress  listenAddress();

private:
    QTcpServer *_tcpServer;
    bool  _tcpServerActive;
    Type  _serverType;

private slots:
    void tcpConnection();
};

#endif // ARNSERVER_HPP
