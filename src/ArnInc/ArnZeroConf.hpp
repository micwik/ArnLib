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

#ifndef ARNZEROCONF_HPP
#define ARNZEROCONF_HPP

#include "ArnLib_global.hpp"
#include "XStringMap.hpp"
#include "MQFlags.hpp"
#include <QHostAddress>
#include <QObject>
#include <QStringList>
#include <QString>
#include <QAbstractSocket>
#include <QAtomicInt>

typedef struct _DNSServiceRef_t *DNSServiceRef;
class QSocketNotifier;
class QTimer;
class QHostInfo;


namespace ArnZeroConf {
//! Errors of ZeroConfig, other values are defined in dns_sd.h
struct Error {
    enum E {
        //! Ok, defined as kDNSServiceErr_NoError in dns_sd.h
        Ok        = 0,
        //! Operation in progress
        Running   = -1,
        //! Bad request sequence
        BadReqSeq = -2,
        //! Operation timeout
        Timeout   = -3,
        //! Unicast DNS lookup fail
        UDnsFail  = -4
    };
    MQ_DECLARE_ENUM( Error)
};

//! States of ZeroConfig, limited valid for each ArnZeroConfB subclass
//// These values must be synced with: ArnDiscover::State
struct State {
    enum E {
        //! Inactive state
        None        = 0x0000,
        //! Registering service in progress
        Registering = 0x0100,
        //! Registering service has finished sucessfully
        Registered  = 0x0001,
        //! isAny(): Registering service in progress or has finished sucessfully
        Register    = 0x0101,
        //! Browsing for service in progress
        Browsing    = 0x0200,
        //! Resolving service in progress
        Resolving   = 0x0400,
        //! Resolving service has finished sucessfully
        Resolved    = 0x0004,
        //! isAny(): Resolving service in progress or has finished sucessfully
        Resolve     = 0x0404,
        //! Lookup host in progress
        LookingUp   = 0x0800,
        //! Lookup host has finished sucessfully
        Lookuped    = 0x0008,
        //! isAny(): Lookup host in progress or has finished sucessfully
        Lookup      = 0x0808,
        //! isAny(): Operation in progress
        InProgress  = 0x0f00
    };
    MQ_DECLARE_FLAGS( State)
};
}  // ArnZeroConf::

MQ_DECLARE_OPERATORS_FOR_FLAGS( ArnZeroConf::State)


//! Base class for Zero Config.
/*!
[About Zero Config](\ref gen_zeroconf)

This class contains methods and data which is usually a superset, i.e. not all data will
be relevant / available for all uses.
*/
class ARNLIBSHARED_EXPORT ArnZeroConfB : public QObject
{
    Q_OBJECT
public:
    ArnZeroConfB( QObject* parent = 0);
    virtual ~ArnZeroConfB();

    //! Returns the socket type for this Zero Config.
    /*! Socket type can be: QAbstractSocket::TcpSocket, QAbstractSocket::UdpSocket,
     *  QAbstractSocket::UnknownSocketType.
     *  Default set by this class is QAbstractSocket::TcpSocket.
     *  QAbstractSocket::UnknownSocketType is only used when socket type can't be determined.
     *  \return current socket type.
     *  \see setSocketType()
     */
    QAbstractSocket::SocketType  socketType()  const;

    //! Sets the socket type for this Zero Config.
    /*! Allowed Socket type is: QAbstractSocket::TcpSocket, QAbstractSocket::UdpSocket.
     *  Default set by this class is QAbstractSocket::TcpSocket.
     *  \param[in] type is one of the allowed types.
     *  \see socketType()
     */
    void  setSocketType( QAbstractSocket::SocketType type);

    //! Returns the service type for this Zero Config
    /*! Service types are standardized by IANA.
     *  \return current service type, e.g. "arn", "ftp" ...
     *  \see setServiceType()
     */
    QString  serviceType()  const;

    //! Returns the service type for this Zero Config
    /*! Service types are standardized by IANA.
     *  The service type used here can be a name, like "arn", or the standard format used
     *  by the Zeroconf specification, e.g. "_arn._tcp".
     *  \param[in] type is the service type (se above).
     *  \see serviceType()
     */
    void  setServiceType( const QString& type);

    //! Returns the domain for this Zero Config.
    /*! Default set by this class is "local.".
     *  \return current domain.
     *  \see setDomain()
     */
    QString  domain()  const;

    //! Sets the domain for this Zero Config.
    /*! Default set by this class is "local.".
     *  \param[in] domain
     *  \see domain()
     */
    void  setDomain( const QString& domain);

    //! Returns the current state of the service
    /*! \retval the state of the service
     */
    ArnZeroConf::State  state()  const;

    //! Returns the full service type for this Zero Config
    /*! Service types are standardized by IANA.
     *  The full service type is the standard format used by the Zeroconf specification,
     *  e.g. "_arn._tcp".
     *  \return current full service type (see above)
     *  \see setServiceType()
     */
    QString  fullServiceType()  const;

    //! \cond ADV
    QByteArray  escapedFullDomain()  const;

protected slots:
    void socketData();
    
protected:
    //! Returns the list of current subtypes
    /*! \retval the subtype list, e.g. ("myGroup1", "myGroup2")
     *  \see setSubTypes()
     *  \see addSubType()
     */
    QStringList  subTypes()  const;

    //! Sets the list of current subtypes
    /*! \param[in] subTypes The new list of subtypes, e.g. ("myGroup1", "myGroup2")
     *  \see subTypes()
     *  \see addSubType()
     *  \see ArnZeroConfBrowser::setSubType()
     */
    void  setSubTypes( const QStringList& subTypes);

    //! Add a subtype to the list of current subtypes
    /*! \param[in] subType the subtype to add, e.g. "myGroup1"
     *  \see subTypes()
     *  \see setSubTypes()
     */
    void  addSubType( const QString& subType);

    //! Returns the port number for connecting to the service
    /*! \retval the port number
     *  \see setPort()
     */
    quint16  port()  const;

    //! Sets the port number for connecting to the service
    /*! When registering a service with a port number of 0, the service will not be found
     *  when browsing, but the service name will be marked as reserved.
     *  \param[in] port the port number
     *  \see port()
     */
    void  setPort( quint16 port);

    //! Returns the service name for this Zero Config
    /*! Service names can be any human readable id. It should be easy to understand, without
     *  any cryptic coding, and can usually be modified by the end user.
     *  \return current service name, e.g. "My House Registry"
     *  \see setServiceName()
     */
    QString  serviceName()  const;

    //! Set the service name for this Zero Config
    /*! Service names can be any human readable id. It should be easy to understand, without
     *  any cryptic coding, and can usually be modified by the end user.
     *  \param[in] name is service name, e.g. "My House Registry"
     *  \see serviceName()
     */
    void  setServiceName( const QString& name);

    //! Returns the host name for this Zero Config
    /*! Usually hostname contain domain, e.g. "myserver.local" but it can also be "myserver",
     *  depending on usage.
     *  \return current host name (se above)
     *  \see setHost()
     */
    QString  host()  const;

    //! Set the host name for this Zero Config
    /*! Usually hostname contain domain, e.g. "myserver.local" but it can also be "myserver",
     *  depending on usage.
     *  \param[in] host is the current host name (se above)
     *  \see host()
     */
    void  setHost( const QString& host);

    //! Load a XStringMap with parameters from the Txt Record
    /*! It is assumed that the Txt Record has already been received.
     *  After loading XStringMap is successfull it contains the parameters from the Txt Record,
     *  e.g. Arn::XStringMap::toXString() can return "protovers=1.0 MyParam=xyz".
     *  \param[out] xsm is the loaded XStringMap if successfull, otherwise undefined.
     *  \retval true if successfull.
     *  \see setTxtRecordMap()
     *  \see Arn::XStringMap
     */
    bool  getTxtRecordMap( Arn::XStringMap& xsm);

    //! Save a XStringMap with parameters to the Txt Record
    /*! The XStringMap contains the parameters to be saved into the Txt Record.
     *  This Txt Record will typically be used later for publishing in zero config.
     *  \param[in] xsm is the XStringMap to be saved into the Txt Record.
     *  \see getTxtRecordMap()
     *  \see Arn::XStringMap
     */
    void  setTxtRecordMap( const Arn::XStringMap& xsm);

    //! Returns the host address for this Zero Config
    /*! This is typically coming from a lookup.
     *  \return current host adress
     *  \see setHostAddr()
     */
    QHostAddress  hostAddr()  const;

    //! Set the host address for this Zero Config
    /*! \param[in] hostAddr
     *  \see hostAddr()
     */
    void  setHostAddr( const QHostAddress &hostAddr);
    
    //! Return the Txt Record for this Zero Config
    /*! It is assumed that the Txt Record has already been received.
     *  The binary format should be the standardized from the Zeroconfig specification.
     *  \return The Txt Record (in binary format)
     *  \see setTxtRecord()
     *  \see getTxtRecordMap()
     */
    QByteArray  txtRecord()  const;

    //! Set the Txt Record for this Zero Config
    /*! The binary format should be the standardized from the Zeroconfig specification.
     *  This Txt Record will typically be used later for publishing in zero config.
     *  \param[in] txt is The Txt Record (in binary format)
     *  \see txtRecord()
     *  \see setTxtRecordMap()
     */
    void  setTxtRecord( const QByteArray& txt);

    //! Return the next id number for zero config objects.
    /*! \return id number
     */
    static int  getNextId();

    void  parseFullDomain( const QByteArray& domainName);

    static QByteArray  escapedName( const QByteArray& name);

    QString  _serviceName;
    QString  _domain;
    QAbstractSocket::SocketType  _socketType;
    QString  _serviceType;
    QString  _host;
    QHostAddress _hostAddr;
    QByteArray  _txtRec;

    ArnZeroConf::State  _state;
    _DNSServiceRef_t*  _sdRef;
    QStringList  _serviceSubTypes;
    int  _iface;
    QString  _txtRecord;

    QSocketNotifier*  _notifier;

    // Source for unique id to all discovered services ...
    static QAtomicInt  _idCount;
    //! \endcond

private:
    quint16  _port;
};


//! Registering a ZeroConfig service.
/*!
This class handles registration of a ZeroConfig service.
The service name can be any string, giving a clear human readable naming of the service.
If the given service name is already in use, it will have a number added to make it unique.
A given TXT record can be registered together with the service.

<b>Example usage</b> \n \code
    // In class declare
    ArnZeroConfRegister*  _advertService;

    // In class code
    _advertService = new ArnZeroConfRegister("My TestService. In the attic", this);
    _advertService->addSubType("server");
    Arn::XStringMap xsmPar;
    xsmPar.add("ver", "1.0").add("server", "1");
    _advertService->setTxtRecordMap( xsmPar);
    connect(_advertService, SIGNAL(registered()), this, SLOT(onRegistered()));
    connect(_advertService, SIGNAL(registrationError(int)),
            this, SLOT(onRegisterError(int)));
    _advertService->registerService();
\endcode
*/
class ARNLIBSHARED_EXPORT ArnZeroConfRegister : public ArnZeroConfB
{
    friend class ArnZeroConfIntern;
    Q_OBJECT
public:
    //! Standard constructor of an ArnZeroConfRegister object
    /*! The service name can be automatically generated based on the system's hostname.
     */
    ArnZeroConfRegister( QObject* parent = 0);

    //! Constructor of an ArnZeroConfRegister object
    /*! All needed parameters for an "arn" service type, using standard arn-port at
     *  this computer.
     *  \param[in] serviceName the human readable naming of the service,
     *                         e.g. "My fantastic service".
     */
    ArnZeroConfRegister( const QString& serviceName, QObject* parent = 0);

    //! Constructor of an ArnZeroConfRegister object
    /*! All needed parameters for a service at this computer.
     *  The service type can be a name or the standard format used by the Zeroconf
     *  specification, e.g. "_arn._tcp".
     *  \param[in] serviceName the human readable naming of the service,
     *                         e.g. "My fantastic service".
     *  \param[in] serviceType the service type, e.g. "arn" or "_arn._tcp".
     *  \param[in] port the service port num
     */
    ArnZeroConfRegister( const QString& serviceName, const QString& serviceType, quint16 port,
                         QObject* parent = 0);

    //! Destructor of an ArnZeroConfRegister object
    /*! If the service is registered, it will be unregistered.
     */
    virtual ~ArnZeroConfRegister();

    //! Returns the list of current subtypes
    /*! \retval the subtype list, e.g. ("myGroup1", "myGroup2")
     *  \see setSubTypes()
     *  \see addSubType()
     */
    QStringList  subTypes()  const
    {return ArnZeroConfB::subTypes();}

    //! Sets the list of current subtypes
    /*! \param[in] subTypes The new list of subtypes, e.g. ("myGroup1", "myGroup2")
     *  \see subTypes()
     *  \see addSubType()
     *  \see ArnZeroConfBrowser::setSubType()
     */
    void  setSubTypes( const QStringList& subtypes)
    {ArnZeroConfB::setSubTypes( subtypes);}

    //! Add a subtype to the list of current subtypes
    /*! \param[in] subType the subtype to add, e.g. "myGroup1"
     *  \see subTypes()
     *  \see setSubTypes()
     */
    void  addSubType( const QString& subtype)
    {ArnZeroConfB::addSubType( subtype);}

    //! Returns the port number for connecting to the service
    /*! \retval the port number
     *  \see setPort()
     */
    quint16  port()  const
    {return ArnZeroConfB::port();}

    //! Sets the port number for connecting to the service
    /*! When registering a service with a port number of 0, the service will not be found
     *  when browsing, but the service name will be marked as reserved.
     *  \param[in] port the port number
     *  \see port()
     */
    void  setPort( quint16 port)
    {ArnZeroConfB::setPort( port);}

    //! Returns the service name for this Zero Config
    /*! Service names can be any human readable id. It should be easy to understand, without
     *  any cryptic coding, and can usually be modified by the end user.
     *  \return current service name, e.g. "My House Registry"
     *  \see setServiceName()
     */
    QString  serviceName()  const
    {return ArnZeroConfB::serviceName();}

    //! Set the service name for this Zero Config
    /*! Service names can be any human readable id. It should be easy to understand, without
     *  any cryptic coding, and can usually be modified by the end user.
     *  \param[in] name is service name, e.g. "My House Registry"
     *  \see serviceName()
     */
    void  setServiceName( const QString& name)
    {ArnZeroConfB::setServiceName( name);}

    //! Returns the host name for this Zero Config
    /*! Usually hostname is empty, automatically using the computers name, but it can
     *  also be like "myserver".
     *  \return current host name (se above)
     *  \see setHost()
     */
    QString  host()  const
    {return ArnZeroConfB::host();}

    //! Set the host name for this Zero Config
    /*! Usually hostname is empty, automatically using the computers name, but it can
     *  also be like "myserver".
     *  \param[in] host is the current host name (se above)
     *  \see host()
     */
    void  setHost( const QString& host)
    {ArnZeroConfB::setHost( host);}

    //! Load a XStringMap with parameters from the Txt Record
    /*! It is assumed that the Txt Record has already been received.
     *  After loading XStringMap is successfull it contains the parameters from the Txt Record,
     *  e.g. Arn::XStringMap::toXString() can return "protovers=1.0 MyParam=xyz".
     *  \param[out] xsm is the loaded XStringMap if successfull, otherwise undefined.
     *  \retval true if successfull.
     *  \see setTxtRecordMap()
     *  \see Arn::XStringMap
     */
    bool  getTxtRecordMap( Arn::XStringMap& xsm)
    {return ArnZeroConfB::getTxtRecordMap( xsm);}

    //! Save a XStringMap with parameters to the Txt Record
    /*! The XStringMap contains the parameters to be saved into the Txt Record.
     *  This Txt Record will typically be used later for publishing in zero config.
     *  \param[in] xsm is the XStringMap to be saved into the Txt Record.
     *  \see getTxtRecordMap()
     *  \see Arn::XStringMap
     */
    void  setTxtRecordMap( const Arn::XStringMap& xsm)
    {ArnZeroConfB::setTxtRecordMap( xsm);}

    //! Return the Txt Record for this Zero Config
    /*! It is assumed that the Txt Record has already been received.
     *  The binary format should be the standardized from the Zeroconfig specification.
     *  \return The Txt Record (in binary format)
     *  \see setTxtRecord()
     *  \see getTxtRecordMap()
     */
    QByteArray  txtRecord()  const
    {return ArnZeroConfB::txtRecord();}

    //! Set the Txt Record for this Zero Config
    /*! The binary format should be the standardized from the Zeroconfig specification.
     *  This Txt Record will typically be used later for publishing in zero config.
     *  \param[in] txt is The Txt Record (in binary format)
     *  \see txtRecord()
     *  \see setTxtRecordMap()
     */
    void  setTxtRecord( const QByteArray& txt)
    {ArnZeroConfB::setTxtRecord( txt);}

    //! Register the service
    /*! Tries to register the service on the local network.
     *  Result is indicated by registered() and registrationError() signals.
     *  \param[in] noAutoRename when true, registration will fail if another service
     *                          with the same service type already is registered with
     *                          the same service name.
     *  \see registered()
     *  \see registrationError()
     */
    void  registerService( bool noAutoRename = false);

    //! Release the service
    /*! If the service is registered, it will be unregistered. Any registration attempts
     *  in progress will be aborted.
     */
    void  releaseService();

signals:
    //! Indicate successfull registration of service
    /*! \see registerService()
     */
    void  registered( QString serviceName);

    //! Indicate unsuccessfull registration of service
    /*! \param[in] code error code.
     *  \see registerService()
     */
    void  registrationError( int code);

private:
    void  init();
};


//! Resolv a ZeroConfig service.
/*!
This class handles resolving of a ZeroConfig service.
The service name can be given directly if known, but typically it comes from ArnZeroConfBrowser.

<b>Example usage</b> \n \code
    // In class code
    ArnZeroConfResolv*  ds = new ArnZeroConfResolv("My TestService. In the attic", this);
    connect( ds, SIGNAL(resolveError(int,int)), this, SLOT(onResolveError(int,int)));
    connect( ds, SIGNAL(resolved(int,QByteArray)), this, SLOT(onResolved(int,QByteArray)));
    ds->resolve();

void XXX::onResolved( int id, QByteArray escFullDomain)
{
    ArnZeroConfResolv*  ds = qobject_cast<ArnZeroConfResolv*>( sender());
    Arn::XStringMap  xsmPar;
    ds->getTxtRecordMap( xsmPar);
    QString  info = QString()
                  +  " Domain=" + ds->domain()
                  +  " Host="   + ds->host()
                  +  " Port="   + QString::number( ds->port())
                  +  " Txt: "   + QString::fromUtf8( xsmPar.toXString().constData());
    QString  ver = xsmPar.valueString("ver");
    ds->releaseService();
    ds->deleteLater();
}
\endcode
*/
class ARNLIBSHARED_EXPORT ArnZeroConfResolve : public ArnZeroConfB
{
    friend class ArnZeroConfIntern;
    Q_OBJECT
public:
    //! Standard constructor of an ArnZeroConfResolv object
    /*!
     */
    ArnZeroConfResolve( QObject* parent = 0);

    //! Constructor of an ArnZeroConfResolv object
    /*! All needed parameters for an "arn" service type.
     *  \param[in] serviceName the human readable naming of the service,
     *                         e.g. "My fantastic service".
     */
    ArnZeroConfResolve( const QString& serviceName, QObject* parent = 0);

    //! Constructor of an ArnZeroConfResolv object
    /*! All needed parameters for a service.
     *  The service type can be a name or the standard format used by the Zeroconf
     *  specification, e.g. "_arn._tcp".
     *  \param[in] serviceName the human readable naming of the service,
     *                         e.g. "My fantastic service".
     *  \param[in] serviceType the service type, e.g. "arn" or "_arn._tcp".
     */
    ArnZeroConfResolve( const QString& serviceName, const QString& serviceType, QObject* parent = 0);

    //! Destructor of an ArnZeroConfResolv object
    /*! If the service is registered, it will be unregistered.
     */
    virtual ~ArnZeroConfResolve();

    //! Returns the id number for this resolv
    /*! \return the id number
     *  \see setId()
     */
    int id() const;

    //! Sets the id number for this this resolv
    /*! This id can be used to identify different resolves when using a common handler.
     *  When not set, it will be automatically assigned during resolve().
     *  \param[in] id the id number
     *  \see id()
     */
    void setId(int id);

    //! Returns the host name for this resolv
    /*! Hostname contain domain, e.g. "myserver.local".
     *  \return current host name (se above)
     */
    QString  host()  const
    {return ArnZeroConfB::host();}

    //! Returns the port number for connecting to the service
    /*! \retval the port number
     */
    quint16  port()  const
    {return ArnZeroConfB::port();}

    //! Returns the service name used for this resolv
    /*! Service names can be any human readable id.
     *  \return current service name, e.g. "My House Registry"
     */
    QString  serviceName()  const
    {return ArnZeroConfB::serviceName();}

    //! Set the service name used for this resolv
    /*! Service names can be any human readable id. It will be used when reolving
     *  the service.
     *  \param[in] name is service name, e.g. "My House Registry"
     *  \see serviceName()
     */
    void  setServiceName( const QString& name)
    {ArnZeroConfB::setServiceName( name);}

    //! Load a XStringMap with parameters from the Txt Record
    /*! It is assumed that the Txt Record has already been received.
     *  After loading XStringMap is successfull it contains the parameters from the Txt Record,
     *  e.g. Arn::XStringMap::toXString() can return "protovers=1.0 MyParam=xyz".
     *  \param[out] xsm is the loaded XStringMap if successfull, otherwise undefined.
     *  \retval true if successfull.
     *  \see Arn::XStringMap
     */
    bool  getTxtRecordMap( Arn::XStringMap& xsm)
    {return ArnZeroConfB::getTxtRecordMap( xsm);}

    //! Return the Txt Record for this Zero Config
    /*! It is assumed that the Txt Record has already been received.
     *  The binary format should be the standardized from the Zeroconfig specification.
     *  \return The Txt Record (in binary format)
     *  \see getTxtRecordMap()
     */
    QByteArray  txtRecord()  const
    {return ArnZeroConfB::txtRecord();}

    //! Resolve the service
    /*! Tries to resolve the service to determine the host and port necessary to establish
     *  a connection.
     *  Result is indicated by resolved() and resolveError() signals.
     *  \param[in] forceMulticast when true, ArnZeroConfResolv will use a multicast request
     *                            to resolve the service, even if the host name is a unicast
     *                            address, i.e. outside the local network.
     *  \see resolved()
     *  \see resolveError()
     */
    void  resolve( bool forceMulticast = false);

    //! Release the resolving
    /*! Any resolve attempts in progress will be aborted.
     */
    void  releaseResolve();
    
signals:
    //! Indicate successfull resolve of service
    /*! \param[in] id is the id number for this resolve
     *  \param[in] escFullDomain is the raw full domain with esc sequences
     *  \see resolve()
     */
    void  resolved( int id, const QByteArray& escFullDomain);

    //! Indicate unsuccessfull resolve of service
    /*! \param[in] id is the id number for this resolve
     *  \param[in] code is the error code.
     *  \see resolve()
     */
    void  resolveError( int id, int code);

private slots:
    void  operationTimeout();

private:
    void  init();

    int  _id;
    QTimer*  _operationTimer;
};


//! Lookup a host.
/*!
This class handles lookup of a host. It can be booth Multicast and Unicast DNS lookup.

<b>Example usage</b> \n \code
\endcode
*/
class ARNLIBSHARED_EXPORT ArnZeroConfLookup : public ArnZeroConfB
{
    friend class ArnZeroConfIntern;
    Q_OBJECT
public:
    //! Standard constructor of an ArnZeroConfLookup object
    ArnZeroConfLookup( QObject* parent = 0);

    //! Constructor of an ArnZeroConfLookup object
    /*! All needed parameters for a lookup of a host.
     *  \param[in] hostName the name of the host.
     */
    ArnZeroConfLookup( const QString& hostName, QObject* parent = 0);

    //! Destructor of an ArnZeroConfLookup object
    /*! If the lookup is ongoing, it will be released.
     */
    virtual ~ArnZeroConfLookup();

    //! Returns the id number for this lookup
    /*! \retval the id number
     *  \see setId()
     */
    int id() const;

    //! Sets the id number for this this lookup
    /*! This id can be used to identify different lookup:s when using a common handler.
     *  When not set, it will be automatically asigned during lookup().
     *  \param[in] id the id number
     *  \see id()
     */
    void setId(int id);

    //! Returns the host name for this Lookup
    /*! Usually hostname contain domain, e.g. "myserver.local" but it can also be "myserver".
     *  \return current host name (se above)
     *  \see setHost()
     */
    QString  host()  const
    {return ArnZeroConfB::host();}

    //! Set the host name for this Lookup
    /*! Usually hostname contain domain, e.g. "myserver.local" but it can also be "myserver".
     *  \param[in] host is the current host name (se above)
     *  \see host()
     */
    void  setHost( const QString& host)
    {ArnZeroConfB::setHost( host);}

    //! Returns the host address for this Lookup
    /*! \return current host adress
     */
    QHostAddress  hostAddr()  const
    {return ArnZeroConfB::hostAddr();}

    //! Lookup the host address
    /*! Tries to lookup the host address necessary to establish a connection.
     *  Result is indicated by lookuped() and lookupError() signals.
     *  \param[in] forceMulticast when true, ArnZeroConfLookup will use a mDns request
     *                            to lookup the host address, even if the host name is a
     *                            unicast address, i.e. outside the local network.
     *  \see lookuped()
     *  \see lookupError()
     */
    void  lookup( bool forceMulticast = false);
    
    //! Release the lookup
    /*! Any lookup attempts in progress will be aborted.
     */
    void  releaseLookup();

    //! Return Force using Qt for DNS lookup
    /*! \retval true if Force using Qt for DNS lookup
     *  \see setForceQtDnsLookup()
     */
    static bool  isForceQtDnsLookup();

    //! Set Force using Qt for DNS lookup
    /*! If mDns lookup doesn't work for a platform, try force using Qt:s built
     *  in DNS-lookup. This is a global setting for all instances of ArnZeroConfLookup.
     *  \param[in] isForceQtDnsLookup
     *  \see isForceQtDnsLookup()
     */
    static void  setForceQtDnsLookup( bool isForceQtDnsLookup);

signals:
    //! Indicate successfull lookup of host
    /*! \param[in] id is the id number for this lookup
     *  \see lookup()
     */
    void  lookuped( int id);

    //! Indicate unsuccessfull lookup of host
    /*! \param[in] id is the id number for this lookup
     *  \param[in] code error code.
     *  \see lookup()
     */
    void  lookupError( int id, int code);

private slots:
    void  operationTimeout();
    void  onIpLookup( const QHostInfo& host);

private:
    void  init();

    int  _id;
    QTimer*  _operationTimer;

    static bool  _isForceQtDnsLookup;
};


//! Browsing for ZeroConfig services.
/*!
This class handles browsing of ZeroConfig services.

<b>Example usage</b> \n \code
    // In class declare
    ArnZeroConfBrowser*  _serviceBrowser;
    QMap<QString,QString>  _activeServices;

    // In class code
    _serviceBrowser = new ArnZeroConfBrowser( this);
    connect(_serviceBrowser, SIGNAL(browseError(int)),
            this, SLOT(onBrowseError(int)));
    connect(_serviceBrowser, SIGNAL(serviceAdded(int,QString,QString)),
            this, SLOT(onServiceAdded(int,QString,QString)));
    connect(_serviceBrowser, SIGNAL(serviceRemoved(int,QString,QString)),
            this, SLOT(onServiceRemoved(int,QString,QString)));
    _serviceBrowser->browse();

void  XXX::onServiceAdded( int id, QString name, QString domain)
{
    _activeServices.insert( name, "Some associeated service info ...");
    ArnZeroConfResolv*  ds = new ArnZeroConfResolv( name, this);
    connect( ds, SIGNAL(resolveError(int,int)), this, SLOT(onResolveError(int,int)));
    connect( ds, SIGNAL(resolved(int,QByteArray)), this, SLOT(onResolved(int,QByteArray)));
    ds->resolve();
}

void  XXX::onServiceRemoved( int id, QString name, QString domain)
{
    _activeServices.remove( name);
}
\endcode
*/
class  ARNLIBSHARED_EXPORT ArnZeroConfBrowser : public ArnZeroConfB
{
    friend class ArnZeroConfIntern;
    Q_OBJECT
public:
    //! Standard constructor of an ArnZeroConfBrowser object
    /*! All needed for browsing an "arn" service type.
     */
    ArnZeroConfBrowser( QObject* parent = 0);

    //! Constructor of an ArnZeroConfBrowser object
    /*! All needed parameters for browsing a service.
     *  The service type can be a name or the standard format used by the
     *  Zeroconf specification, e.g. "_arn._tcp".
     *  \param[in] serviceType the service type, e.g. "arn" or "_arn._tcp".
     */
    ArnZeroConfBrowser( const QString& serviceType, QObject* parent = 0);

    //! Destructor of an ArnZeroConfBrowser object
    /*! If browsing is active, it will be stopped.
     */
    virtual ~ArnZeroConfBrowser();

    //! Set subtype (filter)
    /*! If passing empy subtype, this is taken as subtype (filter) disabled.
     *  When subtype (filter) is enabled, only services that have the same subtype
     *  is discovered.
     *  \param[in] subtype the filter, e.g. "myGroup1"
     *  \see subType()
     *  \see browse()
     *  \see ArnZeroConfRegister::setSubTypes()
     */
    void  setSubType( const QString& subtype);

    //! Return current subtype (filter)
    /*! Empy subtype, is taken as subtype (filter) disabled.
     *  \return subtype, e.g. "myGroup1"
     *  \see setSubType()
     */
    QString  subType();

    //! Return current list of active service names
    /*! \retval the active service names
     *  \see serviceAdded()
     */
    QStringList  activeServiceNames()  const;

    //! Return the id for a service by its service name
    /*! \param[in] name the service name, e.g. "My House Registry"
     *  \return the id for the service
     *  \see serviceAdded()
     */
    int  serviceNameToId( const QString& name);

    //! Return the status of the browsing
    /*! \retval true if browsing is started
     *  \see browse()
     */
    bool  isBrowsing()  const;

    //! Return the next id number for zero config objects.
    /*! \return id number
     */
    static int  getNextId()
    {return ArnZeroConfB::getNextId();}

public slots:
    //! Change state of browsing
    /*! When browsing is started, services will be discovered.
     *  \param[in] enable if true browsing is started, otherwise it is stopped
     *  \see stopBrowse()
     */
    void  browse( bool enable = true);

    //! Stop browsing
    /*! \see browse()
     */
    void  stopBrowse();

signals:
    //! Indicate service has been added / removed
    /*! _id_ will not be reused for any other service, it is unique within this program.
     *  \param[in] isAdded is true when service has been added, otherwise false
     *  \param[in] id is the id number for the service
     *  \param[in] serviceName e.g. "My House Registry"
     *  \param[in] domain  e.g. "local."
     *  \see serviceAdded()
     *  \see serviceRemoved()
     *  \see browse()
     */
    void  serviceChanged( bool isAdded, int id, const QString& serviceName, const QString& domain);

    //! Indicate service has been added (discovered)
    /*! _id_ will not be reused for any other service, it is unique within this program.
     *  \param[in] id is the id number for the service
     *  \param[in] serviceName e.g. "My House Registry"
     *  \param[in] domain  e.g. "local."
     *  \see serviceRemoved()
     *  \see serviceChanged()
     */
    void  serviceAdded( int id, const QString& serviceName, const QString& domain);

    //! Indicate service has been removed
    /*! \param[in] id is the id number for the service
     *  \param[in] serviceName e.g. "My House Registry"
     *  \param[in] domain  e.g. "local."
     *  \see serviceAdded()
     *  \see serviceChanged()
     */
    void  serviceRemoved( int id, const QString& serviceName, const QString& domain);

    //! Indicate unsuccessfull browsing
    /*! \param[in] code error code.
     *  \see browse()
     */
    void  browseError( int errorCode);

private:
    void  init();

    QMap<QString,int>  _activeServiceNames;
};

#endif // ARNZEROCONF_HPP
