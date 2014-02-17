#include "ArnInc/ArnDiscoverRemote.hpp"
#include "ArnInc/ArnZeroConf.hpp"
#include "ArnInc/ArnClient.hpp"
#include "ArnInc/ArnServer.hpp"
#include <QTimer>
#include <QTime>
#include <QMetaObject>
#include <QHostInfo>


///////// ArnDiscoverConnector

ArnDiscoverConnector::ArnDiscoverConnector( ArnClient& client, const QString& id) :
    QObject( &client)
{
    _client = &client;
    _id     = id;
    _discoverHostPrio      = 1;
    _directHostPrio        = 2;
    _resolveRefreshTimeout = 60;
    _directHosts           = new QObject( this);
    _directHosts->setObjectName("dirHosts");
    _resolveRefreshBlocked = false;
    _isResolved            = false;
    _resolveRefreshTime    = 0;
    _resolver              = 0;
    _arnDisHostService     = 0;
    _arnDisHostServicePv   = 0;
    _arnDisHostAddress     = 0;
    _arnDisHostPort        = 0;
}


void  ArnDiscoverConnector::clearDirectHosts()
{
    _client->clearArnList( _directHostPrio);
}


void  ArnDiscoverConnector::addToDirectHosts( const QString& arnHost, quint16 port)
{
    _client->addToArnList( arnHost, port, _directHostPrio);
}


void  ArnDiscoverConnector::setResolver( ArnDiscoverResolver* resolver)
{
    Q_ASSERT( resolver);
    if (!resolver)  return;

    if (!_resolveRefreshTime)  // first time
        _resolveRefreshTime = new QTime;
    _resolver = resolver;

    QMetaObject::invokeMethod( this,
                               "postSetupResolver",
                               Qt::QueuedConnection);
}


QString ArnDiscoverConnector::id() const
{
    return _id;
}


int  ArnDiscoverConnector::resolveRefreshTimeout()  const
{
    return _resolveRefreshTimeout;
}


void  ArnDiscoverConnector::setResolveRefreshTimeout( int resolveRefreshTimeout)
{
    _resolveRefreshTimeout = resolveRefreshTimeout;
}


int  ArnDiscoverConnector::resolvHostPrio()  const
{
    return _discoverHostPrio;
}


void  ArnDiscoverConnector::setResolvHostPrio( int resolvHostPrio)
{
    _discoverHostPrio = resolvHostPrio;
}


int  ArnDiscoverConnector::directHostPrio()  const
{
    return _directHostPrio;
}


void  ArnDiscoverConnector::setDirectHostPrio( int directHostPrio)
{
    _directHostPrio = directHostPrio;
}


void  ArnDiscoverConnector::start()
{
    connect( _client, SIGNAL(tcpConnected(QString,quint16)), this, SLOT(doClientConnected(QString,quint16)));

    QMetaObject::invokeMethod( this,
                               "postSetupClient",
                               Qt::QueuedConnection);
}


void  ArnDiscoverConnector::doClientConnectChanged( int stat, int curPrio)
{
    ArnClient::ConnectStat  cs = ArnClient::ConnectStat::fromInt( stat);

    qDebug() << "ArnDiscoverConnector changed 1: stat=" << stat << " prio=" << curPrio;
    if (!_resolver || (cs == cs.Connecting))  return;
    if (cs == cs.Connected) {
        _resolveRefreshBlocked = false;
        return;
    }
    if (_isResolved || (cs != cs.TriedAll)) {  // Resolv ok or still more to try, consider outdated resolv
        if (curPrio != _discoverHostPrio)  return;  // Not for resolved host
        if ((cs != cs.Error) && (cs != cs.Disconnected))  return;  // Skip any non error
        qDebug() << "ArnDiscoverConnector changed 2:";

        if (_resolveRefreshTime->elapsed() >= _resolveRefreshTimeout * 1000)
            _resolveRefreshBlocked = false;
        if (_resolveRefreshBlocked)  return;
    }

    _resolveRefreshBlocked = true;  // Block for further refresh within lockout time
    _resolveRefreshTime->start();

    qDebug() << "ArnDiscoverConnector changed 3 resolve: service=" << _arnDisHostService->toString();
    bool  forceUpdate = _isResolved;
    _resolver->resolve( _arnDisHostService->toString(), forceUpdate);  // Do a resolve refresh / retry
}


void  ArnDiscoverConnector::postSetupClient()
{
    QString  path;
    QString  connectIdPath = Arn::pathDiscoverConnect + _id + "/";

    path = connectIdPath + "Status/";
    ArnItem*  arnConnectStatus = new ArnItem( path + "value", this);
    *arnConnectStatus = _client->connectStatus();
    connect( _client, SIGNAL(connectionStatusChanged(int,int)), arnConnectStatus, SLOT(setValue(int)));
    typedef ArnClient::ConnectStat CS;
    ArnM::setValue( path + "set",
                    QString("%1=Initialized %2=Connecting %3=Connected %4=Connect_error %5=Disconnected")
                    .arg(CS::Init).arg(CS::Connecting).arg(CS::Connected).arg(CS::Error).arg(CS::Disconnected));

    path = connectIdPath + "Request/";
    ArnItem*  arnConnectReqPV = new ArnItem( path + "value!", this);
    *arnConnectReqPV = "0";
    connect( arnConnectReqPV, SIGNAL(changed(int)), this, SLOT(doClientConnectRequest(int)));
    ArnM::setValue( path + "set", QString("0=Idle 1=Start_connect"));

    ArnClient::HostList  arnHosts = _client->arnList( _directHostPrio);
    int  i = 0;
    foreach (ArnClient::HostAddrPort host, arnHosts) {
        ++i;
        path = connectIdPath + "DirectHosts/Host-" + QString::number(i) + "/";
        ArnItem*  hostAddr = new ArnItem( path + "value", _directHosts);
        ArnItem*  hostPort = new ArnItem( path + "Port/value", _directHosts);
        *hostAddr = host.addr;  // Default addr
        *hostPort = host.port;  // Default port
        hostAddr->addMode( ArnItem::Mode::Save);  // Save mode after default set, will not save default value
        hostPort->addMode( ArnItem::Mode::Save);
        connect( hostAddr, SIGNAL(changed()), this, SLOT(doClientDirHostChanged()));
        connect( hostPort, SIGNAL(changed()), this, SLOT(doClientDirHostChanged()));
    }
    doClientDirHostChanged();  // Any loaded persistent values will be used

    connect( _client, SIGNAL(connectionStatusChanged(int,int)), this, SLOT(doClientConnectChanged(int,int)));
    if (!_resolver)
        emit clientReadyToConnect( _client);
}


void  ArnDiscoverConnector::doClientConnected( QString arnHost, quint16 port)
{
    QString  path = Arn::pathDiscoverConnect + _id + "/UsingHost/";
    ArnM::setValue( path + "value", arnHost);
    ArnM::setValue( path + "Port/value", port);
}


void  ArnDiscoverConnector::doClientDirHostChanged()
{
    QObjectList  dirHostOList = _directHosts->children();
    int  dirHostOListSize = dirHostOList.size();
    Q_ASSERT((dirHostOListSize & 1) == 0);

    //// Rebuild ArnList in client
    _client->clearArnList( _directHostPrio);
    for (int i = 0; i < dirHostOListSize / 2; ++i) {
        ArnItem*  hostAddr = qobject_cast<ArnItem*>( dirHostOList.at( 2 * i + 0));
        Q_ASSERT(hostAddr);
        ArnItem*  hostPort = qobject_cast<ArnItem*>( dirHostOList.at( 2 * i + 1));
        Q_ASSERT(hostPort);
        _client->addToArnList( hostAddr->toString(), quint16( hostPort->toInt()), _directHostPrio);
    }
}


void ArnDiscoverConnector::doClientConnectRequest(int reqCode)
{
    if (reqCode)
        _client->connectToArnList();
}


void  ArnDiscoverConnector::postSetupResolver()
{
    QString  path;
    QString  connectIdPath = Arn::pathDiscoverConnect + _id + "/";

    path = connectIdPath + "DiscoverHost/";
    _arnDisHostServicePv = new ArnItem( path + "Service/value!", this);
    _arnDisHostService   = new ArnItem( path + "Service/value",  this);
    _arnDisHostAddress   = new ArnItem( path + "Host/value", this);
    _arnDisHostPort      = new ArnItem( path + "Host/Port/value", this);
    _arnDisHostStatus    = new ArnItem( path + "Status/value", this);
    typedef ArnZeroConf::Error  Err;
    ArnM::setValue( path + "Status/set",
                    QString("%1=Resolved %2=Resolving %3=Bad_request_sequence %4=Resolv_timeout")
                    .arg(Err::Ok).arg(Err::Running).arg(Err::BadReqSeq).arg(Err::Timeout));

    *_arnDisHostServicePv = _resolver->defaultService();  // Use this default if no active persistent service
    _arnDisHostService->addMode(  ArnItem::Mode::Save);  // Save mode after default set, will not save default value
    *_arnDisHostService = _arnDisHostService->toString();  // Any loaded persistent values will be used as request

    connect( _arnDisHostServicePv,  SIGNAL(changed()), this, SLOT(doClientServicetChanged()));
    connect( _resolver, SIGNAL(infoUpdated(int,ArnDiscoverInfo::State)),
             this, SLOT(doClientResolvChanged(int,ArnDiscoverInfo::State)));

    doClientServicetChanged();  // Perform request
}


void  ArnDiscoverConnector::doClientServicetChanged()
{
    Q_ASSERT(_arnDisHostServicePv);
    QString  serviceName = _arnDisHostServicePv->toString();
    if (serviceName.isEmpty())
        serviceName = _resolver->defaultService();
    *_arnDisHostServicePv = serviceName;  // Current service must be set before resolving

    _resolver->resolve( serviceName, true);  // Force new resolve
}


void  ArnDiscoverConnector::doClientResolvChanged( int index, ArnDiscoverInfo::State state)
{
    Q_ASSERT(_arnDisHostService);
    Q_ASSERT(_arnDisHostAddress);
    Q_ASSERT(_arnDisHostPort);

    ArnDiscoverInfo  info = _resolver->infoByIndex( index);
    if (info.serviceName() != _arnDisHostService->toString())  return;  // Not the current service

    *_arnDisHostAddress = info.hostName();
    *_arnDisHostPort    = info.hostPortString();
    *_arnDisHostStatus  = info.resolvCode();

    if (state <= state.ServiceName) { // New resolv has started
        qDebug() << "ArnDiscoverConnector New resolv started: service=" << info.serviceName();
        _isResolved = false;
        _client->clearArnList( _discoverHostPrio);
    }
    else if (state == state.HostInfo) {
        _isResolved = true;
        _client->clearArnList( _discoverHostPrio);
        _client->addToArnList( info.hostName(), info.hostPort(), _discoverHostPrio);
        if (_client->connectStatus() == ArnClient::ConnectStat::Init) {
            emit clientReadyToConnect( _client);
        }
    }
    else if (state == state.HostInfoErr) {
        _client->clearArnList( _discoverHostPrio);
        if (_client->connectStatus() == ArnClient::ConnectStat::Init) {
            emit clientReadyToConnect( _client);
        }
    }
}


///////// ArnDiscoverRemote

ArnDiscoverRemote::ArnDiscoverRemote( QObject *parent) :
    ArnDiscoverAdvertise( parent)
{
    _servTimer         = new QTimer( this);
    _arnInternalServer = 0;
    _arnDResolver      = 0;
    _defaultService    = "Arn Default Service";
}


void  ArnDiscoverRemote::startUseServer( ArnServer* arnServer, ArnDiscover::Type discoverType)
{
    QHostAddress  addr  = arnServer->listenAddress();
    QString  listenAddr = ((addr == QHostAddress::Any) || (addr == QHostAddress::AnyIPv6))
                          ? QString("Any") : addr.toString();
    int  hostPort = arnServer->port();
    ArnM::setValue( Arn::pathDiscoverThis + "Interface/Listen/value", listenAddr);
    ArnM::setValue( Arn::pathDiscoverThis + "Interface/First/value", ArnServer::getInterface1Address().toString());
    ArnM::setValue( Arn::pathDiscoverThis + "Host/value", QHostInfo::localHostName());
    ArnM::setValue( Arn::pathDiscoverThis + "Host/Port/value", hostPort);

    // Setup advertise, but don't start yet, waiting for service name
    ArnDiscoverAdvertise::advertiseService( discoverType, QString(), hostPort);
}


void  ArnDiscoverRemote::startUseNewServer( ArnDiscover::Type discoverType, int port)
{
    if (!_arnInternalServer)
        delete _arnInternalServer;
    _arnInternalServer = new ArnServer( ArnServer::Type::NetSync, this);
    _arnInternalServer->start( port);

    startUseServer( _arnInternalServer, discoverType);
}


ArnDiscoverConnector*  ArnDiscoverRemote::newConnector( ArnClient& client, const QString& id)
{
    ArnDiscoverConnector*  connector = new ArnDiscoverConnector( client, id);
    connect( connector, SIGNAL(clientReadyToConnect(ArnClient*)), this, SIGNAL(clientReadyToConnect(ArnClient*)));

    return connector;
}


void  ArnDiscoverRemote::postSetupThis()
{
    _servTimer->start( 5000);
    connect( _servTimer, SIGNAL(timeout()), this, SLOT(serviceTimeout()));

    connect( &_arnService,   SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    connect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    QString  servicePath = Arn::pathDiscoverThis + "Service/value";
    _arnServicePv.open( Arn::twinPath( servicePath));
    _arnService.addMode( ArnItem::Mode::Save);
    _arnService.open( servicePath);
    // Any loaded persistent service name has now been sent to firstServiceSetup()

    ArnDiscoverAdvertise::postSetupThis();
}


void  ArnDiscoverRemote::serviceTimeout()
{
    qDebug() << "First service setup timeout, using default.";

    firstServiceSetup( _defaultService);
}


void  ArnDiscoverRemote::firstServiceSetup( QString serviceName)
{
    QString  service = serviceName;
    //if (service.isEmpty())
    //    service = _defaultService;
    qDebug() << "firstServiceSetup: serviceName=" << service;

    _servTimer->stop();
    disconnect( &_arnService,   SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    disconnect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(firstServiceSetup(QString)));
    connect( &_arnServicePv, SIGNAL(changed(QString)), this, SLOT(doServiceChanged(QString)));

    setService( service);
}


void  ArnDiscoverRemote::doServiceChanged( QString val)
{
    qDebug() << "DiscoverRemote Service changed: servname=" << val;
    _arnServicePv = val;
    ArnDiscoverAdvertise::setService( val.isEmpty() ? _defaultService : val);
}


void  ArnDiscoverRemote::serviceRegistered( QString serviceName)
{
    qDebug() << "DiscoverRemote Service registered: serviceName=" << serviceName;

    //_arnServicePv = serviceName;
    ArnM::setValue( Arn::pathDiscoverThis + "UsingService/value", serviceName);

    ArnDiscoverAdvertise::serviceRegistered( serviceName);
}


void  ArnDiscoverRemote::setService( QString service)
{
    if (hasSetupAdvertise())
        _arnService = service;
    else
        ArnDiscoverAdvertise::setService( service);
}


QString  ArnDiscoverRemote::defaultService()  const
{
    return _defaultService;
}


void  ArnDiscoverRemote::setDefaultService( const QString& defaultService)
{
    _defaultService = defaultService;
}
