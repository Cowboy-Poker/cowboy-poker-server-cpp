#pragma once
#include "Session.h"

class GameSession : public PacketSession {
	// 실제 게임 서버 로직이 돌아갈 세션을 Session 상속받아서 작성 후 사용
public:
	~GameSession() { cout << "~GameSession" << endl; }
	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
};