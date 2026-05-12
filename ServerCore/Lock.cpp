#include "pch.h"
#include "Lock.h"
#include "CoreTls.h"
#include "DeadLockProfiler.h"

void Lock::WriteLock(const char* name) {
// Profiler에서 lockguard를 사용하고 있는데 lock을 테스트하기 위해서 lock을 사용하는 건 모순적임
// 그래서 디버그 상황에서만 체크를 하도록 매크로 정의
#if _DEBUG
	GDeadLockProfiler->PushLock(name);
#endif
	// 재귀적으로 동일한 스레드가 이 락을 소유하고 있다면 무조건 성공하게끔
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (lockThreadId == LThreadId) {
		_writeCount++;
		return;
	}

	// 아무도 소유(Write) 및 공유(Read)하고 있지 않을 때, 경합해서 소유권을 얻음
	const int64 beingTick = ::GetTickCount64();
	const uint32 desired = ((LThreadId << 16) & WRITE_THREAD_MASK);
	while (true) {
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; spinCount++) {
			uint32 expected = EMPTY_FLAG;
			if (_lockFlag.compare_exchange_strong(OUT expected, desired)) {
				_writeCount++;
				return;
			}
		}
		if (::GetTickCount64() - beingTick >= ACQUIRE_TIMEOUT_TIC)
			CRASH("WRITE_LOCK_TIMEOUT");

		this_thread::yield();
	}
}

void Lock::WriteUnlock(const char* name) {
#if _DEBUG
	GDeadLockProfiler->PopLock(name);
#endif

	// ReadLock 다 풀기 전에는 WriteUnlock 불가능
	if ((_lockFlag.load() & READ_COUNT_MASK) != 0)
		CRASH("INVALID_UNLOCK_ORDER");

	const int32 lockCount = --_writeCount;
	if (lockCount == 0)
		_lockFlag.store(EMPTY_FLAG);
}

void Lock::ReadLock(const char* name) {
#if _DEBUG
	GDeadLockProfiler->PushLock(name);
#endif

	// 동일한 스레드가 소유하고 있다면 무조건 성공 (W->R 허용이므로)
	const uint32 lockThreadId = (_lockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (lockThreadId == LThreadId) {
		_lockFlag.fetch_add(1);
		return;
	}

	// 아무도 소유하고 있지 않을 때 경합해서 공유 카운트(R_L)을 올림
	const int64 beingTick = ::GetTickCount64();

	while (1) {
		for (uint32 spinCount = 0; spinCount < MAX_SPIN_COUNT; spinCount++) {
			uint32 expected = (_lockFlag.load() & READ_COUNT_MASK);
			if (_lockFlag.compare_exchange_strong(OUT expected, expected + 1))
				return;
		}
		if (::GetTickCount64() - beingTick >= ACQUIRE_TIMEOUT_TIC)
			CRASH("READ_LOCK_TIMEOUT");

		this_thread::yield();
	}
}

void Lock::ReadUnlock(const char* name) {
#if _DEBUG
	GDeadLockProfiler->PopLock(name);
#endif

	if ((_lockFlag.fetch_sub(1) & READ_COUNT_MASK) == 0)
		CRASH("MULTIPLE_UNLOCK");
}
