#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

class Service;

/*--------------------
	Session
--------------------*/

class Session : public IocpObject {
	friend class Listener;
	friend class IocpCore;
	friend class Service;

	enum {
		BUFFER_SIZE = 0x10000,	// 64KB (기본적으로 적당한 버퍼 크기)
	};

public:
	Session();
	virtual ~Session();

public:
	void Send(SendBufferRef sendBuffer);
	bool Connect();						  // 서버끼리 연결할 때, session이 상대방 서버를 대표, 붙는 작업이 필요
	void Disconnect(const WCHAR* cause);  // 해킹 의심 등의 이유로 연결 끊기

	shared_ptr<Service> GetService() { return _service.lock(); }
	void SetService(shared_ptr<Service> service) { _service = service; }

public:
	/* 정보 관련 */
	// 클라이언트 주소 및 소켓
	void SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress getNetAdress() { return _netAddress; }
	SOCKET GetSocket() { return _socket; }
	bool IsConnected() { return _connected; }
	SessionRef GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

private:
	/* 인터페이스 구현 */
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

private:
	/* 전송 관련 */
	bool RegisterConnect();
	bool RegisterDisconnect();
	void RegisterRecv();
	void RegisterSend();

	void ProcessConnect();
	void ProcessDisconnect();
	void ProcessRecv(int32 numOfBytes);
	void ProcessSend(int32 numOfBytes);

	void HandleError(int32 errorCode);

protected:
	/* 컨텐츠 코드에서 재정의(오버라이딩) */
	virtual void OnConnected() {}  // 연결 후 로그를 찍거나 헬스 체크 등 가능
	virtual int32 OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void OnSend(int32 len) {}
	virtual void OnDisconnected() {}

public:
	// Circular Buffer: 복사 비용이 조금 있음
	// [OOOOOOOOOXXXXXXXXXXXXXXXKKKKKKKAAAA-----]
	char _sendBuffer[1000];
	int32 _sendLen = 0;

private:
	weak_ptr<Service> _service;
	SOCKET _socket = INVALID_SOCKET;
	NetAddress _netAddress = {};
	Atomic<bool> _connected = false;

private:
	USE_LOCK;

	/* 수신 관련 */
	RecvBuffer _recvBuffer;

	/* 송신 관련 */
	Queue<SendBufferRef> _sendQueue;
	Atomic<bool> _sendRegistered = false;  // RegisterSend가 걸린 상태인지 확인

private:
	/* IocpEvent 재사용 */
	ConnectEvent _connectEvent;
	DisconnectEvent _disconnectEvent;
	RecvEvent _recvEvent;
	SendEvent _sendEvent;
};

/*--------------------
	PacketSession
--------------------*/
// TCP 특성상 패킷이 모두 도착했다는 보장을 할 수가 없어서 이를 처리하는 세션

struct PacketHeader {
	uint16 size;
	uint16 id;
};

// [size(2Byte)][id(2Byte)][data...]
// size: 패킷 전체 크기 (header + data)
// id: 프로토콜 ID (ex. 1=로그인, 2=회원가입)

class PacketSession : public Session {
public:
	PacketSession();
	virtual ~PacketSession();

	PacketSessionRef GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	virtual int32 OnRecv(BYTE* buffer, int32 len) final;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) abstract;
};
