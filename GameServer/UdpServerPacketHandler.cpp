#include "pch.h"
#include "UdpServerPacketHandler.h"
#include "BufferWriter.h"

/*----------------------------------------------------
    디스패치
----------------------------------------------------*/

void UdpServerPacketHandler::HandlePacket(std::shared_ptr<UdpService> service,
                                          UdpClientContextRef ctx, BYTE *buffer,
                                          int32 len) {
  UdpPacketHeader *header = reinterpret_cast<UdpPacketHeader *>(buffer);

  PacketType type = static_cast<PacketType>(header->id);

  switch (type) {
  case PacketType::C_UDP_HELLO:
    Handle_C_UDP_HELLO(service, ctx);
    break;

  default:
    break;
  }
}

/*----------------------------------------------------
    핸드셰이크
    클라이언트가 sessionId=0 으로 C_UDP_HELLO를 보내면
    서버가 발급한 sessionId를 S_UDP_HANDSHAKE_ACK(350)로 응답. (proto:
lobby_response.S_UDP_HELLO)
----------------------------------------------------*/

void UdpServerPacketHandler::Handle_C_UDP_HELLO(
    std::shared_ptr<UdpService> service, UdpClientContextRef ctx) {
  cout << "C_UDP_HELLO: new client sessionId=" << ctx->sessionId << endl;
  SendBufferRef buf = Make_S_UDP_HANDSHAKE_ACK(ctx->sessionId);
  service->SendTo(ctx->sessionId, buf);
}

/*----------------------------------------------------
    패킷 빌더
----------------------------------------------------*/

static SendBufferRef
BuildPacket(uint64 sessionId, PacketType type,
            std::function<void(BufferWriter &)> writePayload) {
  SendBufferRef buf = MakeShared<SendBuffer>(UDP_MAX_PACKET);
  BufferWriter bw(buf->Buffer(), buf->AllocSize());

  UdpPacketHeader *header = bw.Reserve<UdpPacketHeader>();
  writePayload(bw);

  header->sessionId = sessionId;
  header->sequence = 0;
  header->id = static_cast<uint16>(type);

  buf->SetWriteSize(bw.WriteSize());
  return buf;
}

SendBufferRef
UdpServerPacketHandler::Make_S_UDP_HANDSHAKE_ACK(uint64 sessionId) {
  return BuildPacket(sessionId, PacketType::S_UDP_HANDSHAKE_ACK,
                     [&](BufferWriter &bw) { bw << sessionId; });
}
