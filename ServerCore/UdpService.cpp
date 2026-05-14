#include "pch.h"
#include "SocketUtils.h"
#include "UdpService.h"

/*----------------------------------------------------
    UdpService
----------------------------------------------------*/

UdpService::UdpService(NetAddress bindAddr, IocpCoreRef iocpCore,
                       std::function<UdpSessionRef()> sessionFactory)
    : _bindAddr(bindAddr), _iocpCore(iocpCore),
      _sessionFactory(sessionFactory) {}

UdpService::~UdpService() {}

bool UdpService::Start() {
  _udpSession = _sessionFactory();
  if (_udpSession == nullptr)
    return false;

  SOCKET sock = _udpSession->GetSocket();

  if (SocketUtils::SetReuseAddress(sock, true) == false)
    return false;

  if (SocketUtils::Bind(sock, _bindAddr) == false)
    return false;

  if (_iocpCore->Register(_udpSession) == false)
    return false;

  // 첫 수신 등록
  _udpSession->RegisterRecv();

  return true;
}

/*----------------------------------------------------
    클라이언트 등록 / 제거
----------------------------------------------------*/

UdpClientContextRef UdpService::RegisterClient(const SOCKADDR_IN &addr) {
  WRITE_LOCK;

  uint32 key = SockAddrKey(addr);

  // 이미 등록된 주소라면 기존 컨텍스트 반환
  auto it = _addrMap.find(key);
  if (it != _addrMap.end()) {
    it->second->lastAliveMs = static_cast<int64>(::GetTickCount64());
    return it->second;
  }

  auto ctx = MakeShared<UdpClientContext>();
  ctx->sessionId = GenerateSessionId();
  ctx->addr = addr;
  ctx->lastAliveMs = static_cast<int64>(::GetTickCount64());

  _idMap[ctx->sessionId] = ctx;
  _addrMap[key] = ctx;

  return ctx;
}

void UdpService::RemoveClient(uint64 sessionId) {
  WRITE_LOCK;

  auto it = _idMap.find(sessionId);
  if (it == _idMap.end())
    return;

  uint32 key = SockAddrKey(it->second->addr);
  _addrMap.erase(key);
  _idMap.erase(it);
}

/*----------------------------------------------------
    조회
----------------------------------------------------*/

UdpClientContextRef UdpService::FindByAddr(const SOCKADDR_IN &addr) {
  READ_LOCK;
  auto it = _addrMap.find(SockAddrKey(addr));
  if (it == _addrMap.end())
    return nullptr;
  // 키 충돌 방어: 실제 주소 비교
  auto &ctx = it->second;
  if (ctx->addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
      ctx->addr.sin_port == addr.sin_port)
    return ctx;
  return nullptr;
}

UdpClientContextRef UdpService::FindById(uint64 sessionId) {
  READ_LOCK;
  auto it = _idMap.find(sessionId);
  return (it != _idMap.end()) ? it->second : nullptr;
}

/*----------------------------------------------------
    송신
----------------------------------------------------*/

void UdpService::SendTo(uint64 sessionId, SendBufferRef sendBuffer) {
  UdpClientContextRef ctx = FindById(sessionId);
  if (ctx == nullptr)
    return;
  _udpSession->SendTo(ctx->addr, sendBuffer);
}

void UdpService::Broadcast(SendBufferRef sendBuffer) {
  READ_LOCK;
  for (auto& kv : _idMap)
    _udpSession->SendTo(kv.second->addr, sendBuffer);
}

void UdpService::BroadcastExcept(uint64 excludeSessionId, SendBufferRef sendBuffer) {
  READ_LOCK;
  for (auto& kv : _idMap)
    if (kv.first != excludeSessionId)
      _udpSession->SendTo(kv.second->addr, sendBuffer);
}

/*----------------------------------------------------
    타임아웃 정리 (메인 루프에서 주기적으로 호출)
----------------------------------------------------*/

Vector<uint64> UdpService::Tick() {
  int64 now = static_cast<int64>(::GetTickCount64());

  Vector<uint64> expired;
  {
    READ_LOCK;
    for (auto& kv : _idMap)
      if (now - kv.second->lastAliveMs > SESSION_TIMEOUT_MS)
        expired.push_back(kv.first);
  }

  for (uint64 id : expired) {
    cout << "UdpService: session timeout, removing " << id << endl;
    RemoveClient(id);
  }

  return expired; // 호출자(GameServer)가 퇴장 패킷 전송 담당
}

/*----------------------------------------------------
    SessionId 생성
----------------------------------------------------*/

uint64 UdpService::GenerateSessionId() {
  static Atomic<uint64> sCounter = 1;
  return sCounter.fetch_add(1);
  // fetch_add는 원자적으로 증가시키고 이전 값을 반환
}
