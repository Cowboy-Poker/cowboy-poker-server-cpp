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
