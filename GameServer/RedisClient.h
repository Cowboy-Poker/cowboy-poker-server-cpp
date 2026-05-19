#pragma once
#include <string>

/*----------------------------------------------------
    PlayerRedisInfo
    Redis hash user:{userId} 에서 읽어온 유저 정보.
    모든 필드는 string -> 적절한 타입으로 변환.
----------------------------------------------------*/
struct PlayerRedisInfo {
    std::string userId;
    std::string nickname;
    std::string scene;
    int32  hp        = 0;
    int64  balance   = 0;
    int32  charType  = 0;
    float  posX      = 0.f;
    float  posY      = 1.f;
    float  posZ      = 0.f;
    float  rot       = 0.f;
    int32  weapon    = 0;
    int32  ammoType  = 0;
};

/*----------------------------------------------------
    RedisClient
    hiredis 동기 클라이언트.
    - 단일 연결, 스레드 안전하지 않음.
      호출 측에서 WRITE_LOCK 등으로 보호할 것.
    - 연결 실패 / 명령 실패 시 false / nullopt 반환.
----------------------------------------------------*/
class RedisClient {
public:
    RedisClient() = default;
    ~RedisClient();

    bool Connect(const std::string& host, int port);
    void Disconnect();
    bool IsConnected() const { return _ctx != nullptr; }

    // user:{userId} HGETALL -> PlayerRedisInfo
    // 실패 시 false 반환
    bool GetPlayerInfo(const std::string& userId, PlayerRedisInfo& outInfo);

private:
    struct redisContext* _ctx = nullptr;
};
