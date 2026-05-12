#include "pch.h"
#include "CoreTls.h"

thread_local uint32 LThreadId = 0;
thread_local std::stack<int32> LLockStack;