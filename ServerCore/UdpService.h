#pragma once
#include "IocpCore.h"
#include "NetAddress.h"
#include "UdpSession.h"

/*----------------------------------------------------
    UdpClientContext
    UDP는 TCP처럼 "연결된 소켓"이 없으므로
    서버가 클라이언트 상태를 직접 관리.
----------------------------------------------------*/
struct UdpClientContext {
  uint64 sessionId = 0;
  SOCKADDR_IN addr = {};
  uint32 lastSeq = 0;    // 마지막으로 처리한 수신 sequence
  int64 lastAliveMs = 0; // 마지막 패킷 수신 시각 (GetTickCount64)
};

using UdpClientContextRef = shared_ptr<UdpClientContext>;

/*----------------------------------------------------
    UdpService
    - 단일 SOCK_DGRAM 소켓으로 모든 클라이언트를 처리
    - 핸드셰이크(C_UDP_HELLO) 수신 시 SessionId 발급
    - Broadcast / SendTo 제공
    - 타임아웃 클라이언트 제거 (Tick 호출 필요)
----------------------------------------------------*/
class UdpService : public enable_shared_from_this<UdpService> {
public:
  static constexpr int64 SESSION_TIMEOUT_MS = 1000000; // 1000초 무응답 시 제거

  UdpService(NetAddress bindAddr, IocpCoreRef iocpCore,
             function<UdpSessionRef()> sessionFactory);
  ~UdpService();

  // UdpService 시작 (udpSession 생성 및 소켓 바인딩)
  bool Start();

  IocpCoreRef GetIocpCore() { return _iocpCore; }
  NetAddress GetBindAddress() const { return _bindAddr; }

  /* 클라이언트 등록 / 제거 */
  UdpClientContextRef RegisterClient(const SOCKADDR_IN &addr);
  void RemoveClient(uint64 sessionId);

  /* 조회 */
  UdpClientContextRef FindByAddr(const SOCKADDR_IN &addr);
  UdpClientContextRef FindById(uint64 sessionId);

  /* 송신 */
  void SendTo(uint64 sessionId, SendBufferRef sendBuffer);
  void Broadcast(SendBufferRef sendBuffer);

  /* 메인 루프에서 주기적으로 호출 — 타임아웃 클라이언트 정리 */
  void Tick();

  UdpSessionRef GetUdpSession() { return _udpSession; }

private:
  static uint64 GenerateSessionId();

  NetAddress _bindAddr;
  IocpCoreRef _iocpCore;
  function<UdpSessionRef()> _sessionFactory;
  UdpSessionRef _udpSession;

  USE_LOCK;
  HashMap<uint64, UdpClientContextRef> _idMap;   // sessionId → context
  HashMap<uint32, UdpClientContextRef> _addrMap; // addr key  → context
  // addrMap 키: (ip XOR port) 간단 해시, 충돌은 FindByAddr에서 이중 검사
};

/* SOCKADDR_IN → uint32 키 (같은 주소면 같은 키, 충돌 극소화) */
inline uint32 SockAddrKey(const SOCKADDR_IN &addr) {
  return addr.sin_addr.s_addr ^ static_cast<uint32>(addr.sin_port);
}
