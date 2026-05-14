#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "SendBuffer.h"

/*----------------------------------------------------
    UdpPacketHeader
    [ sessionId:8 ][ sequence:4 ][ id:2 ][ payload ]
    - sessionId : 클라이언트 식별자 (핸드셰이크 시 서버 발급)
    - sequence  : 송신 측 단방향 카운터 (중복/순서 판정)
    - id        : PacketType (protocols/packetTypes/packetTypes.h)
----------------------------------------------------*/

// 컴파일러가 성능을 위해 자동으로 끼워넣는 패딩 바이트를 제거.
// 네트워크 패킷은 바이트 스트림이라 패딩이 있으면 송수신 양쪽 구조체 레이아웃이
// 달라짐.
#pragma pack(push, 1)
struct UdpPacketHeader {
  uint64 sessionId;
  uint32 sequence;
  uint16 id;
};
#pragma pack(pop)

constexpr uint32 UDP_MAX_PAYLOAD = 1200;
constexpr uint32 UDP_MAX_PACKET = sizeof(UdpPacketHeader) + UDP_MAX_PAYLOAD;

/*----------------------------------------------------
    UdpSession
    단일 SOCK_DGRAM 소켓을 IOCP에 등록,
    수신은 WSARecvFrom 비동기, 송신은 WSASendTo 동기.

    송신을 동기로 처리하는 이유:
    UDP는 데이터그램 단위라 WSASendTo 1번에 OVERLAPPED 1개가 필요.
    위치 동기화처럼 유실을 허용하는 패킷은 완료 통보가 불필요하므로
    OVERLAPPED 없이 동기 호출하는 것이 구조가 단순하고 안전함.

    TCP Session과 달리 "연결" 개념이 없으므로
    클라이언트 식별은 UdpService의 SessionId Map으로 수행.
----------------------------------------------------*/
class UdpSession : public IocpObject {
  friend class UdpService;

public:
  UdpSession();
  virtual ~UdpSession();

  /* IocpObject 인터페이스 */
  virtual HANDLE GetHandle() override;
  virtual void Dispatch(class IocpEvent *iocpEvent,
                        int32 numOfBytes = 0) override;

  /* 외부에서 호출 — 특정 주소로 UDP 패킷 동기 송신 */
  void SendTo(const SOCKADDR_IN &destAddr, SendBufferRef sendBuffer);

  SOCKET GetSocket() const { return _socket; }

protected:
  /* 컨텐츠 코드에서 재정의 */
  virtual void OnRecvPacket(const SOCKADDR_IN &senderAddr, BYTE *buffer,
                            int32 len) {}

private:
  void RegisterRecv();
  void ProcessRecv(int32 numOfBytes);

private:
  SOCKET _socket = INVALID_SOCKET;

  /* 수신용 — 비동기이므로 멤버로 유지 */
  RecvEvent   _recvEvent;
  BYTE        _recvBuf[UDP_MAX_PACKET];
  SOCKADDR_IN _recvFromAddr    = {};
  int32       _recvFromAddrLen = sizeof(SOCKADDR_IN);
};
