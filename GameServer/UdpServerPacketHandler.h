#pragma once
#include "UdpService.h"
#include "UdpSession.h"


/*----------------------------------------------------
    UdpServerPacketHandler
    수신된 UDP 패킷을 PacketType 별로 디스패치.
----------------------------------------------------*/
class UdpServerPacketHandler {
public:
  static void HandlePacket(std::shared_ptr<UdpService> service,
                           UdpClientContextRef ctx, BYTE *buffer, int32 len);

private:
  /* 핸드셰이크 */
  static void Handle_C_UDP_HELLO(std::shared_ptr<UdpService> service,
                                 UdpClientContextRef ctx);

  /* 패킷 빌더 */
  static SendBufferRef Make_S_UDP_HANDSHAKE_ACK(uint64 sessionId);
};
