#pragma once
#include "pch.h"
/*----------------
	RefCountable
-----------------*/

class RefCountable {
public:
	RefCountable() : _refCount(1) {}

	virtual ~RefCountable() {}
	// memory leak(메모리 누수)를 예방하기 위해 항상 최상위 클래스에 virtual을 붙임

	int32 GetRefCount() { return _refCount; }

	int32 AddRef() { return ++_refCount; }

	int32 ReleaseRef() {
		int32 refCount = --_refCount;
		if (refCount == 0) {
			delete this;
		}
		return refCount;
	}

protected:
	atomic<int32> _refCount;
};

/*--------------------
	SharedPtr
--------------------*/

template <typename T>
// 포인터의 ref count를 관리하는 스마트 포인터
class TSharedPtr {
public:
	TSharedPtr() {}
	TSharedPtr(T* ptr) { Set(ptr); }

	// 복사
	TSharedPtr(const TSharedPtr& other) { Set(other._ptr); }
	// 이동
	TSharedPtr(TSharedPtr&& other) {
		_ptr = other._ptr;
		other._ptr = nullptr;
	}
	// 상속 관계 복사자
	template <typename U>
	TSharedPtr(const TSharedPtr<U>& other) {
		Set(static_cast<T*>(other._ptr));
	}
	// 만약 Knight 클래스가 Player 클래스를 상속받았다면,
	// TSharedPtr<Knight>를 TSharedPtr<Player>에 넣을 수 있어야함
	// 이 템플릿 생성자가 그 형변환을 가능하게 해줌

	// 소멸
	~TSharedPtr() { Release(); }

public:
	// 복사 연산자
	TSharedPtr& operator=(const TSharedPtr& other) {
		if (_ptr != other._ptr) {
			Release();
			Set(other._ptr);
		}
		return *this;
	}
	// 이동 연산자
	TSharedPtr& operator=(TSharedPtr&& other) {
		Release();
		_ptr = other._ptr;
		other._ptr = nullptr;
		return *this;
	}

	bool operator==(const TSharedPtr& other) const { return _ptr == other._ptr; }
	bool operator==(T* ptr) { return _ptr == ptr; }
	bool operator!=(const TSharedPtr& other) const { return _ptr != other._ptr; }
	bool operator!=(T* ptr) const { return _ptr != ptr; }
	bool operator<(const TSharedPtr& other) const { return _ptr < other._ptr; }
	T* operator*() { return _ptr; }
	const T* operator*() const { return _ptr; }
	operator T*() const { return _ptr; }
	T* operator->() { return _ptr; }
	const T* operator->() const { return _ptr; }
	// get 대신 연산자 오버로딩을 통해 TSharedPtr 클래스를 마치 포인터처럼 사용할 수 있게 함
	// 오버로딩 전: ptr.GetPtr()->hp = 100;
	// 오버로딩 후: ptr->hp = 100;

	bool IsNull() { return _ptr == nullptr; }

private:
	inline void Set(T* ptr) {
		_ptr = ptr;
		if (ptr) ptr->AddRef();
	}

	inline void Release() {
		if (_ptr != nullptr) {
			_ptr->ReleaseRef();
			_ptr = nullptr;
		}
	}

private:
	T* _ptr = nullptr;
};