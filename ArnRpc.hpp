// Copyright (C) 2010-2013 Michael Wiklund.
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

#ifndef ARNRPC_HPP
#define ARNRPC_HPP

#include "Arn.hpp"
#include "XStringMap.hpp"
#include "ArnLib_global.hpp"
#include "MQFlags.hpp"
#include <QGenericArgument>
#include <QString>
#include <QByteArray>
#include <QObject>

#define MQ_ARG(type, label, data) MQArgument<type >(#type, #label, data)

class RpcReceiverStorage;
class DynamicSignals;
class QMetaMethod;
class QRegExp;


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


template <class T>
class MQArgument: public MQGenericArgument
{
public:
    inline MQArgument( const char *aName, const char *aLabel, const T &aData)
        : MQGenericArgument( aName, aLabel, static_cast<const void *>(&aData))
        {}
};


class ARNLIBSHARED_EXPORT ArnRpc : public QObject
{
    Q_OBJECT
public:
    struct Mode{
        enum E {
            Provider      = 0x01,
            AutoDestroy   = 0x02,
            UuidPipe      = 0x04,
            NoDefaultArgs = 0x08,
            Debug         = 0x10
        };
        MQ_DECLARE_FLAGS( Mode)
    };

    explicit  ArnRpc( QObject* parent = 0);
    QString  pipePath() const;
    bool  open( QString pipePath);
    void  setPipe( ArnItem* pipe);
    void  setReceiver( QObject* receiver);
    void  setMethodPrefix( QString prefix);
    void  setIncludeSender( bool v);
    void  setMode( Mode mode);
    Mode  mode()  const;
    void  addSenderSignals( QObject* sender, QString prefix);
    bool invoke( const QString& funcName,
                 MQGenericArgument val0 = MQGenericArgument(0),
                 MQGenericArgument val1 = MQGenericArgument(),
                 MQGenericArgument val2 = MQGenericArgument(),
                 MQGenericArgument val3 = MQGenericArgument(),
                 MQGenericArgument val4 = MQGenericArgument(),
                 MQGenericArgument val5 = MQGenericArgument(),
                 MQGenericArgument val6 = MQGenericArgument(),
                 MQGenericArgument val7 = MQGenericArgument());

    void  sendText( QString txt);
    ArnRpc*  rpcSender();
    static ArnRpc*  rpcSender( QObject* receiver);
    static void  batchConnect( const QObject* sender, const QRegExp& rgx,
                               const QObject* receiver, const QString &replace,
                               Mode mode = Mode());

    void  batchConnect( const QRegExp& rgx,
                        const QObject* receiver, const QString &replace,
                        Mode mode = Mode()) {
        batchConnect( this, rgx, receiver, replace, mode);
    }

    void  batchConnect( const QObject* sender, const QRegExp& rgx,
                        const QString &replace,
                        Mode mode = Mode()) {
        batchConnect( sender, rgx, this, replace, mode);
    }

signals:
    void  pipeClosed();
    void  textReceived( QString text);

public slots:

private slots:
    void  pipeInput( QByteArray data);
    void  destroyPipe();

protected:
    void  errorLog( QString errText, ArnError err = ArnError::Undef, void* reference = 0);

private:
    bool  xsmAddArg( XStringMap& xsm, const MQGenericArgument& arg, uint index, int& nArg);
    bool  xsmLoadArg( const XStringMap& xsm, QGenericArgument& arg, int &index, const QByteArray& methodName);
    void  funcHelp( const XStringMap &xsm);
    void  funcHelpMethod( const QMetaMethod& method, QByteArray name, int parNumMin);

    DynamicSignals*  _dynamicSignals;
    RpcReceiverStorage*  _receiverStorage;
    ArnItem*  _pipe;
    QObject*  _receiver;
    QByteArray  _methodPrefix;
    bool  _isIncludeSender;
    Mode  _mode;
    bool  _legacy;
};

MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnRpc::Mode)
Q_DECLARE_METATYPE(ArnRpc*)

#endif // ARNRPC_HPP
