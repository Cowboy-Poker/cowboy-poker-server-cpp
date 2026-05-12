#pragma once
#include "Types.h"

/*-----------------
	RW SpinLock
------------------*/

/*
[WWWWWWWW][WWWWWWWW][RRRRRRRR][RRRRRRRR]
W: WriteFlag (Excusive Lock Owner ThreadId)
R: ReadFlag (Shared Lock Count)
32bit int로 상위, 하위 16비트로 모두 저장할 예정

동일 스레드일 경우
W_L -> W_L -> R_L (O)
R_L -> W_L (X)
*/

class Lock
{
	enum :uint32
	{
		ACQUIRE_TIMEOUT_TIC = 10'000, // 최대로 기다려줄 틱
		MAX_SPIN_COUNT = 5'000, // 스핀 카운트를 최대 몇번 돌 것인지
		WRITE_THREAD_MASK = 0XFFFF'0000, // 상위 16비트를 뽑아오기 위한 마스크
		READ_COUNT_MASK = 0X0000'FFFF,
		EMPTY_FLAG = 0X0000'0000
	};
public:
	void WriteLock(const char* name);
	void WriteUnlock(const char* name);
	void ReadLock(const char* name);
	void ReadUnlock(const char* name);

private:
	Atomic<uint32> _lockFlag=EMPTY_FLAG;

	uint16 _writeCount = 0;
};

/*----------------
	LockGuards
----------------*/

class ReadLockGuard
{
public:
	ReadLockGuard(Lock& lock, const char* name) :_lock(lock), _name(name) { _lock.ReadLock(name); }
	~ReadLockGuard() { _lock.ReadUnlock(_name); }

private:
	Lock& _lock;
	const char* _name;
};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock, const char* name) :_lock(lock), _name(name) { _lock.WriteLock(name); }
	~WriteLockGuard() { _lock.WriteUnlock(_name); }

private:
	Lock& _lock;
	const char* _name;
};