#include "pch.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "RedisClient.h"
#include "UdpServerPacketHandler.h"
extern RedisClient GRedisClient;

/*----------------------------------------------------
    디스패치
----------------------------------------------------*/

void UdpServerPacketHandler::HandlePacket(std::shared_ptr<UdpService> service,
    UdpClientContextRef ctx, BYTE* buffer,
    int32 len) {
    UdpPacketHeader* header = reinterpret_cast<UdpPacketHeader*>(buffer);
    BYTE* payload = buffer + sizeof(UdpPacketHeader);
    int32  payloadLen = len - static_cast<int32>(sizeof(UdpPacketHeader));

    PacketType type = static_cast<PacketType>(header->id);

    switch (type) {
    case PacketType::C_UDP_HELLO:
        Handle_C_UDP_HELLO(service, ctx, payload, payloadLen);
        break;

    case PacketType::C_LOBBY_MOVE:
        Handle_C_LOBBY_MOVE(service, ctx, payload, payloadLen);
        break;

    case PacketType::C_LOBBY_HEARTBEAT:
        Handle_C_LOBBY_HEARTBEAT(ctx);
        break;



    default:
        break;
    }
}

/*----------------------------------------------------
    핸드셰이크
    클라이언트가 sessionId=0 으로 C_UDP_HELLO를 보내면
    서버가 발급한 sessionId를 S_UDP_HANDSHAKE_ACK(350)으로 응답.
----------------------------------------------------*/

void UdpServerPacketHandler::Handle_C_UDP_HELLO(
    std::shared_ptr<UdpService> service, UdpClientContextRef ctx,
    BYTE* payload, int32 payloadLen) {

    if (payloadLen < 2) return;

    BufferReader br(payload, static_cast<uint32>(payloadLen));
    uint16 userIdLen = 0;
    br >> userIdLen;
    if (br.FreeSize() < userIdLen) return;

    ctx->userId = std::string(reinterpret_cast<const char*>(payload + 2), userIdLen);
    cout << "C_UDP_HELLO: sessionId=" << ctx->sessionId
        << " userId=" << ctx->userId << endl;

    SendBufferRef ackBuf = Make_S_UDP_HANDSHAKE_ACK(ctx->sessionId);
    service->SendTo(ctx->sessionId, ackBuf);

    PlayerRedisInfo info;
    if (GRedisClient.GetPlayerInfo(ctx->userId, info)) {
        SendBufferRef infoBuf = Make_S_PLAYER_INFO(ctx->sessionId, info);
        service->SendTo(ctx->sessionId, infoBuf);
    }
    else {
        cout << "Redis lookup failed for userId=" << ctx->userId << endl;
    }
}

/*----------------------------------------------------
    로비 이동
    payload: [posX:f32][posY:f32][posZ:f32][rot:f32] = 16바이트

    수신 후 발신자를 제외한 모든 클라이언트에게
    S_LOBBY_PLAYER_MOVE(352)로 위치를 릴레이한다.
----------------------------------------------------*/

void UdpServerPacketHandler::Handle_C_LOBBY_MOVE(
    std::shared_ptr<UdpService> service, UdpClientContextRef ctx,
    BYTE* payload, int32 payloadLen) {
  // posX+posY+posZ+rot (f32 x4 = 16) + animState (u8 = 1) = 17바이트 최소
  if (payloadLen < 17)
    return;

  BufferReader br(payload, static_cast<uint32>(payloadLen));
  float posX, posY, posZ, rot;
  BYTE animState;
  br >> posX >> posY >> posZ >> rot >> animState;

  SendBufferRef buf = Make_S_LOBBY_PLAYER_MOVE(ctx->sessionId, ctx->userId, posX, posY, posZ, rot, animState);
  service->BroadcastExcept(ctx->sessionId, buf);
}

/*----------------------------------------------------
    하트비트 — lastAliveMs 갱신만 수행
----------------------------------------------------*/

void UdpServerPacketHandler::Handle_C_LOBBY_HEARTBEAT(UdpClientContextRef ctx) {
    ctx->lastAliveMs = static_cast<int64>(::GetTickCount64());
}

/*----------------------------------------------------
    패킷 빌더 — std::function/람다 없이 직접 작성
----------------------------------------------------*/

SendBufferRef UdpServerPacketHandler::Make_S_UDP_HANDSHAKE_ACK(uint64 sessionId) {
    SendBufferRef buf = MakeShared<SendBuffer>(UDP_MAX_PACKET);
    BufferWriter  bw(buf->Buffer(), buf->AllocSize());

    UdpPacketHeader* header = bw.Reserve<UdpPacketHeader>();
    bw << sessionId;

    header->sessionId = sessionId;
    header->sequence = 0;
    header->id = static_cast<uint16>(PacketType::S_UDP_HANDSHAKE_ACK);

    buf->SetWriteSize(bw.WriteSize());
    return buf;
}

/*----------------------------------------------------
    퇴장 브로드캐스트 — UdpService 타임아웃 콜백으로 호출
----------------------------------------------------*/

void UdpServerPacketHandler::OnClientLeave(std::shared_ptr<UdpService> service,
                                           uint64 sessionId) {
  cout << "Player leave: sessionId=" << sessionId << endl;
  SendBufferRef buf = Make_S_LOBBY_PLAYER_LEAVE(sessionId);
  service->Broadcast(buf); // 이미 Map에서 빠진 뒤라 전체 브로드캐스트해도 무방
}

// S_LOBBY_PLAYER_LEAVE payload: [leftSessionId:u64] = 8바이트
SendBufferRef UdpServerPacketHandler::Make_S_LOBBY_PLAYER_LEAVE(uint64 leftSessionId) {
  SendBufferRef buf = MakeShared<SendBuffer>(UDP_MAX_PACKET);
  BufferWriter  bw(buf->Buffer(), buf->AllocSize());

  UdpPacketHeader* header = bw.Reserve<UdpPacketHeader>();
  bw << leftSessionId;

  header->sessionId = 0;
  header->sequence  = 0;
  header->id        = static_cast<uint16>(PacketType::S_LOBBY_PLAYER_LEAVE);

  buf->SetWriteSize(bw.WriteSize());
  return buf;
}

// S_LOBBY_PLAYER_MOVE payload:
// [moverSessionId:u64][posX:f32][posY:f32][posZ:f32][rot:f32][animState:u8] = 25바이트
// animState bitmask: 0x01=점프중  0x02=조준중
SendBufferRef UdpServerPacketHandler::Make_S_LOBBY_PLAYER_MOVE(uint64 moverSessionId,
    const std::string& userId, float posX, float posY, float posZ, float rot, BYTE animState) {
    SendBufferRef buf = MakeShared<SendBuffer>(UDP_MAX_PACKET);
    BufferWriter  bw(buf->Buffer(), buf->AllocSize());

    UdpPacketHeader* header = bw.Reserve<UdpPacketHeader>();
    bw << moverSessionId << posX << posY << posZ << rot << animState;
    uint16 userIdLen = static_cast<uint16>(userId.size());
    bw << userIdLen;
    for (uint16 i = 0; i < userIdLen; i++)
        bw << (BYTE)userId[i];

    header->sessionId = moverSessionId;
    header->sequence = 0;
    header->id = static_cast<uint16>(PacketType::S_LOBBY_PLAYER_MOVE);

    buf->SetWriteSize(bw.WriteSize());
    return buf;
}

SendBufferRef UdpServerPacketHandler::Make_S_PLAYER_INFO(
    uint64 sessionId, const PlayerRedisInfo& info) {

    SendBufferRef buf = MakeShared<SendBuffer>(UDP_MAX_PACKET);
    BufferWriter  bw(buf->Buffer(), buf->AllocSize());

    UdpPacketHeader* header = bw.Reserve<UdpPacketHeader>();

    bw << sessionId;
    bw << info.hp;
    bw << info.balance;
    bw << info.charType;
    bw << info.posX << info.posY << info.posZ << info.rot;
    bw << info.weapon;
    bw << info.ammoType;

    uint16 nickLen = static_cast<uint16>(info.nickname.size());
    bw << nickLen;
    if (nickLen > 0)
        for (uint16 i = 0; i < nickLen; i++)
            bw << (BYTE)info.nickname[i];

    uint16 sceneLen = static_cast<uint16>(info.scene.size());
    bw << sceneLen;
    if (sceneLen > 0)
        for (uint16 i = 0; i < sceneLen; i++)
            bw << (BYTE)info.scene[i];

    header->sessionId = sessionId;
    header->sequence = 0;
    header->id = static_cast<uint16>(PacketType::S_PLAYER_INFO);

    buf->SetWriteSize(bw.WriteSize());
    return buf;
}