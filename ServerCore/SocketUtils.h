#pragma once
#include <winSock2.h>
#include "NetAddress.h"

/*-----------------
	Socket Utils
------------------*/

class SocketUtils {
public:
	static LPFN_CONNECTEX ConnectEx;
	static LPFN_DISCONNECTEX DisconnectEx;
	static LPFN_ACCEPTEX AcceptEx;

public:
	static void Init();
	static void Clear();

	static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
	static SOCKET CreateSocket();

	// Socket Option
	static bool SetLinger(SOCKET socket, uint16 onoff, uint16 linger);
	static bool SetReuseAddress(SOCKET socket, bool flag);
	static bool SetReceiveBufferSize(SOCKET socket, int32 size);
	static bool SetSendBufferSize(SOCKET socket, int32 size);
	static bool SetNoDelay(SOCKET socket, bool flag);
	static bool SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket);

	static bool Bind(SOCKET socket, NetAddress netAddr);
	static bool BindAnyAddress(SOCKET socket, uint32 port);
	static bool Listen(SOCKET socket, int32 backlog = SOMAXCONN);
	static void Close(SOCKET& socket);
};

template <typename T>
static inline bool SetSockOpt(SOCKET socket, int32 level, int32 optName, T optVal) {
	return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<const char*>(&optVal), sizeof(T));
}