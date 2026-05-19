#pragma once

enum class PacketType
{
    // login 100
    C_LOGIN = 100,
    S_LOGIN = 101,
    C_REGISTER = 102,
    S_REGISTER = 103,

    // poker 200
    C_GET_ROOM_LIST = 200,
    S_GET_ROOM_LIST = 201,
    C_CREATE_ROOM = 202,
    S_CREATE_ROOM = 203,
    C_JOIN_ROOM = 204,
    S_JOIN_ROOM = 205,
    C_LEAVE_ROOM = 206,
    S_LEAVE_ROOM = 207,
    C_START_GAME = 208,
    S_START_GAME = 209,
    C_BET_ACTION = 210,
    S_BET_TURN = 211,
    S_BET_RESULT = 212,
    S_OPEN_COMMUNITY_CARDS = 213,
    S_GAME_RESULT = 214,
    S_NOTICE = 215,

    // UDP — 로비 동기화 300
    C_UDP_HELLO = 300, // 첫 연결 핸드셰이크 (sessionId = 0으로 전송)
    C_LOBBY_MOVE = 301, // 로비 캐릭터 이동 (TransformInfo)
    C_LOBBY_HEARTBEAT = 302, // 연결 유지 핑
    C_PURCHASE_WEAPON = 303, // 총기 구매 요청 [userIdLen:u16][userId:utf8][weaponType:i32]
    // 이름 S_UDP_HELLO 는 일부 WinSock/Windows 조합에서 전처리기와 충돌할 수 있음.
    S_UDP_HANDSHAKE_ACK = 350, // lobby_response.S_UDP_HELLO 와 동일 의미 (sessionId 응답)
    S_LOBBY_PLAYER_MOVE = 352, // 특정 플레이어 이동 브로드캐스트
    S_LOBBY_PLAYER_LEAVE = 353, // 플레이어 퇴장 — 클라이언트는 해당 sessionId 캐릭터 제거
    S_PLAYER_INFO = 354, // 핸드셰이크 후 Redis 조회 결과 (내 유저 정보 전달)
    S_PURCHASE_WEAPON_RESULT = 355, // 총기 구매 결과 [sessionId:u64][success:u8][weaponType:i32][balance:i64]
};
