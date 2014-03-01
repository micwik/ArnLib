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

#include "ArnInc/ArnZeroConf.hpp"
#include "ArnInc/Arn.hpp"
#include "ArnInc/ArnLib.hpp"
#ifdef MDNS_INTERN
#  include "mDNS/ArnMDns.hpp"
#  include "mDNS/mDNSShared/dns_sd.h"
#else
#  include <dns_sd.h>
#endif
#include <QSocketNotifier>
#include <QHostInfo>
#include <QTimer>
#include <QtEndian>

using Arn::XStringMap;


//// Used to hide dns_sd details from the header
class ArnZeroConfIntern
{
public:
    static void DNSSD_API  registerServiceCallback( 
                               DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errCode,
                               const char* name, const char* regtype, const char* domain, void* context);
    static void DNSSD_API  resolveServiceCallback( 
                               DNSServiceRef sdRef, DNSServiceFlags flags, quint32 iface,
                               DNSServiceErrorType errCode, const char* fullname, const char* host,
                               quint16 port, quint16 txtLen, const unsigned char* txt, void* context);
    static void DNSSD_API  browseServiceCallback( 
                               DNSServiceRef sdRef, DNSServiceFlags flags, quint32 iface, 
                               DNSServiceErrorType errCode, const char* serviceName, const char* regtype,
                               const char* replyDomain, void* context);
    static void DNSSD_API  lookupHostCallback(
                               DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                               DNSServiceErrorType errCode, const char *hostname, 
                               const struct sockaddr *address, uint32_t ttl, void *context);
};


QAtomicInt  ArnZeroConfB::_idCount(1);


ArnZeroConfB::ArnZeroConfB( QObject* parent)
    : QObject( parent)
{
    _port        = Arn::defaultTcpPort;
    _iface       = 0;
    _notifier    = 0;
    _state       = ArnZeroConf::State::None;
    _serviceType = "arn";
    _socketType  = QAbstractSocket::TcpSocket;
    _domain      = "local.";  // Default
    _sdRef       = 0;

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


QByteArray  ArnZeroConfB::escapedName( const QByteArray& name)
{
    QByteArray  retVal;
    const char*  namep = name.constData();
    int  len = name.size();

    for (int i = 0; i < len; ++i) {
        uchar  c = *namep++;
        if (c <= ' ') {
            retVal += '\\';
            retVal += QByteArray::number(c).rightJustified(3, '0');
        }
        else if ((c == '.') || (c == '\\')) {
            retVal += '\\';
            retVal += char(c);
        }
        else
            retVal += char(c);
    }

    return retVal;
}


int  ArnZeroConfB::getNextId()
{
    return ArnZeroConfB::_idCount.fetchAndAddRelaxed(1);
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
    if (!subType.isEmpty() && !_serviceSubTypes.contains( subType))
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


ArnZeroConf::State  ArnZeroConfB::state()  const
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


QHostAddress  ArnZeroConfB::hostAddr()  const
{
    return _hostAddr;
}

void  ArnZeroConfB::setHostAddr( const QHostAddress &hostAddr)
{
    _hostAddr = hostAddr;
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
    if (state() != ArnZeroConf::State::None)
        releaseService();

#ifdef MDNS_INTERN
    ArnMDns::detach();
#endif
}


void  ArnZeroConfRegister::registerService( bool noAutoRename)
{
    if (state() != ArnZeroConf::State::None) {
        qWarning() << "ZeroConfRegister: Error register service while not in None state";
        emit registrationError(0);
        return;
    }

    QByteArray  txtRec = txtRecord();
    if (txtRec.isEmpty())
        txtRec.fill(0, 1);

    QByteArray  serviceTypes = fullServiceType().toUtf8();
    foreach (const QString& subType, _serviceSubTypes) {
        serviceTypes += ",_" + escapedName( subType.toUtf8());
    }

    if (Arn::debugZeroConf)  qDebug() << "Register: serviceTypes=" << serviceTypes.constData() << "";
    DNSServiceErrorType err;
    err = DNSServiceRegister(&_sdRef,
                             noAutoRename ? kDNSServiceFlagsNoAutoRename : 0,
                             _iface,
                             serviceName().toUtf8().constData(),
                             serviceTypes.constData(),
                             domain().toUtf8().constData(),
                             host().toUtf8().constData(),
                             qToBigEndian( port()),
                             txtRec.length(),
                             txtRec.constData(),
                             ArnZeroConfIntern::registerServiceCallback,
                             this);
    if (err != kDNSServiceErr_NoError) {
        _state = ArnZeroConf::State::None;
        emit registrationError(err);
    }
    else {
        _state = ArnZeroConf::State::Registering;
#ifndef MDNS_INTERN
        _notifier = new QSocketNotifier( DNSServiceRefSockFD( _serviceRef), QSocketNotifier::Read, this);
        connect( _notifier, SIGNAL(activated(int)), this, SLOT(socketData()));
#endif
    }
}


void  ArnZeroConfRegister::releaseService()
{
    if (!state().isAny( ArnZeroConf::State::Register)) {
        qWarning() << "ZeroConfRegister release: unregistered service";
    }
    else {
        DNSServiceRefDeallocate( _sdRef);
        _state = ArnZeroConf::State::None;
#ifndef MDNS_INTERN
        _notifier->deleteLater();
        _notifier = 0;
#endif
    }
}


void DNSSD_API  ArnZeroConfIntern::registerServiceCallback( 
                    DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errCode,
                    const char* name, const char* regtype, const char* domain, void* context)
{
    Q_UNUSED(sdRef);
    Q_UNUSED(flags);
    Q_UNUSED(regtype);
    if (Arn::debugZeroConf)  qDebug() << "Register callback: name=" << name << " regtype=" << regtype 
                                      << " domain=" << domain;
    ArnZeroConfRegister*  self = reinterpret_cast<ArnZeroConfRegister*>(context);
    if (errCode == kDNSServiceErr_NoError) {
        QString  servName = QString::fromUtf8( name);
        self->setServiceName( servName);
        self->setDomain( QString::fromUtf8( domain));
        self->_state = ArnZeroConf::State::Registered;
        emit self->registered( servName);
    }
    else {
        self->_state = ArnZeroConf::State::None;
        emit self->registrationError( errCode);
    }
}


////////////////// Resolve

void  ArnZeroConfResolve::init()
{
    _id = -1;
    _operationTimer = new QTimer( this);
    _operationTimer->setInterval(2000);
    connect( _operationTimer, SIGNAL(timeout()), this, SLOT(operationTimeout()));

#ifdef MDNS_INTERN
    ArnMDns::attach();
#endif
}


ArnZeroConfResolve::ArnZeroConfResolve(QObject* parent)
    : ArnZeroConfB( parent)
{
    init();
}


ArnZeroConfResolve::ArnZeroConfResolve(const QString& serviceName, QObject* parent)
    : ArnZeroConfB( parent)
{
    setServiceName( serviceName);

    init();
}


ArnZeroConfResolve::ArnZeroConfResolve( const QString& serviceName, const QString& serviceType,
                                      QObject* parent)
    : ArnZeroConfB( parent)
{
    setServiceName( serviceName);
    setServiceType( serviceType);

    init();
}


ArnZeroConfResolve::~ArnZeroConfResolve()
{
    releaseResolve();

#ifdef MDNS_INTERN
    ArnMDns::detach();
#endif
}


int  ArnZeroConfResolve::id()  const
{
    return _id;
}


void  ArnZeroConfResolve::setId( int id)
{
    _id = id;
}


void  ArnZeroConfResolve::resolve( bool forceMulticast)
{
    if (_id < 0)  // No valid id set, get one
        _id = ArnZeroConfB::getNextId();

    if (state().isAny( ArnZeroConf::State::InProgress)) {
        qWarning() << "ZeroConfResolv: Error resolve service when operation still in progress";
        emit resolveError( _id, ArnZeroConf::Error::BadReqSeq);
        return;
    }
    releaseResolve();

    DNSServiceErrorType  err;
    err = DNSServiceResolve(&_sdRef,
                            (forceMulticast ? kDNSServiceFlagsForceMulticast : 0),
                            _iface,
                            serviceName().toUtf8().constData(),
                            fullServiceType().toUtf8().constData(),
                            domain().toUtf8().constData(),
                            ArnZeroConfIntern::resolveServiceCallback,
                            this);
    if (err != kDNSServiceErr_NoError) {
        _sdRef = 0;
        _state.set( ArnZeroConf::State::Resolve, false);
        emit resolveError( _id, err);
    }
    else {
        _state.set( ArnZeroConf::State::Resolving);
        _operationTimer->start();

#ifndef MDNS_INTERN
        _notifier = new QSocketNotifier( DNSServiceRefSockFD( _serviceRef), QSocketNotifier::Read, this);
        connect( _notifier, SIGNAL(activated(int)), this, SLOT(socketData()));
#endif
    }
}


void  ArnZeroConfResolve::releaseResolve()
{
    _operationTimer->stop();

    if (_sdRef) {
        DNSServiceRefDeallocate( _sdRef);
        _sdRef = 0;
        
#ifndef MDNS_INTERN
        _notifier->deleteLater();
        _notifier = 0;
#endif
    }
    _state.set( ArnZeroConf::State::Resolve, false);
}


void  ArnZeroConfResolve::operationTimeout()
{
    releaseResolve();
    emit resolveError( _id, ArnZeroConf::Error::Timeout);
}


void DNSSD_API  ArnZeroConfIntern::resolveServiceCallback(
                    DNSServiceRef sdRef, DNSServiceFlags flags, quint32 iface, 
                    DNSServiceErrorType errCode, const char* fullname, const char* host, quint16 port,
                    quint16 txtLen, const unsigned char* txt, void* context)
{
    Q_UNUSED(sdRef);
    Q_UNUSED(flags);
    ArnZeroConfResolve*  self = reinterpret_cast<ArnZeroConfResolve*>(context);
    Q_ASSERT(self);

    self->_operationTimer->stop();

    if (self->_id < 0)  // No valid id set, get one
        self->_id = ArnZeroConfB::getNextId();

    if (Arn::debugZeroConf)  qDebug() << "Resolve callback errCode=" << errCode;
    if (errCode == kDNSServiceErr_NoError) {
        QString  resHost = QString::fromUtf8( host);
        if (resHost.endsWith('.'))  // MW: Remove strangely added "."
            resHost.resize( resHost.size() - 1);
        self->parseFullDomain( fullname);
        self->setHost( resHost);
        self->setPort( qFromBigEndian( port));
        self->setTxtRecord( txtLen > 0 ? QByteArray((const char*) txt, txtLen) : QByteArray());
        self->_iface = iface;
        self->_state.set( ArnZeroConf::State::Resolving, false);
        self->_state.set( ArnZeroConf::State::Resolved);
        emit self->resolved( self->_id, fullname);
    }
    else {
        self->_state.set( ArnZeroConf::State::Resolve, false);
        emit self->resolveError( self->_id, errCode);
    }
}


////////////////// Lookup

void  ArnZeroConfLookup::init()
{
    _id = -1;
    _operationTimer = new QTimer( this);
    _operationTimer->setInterval(2000);
    connect( _operationTimer, SIGNAL(timeout()), this, SLOT(operationTimeout()));

#ifdef MDNS_INTERN
    ArnMDns::attach();
#endif
}


ArnZeroConfLookup::ArnZeroConfLookup( QObject* parent)
    : ArnZeroConfB( parent)
{
    init();
}


ArnZeroConfLookup::ArnZeroConfLookup( const QString& hostName, QObject* parent)
    : ArnZeroConfB( parent)
{
    setHost( hostName);

    init();
}


ArnZeroConfLookup::~ArnZeroConfLookup()
{
    releaseLookup();

#ifdef MDNS_INTERN
    ArnMDns::detach();
#endif
}


int  ArnZeroConfLookup::id()  const
{
    return _id;
}


void  ArnZeroConfLookup::setId( int id)
{
    _id = id;
}


void  ArnZeroConfLookup::lookup( bool forceMulticast)
{
    if (_id < 0)  // No valid id set, get one
        _id = ArnZeroConfB::getNextId();

    if (state().isAny( ArnZeroConf::State::InProgress)) {
        qWarning() << "ZeroConfLookup: Error lookup host when operation still in progress";
        emit lookupError( _id, ArnZeroConf::Error::BadReqSeq);
        return;
    }
    releaseLookup();
    
    // Unicast lookup
    if (!forceMulticast && !_host.endsWith(".local")) {
        int  ipLookupId = QHostInfo::lookupHost( _host, this, SLOT(onIpLookup(QHostInfo)));
        if (Arn::debugZeroConf)  qDebug() << "ZeroConfLookup: host=" << _host << " lookupId=" << ipLookupId;
        _state.set( ArnZeroConf::State::LookingUp);
        return;
    }

    // Multicast lookup
    DNSServiceErrorType err;
    err = DNSServiceGetAddrInfo(&_sdRef,
                                (forceMulticast ? kDNSServiceFlagsForceMulticast : 0),                                
                                _iface,
                                kDNSServiceProtocol_IPv4,
                                _host.toUtf8().constData(),
                                ArnZeroConfIntern::lookupHostCallback,
                                this);
    if (err != kDNSServiceErr_NoError) {
        _sdRef = 0;
        _state.set( ArnZeroConf::State::Lookup, false);
        emit lookupError( _id, err);
    }
    else {
        _state.set( ArnZeroConf::State::LookingUp);
        _operationTimer->start();

#ifndef MDNS_INTERN
        _notifier = new QSocketNotifier( DNSServiceRefSockFD( _serviceRef), QSocketNotifier::Read, this);
        connect( _notifier, SIGNAL(activated(int)), this, SLOT(socketData()));
#endif
    }
}


void  ArnZeroConfLookup::releaseLookup()
{
    _operationTimer->stop();

    if (_sdRef) {
        DNSServiceRefDeallocate( _sdRef);
        _sdRef = 0;

#ifndef MDNS_INTERN
        _notifier->deleteLater();
        _notifier = 0;
#endif
    }
    _state.set( ArnZeroConf::State::Lookup, false);
}


void  ArnZeroConfLookup::operationTimeout()
{
    releaseLookup();
    emit lookupError( _id, ArnZeroConf::Error::Timeout);
}


void  ArnZeroConfLookup::onIpLookup( const QHostInfo &host)
{
    if (host.error() == QHostInfo::NoError) {
        if (Arn::debugZeroConf)  
            foreach (const QHostAddress &address, host.addresses())
                qDebug() << "Lookup uDNS callback, Found address:" << address.toString();

        _hostAddr = host.addresses().first();
        if (Arn::debugZeroConf)  qDebug() << "Lookup uDNS callback: hostName=" << _host 
                                          << " ip=" << _hostAddr.toString();
        _state.set( ArnZeroConf::State::LookingUp, false);
        _state.set( ArnZeroConf::State::Lookuped);
        emit lookuped( _id);
    }
    else {
        if (Arn::debugZeroConf)  qDebug() << "ZeroConfLookup uDNS failed:" << host.errorString();
        _state.set( ArnZeroConf::State::Lookup, false);
        emit lookupError( _id, ArnZeroConf::Error::UDnsFail);
    }
}


void DNSSD_API  ArnZeroConfIntern::lookupHostCallback( 
                    DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, 
                    DNSServiceErrorType errCode, const char *hostname, const sockaddr *address,
                    uint32_t ttl, void *context)
{
    Q_UNUSED(sdRef);
    Q_UNUSED(flags);
    Q_UNUSED(interfaceIndex);
    Q_UNUSED(hostname);
    Q_UNUSED(ttl);
    ArnZeroConfLookup*  self = reinterpret_cast<ArnZeroConfLookup*>(context);
    Q_ASSERT(self);

    self->_operationTimer->stop();

    if (self->_id < 0)  // No valid id set, get one
        self->_id = ArnZeroConfB::getNextId();

    if (Arn::debugZeroConf)  qDebug() << "Resolve Lookup callback hostName=" << hostname 
                                      << " errCode=" << errCode;
    if (errCode == kDNSServiceErr_NoError) {
        QHostAddress  hostAddr( address);
        self->_hostAddr = hostAddr;
        if (Arn::debugZeroConf)  qDebug() << "Resolve Lookup callback hostName=" << hostname 
                                          << " ip=" << hostAddr.toString();
        self->_state.set( ArnZeroConf::State::LookingUp, false);
        self->_state.set( ArnZeroConf::State::Lookuped);
        emit self->lookuped( self->_id);
    }
    else {
        self->_state.set( ArnZeroConf::State::Lookup, false);
        emit self->lookupError( self->_id, errCode);
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
    return _activeServiceNames.keys();
}


int  ArnZeroConfBrowser::serviceNameToId( const QString& name)
{
    return _activeServiceNames.value( name, -1);
}


bool  ArnZeroConfBrowser::isBrowsing()  const
{
    return _state.is( ArnZeroConf::State::Browsing);
}


void  ArnZeroConfBrowser::setSubType( const QString& subtype)
{
    setSubTypes( subtype.isEmpty() ? QStringList() : QStringList( subtype));
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
    if (state() != ArnZeroConf::State::None)  return;  // Already browsing

    _activeServiceNames.clear();

    QByteArray  serviceTypes = fullServiceType().toUtf8();
    if (!_serviceSubTypes.isEmpty()) {
        serviceTypes += ",_" + escapedName( _serviceSubTypes.at(0).toUtf8());
    }

    DNSServiceErrorType err;
    err = DNSServiceBrowse(&_sdRef,
                           0,
                           _iface,
                           serviceTypes.constData(),
                           domain().toUtf8().constData(),
                           ArnZeroConfIntern::browseServiceCallback,
                           this);
    if (err) {
        emit browseError(err);
    }
    else {
        _state = ArnZeroConf::State::Browsing;
#ifndef MDNS_INTERN
        _notifier = new QSocketNotifier( DNSServiceRefSockFD( _serviceRef), QSocketNotifier::Read, this);
        connect( _notifier, SIGNAL(activated(int)), this, SLOT(socketData()));
#endif
    }
}


void ArnZeroConfBrowser::stopBrowse()
{
    if (state() == ArnZeroConf::State::Browsing) {
        DNSServiceRefDeallocate( _sdRef);
#ifndef MDNS_INTERN
        _notifier->deleteLater();
        _notifier = 0;
#endif
    }
    _state = ArnZeroConf::State::None;
}


void DNSSD_API  ArnZeroConfIntern::browseServiceCallback(
                    DNSServiceRef sdRef, DNSServiceFlags flags, quint32 iface, 
                    DNSServiceErrorType errCode, const char* serviceName, const char* regtype,
                    const char* replyDomain, void* context)
{
    Q_UNUSED(sdRef);
    Q_UNUSED(iface);
    Q_UNUSED(regtype);

    ArnZeroConfBrowser* self = reinterpret_cast<ArnZeroConfBrowser*>(context);
    if (errCode == kDNSServiceErr_NoError) {
        bool  isAdded = (flags & kDNSServiceFlagsAdd) != 0;
        QString  servName  = QString::fromUtf8( serviceName);
        QString  repDomain = QString::fromUtf8( replyDomain);
        if (isAdded != self->_activeServiceNames.contains( servName)) {  // Not multiple add or remove, ok
            int  id = 0;
            if (isAdded) {
                id = ArnZeroConfB::getNextId();
                self->_activeServiceNames.insert( servName, id);
                emit self->serviceAdded( id, servName, repDomain);
            }
            else {
                id = self->_activeServiceNames.value( servName);
                self->_activeServiceNames.remove( servName);
                emit self->serviceRemoved( id, servName, repDomain);
            }
            emit self->serviceChanged( id, isAdded, servName, repDomain);
        }
    }
    else {
        emit self->browseError( errCode);
    }
}
