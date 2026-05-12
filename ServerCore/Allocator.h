#pragma once
#include "pch.h"

/*-------------------------
	BaseAllocator
-------------------------*/

class BaseAllocator {
public:
	static void* Alloc(int32 size);
	static void Release(void* ptr);
};

/*-------------------------
	StompAllocator
-------------------------*/

// 언리얼 엔진에서도 마련이 되어있음
// 광범위하게 사용되는 정책
// 뭔가를 효율적으로 돌리는 건 아니고 버그를 잡는데 유용함
// 특히 메모리 오염 버그를 잘 해결해 줌

class StompAllocator {
	enum { PAGE_SIZE = 0x1000 };

public:
	static void* Alloc(int32 size);
	static void Release(void* ptr);
};

/*-------------------------
	STL Allocator
-------------------------*/

template <typename T>
class StlAllocator {
public:
	using value_type = T;

	StlAllocator() {}

	template <typename Other>
	StlAllocator(const StlAllocator<Other>&) {}
	// 오류 해결용으로 작성

	T* allocate(size_t count) {
		const int32 size = static_cast<int32>(count * sizeof(T));
		return static_cast<T*>(xxalloc(size));
	}

	void deallocate(T* ptr, size_t count) { xxrelease(ptr); }
};