#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

/*--------------------
	Session
--------------------*/

Session::Session() : _recvBuffer(BUFFER_SIZE) {
	_socket = SocketUtils::CreateSocket();
	// 매번 세션을 만들 때마다 소켓을 만들고, 다시 삭제하는 것은 성능상 부담이 됨
	// 따라서 소켓을 미리 만들어두고, 세션을 만들 때마다 소켓을 재사용할 수 있음
}

Session::~Session() {
	SocketUtils::Close(_socket);
}

void Session::Send(SendBufferRef sendBuffer) {
	if (IsConnected() == false)
		return;

	// 현재 RegisterSend가 걸리지 않은 상태라면, 걸어줌
	// RegisterSend를 하나만 거는 이유: 최대한 뭉쳐서 보낼 수 있음
	bool registerSend = false;
	{
		WRITE_LOCK;

		_sendQueue.push(sendBuffer);
		// RegisterSend가 걸려있는 상태라면 sendBuffer에 넣으려 하지 않고(RegisterSend 호출 대신) sendQueue에 보관
		if (_sendRegistered.exchange(true) == false)
			registerSend = true;
	}
	if (registerSend)
		RegisterSend();
}

bool Session::Connect() {
	return RegisterConnect();
}

void Session::Disconnect(const WCHAR* cause) {
	// 이미 연결 끊어져 있으면 종료
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	wcout << "Disconnect: " << cause << endl;

	RegisterDisconnect();
}

HANDLE Session::GetHandle() {
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes) {
	switch (iocpEvent->eventType) {
		case EventType::Connect:
			ProcessConnect();
			break;
		case EventType::Disconnect:
			ProcessDisconnect();
			break;
		case EventType::Recv:
			ProcessRecv(numOfBytes);
			break;
		case EventType::Send:
			ProcessSend(numOfBytes);
			break;
		default:
			ASSERT_CRASH("Invalid event type");
			break;
	}
}

bool Session::RegisterConnect() {
	if (IsConnected() == true)
		return false;

	if (GetService()->GetServiceType() != ServiceType::Client)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::BindAnyAddress(_socket, 0 /*남는거*/) == false)
		return false;
	// 아무 포트나 IP랑 연동시켜줌
	// ConnectEx 쓰려면 꼭 필요

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this();  // ADD_REF

	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();
	// Service가 Client 타입이기 때문에 내가 붙어야 하는 서버의 주소가 들어있음

	if (SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0,
							   &numOfBytes, &_connectEvent) == false) {
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING) {
			_connectEvent.owner = nullptr;	// RELEASE_REF
			return false;
		}
	}

	return true;
}

bool Session::RegisterDisconnect() {
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this();  // ADD_REF

	// TF_REUSE_SOCKET: 소켓을 닫지 않고 재사용할 수 있도록 함
	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0)) {
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING) {
			_disconnectEvent.owner = nullptr;  // RELEASE_REF
			return false;
		}
	}
	return true;
}

void Session::RegisterRecv() {
	if (IsConnected() == false)
		return;

	_recvEvent.Init();
	_recvEvent.owner = shared_from_this();
	// ADD_REF해서 WSARecv에 등록한 뒤로 절대 삭제되지 않게 함

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, &flags, &_recvEvent, nullptr)) {
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING) {
			HandleError(errorCode);
			_recvEvent.owner = nullptr;	 // RELEASE_REF
			return;
		}
	}
}

void Session::RegisterSend() {
	if (IsConnected() == false)
		return;

	_sendEvent.Init();
	_sendEvent.owner = shared_from_this();	// ADD_REF

	// 보낼 데이터를 sendEvent에 등록
	{
		WRITE_LOCK;
		// 또 Lock을 잡는 이유: 나중에 경우에 따라서 Send에서 Lock을 안 잡고 호출할 수도 있어서 이중으로 확인
		// Lock.cpp 보면 재귀적 Lock을 구현해서 같은 스레드라면 또 다시 잡을 수 있게 함

		int32 writeSize = 0;
		while (_sendQueue.empty() == false) {
			SendBufferRef sendBuffer = _sendQueue.front();

			writeSize += sendBuffer->WriteSize();
			// TODO: writeSize가 너무 크면 오류 발생

			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);
		}
	}

	// _sendQueue: "보내려고 줄 서 있는 데이터"
	// _sendEvent.sendBuffers: "지금 OS가 보내고 있는 중이라서, 절대로 메모리에서 사라지면 안 되는 데이터 (인질)"
	// wsaBufs: "OS에게 알려주기 위한 주소록"

	// Scatter-Gather (흩어져 있는 데이터들을 모아서 한 번에 보내는 기법)
	Vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent.sendBuffers.size());
	for (SendBufferRef sendBuffer : _sendEvent.sendBuffers) {
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}
	// 복사에 cost가 들어감

	DWORD numOfBytes = 0;
	// WSASend는 thread safe하지 않음
	// 따라서 락을 걸어서 동시성 문제를 해결해야 함
	// (Session::Send 함수에서 RegisterSend 함수 호출 전에 락을 걸어서 해결)
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0,
								  &_sendEvent, nullptr)) {
		// 매번 1개씩 보내기 때문에 성능이 떨어짐
		// 스캐터 개더 패턴을 활용해서 해결해야 함
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING) {
			HandleError(errorCode);
			_sendEvent.owner = nullptr;		 // RELEASE_REF
			_sendEvent.sendBuffers.clear();	 // RELEASE_REF
			_sendRegistered.store(false);
		}
	}
}

// 원래 Client 입장에서 서버로 연결 시도할 때 세션 등록, 수신 등록, 송신 등록 할 때 사용
// 현재 서버가 Client 입장으로 다른 서버에 Connect 시도할 때도 공용으로 사용
void Session::ProcessConnect() {
	_connectEvent.owner = nullptr;	// RELEASE_REF

	_connected.store(true);

	// 세션 등록
	GetService()->AddSession(GetSessionRef());

	// 컨텐츠 코드에서 재정의(오버라이딩)할 OnConnected 호출
	OnConnected();

	// 수신 등록 (낚시대 던지기)
	RegisterRecv();

	// 송신 등록
}

void Session::ProcessDisconnect() {
	_disconnectEvent.owner = nullptr;  // RELEASE_REF

	OnDisconnected();  // 컨텐츠 코드에서 재정의(오버라이딩)할 OnDisconnected 호출
	GetService()->ReleaseSession(GetSessionRef());
}

void Session::ProcessRecv(int32 numOfBytes) {
	_recvEvent.owner = nullptr;	 // RELEASE_REF

	if (numOfBytes == 0) {	// 연결 끊김
		Disconnect(L"Recv 0 byte");
		return;
	}

	// _writePos를 numOfBytes만큼 뒤로 밈
	if (_recvBuffer.OnWrite(numOfBytes) == false) {
		Disconnect(L"OnWrite Overflow");
		return;
	}

	int32 dataSize = _recvBuffer.DataSize();  // 입력된 데이터 사이즈
	// 컨텐츠 코드에서 재정의(오버라이딩)할 OnRecv 호출
	int32 processLen = OnRecv(_recvBuffer.ReadPos(), dataSize);	 // 실제 처리한 data length를 반환
	if (processLen < 0 || dataSize < processLen ||
		_recvBuffer.OnRead(processLen) == false)  // processLen만큼 _readPos를 뒤로 밈
	{
		Disconnect(L"OnRead Overflow");
		return;
	}

	// 커서 정리 (동일하면 0으로 이동)
	_recvBuffer.Clean();

	// 다시 수신 등록 (낚시대 다시 던지기)
	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes) {
	// Completion Port(Queue)까진 순서대로 끝나지만 IocpCore::Dispatch에서
	// ProcessSend를 호출하는 부분에서는 순서가 보장되지 않음
	_sendEvent.owner = nullptr;		 // RELEASE_REF
	_sendEvent.sendBuffers.clear();	 // RELEASE_REF

	if (numOfBytes == 0) {
		Disconnect(L"Send 0 byte");
		return;
	}

	// 컨텐츠 코드에서 재정의(오버라이딩)할 OnSend 호출
	OnSend(numOfBytes);

	WRITE_LOCK;
	if (_sendQueue.empty()) {
		// 완료됐으면 다음 Send가 호출됐을 때, RegisterSend를 호출할 수 있게끔 _sendRegistered를 false로 변경
		_sendRegistered.store(false);
	} else {
		// WSASend 후에 Send를 호출하여 sendQueue에 보낼 데이터가 찾다면 바로 RegisterSend 호출
		RegisterSend();
	}
}

void Session::HandleError(int32 errorCode) {
	switch (errorCode) {
		case WSAECONNRESET:
		case WSAECONNABORTED:
			Disconnect(L"HandleError");
			break;
		default:
			// TODO: LOG
			cout << "Handle Error: " << errorCode << endl;
			break;
	}
}

/*--------------------
	PacketSession
--------------------*/

PacketSession::PacketSession() {}

PacketSession::~PacketSession() {}

int32 PacketSession::OnRecv(BYTE* buffer, int32 len) {
	int32 processLen = 0;
	while (true) {
		int32 dataSize = len - processLen;
		// 최소한 헤더는 파싱할 수 있는지
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
		// 헤더에 기록된 패킷 크기를 파싱할 수 있는지
		if (dataSize < header.size)
			break;

		// 패킷 조립 성공
		OnRecvPacket(&buffer[processLen], header.size);

		processLen += header.size;
	}
	return processLen;
}