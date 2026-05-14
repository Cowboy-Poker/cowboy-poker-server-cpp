#pragma once
#include "UdpService.h"
#include "UdpSession.h"


/*----------------------------------------------------
    UdpGameSession
    UdpService에 등록되는 단일 소켓 세션.
    OnRecvPacket에서 수신 주소로 UdpClientContext를 조회하고
    패킷 타입에 따라 핸들러를 디스패치.
----------------------------------------------------*/
class UdpGameSession : public UdpSession {
public:
  void SetService(shared_ptr<UdpService> service) { _service = service; }

protected:
  virtual void OnRecvPacket(const SOCKADDR_IN &senderAddr, BYTE *buffer,
                            int32 len) override;

private:
  weak_ptr<UdpService> _service;
};

using UdpGameSessionRef = shared_ptr<UdpGameSession>;
