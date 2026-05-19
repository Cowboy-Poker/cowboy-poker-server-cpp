#pragma once
#include "UdpService.h"
#include "UdpSession.h"
#include "RedisClient.h"

/*----------------------------------------------------
    UdpServerPacketHandler
    수신된 UDP 패킷을 PacketType 별로 디스패치.

    [현재 지원 패킷]
    클라 → 서버
      C_UDP_HELLO       (300) 핸드셰이크
      C_LOBBY_MOVE      (301) 위치/회전 전송
      C_LOBBY_HEARTBEAT (302) 연결 유지

    서버 → 클라
      S_UDP_HANDSHAKE_ACK (350) 세션 ID 응답
      S_LOBBY_PLAYER_MOVE (352) 다른 플레이어 위치 릴레이
----------------------------------------------------*/
class UdpServerPacketHandler
{
public:
    static void HandlePacket(std::shared_ptr<UdpService> service,
        UdpClientContextRef ctx,
        BYTE* buffer, int32 len);

private:
    /* 핸드셰이크 */
    static void Handle_C_UDP_HELLO(std::shared_ptr<UdpService> service,
        UdpClientContextRef ctx,
        BYTE* payload, int32 payloadLen);

    /* 로비 이동 */
    static void Handle_C_LOBBY_MOVE(std::shared_ptr<UdpService> service,
        UdpClientContextRef ctx,
        BYTE* payload, int32 payloadLen);


    /* 하트비트 */
    static void Handle_C_LOBBY_HEARTBEAT(UdpClientContextRef ctx);

    /* 패킷 빌더 */
    static SendBufferRef Make_S_UDP_HANDSHAKE_ACK(uint64 sessionId);
    static SendBufferRef Make_S_LOBBY_PLAYER_MOVE(uint64 moverSessionId,
        float posX, float posY,
        float posZ, float rot,
        BYTE animState);
    static SendBufferRef Make_S_LOBBY_PLAYER_LEAVE(uint64 leftSessionId);
    static SendBufferRef Make_S_PLAYER_INFO(uint64 sessionId,
        const PlayerRedisInfo& info);

public:
    /* UdpService onClientLeave 콜백으로 등록 */
    static void OnClientLeave(std::shared_ptr<UdpService> service, uint64 sessionId);
};
