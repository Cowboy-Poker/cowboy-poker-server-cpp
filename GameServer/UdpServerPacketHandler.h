#pragma once
#include "UdpService.h"
#include "UdpSession.h"
#include "RedisClient.h"

/*----------------------------------------------------
    UdpServerPacketHandler
    로비 UDP 패킷을 PacketType 기준으로 분기·처리.

    [클라이언트 -> 서버]
    첫 연결 / 동기화
      C_UDP_HELLO       (300) 핸드셰이크
      C_LOBBY_MOVE      (301) 이동/회전 브로드캐스트
      C_LOBBY_HEARTBEAT (302) 연결 유지
      C_PURCHASE_WEAPON (303) 총기 구매 (기존 무기 대체)
      C_GUN_SHOT_LOBBY  (305) 로비 발사 (ammo 1 차감)

    [서버 -> 클라이언트]
      S_UDP_HANDSHAKE_ACK     (350) 세션 ID 응답
      S_LOBBY_PLAYER_MOVE       (352) 다른 플레이어 이동 릴레이
      S_LOBBY_PLAYER_LEAVE      (353) 퇴장 알림
      S_PLAYER_INFO             (354) Redis 유저 정보 (HELLO·구매 성공 후)
      S_PURCHASE_WEAPON_RESULT  (355) 구매 성공/실패 (실패 시 balance 참고)
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

    /* 총기 구매 (C_PURCHASE_WEAPON) */
    static void Handle_C_BUY_WEAPON(std::shared_ptr<UdpService> service,
        UdpClientContextRef ctx,
        BYTE* payload, int32 payloadLen);

    /* 로비 발사 */
    static void Handle_C_GUN_SHOT_LOBBY(std::shared_ptr<UdpService> service,
        UdpClientContextRef ctx);

    /* 패킷 빌더 */
    static SendBufferRef Make_S_UDP_HANDSHAKE_ACK(uint64 sessionId);
    static SendBufferRef Make_S_LOBBY_PLAYER_MOVE(uint64 moverSessionId,
        const std::string& userId,
        float posX, float posY,
        float posZ, float rot,
        BYTE animState);
    static SendBufferRef Make_S_LOBBY_PLAYER_LEAVE(uint64 leftSessionId);
    static SendBufferRef Make_S_PLAYER_INFO(uint64 sessionId,
        const PlayerRedisInfo& info);
    static SendBufferRef Make_S_BUY_WEAPON_RESULT(uint64 sessionId, bool success,
        int32 weaponType, int64 balance);

public:
    /* UdpService onClientLeave 콜백에서 호출 */
    static void OnClientLeave(std::shared_ptr<UdpService> service, uint64 sessionId);
};