#pragma once
#include "Allocator.h"

template <typename Type, typename... Args>
Type* xnew(Args&&... args) {  // 새로운 new 연산자 오버로딩
	Type* memory = static_cast<Type*>(xxalloc(sizeof(Type)));

	// placement new: 어떤 객체의 생성자를 호출하는 또다른 문법
	// new (memory) Type();
	new (memory) Type(std::forward<Args>(args)...);
	// forward: 가변인자를 통해 생성자를 호출
	return memory;
}

template <typename Type>
void xdelete(Type* obj) {
	obj->~Type();  // 소멸자 직접 호출
	xxrelease(obj);
}

template <typename Type, typename... Args>
shared_ptr<Type> MakeShared(Args&&... args) {
	return shared_ptr<Type>{xnew<Type>(forward<Args>(args)...), xdelete<Type>};
}