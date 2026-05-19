#include "pch.h"
#include "RedisClient.h"
#include <hiredis/hiredis.h>

RedisClient::~RedisClient() {
    Disconnect();
}

bool RedisClient::Connect(const std::string& host, int port) {
    Disconnect();
    _ctx = redisConnect(host.c_str(), port);
    if (_ctx == nullptr || _ctx->err) {
        if (_ctx) {
            cout << "RedisClient Connect error: " << _ctx->errstr << endl;
            redisFree(_ctx);
            _ctx = nullptr;
        }
        return false;
    }
    cout << "RedisClient connected to " << host << ":" << port << endl;
    return true;
}

void RedisClient::Disconnect() {
    if (_ctx) {
        redisFree(_ctx);
        _ctx = nullptr;
    }
}

bool RedisClient::GetPlayerInfo(const std::string& userId, PlayerRedisInfo& outInfo) {
    if (!IsConnected()) return false;

    std::string key = "user:" + userId;
    redisReply* reply = reinterpret_cast<redisReply*>(
        redisCommand(_ctx, "HGETALL %s", key.c_str()));

    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR || reply->type == REDIS_REPLY_NIL) {
        cout << "RedisClient HGETALL failed for key: " << key << endl;
        if (reply) freeReplyObject(reply);
        return false;
    }

    // HGETALL 결과: [field, value, field, value, ...] 짝수 배열
    outInfo.userId = userId;
    for (size_t i = 0; i + 1 < reply->elements; i += 2) {
        std::string field(reply->element[i]->str);
        std::string value(reply->element[i + 1]->str);

        if      (field == "nickname")  outInfo.nickname = value;
        else if (field == "scene")     outInfo.scene    = value;
        else if (field == "hp")        outInfo.hp       = stoi(value);
        else if (field == "balance")   outInfo.balance  = stoll(value);
        else if (field == "char_type") outInfo.charType = stoi(value);
        else if (field == "pos_x")     outInfo.posX     = stof(value);
        else if (field == "pos_y")     outInfo.posY     = stof(value);
        else if (field == "pos_z")     outInfo.posZ     = stof(value);
        else if (field == "rot")       outInfo.rot      = stof(value);
        else if (field == "weapon")    outInfo.weapon   = stoi(value);
        else if (field == "ammo_type") outInfo.ammoType = stoi(value);
    }

    freeReplyObject(reply);
    return true;
}

bool RedisClient::GetBalance(const std::string& userId, int64& outBalance) {
    if (!IsConnected()) return false;
    std::string key = "user:" + userId;
    redisReply* reply = reinterpret_cast<redisReply*>(
        redisCommand(_ctx, "HGET %s balance", key.c_str()));
    if (!reply || reply->type != REDIS_REPLY_STRING) {
        if (reply) freeReplyObject(reply);
        return false;
    }
    outBalance = stoll(reply->str);
    freeReplyObject(reply);
    return true;
}

bool RedisClient::PurchaseWeapon(const std::string& userId, int64 newBalance, int32 weaponType) {
    if (!IsConnected()) return false;
    std::string key = "user:" + userId;
    std::string balanceStr = std::to_string(newBalance);
    std::string weaponStr = std::to_string(weaponType);

    // balance 갱신
    redisReply* r1 = reinterpret_cast<redisReply*>(
        redisCommand(_ctx, "HSET %s balance %s", key.c_str(), balanceStr.c_str()));
    if (!r1) return false;
    if (r1->type == REDIS_REPLY_ERROR)
        cout << "[Redis] HSET balance error: " << r1->str << endl;
    freeReplyObject(r1);

    // weapon 갱신
    redisReply* r2 = reinterpret_cast<redisReply*>(
        redisCommand(_ctx, "HSET %s weapon %s", key.c_str(), weaponStr.c_str()));
    if (!r2) return false;
    if (r2->type == REDIS_REPLY_ERROR)
        cout << "[Redis] HSET weapon error: " << r2->str << endl;
    freeReplyObject(r2);

    return true;
}