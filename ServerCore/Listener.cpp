#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"
#include "Service.h"

/*---------------
	Listener
---------------*/

Listener::~Listener() {
	SocketUtils::Close(_socket);
	for (AcceptEvent* acceptEvent : _acceptEvents) {
		xdelete(acceptEvent);
	}
}

bool Listener::StartAccept(ServerServiceRef service) {
	_service = service;
	if (_service == nullptr)
		return false;

	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
		return false;

	if (service->GetIocpCore()->Register(shared_from_this()) == false)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::SetLinger(_socket, 1, 0) == false)
		return false;

	if (SocketUtils::Bind(_socket, _service->GetNetAddress()) == false)
		return false;

	if (SocketUtils::Listen(_socket) == false)
		return false;

	const int32 acceptCount = _service->GetMaxSessionCount();
	for (int32 i = 0; i < acceptCount; i++) {
		AcceptEvent* acceptEvent = xnew<AcceptEvent>();
		acceptEvent->owner = shared_from_this();
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}

	return true;
}

void Listener::CloseSocket() {
	SocketUtils::Close(_socket);
}

HANDLE Listener::GetHandle() {
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent) {
	SessionRef session = _service->CreateSession();	 // IOCP에 등록까지 해버림
	// 나중에 컨텐츠를 만들면서 sessionFactory를 직접 설정하고 Create하면 원하는 Session을 만들 수 있음

	acceptEvent->Init();
	acceptEvent->session = session;

	DWORD bytesRecived = 0;
	// Client  socket은 미리 만들어서 CP에 올라간 상태
	// AcceptEx 이후 어떤 클라가 접속 시도 하면 IocpCore::Dispatch에서 반응 -> IocpObject::Dispatch(Listener::Dispatch)
	if (false == SocketUtils::AcceptEx(_socket, session->GetSocket(), session->_recvBuffer.WritePos(), 0,
									   sizeof(SOCKADDR_IN) + 16,
									   sizeof(SOCKADDR_IN) + 16, OUT & bytesRecived,
									   static_cast<LPOVERLAPPED>(acceptEvent))) {
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING) {
			// RegisterAccept는 끊기면 안 됨 (낚시대가 미끄러진 상황. 낚시대를 다시 던져야 함)
			RegisterAccept(acceptEvent);
			return;
		}
	}
}

void Listener::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes) {
	ASSERT_CRASH(iocpEvent->eventType == EventType::Accept);

	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);

	ProcessAccept(acceptEvent);
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent) {
	SessionRef session = acceptEvent->session;

	// Client socket은 원래 listenSocket의 속성을 그대로 받아야 하는데 AcceptEx에선 이 작업을 해주지 않음
	// SetUpdateAcceptSocket 함수를 통해 이 작업을 해줌
	if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _socket)) {
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddress = sizeof(sockAddress);
	if (SOCKET_ERROR ==
		::getpeername(session->GetSocket(), reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddress)) {
		RegisterAccept(acceptEvent);
		return;
	}

	session->SetNetAddress(NetAddress(sockAddress));

	cout << "Client Connected!!" << endl;

	session->ProcessConnect();

	RegisterAccept(acceptEvent);
}
