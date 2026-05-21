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
    int32  ammoType  = 0;  // Redis "ammo_type" — 탄약 강화 단계 (0=기본, 상위는 추후)
    int32  ammo      = 0;  // Redis "ammo" — 잔탄 수
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

    // balance 조회
    bool GetBalance(const std::string& userId, int64& outBalance);

    // balance 차감 + weapon 교체(기존 총기 대체) + ammo_type=0 + ammo 5탄창
    bool PurchaseWeapon(const std::string& userId, int64 newBalance, int32 weaponType,
                        int32 magazineAmmo, int32 ammoEnhanceLevel = 0);

    // 로비 발사: ammo 1발 차감. outRemainingAmmo = 차감 후 남은 탄 수
    bool ConsumeLobbyAmmo(const std::string& userId, int32& outRemainingAmmo);

    bool SetAmmo(const std::string& userId, int32 ammo);

private:
    struct redisContext* _ctx = nullptr;
};
