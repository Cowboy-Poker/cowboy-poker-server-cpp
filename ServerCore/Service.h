#pragma once
#include "NetAddress.h"
#include "IocpCore.h"
#include "Listener.h"
#include <functional>

enum class ServiceType : uint8 { Server, Client };

/*----------------
	Service
-----------------*/

using SessionFactory = function<SessionRef(void)>;

class Service : public enable_shared_from_this<Service> {
public:
	Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	// address: Server면 자신의 주소, Client면 client 주소
	// core: core를 하나만 만들어서 공유하거나, core를 여러 개 사용해서 분리할 수도 있음
	// factory: Session을 만들어내는 함수
	// maxSessionCount: 최대 Session의 개수(최대 동접수)

	virtual ~Service();

	virtual bool Start() abstract;
	bool CanStart() { return _sessionFactory != nullptr; }

	virtual void CloseService();
	void SetSessionFactory(SessionFactory func) { _sessionFactory = func; }

	SessionRef CreateSession();								  // SessionFactory로 Session을 만들어서 _iocpCore에 등록
	void AddSession(SessionRef session);					  // 이미 만들어진 Session을 추가
	void ReleaseSession(SessionRef session);				  // Session을 해제
	int32 GetCurrentSessionCount() { return _sessionCount; }  // 현재 동접 확인
	int32 GetMaxSessionCount() { return _maxSessionCount; }	  // 최대 동접 확인

public:
	ServiceType GetServiceType() { return _type; }
	NetAddress GetNetAddress() { return _netAddress; }
	IocpCoreRef& GetIocpCore() { return _iocpCore; }

protected:
	USE_LOCK;

	ServiceType _type;
	NetAddress _netAddress = {};
	IocpCoreRef _iocpCore;

	Set<SessionRef> _sessions;
	int32 _sessionCount = 0;
	int32 _maxSessionCount = 1;
	SessionFactory _sessionFactory;
};

/*------------------
	ClientService
------------------*/

class ClientService : public Service {
public:
	ClientService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~ClientService() {}

	virtual bool Start() override;	// Client쪽 주소로 연결
	virtual void CloseService() override;

private:
	ListenerRef _listener;
};

/*------------------
	ServerService
------------------*/

class ServerService : public Service {
public:
	ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~ServerService() {}

	virtual bool Start() override;	// Server쪽 주소로 연결
	virtual void CloseService() override;

private:
	ListenerRef _listener = nullptr;
};