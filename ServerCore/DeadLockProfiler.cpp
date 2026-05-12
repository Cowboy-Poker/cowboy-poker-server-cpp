#include "pch.h"
#include "DeadLockProfiler.h"

/*--------------------
	DeadLockProfiler
---------------------*/

void DeadLockProfiler::PushLock(const char* name)
{
	LockGuard guard(_lock);

	// 아이디를 찾거나 발급 (0부터)
	int32 lockId = 0;

	auto findIt = _nameToId.find(name);
	if (findIt == _nameToId.end())
	{
		lockId = static_cast<int32>(_nameToId.size());
		_nameToId[name] = lockId;
		_idToName[lockId] = name;
	}
	else
	{
		lockId = findIt->second;
	}

	// 잡고 있는 락이 있다면
	if (LLockStack.empty() == false)
	{
		// 현재 구현한 RW Lock이 재귀적 락을 허용하는 중임
		// 재귀적 락 상황이면 의미가 없기 때문에 그냥 넘어감
		// (새로운 간선이 아니라 A -> A 같은 본인 노드로 돌아오는 간선)
		const int32 prevId = LLockStack.top();
		// 여기서 prevId는 본인 스레드가 걸었던 이전 lock을 가져와야 함
		// 본 코드에선 _lockStack을 DeadLockProfiler에서 한꺼번에 관리하기 때문에
		// 다른 스레드가 걸었던 lock을 본인이 걸었던 이전 lock이라고 착각하고 이상한 간선을 만들게 됨
		// 다른 스레드와의 데드락을 감지하는 장치는 _lockHistory와 CheckCycle()이고
		// _lockStack은 각 스레드가 어떤 순서로 lock을 잡고 있는지를 나타내는 것임
		// _lockStack은 이 정보(현재 스레드가 잡은 lock 순서)를 활용해 _lockHistory에 간선을 추가하는 역할을 함 (마지막에 잡은 lock에서 현재 잡고자 하는 lock으로 간선을 추가)
		if (lockId != prevId)
		{
			// 기존에 발견되지 않은 케이스라면 데드락 여부를 다시 확인 (사이클 판별)
			set<int32>& history = _lockHistory[prevId];
			if (history.find(lockId) == history.end())
			{
				history.insert(lockId);
				CheckCycle();
			}
		}
	}

	LLockStack.push(lockId);
}

void DeadLockProfiler::PopLock(const char* name)
{
	LockGuard guard(_lock);
	
	if (LLockStack.empty()) // 혹시 모를 스택 비어있을 에러 예방
		CRASH("MULTIPLE_UNLOCK");

	int32 lockId = _nameToId[name];
	if (LLockStack.top() != lockId) // 혹시 모를 스택 push 에러 예방
		CRASH("INVALID_UNLOCK");

	LLockStack.pop();
}

void DeadLockProfiler::CheckCycle()
{
	const int32 lockCount = static_cast<int32>(_nameToId.size());
	// 초기화
	_discoveredOrder = vector<int32>(lockCount, -1);
	_discoveredCount = 0;
	_finished = vector<bool>(lockCount, false);
	_parent = vector<int32>(lockCount, -1);

	for (int32 lockId = 0; lockId < lockCount; lockId++)
		Dfs(lockId);

	// 연산이 끝났으면 정리
	_discoveredOrder.clear();
	_finished.clear();
	_parent.clear();
}

void DeadLockProfiler::Dfs(int32 here)
{
	if (_discoveredOrder[here] != -1) // 이미 방문을 했다
		return;

	_discoveredOrder[here] = _discoveredCount++;

	// 모든 인접한 정점을 순회
	auto findIt = _lockHistory.find(here);
	if (findIt == _lockHistory.end())
	{
		_finished[here] = true;
		return;
	}

	set<int32>& nextSet = findIt->second;
	for (int32 there : nextSet)
	{
		// 아직 방문한 적이 없다면 방문한다
		if (_discoveredOrder[there] == -1)
		{
			_parent[there] = here;
			Dfs(there);
			continue;
		}

		// here가 there보다 먼저 발견되었다면, there는 here의 후손 (순방향 간선)
		if (_discoveredOrder[here] < _discoveredOrder[there])
			continue;

		// 순방향이 아니고, Dfs(there)가 아직 종료하지 않았다면, there는 here의 선조 (역방향 간선)
		if (_finished[there] == false)
		{
			printf("%s -> %s\n", _idToName[here], _idToName[there]);

			int32 now = here;
			while (1)
			{
				printf("%s -> %s\n", _idToName[_parent[now]], _idToName[now]);
				now = _parent[now];
				if (now == there)
					break;
			}

			CRASH("DEADLOCK_DETECTED");
		}
	}
	_finished[here] = true;

}
