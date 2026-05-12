#pragma once

class GameSession;

using GameSessionRef = shared_ptr<GameSession>;

// 컨텐츠 제작용 클래스
class GameSessionManager {
public:
	void Add(GameSessionRef session);
	void Remove(GameSessionRef session);
	void Broadcast(SendBufferRef sendBuffer);

private:
	USE_LOCK;
	Set<GameSessionRef> _sessions;
};

extern GameSessionManager GSessionManager;