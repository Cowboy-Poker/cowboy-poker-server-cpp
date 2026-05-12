#pragma once

#include "Types.h"

enum class EventType : uint8 {
	Connect,
	Disconnect,
	Accept,
	// PreRecv, // 0 byte recv
	Recv,
	Send,
};

class Session;

/*------------------
	IocpEvent
-------------------*/

// 상속을 받으면 무조건 Offset 0번에 OVERLAPPED 구조체가 있게 됨
// OVERLAPPED 처럼 사용 가능함
class IocpEvent : public OVERLAPPED {
public:
	IocpEvent(EventType type);
	// virtual 쓰면 안 됨
	// virtual 때문에 가상 함수 테이블(vtable 포인터)이 Offset 0번을 차지하게 됨
	// 그러면 맨 처음에 있던 OVERLAPPED 구조체가 사라지게 됨

	void Init();

public:
	EventType eventType;
	IocpObjectRef owner;
};

/*------------------
	ConnectEvent
-------------------*/

class ConnectEvent : public IocpEvent {
public:
	ConnectEvent() : IocpEvent(EventType::Connect) {}

private:
};

/*------------------
	DisconnectEvent
-------------------*/

class DisconnectEvent : public IocpEvent {
public:
	DisconnectEvent() : IocpEvent(EventType::Disconnect) {}

private:
};

/*------------------
	AcceptEvent
-------------------*/

class AcceptEvent : public IocpEvent {
public:
	AcceptEvent() : IocpEvent(EventType::Accept) {}

public:
	SessionRef session = nullptr;  // client session
};

/*------------------
	RecvEvent
-------------------*/

class RecvEvent : public IocpEvent {
public:
	RecvEvent() : IocpEvent(EventType::Recv) {}

private:
};

/*------------------
	SendEvent
-------------------*/

class SendEvent : public IocpEvent {
public:
	SendEvent() : IocpEvent(EventType::Send) {}

	Vector<SendBufferRef> sendBuffers;
	// queue(_sendQueue)에서 데이터를 빼내는 순간
	// Ref Count가 사라지는 오류를 방지하는 동시에
	// "WSASend완료 시점까지 보존용 Vector"

private:
};