#include "pch.h"
#include "SocketUtils.h"
#include <winSock2.h>

/*-----------------
	Socket Utils
------------------*/

LPFN_CONNECTEX SocketUtils::ConnectEx = nullptr;
LPFN_DISCONNECTEX SocketUtils::DisconnectEx = nullptr;
LPFN_ACCEPTEX SocketUtils::AcceptEx = nullptr;
// 기본적인 connect, accept, closesocket 함수들과 다른
// Windows의 고성능 네트워크 모델인 IOCP를 제대로 활용하기 위해 만들어진 확장 함수들
// 컴파일 타임에 직접 링크해서 쓸 수 있지만, 런타임에 함수 포인터를 직접 얻어와서 호출하는 것이 성능상 유리함

void SocketUtils::Init() {
	WSADATA wsaData;
	ASSERT_CRASH(::WSAStartup(MAKEWORD(2, 2), OUT & wsaData) == 0);

	/* 런타임에 함수 포인터를 얻어오는 API */
	SOCKET dummySocket = CreateSocket();
	ASSERT_CRASH(BindWindowsFunction(dummySocket, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&ConnectEx)));
	ASSERT_CRASH(BindWindowsFunction(dummySocket, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&DisconnectEx)));
	ASSERT_CRASH(BindWindowsFunction(dummySocket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx)));
	// 각각의 주소를 불러옴

	Close(dummySocket);
}

void SocketUtils::Clear() {
	::WSACleanup();
}

bool SocketUtils::BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn) {
	DWORD bytes = 0;
	return SOCKET_ERROR != ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), fn, sizeof(*fn),
									  OUT & bytes, NULL, NULL);
	// ConnectEx/DisconnectEx/AcceptEx 함수들의 주소를 runtime 도중에 불러오기 위한 함수
}

SOCKET SocketUtils::CreateSocket() {
	return ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	// ::socket에서는 기본적으로 WSA_FLAG_OVERLAPPED 플래그가 있어서 따로 설정할 필요가 없었음
	// ::WSASocket에서는 기본적으로 WSA_FLAG_OVERLAPPED 플래그가 없어서 따로 설정해야 함
}

bool SocketUtils::SetLinger(SOCKET socket, uint16 onoff, uint16 linger) {
	LINGER option;
	option.l_onoff = onoff;
	option.l_linger = linger;
	return SetSockOpt(socket, SOL_SOCKET, SO_LINGER, option);
}

bool SocketUtils::SetReuseAddress(SOCKET socket, bool flag) {
	return SetSockOpt(socket, SOL_SOCKET, SO_REUSEADDR, flag);
}

bool SocketUtils::SetReceiveBufferSize(SOCKET socket, int32 size) {
	return SetSockOpt(socket, SOL_SOCKET, SO_RCVBUF, size);
}

bool SocketUtils::SetSendBufferSize(SOCKET socket, int32 size) {
	return SetSockOpt(socket, SOL_SOCKET, SO_SNDBUF, size);
}

bool SocketUtils::SetNoDelay(SOCKET socket, bool flag) {
	return SetSockOpt(socket, IPPROTO_TCP, TCP_NODELAY, flag);
}

// listenSocket의 특성을 clientSocket에 그대로 적용해주는 함수
bool SocketUtils::SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket) {
	return SetSockOpt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, listenSocket);
}

bool SocketUtils::Bind(SOCKET socket, NetAddress netAddr) {
	return SOCKET_ERROR !=
		   ::bind(socket, reinterpret_cast<const SOCKADDR*>(&netAddr.GetSockAddr()), sizeof(SOCKADDR_IN));
}

bool SocketUtils::BindAnyAddress(SOCKET socket, uint32 port) {
	SOCKADDR_IN myAddr;
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myAddr.sin_port = htons(port);

	return SOCKET_ERROR != ::bind(socket, reinterpret_cast<const SOCKADDR*>(&myAddr), sizeof(myAddr));
}

bool SocketUtils::Listen(SOCKET socket, int32 backlog) {
	return SOCKET_ERROR != ::listen(socket, backlog);
}

void SocketUtils::Close(SOCKET& socket) {
	if (socket != INVALID_SOCKET)
		::closesocket(socket);
	socket = INVALID_SOCKET;
}
