#include "pch.h"
#include "SocketUtils.h"
#include "UdpSession.h"

/*----------------------------------------------------
    UdpSession
----------------------------------------------------*/

UdpSession::UdpSession()
{
    _socket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    ASSERT_CRASH(_socket != INVALID_SOCKET);
}

UdpSession::~UdpSession()
{
    SocketUtils::Close(_socket);
}

HANDLE UdpSession::GetHandle()
{
    return reinterpret_cast<HANDLE>(_socket);
}

void UdpSession::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
    switch (iocpEvent->eventType)
    {
    case EventType::Recv:
        ProcessRecv(numOfBytes);
        break;
    default:
        ASSERT_CRASH("UdpSession: unexpected event type");
        break;
    }
}

/*----------------------------------------------------
    송신 — 동기 WSASendTo (OVERLAPPED 없음)
    UDP 위치 동기화 패킷은 유실을 허용하므로
    완료 통보가 불필요. 동기 호출로 구조를 단순화.
    WSASendTo는 thread-safe하지 않으나, 데이터그램 단위
    원자성이 보장되므로 단순 병렬 호출은 허용됨.
----------------------------------------------------*/

void UdpSession::SendTo(const SOCKADDR_IN& destAddr, SendBufferRef sendBuffer)
{
    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
    wsaBuf.len = static_cast<ULONG>(sendBuffer->WriteSize());

    DWORD numOfBytes = 0;
    if (SOCKET_ERROR == ::WSASendTo(_socket, &wsaBuf, 1, OUT &numOfBytes, 0,
        reinterpret_cast<const SOCKADDR*>(&destAddr), sizeof(SOCKADDR_IN),
        nullptr, nullptr))  // OVERLAPPED = nullptr → 동기 호출
    {
        int32 err = ::WSAGetLastError();
        cout << "UdpSession SendTo error: " << err << endl;
    }
}

/*----------------------------------------------------
    수신 — 비동기 WSARecvFrom
----------------------------------------------------*/

void UdpSession::RegisterRecv()
{
    _recvEvent.Init();
    _recvEvent.owner = shared_from_this();
    _recvFromAddrLen = sizeof(SOCKADDR_IN);

    WSABUF wsaBuf;
    wsaBuf.buf = reinterpret_cast<char*>(_recvBuf);
    wsaBuf.len = static_cast<ULONG>(UDP_MAX_PACKET);

    DWORD numOfBytes = 0;
    DWORD flags      = 0;

    if (SOCKET_ERROR == ::WSARecvFrom(_socket, &wsaBuf, 1, OUT &numOfBytes, &flags,
        reinterpret_cast<SOCKADDR*>(&_recvFromAddr), reinterpret_cast<LPINT>(&_recvFromAddrLen),
        &_recvEvent, nullptr))
    {
        int32 err = ::WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            // 10054 (WSAECONNRESET): 클라이언트가 종료될 때 Windows가 올려보내는 ICMP 에러.
            // UDP는 비연결이므로 소켓을 죽이지 말고 루프를 재등록해서 계속 수신.
            _recvEvent.owner = nullptr;
            if (err != WSAECONNRESET)
                cout << "UdpSession RecvFrom error: " << err << endl;
            RegisterRecv();
        }
    }
}

void UdpSession::ProcessRecv(int32 numOfBytes)
{
    _recvEvent.owner = nullptr;

    // numOfBytes == 0 이면 ICMP Port Unreachable(10054) 같은 에러 완료 통보.
    // 패킷 처리는 건너뛰고 바로 재등록해서 수신 루프를 유지.
    if (numOfBytes >= static_cast<int32>(sizeof(UdpPacketHeader)))
        OnRecvPacket(_recvFromAddr, _recvBuf, numOfBytes);

    RegisterRecv();
}
