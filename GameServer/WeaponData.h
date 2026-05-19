#pragma once
#include "pch.h"
#include <unordered_map>
#include <string>

/*----------------------------------------------------
    WeaponInfo
    총기 정적 데이터 (서버 권위).
    weaponType = common.proto WeaponType enum 값.

    WEAPON_REVOLVER_A = 4  -> Revolver Lv1
    WEAPON_REVOLVER_B = 5  -> Revolver Lv2
    WEAPON_RIFLE_A    = 1  -> Rifle
    WEAPON_SHOTGUN_A  = 2  -> Shotgun Lv1
    WEAPON_SHOTGUN_B  = 3  -> Shotgun Lv2
----------------------------------------------------*/
struct WeaponInfo {
    std::string displayName;
    int64 price;        // 구매 가격 ($)
    int32 pelletCount;  // 발사당 투사체 수 (샷건 = 10)
    float fireRate;     // 발사 간격 (초)
    int32 magazineSize; // 탄창 크기
    int32 damageHead;   // 머리 데미지 (샷건은 단일 펠릿 기준)
    int32 damageBody;   // 몸통 데미지
    int32 damageLimb;   // 팔다리 데미지
};

// weaponType -> WeaponInfo 조회 테이블
inline const std::unordered_map<int32, WeaponInfo>& GetWeaponTable() {
    static const std::unordered_map<int32, WeaponInfo> table = {
        // weaponType 4: Revolver Lv1
        { 4, { "Revolver Lv1",   500,   1, 0.5f,  6,  120, 40, 20 } },
        // weaponType 5: Revolver Lv2
        { 5, { "Revolver Lv2",  5000,   1, 0.5f,  6,  120, 40, 20 } },
        // weaponType 1: Rifle
        { 1, { "Rifle",        10000,   1, 0.3f,  8,  150, 60, 30 } },
        // weaponType 2: Shotgun Lv1
        { 2, { "Shotgun Lv1",    300,  10, 0.2f,  2,   30, 10,  5 } },
        // weaponType 3: Shotgun Lv2
        { 3, { "Shotgun Lv2",   3000,  10, 0.2f,  2,   30, 10,  5 } },
    };
    return table;
}

inline const WeaponInfo* FindWeapon(int32 weaponType) {
    auto& table = GetWeaponTable();
    auto it = table.find(weaponType);
    if (it == table.end()) return nullptr;
    return &it->second;
}
