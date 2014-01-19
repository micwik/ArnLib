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

#ifndef ARNZEROCONF_HPP
#define ARNZEROCONF_HPP

#include "ArnLib_global.hpp"
#include "XStringMap.hpp"
#include "MQFlags.hpp"
#include <QObject>
#include <QStringList>
#include <QString>
#include <QAbstractSocket>
#include <QAtomicInt>

typedef struct _DNSServiceRef_t *DNSServiceRef;
class QSocketNotifier;


class ARNLIBSHARED_EXPORT ArnZeroConfB : public QObject
{
    Q_OBJECT
public:
    //! States of ZeroConfig, limited valid for each subclass
    struct State {
        enum E {
            //! Inactive state
            None,
            //! Registering service in progress
            Registering,
            //! Registering service has finished sucessfully
            Registered,
            //! Resolving service in progress
            Resolving,
            //! Resolving service has finished sucessfully
            Resolved,
            //! Browsing for service in progress
            Browsing
        };
        MQ_DECLARE_ENUM( State)
    };

    ArnZeroConfB( QObject* parent = 0);
    virtual ~ArnZeroConfB();

    QAbstractSocket::SocketType  socketType()  const;
    void  setSocketType( QAbstractSocket::SocketType type);
    QString  serviceType()  const;
    void  setServiceType( const QString& type);
    QString  domain()  const;
    void  setDomain( const QString& domain);

    //! Returns the current state of the service
    /*! \retval the state of the service
     */
    State  state()  const;

    QString  fullServiceType()  const;
    QByteArray  escapedFullDomain()  const;

protected slots:
    void socketData();

protected:
    //! Returns the list of current subtypes
    /*! \retval the subtype list
     *  \see setSubTypes()
     *  \see addSubType()
     */
    QStringList  subTypes()  const;

    //! Sets the list of current subtypes
    /*! \param[in] subTypes The new list of subtypes.
     *  \see subTypes()
     *  \see addSubType()
     */
    void  setSubTypes( const QStringList& subTypes);

    //! Add a subtype to the list of current subtypes
    /*! \param[in] subType the subtype to add.
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
    /*! When registering a service with a port number of 0, the service will not be found when browsing,
     *  but the service name will be marked as reserved.
     *  \param[in] port the port number
     *  \see port()
     */
    void  setPort( quint16 port);

    QString  serviceName()  const;
    void  setServiceName( const QString& name);
    QString  host()  const;
    void  setHost( const QString& host);
    QByteArray  txtRecord()  const;
    void  setTxtRecord( const QByteArray& txt);
    bool  getTxtRecordMap( XStringMap& xsm);
    void  setTxtRecordMap( const XStringMap& xsm);

    void  parseFullDomain( const QByteArray& domainName);
    static int  getNextId();

    static QByteArray  escapedName( const QByteArray& name);

    QString  _serviceName;
    QString  _domain;
    QAbstractSocket::SocketType  _socketType;
    QString  _serviceType;
    QString  _host;
    QByteArray  _txtRec;

    State  _state;
    _DNSServiceRef_t*  _serviceRef;
    QStringList  _serviceSubTypes;
    int  _iface;
    QString  _txtRecord;

    QSocketNotifier*  _notifier;

    // Source for unique id to all discovered services ...
    static QAtomicInt  _idCount;

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
    XStringMap xsmPar;
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
    /*! All needed parameters for an "arn" service type, using standard arn-port at this computer.
     *  \param[in] serviceName the human readable naming of the service, e.g. "My fantastic service".
     */
    ArnZeroConfRegister( const QString& serviceName, QObject* parent = 0);

    //! Constructor of an ArnZeroConfRegister object
    /*! All needed parameters for a service at this computer.
     *  The service type can be a name or the standard format used by the Zeroconf specification, e.g. "_arn._tcp".
     *  \param[in] serviceName the human readable naming of the service, e.g. "My fantastic service".
     *  \param[in] serviceType the service type, e.g. "arn" or "_arn._tcp".
     *  \param[in] port the service port num
     */
    ArnZeroConfRegister( const QString& serviceName, const QString& serviceType, quint16 port,
                         QObject* parent = 0);

    //! Destructor of an ArnZeroConfRegister object
    /*! If the service is registered, it will be unregistered.
     */
    virtual ~ArnZeroConfRegister();

    QStringList  subTypes()  const
    {return ArnZeroConfB::subTypes();}

    void  setSubTypes( const QStringList& subtypes)
    {ArnZeroConfB::setSubTypes( subtypes);}

    void  addSubType( const QString& subtype)
    {ArnZeroConfB::addSubType( subtype);}

    quint16  port()  const
    {return ArnZeroConfB::port();}

    void  setPort( quint16 port)
    {ArnZeroConfB::setPort( port);}

    QString  serviceName()  const
    {return ArnZeroConfB::serviceName();}

    void  setServiceName( const QString& name)
    {ArnZeroConfB::setServiceName( name);}

    QString  host()  const
    {return ArnZeroConfB::host();}

    void  setHost( const QString& host)
    {ArnZeroConfB::setHost( host);}

    QByteArray  txtRecord()  const
    {return ArnZeroConfB::txtRecord();}

    void  setTxtRecord( const QByteArray& txt)
    {ArnZeroConfB::setTxtRecord( txt);}

    bool  getTxtRecordMap( XStringMap& xsm)
    {return ArnZeroConfB::getTxtRecordMap( xsm);}

    void  setTxtRecordMap( const XStringMap& xsm)
    {ArnZeroConfB::setTxtRecordMap( xsm);}

    //! Register the service
    /*! Tries to register the service on the local network.
     *  Result is indicated by registered() and registrationError() signals.
     *  \param[in] noAutoRename when true, registration will fail if another service with the same service type
     *                          already is registered with the same service name.
     * \see registered()
     * \see registrationError()
     */
    void  registerService( bool noAutoRename = false);

    //! Release the service
    /*! If the service is registered, it will be unregistered. Any registration attempts in progress will be aborted.
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
    connect( ds, SIGNAL(resolveError(int)), this, SLOT(onResolveError(int)));
    connect( ds, SIGNAL(resolved(QByteArray)), this, SLOT(onResolved(QByteArray)));
    ds->resolve();

void XXX::onResolved( QByteArray escFullDomain)
{
    ArnZeroConfResolv*  ds = qobject_cast<ArnZeroConfResolv*>( sender());
    XStringMap  xsmPar;
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
class ARNLIBSHARED_EXPORT ArnZeroConfResolv : public ArnZeroConfB
{
    friend class ArnZeroConfIntern;
    Q_OBJECT
public:
    //! Standard constructor of an ArnZeroConfResolv object
    /*!
     */
    ArnZeroConfResolv( QObject* parent = 0);

    //! Constructor of an ArnZeroConfResolv object
    /*! All needed parameters for an "arn" service type.
     *  \param[in] serviceName the human readable naming of the service, e.g. "My fantastic service".
     */
    ArnZeroConfResolv( const QString& serviceName, QObject* parent = 0);

    //! Constructor of an ArnZeroConfResolv object
    /*! All needed parameters for a service.
     *  The service type can be a name or the standard format used by the Zeroconf specification, e.g. "_arn._tcp".
     *  \param[in] serviceName the human readable naming of the service, e.g. "My fantastic service".
     *  \param[in] serviceType the service type, e.g. "arn" or "_arn._tcp".
     */
    ArnZeroConfResolv( const QString& serviceName, const QString& serviceType, QObject* parent = 0);

    //! Destructor of an ArnZeroConfResolv object
    /*! If the service is registered, it will be unregistered.
     */
    virtual ~ArnZeroConfResolv();

    //! Returns the id number for this resolv
    /*! \retval the id number
     *  \see setId()
     */
    int id() const;

    //! Sets the id number for this this resolv
    /*! This id can be used to identify different resolves when using a common handler.
     *  When not set, the default will allways be -1.
     *  \param[in] id the id number
     *  \see id()
     */
    void setId(int id);

    QString  host()  const
    {return ArnZeroConfB::host();}

    quint16  port()  const
    {return ArnZeroConfB::port();}

    QString  serviceName()  const
    {return ArnZeroConfB::serviceName();}

    void  setServiceName( const QString& name)
    {ArnZeroConfB::setServiceName( name);}

    QByteArray  txtRecord()  const
    {return ArnZeroConfB::txtRecord();}

    bool  getTxtRecordMap( XStringMap& xsm)
    {return ArnZeroConfB::getTxtRecordMap( xsm);}

    //! Resolve the service
    /*! Tries to resolve the service to determine the host and port necessary to establish a connection.
     *  Result is indicated by resolved() and resolveError() signals.
     *  \param[in] forceMulticast when true, ArnZeroConfResolv will use a multicast request to resolve the service,
     *                            even if the host name is a unicast address, i.e. outside the local network.
     * \see resolved()
     * \see resolveError()
     */
    void  resolve( bool forceMulticast = false);

    //! Release the service
    /*! If the service is registered, it will be unregistered. Any resolve attempts in progress will be aborted.
     */
    void  releaseService();

signals:
    //! Indicate successfull resolve of service
    /*! \see resolve()
     */
    void  resolved( int id, const QByteArray& escFullDomain);

    //! Indicate unsuccessfull resolve of service
    /*! \param[in] code error code.
     *  \see resolve()
     */
    void  resolveError( int code);

private:
    void  init();

    int  _id;
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
    connect( ds, SIGNAL(resolveError(int)), this, SLOT(onResolveError(int)));
    connect( ds, SIGNAL(resolved(QByteArray)), this, SLOT(onResolved(QByteArray)));
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
     *  The service type can be a name or the standard format used by the Zeroconf specification, e.g. "_arn._tcp".
     *  \param[in] serviceType the service type, e.g. "arn" or "_arn._tcp".
     */
    ArnZeroConfBrowser( const QString& serviceType, QObject* parent = 0);

    //! Destructor of an ArnZeroConfBrowser object
    /*! If browsing is active, it will be stopped.
     */
    virtual ~ArnZeroConfBrowser();

    //! Set subtype (filter)
    /*! If passing empy subtype, this is taken as subtype (filter) disabled
     *  \param[in] subtype the filter
     */
    void  setSubType( const QString& subtype);

    QString  subType();

    QStringList  activeServiceNames()  const;
    int  serviceNameToId( const QString& name);
    bool  isBrowsing()  const;

    static int  getNextId()
    { return ArnZeroConfB::getNextId();}

public slots:
    void  browse( bool enable = true);
    void  stopBrowse();

signals:
    void  browseError( int errorCode);
    void  serviceChanged( bool isAdded, int id, const QString& serviceName, const QString& domain);
    void  serviceAdded( int id, const QString& serviceName, const QString& domain);
    void  serviceRemoved( int id, const QString& serviceName, const QString& domain);

private:
    void  init();

    QMap<QString,int>  _activeServiceNames;
};

#endif // ARNZEROCONF_HPP
