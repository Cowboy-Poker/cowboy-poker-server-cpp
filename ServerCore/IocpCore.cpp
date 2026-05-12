#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"
#include <ioapiset.h>

/*-----------------
	IocpCore
------------------*/

IocpCore::IocpCore() {
	_iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore() {
	::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(IocpObjectRef iocpObject) {
	return ::CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle,
									/*key*/ 0, 0);
}

// CP에 완료된 작업이 있나 탐색
bool IocpCore::Dispatch(uint32 timeoutMs) {
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	IocpEvent* iocpEvent = nullptr;

	if (::GetQueuedCompletionStatus(_iocpHandle, OUT & numOfBytes, OUT & key,
									OUT reinterpret_cast<LPOVERLAPPED*>(&iocpEvent), timeoutMs)) {
		IocpObjectRef iocpObject = iocpEvent->owner;
		iocpObject->Dispatch(iocpEvent, numOfBytes);
		// listener, session 각 Dispatch 함수 호출
	} else {
		int32 errCode = ::WSAGetLastError();
		switch (errCode) {
			case WAIT_TIMEOUT:
				return false;
			default:
				// TODO: 에러 로그 찍기
				IocpObjectRef iocpObject = iocpEvent->owner;
				iocpObject->Dispatch(iocpEvent, numOfBytes);
				break;
		}
	}

	return true;
}
