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

#include "ArnZeroConf.hpp"
#ifdef MDNS_INTERN
#  include "mDNS/ArnMDns.hpp"
#  include "mDNS/mDNSShared/dns_sd.h"
#else
#  include <dns_sd.h>
#endif
#include <QSocketNotifier>
#include <QtEndian>


//// Used to hide dns_sd details from the header
class ArnZeroConfIntern
{
public:
    static void  registerServiceCallback(DNSServiceRef service, DNSServiceFlags flags, DNSServiceErrorType errCode,
                                    const char* name, const char* regtype, const char* domain, void* context);

    static void  resolveServiceCallback(DNSServiceRef service, DNSServiceFlags flags, quint32 iface,
                                   DNSServiceErrorType errCode, const char* fullname, const char* host, quint16 port,
                                   quint16 txtLen, const unsigned char* txt, void* context);
    static void  browseServiceCallback(DNSServiceRef service, DNSServiceFlags flags, quint32 iface, DNSServiceErrorType errCode,
                                   const char* serviceName, const char* regtype, const char* replyDomain, void* context);
};


ArnZeroConfB::ArnZeroConfB( QObject* parent)
    : QObject( parent)
{
    _port        = 2022;
    _iface       = 0;
    _notifier    = 0;
    _state       = State::None;
    _serviceType = "arn";
    _socketType  = QAbstractSocket::TcpSocket;
    _domain      = "local.";  // Default

#ifndef MDNS_INTERN
    // In case using Avahi, stop stupid warnings about libdns_sd
    qputenv("AVAHI_COMPAT_NOWARN", "1");
#endif
}


ArnZeroConfB::~ArnZeroConfB()
{
}


void  ArnZeroConfB::parseFullDomain( const QByteArray& domainName)
{
    QRegExp rx("^((?:\\\\{2,2}|\\\\\\.|\\\\\\d{3,3}|[^\\\\\\.])+)\\.(.+\\._(?:tcp|udp))\\.(.+)");
    if (rx.indexIn( QString::fromUtf8( domainName.constData())) != -1) {
        qDebug() << "Servname: capNum=" << rx.captureCount();
        setServiceType(rx.cap(2));
        setDomain(rx.cap(3));

        QByteArray servName = rx.cap(1).toUtf8();
        for (int i= 0; i < servName.length(); ++i) {
            if (servName.at(i) != '\\')  continue;

            char c = servName.at(i + 1);
            if ((c == '\\') || (c == '.'))
                servName.remove(i, 1);
            else if (isdigit(c))
                servName.replace(i, 4, QByteArray(1, char(servName.mid(i + 1, 3).toInt())));
        }

        setServiceName( QString::fromUtf8( servName.constData(), servName.length()));
    }
}


QStringList  ArnZeroConfB::subTypes()  const
{
    return _serviceSubTypes;
}


void  ArnZeroConfB::setSubTypes( const QStringList& subTypes)
{
    _serviceSubTypes = subTypes;
}


void  ArnZeroConfB::addSubType( const QString& subType)
{
    _serviceSubTypes += subType;
}


quint16  ArnZeroConfB::port()  const
{
    return _port;
}


void  ArnZeroConfB::setPort( quint16 port)
{
    _port = port;
}


ArnZeroConfB::State  ArnZeroConfB::state()  const
{
    return _state;
}


QByteArray  ArnZeroConfB::txtRecord()  const
{
        return _txtRec;
}


void  ArnZeroConfB::setTxtRecord( const QByteArray& txt)
{
    _txtRec = txt;
}


bool  ArnZeroConfB::getTxtRecordMap( XStringMap& xsm)
{
    xsm.clear();
    QByteArray  txtRec = _txtRec;
    forever {
        if (txtRec.isEmpty())  return true;  // No more params
        int parLen = quint8( txtRec.at(0));
        if (parLen > (txtRec.size() - 1))  return false;  // Too long parameter length
        QByteArray  par( txtRec.constData() + 1, parLen);
        txtRec.remove(0, parLen + 1);

        int  eqPos = par.indexOf('=');
        xsm.add( par.mid(0, eqPos), par.mid( eqPos + 1));
    }
}


void  ArnZeroConfB::setTxtRecordMap( const XStringMap& xsm)
{
    _txtRec.clear();
    for (int i = 0; i < xsm.size(); ++i) {
        QByteArray  txtPar = xsm.key(i) + "=" + xsm.value(i);
        txtPar.insert(0, quint8( txtPar.length()));
        _txtRec += txtPar;
    }
}


QString  ArnZeroConfB::serviceName()  const
{
    return _serviceName;
}


void  ArnZeroConfB::setServiceName( const QString& name)
{
    _serviceName = name;
}


QAbstractSocket::SocketType  ArnZeroConfB::socketType()  const
{
    return _socketType;
}


void  ArnZeroConfB::setSocketType( QAbstractSocket::SocketType type)
{
    _socketType = type;
}


QString  ArnZeroConfB::serviceType()  const
{
    return _serviceType;
}


void  ArnZeroConfB::setServiceType( const QString& type)
{
    QStringList tp = type.split('.');
    setSocketType( QAbstractSocket::TcpSocket);  // Default

    if ((tp.size() == 2) && (tp.at(0).size() > 0) && (tp.at(0)[0] == '_')) {
        _serviceType = tp.at(0).mid(1);
        if (tp[1] == "_udp") {
            setSocketType( QAbstractSocket::UdpSocket);
        }
        else if (tp[1] != "_tcp") {
            setSocketType( QAbstractSocket::UnknownSocketType);
        }
    }
    else {
        _serviceType = type;
    }
}


QString  ArnZeroConfB::domain()  const
{
    return _domain;
}


void  ArnZeroConfB::setDomain( const QString& domain)
{
    _domain = domain;
}


QString  ArnZeroConfB::host()  const
{
    return _host;
}


void  ArnZeroConfB::setHost( const QString& host)
{
    _host = host;
}


QString  ArnZeroConfB::fullServiceType()  const
{
    QString  ret = "_" + _serviceType + "._";
    if (_socketType == QAbstractSocket::TcpSocket)
        ret += "tcp";
    else
        ret += "udp";

    return ret;
}


QByteArray  ArnZeroConfB::escapedFullDomain()  const
{
    char buffer[kDNSServiceMaxDomainName] = "";
    int err = DNSServiceConstructFullName(buffer,
                                          _serviceName.toUtf8().constData(),
                                          fullServiceType().toUtf8().constData(),
                                          domain().toUtf8().constData());
    if (err)
        return QByteArray(); // error
    return buffer;
}


void  ArnZeroConfB::socketData()
{
#ifndef MDNS_INTERN
    DNSServiceProcessResult( _serviceRef);
#endif
}


////////////////// Register

void  ArnZeroConfRegister::init()
{
#ifdef MDNS_INTERN
    ArnMDns::attach();
#endif
}


ArnZeroConfRegister::ArnZeroConfRegister(QObject* parent)
    : ArnZeroConfB( parent)
{
    init();
}


ArnZeroConfRegister::ArnZeroConfRegister( const QString& serviceName, QObject* parent)
    : ArnZeroConfB( parent)
{
    setServiceName( serviceName);

    init();
}


ArnZeroConfRegister::ArnZeroConfRegister( const QString& serviceName, const QString& serviceType,
                                          quint16 port, QObject* parent)
    : ArnZeroConfB( parent)
{
    setServiceName( serviceName);
    setServiceType( serviceType);
    setPort( port);

    init();
}


ArnZeroConfRegister::~ArnZeroConfRegister()
{
    if (state() != State::None)
        releaseService();

#ifdef MDNS_INTERN
    ArnMDns::detach();
#endif
}


void  ArnZeroConfRegister::registerService( bool noAutoRename)
{
    if (state() != State::None) {
        qWarning() << "ZeroConfRegister: Error register service while not in None state";
        emit registrationError(0);
        return;
    }

    QByteArray  txtRec = txtRecord();
    if (txtRec.isEmpty())
        txtRec.fill(0, 1);

    QStringList  serviceTypes( fullServiceType());
    serviceTypes += _serviceSubTypes;

    qDebug() << "Register: subTypes=" << serviceTypes.join(",_").toUtf8().constData()
             << "";
    DNSServiceErrorType err;
    err = DNSServiceRegister(&_serviceRef,
                             noAutoRename ? kDNSServiceFlagsNoAutoRename : 0,
                             _iface,
                             serviceName().toUtf8().constData(),
                             serviceTypes.join(",_").toUtf8().constData(),
                             domain().toUtf8().constData(),
                             host().toUtf8().constData(),
                             qToBigEndian( port()),
                             txtRec.length(),
                             txtRec.constData(),
                             ArnZeroConfIntern::registerServiceCallback,
                             this);
    if (err != kDNSServiceErr_NoError) {
        _state = State::None;
        emit registrationError(err);
    }
    else {
        _state = State::Registering;
#ifndef MDNS_INTERN
        _notifier = new QSocketNotifier( DNSServiceRefSockFD( _serviceRef), QSocketNotifier::Read, this);
        connect( _notifier, SIGNAL(activated(int)), this, SLOT(socketData()));
#endif
    }
}


void  ArnZeroConfRegister::releaseService()
{
    if ((state() != State::Registered) && (state() != State::Registering)) {
        qWarning() << "ZeroConfRegister release: unregistered service";
    }
    else {
        DNSServiceRefDeallocate( _serviceRef);
        _state = State::None;
#ifndef MDNS_INTERN
        _notifier->deleteLater();
        _notifier = 0;
#endif
    }
}


void  ArnZeroConfIntern::registerServiceCallback( DNSServiceRef service, DNSServiceFlags flags,
        DNSServiceErrorType errCode, const char* name, const char* regtype, const char* domain, void* context)
{
    Q_UNUSED(service);
    Q_UNUSED(flags);
    Q_UNUSED(regtype);
    qDebug() << "Register callback: name=" << name << " regtype=" << regtype << " domain=" << domain;
    ArnZeroConfRegister*  self = reinterpret_cast<ArnZeroConfRegister*>(context);
    qDebug() << "Register callback: Iregtype=" << self->serviceType();
    if (errCode == kDNSServiceErr_NoError) {
        QString  servName = QString::fromUtf8( name);
        self->setServiceName( servName);
        self->setDomain( QString::fromUtf8( domain));
        self->_state = ArnZeroConfB::State::Registered;
        emit self->registered( servName);
    }
    else {
        self->_state = ArnZeroConfB::State::None;
        emit self->registrationError( errCode);
    }
}


////////////////// Resolv

void  ArnZeroConfResolv::init()
{
#ifdef MDNS_INTERN
    ArnMDns::attach();
#endif
}


ArnZeroConfResolv::ArnZeroConfResolv(QObject* parent)
    : ArnZeroConfB( parent)
{
    init();
}


ArnZeroConfResolv::ArnZeroConfResolv(const QString& serviceName, QObject* parent)
    : ArnZeroConfB( parent)
{
    setServiceName( serviceName);

    init();
}


ArnZeroConfResolv::ArnZeroConfResolv( const QString& serviceName, const QString& serviceType,
                                      QObject* parent)
    : ArnZeroConfB( parent)
{
    setServiceName( serviceName);
    setServiceType( serviceType);

    init();
}


ArnZeroConfResolv::~ArnZeroConfResolv()
{
    if (state() != State::None)
        releaseService();

#ifdef MDNS_INTERN
    ArnMDns::detach();
#endif
}


void  ArnZeroConfResolv::resolve(bool forceMulticast)
{
    if ((state() != State::None) && (state() != State::Resolved)) {
        qWarning() << "ZeroConfResolv: Error resolve service when not None or Resolved state";
        emit resolveError(0);
        return;
    }
    if (state() == State::Resolved)
        releaseService();

    DNSServiceErrorType err;
    err = DNSServiceResolve(&_serviceRef,
                            (forceMulticast ? kDNSServiceFlagsForceMulticast : 0),
                            _iface,
                            serviceName().toUtf8().constData(),
                            fullServiceType().toUtf8().constData(),
                            domain().toUtf8().constData(),
                            ArnZeroConfIntern::resolveServiceCallback,
                            this);
    if (err != kDNSServiceErr_NoError) {
        _state = State::None;
        emit resolveError(err);
    }
    else {
        _state = State::Resolving;
#ifndef MDNS_INTERN
        _notifier = new QSocketNotifier( DNSServiceRefSockFD( _serviceRef), QSocketNotifier::Read, this);
        connect( _notifier, SIGNAL(activated(int)), this, SLOT(socketData()));
#endif
    }
}


void  ArnZeroConfResolv::releaseService()
{
    if ((state() != State::Resolved) && (state() != State::Resolving)) {
        qWarning() << "ZeroConfResolv release: unresolved service";
    }
    else {
        DNSServiceRefDeallocate( _serviceRef);
        _state = State::None;
#ifndef MDNS_INTERN
        _notifier->deleteLater();
        _notifier = 0;
#endif
    }
}


void  ArnZeroConfIntern::resolveServiceCallback(DNSServiceRef service, DNSServiceFlags flags, quint32 iface,
        DNSServiceErrorType errCode, const char* fullname, const char* host, quint16 port, quint16 txtLen,
        const unsigned char* txt, void* context)
{
    Q_UNUSED(service);
    Q_UNUSED(flags);
    ArnZeroConfResolv* self = reinterpret_cast<ArnZeroConfResolv*>(context);

    if(errCode == kDNSServiceErr_NoError) {
        self->parseFullDomain( fullname);
        self->setHost( host);
        self->setPort( qFromBigEndian( port));
        self->setTxtRecord( txtLen > 0 ? QByteArray((const char*) txt, txtLen) : QByteArray());
        self->_iface = iface;
        self->_state = ArnZeroConfB::State::Resolved;
        emit self->resolved( fullname);
    }
    else {
        self->_state = ArnZeroConfB::State::None;
        emit self->resolveError( errCode);
    }
}


////////////////// Browse

void  ArnZeroConfBrowser::init()
{
#ifdef MDNS_INTERN
    ArnMDns::attach();
#endif
}


ArnZeroConfBrowser::ArnZeroConfBrowser(QObject* parent)
    : ArnZeroConfB( parent)
{
    init();
}


ArnZeroConfBrowser::ArnZeroConfBrowser( const QString& serviceType, QObject* parent)
    : ArnZeroConfB( parent)
{
    setServiceType( serviceType);
    init();
}


ArnZeroConfBrowser::~ArnZeroConfBrowser() {
    if(isBrowsing())
        stopBrowse();

#ifdef MDNS_INTERN
    ArnMDns::detach();
#endif
}


QStringList ArnZeroConfBrowser::activeServiceNames() const
{
    return _activeServiceNames;
}


bool  ArnZeroConfBrowser::isBrowsing()  const
{
    return _state == State::Browsing;
}


void  ArnZeroConfBrowser::setSubType( const QString& subtype)
{
    setSubTypes( QStringList( subtype));
}


QString ArnZeroConfBrowser::subType()
{
    QStringList  subT = subTypes();
    if (subT.isEmpty())  return QString();
    return subT.at(0);
}


void ArnZeroConfBrowser::browse( bool enable)
{
    if (!enable)  return stopBrowse();

    if ((state() != State::None) && (state() != State::Resolved)) {
        qWarning() << "ZeroConfBrowser: Error starting browse service when not None or Resolved state";
        emit browseError(0);
        return;
    }

    QStringList serviceTypes( fullServiceType());
    serviceTypes += _serviceSubTypes;

    DNSServiceErrorType err;
    err = DNSServiceBrowse(&_serviceRef,
                           0,
                           _iface,
                           serviceTypes.join(",_").toUtf8().constData(),
                           domain().toUtf8().constData(),
                           ArnZeroConfIntern::browseServiceCallback,
                           this);
    if (err) {
        emit browseError(err);
    }
    else {
        _state = State::Browsing;
#ifndef MDNS_INTERN
        _notifier = new QSocketNotifier( DNSServiceRefSockFD( _serviceRef), QSocketNotifier::Read, this);
        connect( _notifier, SIGNAL(activated(int)), this, SLOT(socketData()));
#endif
    }
}


void ArnZeroConfBrowser::stopBrowse()
{
    if (state() == State::Browsing) {
        DNSServiceRefDeallocate( _serviceRef);
#ifndef MDNS_INTERN
        _notifier->deleteLater();
        _notifier = 0;
#endif
    }
    _state = State::None;
}


void  ArnZeroConfIntern::browseServiceCallback( DNSServiceRef service, DNSServiceFlags flags, quint32 iface,
        DNSServiceErrorType errCode, const char* serviceName, const char* regtype, const char* replyDomain, void* context)
{
    Q_UNUSED(service);
    Q_UNUSED(iface);
    //Q_UNUSED(regtype);

    qDebug() << "Browse CB: regType=" << regtype << " replyDomain=" << replyDomain;
    ArnZeroConfBrowser* self = reinterpret_cast<ArnZeroConfBrowser*>(context);
    if (errCode == kDNSServiceErr_NoError) {
        bool  isAdded = (flags & kDNSServiceFlagsAdd) != 0;
        QString  servName  = QString::fromUtf8( serviceName);
        QString  repDomain = QString::fromUtf8( replyDomain);
        if (isAdded != self->_activeServiceNames.contains( servName)) {  // Not multiple add or remove, ok
            if (isAdded) {
                self->_activeServiceNames += servName;
                emit self->serviceAdded( servName, repDomain);
            }
            else {
                self->_activeServiceNames.removeOne( servName);
                emit self->serviceRemoved( servName, repDomain);
            }
            emit self->serviceChanged( isAdded, servName, repDomain);
        }
    }
    else {
        emit self->browseError( errCode);
    }
}
