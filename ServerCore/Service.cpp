#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

Service::Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: _type(type), _netAddress(address), _iocpCore(core), _sessionFactory(factory), _maxSessionCount(maxSessionCount) {}

Service::~Service() {}

void Service::CloseService() {
	// TODO
}

SessionRef Service::CreateSession() {
	SessionRef session = _sessionFactory();
	session->SetService(shared_from_this());
	if (_iocpCore->Register(session) == false)
		return nullptr;

	return session;
}

void Service::AddSession(SessionRef session) {
	WRITE_LOCK;
	_sessionCount++;
	_sessions.insert(session);
}

void Service::ReleaseSession(SessionRef session) {
	WRITE_LOCK;
	ASSERT_CRASH(_sessions.erase(session) != 0);
	_sessionCount--;
}

/*------------------
	ClientService
------------------*/

ClientService::ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Client, targetAddress, core, factory, maxSessionCount) {}

bool ClientService::Start() {
	if (CanStart() == false)
		return false;

	const int32 sessionCount = GetMaxSessionCount();
	for (int32 i = 0; i < sessionCount; i++) {
		SessionRef session = CreateSession();
		if (session->Connect() == false) {
			return false;
		}
	}

	return true;
}

void ClientService::CloseService() {}

/*------------------
	ServerService
------------------*/

ServerService::ServerService(NetAddress localAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Server, localAddress, core, factory, maxSessionCount) {}

bool ServerService::Start() {
	if (CanStart() == false)
		return false;

	_listener = MakeShared<Listener>();
	if (_listener == nullptr)
		return false;

	ServerServiceRef service = static_pointer_cast<ServerService>(shared_from_this());
	// enable_shared_from_this가 Service에 걸려있기 때문에, shared_from_this()에서 Service를 뱉어주고 있음
	// ServerService로 캐스팅해서 Listener에 전달
	if (_listener->StartAccept(service) == false)
		return false;

	return true;
}

void ServerService::CloseService() {
	// TODO
	Service::CloseService();
}
